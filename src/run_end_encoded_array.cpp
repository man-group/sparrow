// Copyright 2024 Man Group Operations Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sparrow/run_end_encoded_array.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

#include "sparrow/array.hpp"
#include "sparrow/debug/copy_tracker.hpp"
#include "sparrow/layout/array_helper.hpp"
#include "sparrow/layout/array_registry.hpp"
#include "sparrow/primitive_array.hpp"

namespace sparrow
{
    template <class T>
    concept usable_array = (mpl::is_type_instance_of_v<T, primitive_array>
                            || mpl::is_type_instance_of_v<T, primitive_array_impl>)
                           && (std::same_as<typename T::inner_value_type, std::int16_t>
                               || std::same_as<typename T::inner_value_type, std::int32_t>
                               || std::same_as<typename T::inner_value_type, std::int64_t>);

    namespace
    {
        template <class ARRAY>
        constexpr bool supports_encoded_value_insert_v = is_nullable_v<typename ARRAY::value_type>
                                                         && requires(
                                                             ARRAY& array,
                                                             typename ARRAY::const_iterator pos,
                                                             const typename ARRAY::value_type& typed_value,
                                                             std::size_t count
                                                         ) { array.insert(pos, typed_value, count); };

        template <class ARRAY>
        constexpr bool supports_encoded_value_erase_v = requires(
            ARRAY& array,
            typename ARRAY::const_iterator first,
            typename ARRAY::const_iterator last
        ) { array.erase(first, last); };

        template <class ACC_LENGTH_TYPE>
        auto run_length_at(const ACC_LENGTH_TYPE* acc_length_data, std::size_t index) -> std::int64_t
        {
            return index == 0 ? static_cast<std::int64_t>(acc_length_data[index])
                              : static_cast<std::int64_t>(acc_length_data[index] - acc_length_data[index - 1]);
        }

        auto to_int64(std::size_t value) -> std::int64_t
        {
            SPARROW_ASSERT_TRUE(std::in_range<std::int64_t>(value));
            return static_cast<std::int64_t>(value);
        }

        auto to_uint64(std::size_t value) -> std::uint64_t
        {
            SPARROW_ASSERT_TRUE(std::in_range<std::uint64_t>(value));
            return static_cast<std::uint64_t>(value);
        }

        auto checked_add_u64(std::uint64_t lhs, std::size_t rhs) -> std::uint64_t
        {
            const auto rhs_u64 = to_uint64(rhs);
            SPARROW_ASSERT_TRUE(lhs <= (std::numeric_limits<std::uint64_t>::max() - rhs_u64));
            return lhs + rhs_u64;
        }

        auto checked_sub_u64(std::uint64_t lhs, std::size_t rhs) -> std::uint64_t
        {
            const auto rhs_u64 = to_uint64(rhs);
            SPARROW_ASSERT_TRUE(lhs >= rhs_u64);
            return lhs - rhs_u64;
        }

        template <class ACC_LENGTH_TYPE, class HAS_VALUE_FUNC>
        auto extract_length_and_null_count_from_data(
            const ACC_LENGTH_TYPE* acc_length_data,
            std::size_t raw_size,
            std::int64_t raw_null_count,
            HAS_VALUE_FUNC has_value
        ) -> std::pair<std::int64_t, std::int64_t>
        {
            if (raw_size == 0)
            {
                return {0, 0};
            }

            const auto length = static_cast<std::int64_t>(acc_length_data[raw_size - 1]);
            if (raw_null_count == 0)
            {
                return {0, length};
            }

            std::int64_t null_count = 0;
            if (raw_null_count < 0)
            {
                for (std::size_t i = 0; i < raw_size; ++i)
                {
                    if (!has_value(i))
                    {
                        null_count += run_length_at(acc_length_data, i);
                    }
                }
                return {null_count, length};
            }

            std::int64_t remaining_null_runs = raw_null_count;
            for (std::size_t i = 0; i < raw_size && remaining_null_runs != 0; ++i)
            {
                if (!has_value(i))
                {
                    null_count += run_length_at(acc_length_data, i);
                    --remaining_null_runs;
                }
            }
            return {null_count, length};
        }

        template <class F>
        void with_mutable_acc_lengths(array& wrapper, F&& func)
        {
            wrapper.visit(
                [&func](const auto& typed_wrapper)
                {
                    using array_type = std::remove_cvref_t<decltype(typed_wrapper)>;

                    if constexpr (usable_array<array_type>)
                    {
                        std::forward<F>(func)(const_cast<array_type&>(typed_wrapper));
                    }
                    else
                    {
                        throw std::invalid_argument(
                            "run_end_encoded_array accumulated lengths must be int16/int32/int64"
                        );
                    }
                }
            );
        }

        template <class ARRAY>
        void insert_acc_length_value(ARRAY& acc_lengths, std::size_t pos, std::uint64_t value)
        {
            using acc_type = typename ARRAY::inner_value_type;
            SPARROW_ASSERT_TRUE(std::in_range<acc_type>(value));
            const auto insert_pos = std::next(acc_lengths.cbegin(), static_cast<std::ptrdiff_t>(pos));
            static_cast<void>(
                acc_lengths.insert(insert_pos, nullable<acc_type>(static_cast<acc_type>(value), true))
            );
        }

        template <class ARRAY>
        void erase_acc_length_values(ARRAY& acc_lengths, std::size_t pos, std::size_t count)
        {
            if (count == 0)
            {
                return;
            }
            const auto first = std::next(acc_lengths.cbegin(), static_cast<std::ptrdiff_t>(pos));
            const auto last = std::next(first, static_cast<std::ptrdiff_t>(count));
            static_cast<void>(acc_lengths.erase(first, last));
        }

        template <class ARRAY>
        void set_acc_length_value(ARRAY& acc_lengths, std::size_t index, std::uint64_t value)
        {
            using acc_type = typename ARRAY::inner_value_type;
            SPARROW_ASSERT_TRUE(std::in_range<acc_type>(value));
            auto entry = acc_lengths[index];
            entry.get() = static_cast<acc_type>(value);
            entry.null_flag() = true;
        }

        template <class ARRAY>
        void shift_acc_length_values(ARRAY& acc_lengths, std::size_t start_index, std::int64_t delta)
        {
            using acc_type = typename ARRAY::inner_value_type;
            for (std::size_t i = start_index; i < acc_lengths.size(); ++i)
            {
                auto entry = acc_lengths[i];
                const std::int64_t shifted_value = static_cast<std::int64_t>(entry.get()) + delta;
                SPARROW_ASSERT_TRUE(shifted_value > 0);
                SPARROW_ASSERT_TRUE(std::in_range<acc_type>(shifted_value));
                entry.get() = static_cast<acc_type>(shifted_value);
                entry.null_flag() = true;
            }
        }

        void insert_encoded_child_value(array& child, std::size_t pos, const array_traits::value_type& value)
        {
            using value_variant_type = typename array_traits::value_type::base_type;

            child.visit(
                [&](const auto& child_impl)
                {
                    using child_array_type = std::decay_t<decltype(child_impl)>;
                    if constexpr (supports_encoded_value_insert_v<child_array_type>)
                    {
                        bool inserted = false;
                        std::visit(
                            [&](const auto& typed_value)
                            {
                                if constexpr (std::same_as<std::decay_t<decltype(typed_value)>, typename child_array_type::value_type>)
                                {
                                    auto& mutable_child = const_cast<child_array_type&>(child_impl);
                                    auto insert_pos = std::next(
                                        mutable_child.cbegin(),
                                        static_cast<std::ptrdiff_t>(pos)
                                    );
                                    static_cast<void>(mutable_child.insert(insert_pos, typed_value, 1));
                                    inserted = true;
                                }
                            },
                            static_cast<const value_variant_type&>(value)
                        );

                        if (!inserted)
                        {
                            throw std::invalid_argument(
                                "run_end_encoded_array value type does not match encoded values child array"
                            );
                        }
                    }
                    else
                    {
                        throw std::runtime_error(
                            "run_end_encoded_array encoded values child must support insert(value, count)"
                        );
                    }
                }
            );
        }

        void validate_encoded_child_erase(const array& child)
        {
            child.visit(
                [](const auto& child_impl)
                {
                    using child_array_type = std::decay_t<decltype(child_impl)>;
                    if constexpr (!supports_encoded_value_erase_v<child_array_type>)
                    {
                        throw std::runtime_error(
                            "run_end_encoded_array encoded values child must support erase(first, last)"
                        );
                    }
                }
            );
        }
    }

    namespace copy_tracker
    {
        template <>
        SPARROW_API std::string key<run_end_encoded_array>()
        {
            return "run_end_encoded_array";
        }
    }

    run_end_encoded_array::run_end_encoded_array(arrow_proxy proxy)
        : m_proxy(std::move(proxy))
        , m_encoded_length(m_proxy.children()[0].length())
        , p_acc_lengths_array(array(m_proxy.children()[0].view()))
        , p_encoded_values_array(array(m_proxy.children()[1].view()))
        , m_acc_lengths(run_end_encoded_array::get_acc_lengths_ptr(p_acc_lengths_array))
    {
    }

    run_end_encoded_array::run_end_encoded_array(const self_type& rhs)
        : run_end_encoded_array(rhs.m_proxy)
    {
        copy_tracker::increase(copy_tracker::key<run_end_encoded_array>());
    }

    auto run_end_encoded_array::operator=(const self_type& rhs) -> self_type&
    {
        copy_tracker::increase(copy_tracker::key<run_end_encoded_array>());
        if (this != &rhs)
        {
            m_proxy = rhs.m_proxy;
            m_encoded_length = rhs.m_encoded_length;
            p_acc_lengths_array = array(m_proxy.children()[0].view());
            p_encoded_values_array = array(m_proxy.children()[1].view());
            m_acc_lengths = run_end_encoded_array::get_acc_lengths_ptr(p_acc_lengths_array);
        }
        return *this;
    }

    run_end_encoded_reference::run_end_encoded_reference(run_end_encoded_array& array, std::size_t index)
        : run_end_encoded_reference(array, index, array.find_run_index(static_cast<std::uint64_t>(index)))
    {
    }

    run_end_encoded_reference::run_end_encoded_reference(
        run_end_encoded_array& array,
        std::size_t index,
        std::size_t run_index
    )
        : base_type(run_end_encoded_array::materialize_value(array.encoded_value(run_index)))
        , p_array(&array)
        , m_index(index)
        , m_run_index(run_index)
    {
    }

    run_end_encoded_reference& run_end_encoded_reference::operator=(const run_end_encoded_reference& rhs)
    {
        if (this != &rhs)
        {
            *this = static_cast<const value_type&>(rhs);
        }
        return *this;
    }

    run_end_encoded_reference& run_end_encoded_reference::operator=(const const_reference& rhs)
    {
        operator=(run_end_encoded_array::materialize_value(rhs));
        return *this;
    }

    run_end_encoded_reference& run_end_encoded_reference::operator=(const value_type& rhs)
    {
        p_array->replace_logical_value(m_index, rhs);
        refresh();
        return *this;
    }

    run_end_encoded_reference::operator const_reference() const
    {
        return current_value();
    }

    auto run_end_encoded_reference::current_value() const -> const_reference
    {
        SPARROW_ASSERT_TRUE(m_index < p_array->size());

        if (m_run_index >= p_array->m_encoded_length || m_index < p_array->run_start(m_run_index)
            || m_index >= p_array->run_end(m_run_index))
        {
            m_run_index = p_array->find_run_index(static_cast<std::uint64_t>(m_index));
        }

        return p_array->encoded_value(m_run_index);
    }

    void run_end_encoded_reference::refresh()
    {
        static_cast<base_type&>(*this) = run_end_encoded_array::materialize_value(current_value());
    }

    auto run_end_encoded_array::size() const -> size_type
    {
        return m_proxy.length();
    }

    auto run_end_encoded_array::empty() const -> bool
    {
        return size() == 0;
    }

    std::optional<std::string_view> run_end_encoded_array::name() const
    {
        return m_proxy.name();
    }

    std::optional<key_value_view> run_end_encoded_array::metadata() const
    {
        return m_proxy.metadata();
    }

    auto run_end_encoded_array::operator[](std::uint64_t i) -> reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return reference(*this, static_cast<size_type>(i));
    }

    auto run_end_encoded_array::get_acc_length(std::uint64_t run_index) const -> std::uint64_t
    {
        auto ret = std::visit(
            [run_index](auto&& acc_lengths_ptr) -> std::uint64_t
            {
                return static_cast<std::uint64_t>(acc_lengths_ptr[run_index]);
            },
            m_acc_lengths
        );
        return ret;
    }

    arrow_proxy& run_end_encoded_array::get_arrow_proxy()
    {
        return m_proxy;
    }

    const arrow_proxy& run_end_encoded_array::get_arrow_proxy() const
    {
        return m_proxy;
    }

    auto run_end_encoded_array::begin() -> iterator
    {
        return iterator(this, 0, 0);
    }

    auto run_end_encoded_array::end() -> iterator
    {
        return iterator(this, size(), m_encoded_length);
    }

    auto run_end_encoded_array::begin() const -> const_iterator
    {
        return this->cbegin();
    }

    auto run_end_encoded_array::end() const -> const_iterator
    {
        return this->cend();
    }

    auto run_end_encoded_array::cbegin() const -> const_iterator
    {
        return const_iterator(this, 0, 0);
    }

    auto run_end_encoded_array::cend() const -> const_iterator
    {
        return const_iterator(this, size(), m_encoded_length);
    }

    auto run_end_encoded_array::rbegin() -> reverse_iterator
    {
        return reverse_iterator(end());
    }

    auto run_end_encoded_array::rend() -> reverse_iterator
    {
        return reverse_iterator(begin());
    }

    auto run_end_encoded_array::rbegin() const -> const_reverse_iterator
    {
        return crbegin();
    }

    auto run_end_encoded_array::rend() const -> const_reverse_iterator
    {
        return crend();
    }

    auto run_end_encoded_array::crbegin() const -> const_reverse_iterator
    {
        return const_reverse_iterator(cend());
    }

    auto run_end_encoded_array::crend() const -> const_reverse_iterator
    {
        return const_reverse_iterator(cbegin());
    }

    auto run_end_encoded_array::front() const -> array_traits::const_reference
    {
        return operator[](0);
    }

    auto run_end_encoded_array::front() -> reference
    {
        return operator[](0);
    }

    auto run_end_encoded_array::back() const -> array_traits::const_reference
    {
        return operator[](size() - 1);
    }

    auto run_end_encoded_array::back() -> reference
    {
        return operator[](size() - 1);
    }

    auto run_end_encoded_array::insert(const_iterator pos, const value_type& value) -> iterator
    {
        return insert(pos, value, 1);
    }

    auto run_end_encoded_array::insert(const_iterator pos, const value_type& value, size_type count) -> iterator
    {
        const size_type index = static_cast<size_type>(std::distance(cbegin(), pos));
        if (count != 0)
        {
            insert_logical_values(index, value, count);
        }
        return sparrow::next(begin(), static_cast<std::ptrdiff_t>(index));
    }

    auto run_end_encoded_array::erase(const_iterator pos) -> iterator
    {
        return erase(pos, sparrow::next(pos, 1));
    }

    auto run_end_encoded_array::erase(const_iterator first, const_iterator last) -> iterator
    {
        const size_type index = static_cast<size_type>(std::distance(cbegin(), first));
        const size_type count = static_cast<size_type>(std::distance(first, last));
        if (count != 0)
        {
            erase_logical_values(index, count);
        }
        return sparrow::next(begin(), static_cast<std::ptrdiff_t>(index));
    }

    void run_end_encoded_array::push_back(const value_type& value)
    {
        static_cast<void>(insert(cend(), value));
    }

    void run_end_encoded_array::pop_back()
    {
        SPARROW_ASSERT_TRUE(!empty());
        static_cast<void>(erase(sparrow::next(cbegin(), static_cast<std::ptrdiff_t>(size() - 1))));
    }

    void run_end_encoded_array::resize(size_type new_length, const value_type& value)
    {
        const size_type current_size = size();
        if (new_length < current_size)
        {
            static_cast<void>(erase(sparrow::next(cbegin(), static_cast<std::ptrdiff_t>(new_length)), cend()));
        }
        else if (new_length > current_size)
        {
            static_cast<void>(insert(cend(), value, new_length - current_size));
        }
    }

    void run_end_encoded_array::clear()
    {
        if (!empty())
        {
            static_cast<void>(erase(cbegin(), cend()));
        }
    }

    bool operator==(const run_end_encoded_array& lhs, const run_end_encoded_array& rhs)
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }

        auto lhs_it = lhs.cbegin();
        auto rhs_it = rhs.cbegin();
        const auto lhs_end = lhs.cend();

        for (; lhs_it != lhs_end; ++lhs_it, ++rhs_it)
        {
            if (!(*lhs_it == *rhs_it))
            {
                return false;
            }
        }

        return true;
    }

    auto run_end_encoded_array::get_acc_lengths_ptr(const array& ar) -> acc_length_ptr_variant_type
    {
        return ar.visit(
            [](const auto& actual_arr) -> acc_length_ptr_variant_type
            {
                using array_type = std::decay_t<decltype(actual_arr)>;

                if constexpr (usable_array<array_type>)
                {
                    return actual_arr.data();
                }
                else
                {
                    throw std::invalid_argument("array type not supported");
                }
            }
        );
    }

    auto run_end_encoded_array::operator[](std::uint64_t i) const -> array_traits::const_reference
    {
        return p_encoded_values_array[find_run_index(i)];
    }

    auto run_end_encoded_array::find_run_index(std::uint64_t logical_index) const -> size_type
    {
        SPARROW_ASSERT_TRUE(logical_index < size());
        return visit(
            [logical_index, this](const auto& acc_lengths_ptr) -> size_type
            {
                const auto it = std::upper_bound(
                    acc_lengths_ptr,
                    acc_lengths_ptr + this->m_encoded_length,
                    logical_index
                );
                return static_cast<size_type>(std::distance(acc_lengths_ptr, it));
            },
            m_acc_lengths
        );
    }

    auto run_end_encoded_array::run_start(size_type run_index) const -> std::uint64_t
    {
        return run_index == 0 ? 0 : get_acc_length(run_index - 1);
    }

    auto run_end_encoded_array::run_end(size_type run_index) const -> std::uint64_t
    {
        return get_acc_length(run_index);
    }

    auto run_end_encoded_array::encoded_value(size_type run_index) const -> const_reference
    {
        return p_encoded_values_array[run_index];
    }

    bool run_end_encoded_array::encoded_values_equal(size_type lhs_run_index, size_type rhs_run_index) const
    {
        return encoded_value(lhs_run_index) == encoded_value(rhs_run_index);
    }

    bool run_end_encoded_array::encoded_value_equals(size_type run_index, const value_type& value) const
    {
        return materialize_value(encoded_value(run_index)) == value;
    }

    void run_end_encoded_array::insert_encoded_value(size_type run_index, const value_type& value)
    {
        insert_encoded_child_value(p_encoded_values_array, run_index, value);
    }

    void run_end_encoded_array::erase_encoded_values(size_type run_index, size_type count)
    {
        if (count == 0)
        {
            return;
        }
        validate_encoded_child_erase(p_encoded_values_array);
        p_encoded_values_array.erase(
            p_encoded_values_array.cbegin() + static_cast<std::ptrdiff_t>(run_index),
            p_encoded_values_array.cbegin() + static_cast<std::ptrdiff_t>(run_index + count)
        );
    }

    void run_end_encoded_array::merge_adjacent_runs(size_type left_run_index)
    {
        if ((left_run_index + 1) >= m_encoded_length
            || !encoded_values_equal(left_run_index, left_run_index + 1))
        {
            return;
        }

        const auto merged_run_end = run_end(left_run_index + 1);
        with_mutable_acc_lengths(
            p_acc_lengths_array,
            [left_run_index, merged_run_end](auto& acc_lengths)
            {
                set_acc_length_value(acc_lengths, left_run_index, merged_run_end);
                erase_acc_length_values(acc_lengths, left_run_index + 1, 1);
            }
        );
        erase_encoded_values(left_run_index + 1, 1);
    }

    void run_end_encoded_array::insert_logical_values(
        size_type index,
        const value_type& value,
        size_type count,
        bool refresh_state
    )
    {
        throw_if_sliced_for_mutation("run_end_encoded_array::insert");
        SPARROW_ASSERT_TRUE(index <= size());

        if (count == 0)
        {
            return;
        }

        const auto count_delta = to_int64(count);
        const auto logical_index = to_uint64(index);
        const auto insertion_end = checked_add_u64(logical_index, count);
        const auto appended_end = checked_add_u64(to_uint64(size()), count);
        const auto update_acc_lengths = [this](auto&& updater)
        {
            with_mutable_acc_lengths(p_acc_lengths_array, std::forward<decltype(updater)>(updater));
        };

        if (m_encoded_length == 0)
        {
            insert_encoded_value(0, value);
            update_acc_lengths(
                [count](auto& acc_lengths)
                {
                    insert_acc_length_value(acc_lengths, 0, count);
                }
            );
            finalize_mutation(refresh_state);
            return;
        }

        if (index == size())
        {
            if (encoded_value_equals(m_encoded_length - 1, value))
            {
                update_acc_lengths(
                    [this, count_delta](auto& acc_lengths)
                    {
                        shift_acc_length_values(acc_lengths, m_encoded_length - 1, count_delta);
                    }
                );
            }
            else
            {
                insert_encoded_value(m_encoded_length, value);
                update_acc_lengths(
                    [this, appended_end](auto& acc_lengths)
                    {
                        insert_acc_length_value(acc_lengths, m_encoded_length, appended_end);
                    }
                );
            }
            finalize_mutation(refresh_state);
            return;
        }

        const size_type run_index = find_run_index(index);
        const std::uint64_t current_run_start = run_start(run_index);
        const std::uint64_t current_run_end = run_end(run_index);
        const auto shifted_run_end = checked_add_u64(current_run_end, count);

        if (index == current_run_start)
        {
            if (run_index > 0 && encoded_value_equals(run_index - 1, value))
            {
                update_acc_lengths(
                    [run_index, count_delta](auto& acc_lengths)
                    {
                        shift_acc_length_values(acc_lengths, run_index - 1, count_delta);
                    }
                );
            }
            else if (encoded_value_equals(run_index, value))
            {
                update_acc_lengths(
                    [run_index, count_delta](auto& acc_lengths)
                    {
                        shift_acc_length_values(acc_lengths, run_index, count_delta);
                    }
                );
            }
            else
            {
                insert_encoded_value(run_index, value);
                update_acc_lengths(
                    [run_index, insertion_end, count_delta](auto& acc_lengths)
                    {
                        insert_acc_length_value(acc_lengths, run_index, insertion_end);
                        shift_acc_length_values(acc_lengths, run_index + 1, count_delta);
                    }
                );
            }
            finalize_mutation(refresh_state);
            return;
        }

        if (encoded_value_equals(run_index, value))
        {
            update_acc_lengths(
                [run_index, count_delta](auto& acc_lengths)
                {
                    shift_acc_length_values(acc_lengths, run_index, count_delta);
                }
            );
            finalize_mutation(refresh_state);
            return;
        }

        const value_type current_run_value = materialize_value(encoded_value(run_index));
        insert_encoded_value(run_index + 1, value);
        insert_encoded_value(run_index + 2, current_run_value);

        update_acc_lengths(
            [run_index, logical_index, insertion_end, shifted_run_end, count_delta](auto& acc_lengths)
            {
                set_acc_length_value(acc_lengths, run_index, logical_index);
                insert_acc_length_value(acc_lengths, run_index + 1, insertion_end);
                insert_acc_length_value(acc_lengths, run_index + 2, shifted_run_end);
                shift_acc_length_values(acc_lengths, run_index + 3, count_delta);
            }
        );
        finalize_mutation(refresh_state);
    }

    void
    run_end_encoded_array::insert_logical_value(size_type index, const value_type& value, bool refresh_state)
    {
        insert_logical_values(index, value, 1, refresh_state);
    }

    void run_end_encoded_array::erase_logical_values(size_type index, size_type count, bool refresh_state)
    {
        throw_if_sliced_for_mutation("run_end_encoded_array::erase");
        if (count == 0)
        {
            return;
        }

        SPARROW_ASSERT_TRUE(index < size());
        SPARROW_ASSERT_TRUE(count <= (size() - index));

        const auto count_delta = to_int64(count);

        const size_type first_run_index = find_run_index(index);
        const size_type last_run_index = (index + count == size()) ? (m_encoded_length - 1)
                                                                   : find_run_index(index + count - 1);
        const std::uint64_t first_run_start = run_start(first_run_index);
        const std::uint64_t last_run_end = run_end(last_run_index);
        const bool keep_left_fragment = index > first_run_start;
        const bool keep_right_fragment = (index + count) < last_run_end;
        const auto logical_index = to_uint64(index);
        const auto shortened_last_run_end = checked_sub_u64(last_run_end, count);
        const auto no_merge_candidate = std::numeric_limits<size_type>::max();
        const auto update_acc_lengths = [this](auto&& updater)
        {
            with_mutable_acc_lengths(p_acc_lengths_array, std::forward<decltype(updater)>(updater));
        };
        const auto erase_runs_and_update_acc_lengths =
            [this, &update_acc_lengths](size_type first_run, size_type run_count, auto&& updater)
        {
            erase_encoded_values(first_run, run_count);
            update_acc_lengths(std::forward<decltype(updater)>(updater));
        };
        const auto refresh_and_merge = [this](size_type merge_candidate)
        {
            refresh_cache();
            if (merge_candidate < m_encoded_length)
            {
                merge_adjacent_runs(merge_candidate);
            }
        };

        if (first_run_index == last_run_index)
        {
            if (keep_left_fragment || keep_right_fragment)
            {
                update_acc_lengths(
                    [first_run_index, shortened_last_run_end, count_delta](auto& acc_lengths)
                    {
                        set_acc_length_value(acc_lengths, first_run_index, shortened_last_run_end);
                        shift_acc_length_values(acc_lengths, first_run_index + 1, -count_delta);
                    }
                );
                finalize_mutation(refresh_state);
                return;
            }

            erase_runs_and_update_acc_lengths(
                first_run_index,
                1,
                [first_run_index, count_delta](auto& acc_lengths)
                {
                    erase_acc_length_values(acc_lengths, first_run_index, 1);
                    shift_acc_length_values(acc_lengths, first_run_index, -count_delta);
                }
            );

            if (first_run_index > 0)
            {
                refresh_and_merge(first_run_index - 1);
            }
            finalize_mutation(refresh_state);
            return;
        }

        size_type merge_candidate = no_merge_candidate;

        if (keep_left_fragment && keep_right_fragment)
        {
            erase_runs_and_update_acc_lengths(
                first_run_index + 1,
                last_run_index - first_run_index - 1,
                [first_run_index, last_run_index, logical_index, shortened_last_run_end, count_delta](
                    auto& acc_lengths
                )
                {
                    set_acc_length_value(acc_lengths, first_run_index, logical_index);
                    set_acc_length_value(acc_lengths, last_run_index, shortened_last_run_end);
                    erase_acc_length_values(acc_lengths, first_run_index + 1, last_run_index - first_run_index - 1);
                    shift_acc_length_values(acc_lengths, first_run_index + 2, -count_delta);
                }
            );
            merge_candidate = first_run_index;
        }
        else if (keep_left_fragment)
        {
            erase_runs_and_update_acc_lengths(
                first_run_index + 1,
                last_run_index - first_run_index,
                [first_run_index, logical_index, last_run_index, count_delta](auto& acc_lengths)
                {
                    set_acc_length_value(acc_lengths, first_run_index, logical_index);
                    erase_acc_length_values(acc_lengths, first_run_index + 1, last_run_index - first_run_index);
                    shift_acc_length_values(acc_lengths, first_run_index + 1, -count_delta);
                }
            );
            merge_candidate = first_run_index;
        }
        else if (keep_right_fragment)
        {
            erase_runs_and_update_acc_lengths(
                first_run_index,
                last_run_index - first_run_index,
                [first_run_index, last_run_index, shortened_last_run_end, count_delta](auto& acc_lengths)
                {
                    set_acc_length_value(acc_lengths, last_run_index, shortened_last_run_end);
                    erase_acc_length_values(acc_lengths, first_run_index, last_run_index - first_run_index);
                    shift_acc_length_values(acc_lengths, first_run_index + 1, -count_delta);
                }
            );
            if (first_run_index > 0)
            {
                merge_candidate = first_run_index - 1;
            }
        }
        else
        {
            erase_runs_and_update_acc_lengths(
                first_run_index,
                last_run_index - first_run_index + 1,
                [first_run_index, last_run_index, count_delta](auto& acc_lengths)
                {
                    erase_acc_length_values(acc_lengths, first_run_index, last_run_index - first_run_index + 1);
                    shift_acc_length_values(acc_lengths, first_run_index, -count_delta);
                }
            );
            if (first_run_index > 0)
            {
                merge_candidate = first_run_index - 1;
            }
        }

        if (merge_candidate != no_merge_candidate)
        {
            refresh_and_merge(merge_candidate);
        }

        finalize_mutation(refresh_state);
    }

    void run_end_encoded_array::erase_logical_value(size_type index, bool refresh_state)
    {
        erase_logical_values(index, 1, refresh_state);
    }

    void run_end_encoded_array::replace_logical_value(size_type index, const value_type& value)
    {
        SPARROW_ASSERT_TRUE(index < size());
        if (materialize_value(static_cast<const self_type&>(*this)[index]) == value)
        {
            return;
        }
        erase_logical_value(index, false);
        insert_logical_value(index, value, true);
    }

    void run_end_encoded_array::refresh_cache()
    {
        m_encoded_length = detail::array_access::get_arrow_proxy(p_acc_lengths_array).length();
        m_acc_lengths = run_end_encoded_array::get_acc_lengths_ptr(p_acc_lengths_array);
        m_proxy.set_length(
            m_encoded_length == 0 ? 0 : static_cast<size_type>(get_acc_length(m_encoded_length - 1))
        );
    }

    void run_end_encoded_array::refresh_after_mutation()
    {
        refresh_cache();
        SPARROW_ASSERT_TRUE(
            m_encoded_length == detail::array_access::get_arrow_proxy(p_encoded_values_array).length()
        );

        const auto [null_count, length] = extract_length_and_null_count(
            p_acc_lengths_array,
            p_encoded_values_array
        );

        SPARROW_ASSERT_TRUE(static_cast<size_type>(length) == size());
        m_proxy.set_null_count(null_count);
    }

    void run_end_encoded_array::finalize_mutation(bool refresh_state)
    {
        if (refresh_state)
        {
            refresh_after_mutation();
        }
        else
        {
            refresh_cache();
        }
    }

    void run_end_encoded_array::throw_if_sliced_for_mutation(const char* operation) const
    {
        if (m_proxy.offset() != 0)
        {
            throw std::logic_error(std::string(operation) + " does not support sliced arrays (non-zero offset)");
        }
    }

    auto run_end_encoded_array::materialize_value(const const_reference& value) -> value_type
    {
        using return_type = value_type;
        using const_reference_base_type = typename const_reference::base_type;

        return std::visit(
            [](const auto& typed_value) -> return_type
            {
                using nullable_type = std::decay_t<decltype(typed_value)>;
                using source_value_type = std::remove_cvref_t<typename nullable_type::value_type>;

                if constexpr (std::same_as<source_value_type, std::string_view>)
                {
                    return return_type(
                        nullable<std::string>(std::string(typed_value.get()), typed_value.has_value())
                    );
                }
                else if constexpr (std::same_as<source_value_type, sequence_view<const byte_t>>)
                {
                    const auto bytes = typed_value.get();
                    return return_type(
                        nullable<std::vector<byte_t>>(
                            std::vector<byte_t>(bytes.begin(), bytes.end()),
                            typed_value.has_value()
                        )
                    );
                }
                else
                {
                    using stored_type = std::remove_cvref_t<decltype(typed_value.get())>;
                    return return_type(
                        nullable<stored_type>(stored_type(typed_value.get()), typed_value.has_value())
                    );
                }
            },
#if SPARROW_GCC_11_2_WORKAROUND
            static_cast<const const_reference_base_type&>(value)
#else
            value
#endif
        );
    }

    std::pair<std::int64_t, std::int64_t>
    run_end_encoded_array::extract_length_and_null_count(const array& acc_lengths_arr, const array& encoded_values_arr)
    {
        SPARROW_ASSERT_TRUE(acc_lengths_arr.size() == encoded_values_arr.size());

        const std::int64_t raw_null_count = detail::array_access::get_arrow_proxy(encoded_values_arr).null_count();
        const auto raw_size = acc_lengths_arr.size();
        return acc_lengths_arr.visit(
            [&](const auto& acc_lengths_array) -> std::pair<std::int64_t, std::int64_t>
            {
                if constexpr (usable_array<std::decay_t<decltype(acc_lengths_array)>>)
                {
                    return extract_length_and_null_count_from_data(
                        acc_lengths_array.data(),
                        raw_size,
                        raw_null_count,
                        [&encoded_values_arr](std::size_t index)
                        {
                            return encoded_values_arr[index].has_value();
                        }
                    );
                }
                else
                {
                    throw std::invalid_argument("array type not supported");
                }
            }
        );
    }

}

#if defined(__cpp_lib_format)

auto std::formatter<sparrow::run_end_encoded_array>::format(
    const sparrow::run_end_encoded_array& ar,
    std::format_context& ctx
) const -> decltype(ctx.out())
{
    std::format_to(ctx.out(), "Run end encoded [size={}] <", ar.size());

    if (ar.empty())
    {
        return std::format_to(ctx.out(), ">");
    }

    std::for_each(
        ar.cbegin(),
        sparrow::next(ar.cbegin(), ar.size() - 1),
        [&ctx](const auto& value)
        {
            std::format_to(ctx.out(), "{}, ", value);
        }
    );

    return std::format_to(ctx.out(), "{}>", ar.back());
}

namespace sparrow
{
    std::ostream& operator<<(std::ostream& os, const run_end_encoded_array& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

#endif
