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

#include "sparrow/layout/run_end_encoded_layout/run_end_encoded_array.hpp"

#include "sparrow/array.hpp"
#include "sparrow/layout/array_helper.hpp"
#include "sparrow/layout/dispatch.hpp"
#include "sparrow/layout/primitive_layout/primitive_array.hpp"
#include "sparrow/utils/repeat_container.hpp"

namespace sparrow
{
    run_end_encoded_array::run_end_encoded_array(arrow_proxy proxy)
        : m_proxy(std::move(proxy))
        , m_encoded_length(m_proxy.children()[0].length())
        , p_acc_lengths_array(array_factory(m_proxy.children()[0].view()))
        , p_encoded_values_array(array_factory(m_proxy.children()[1].view()))
        , m_acc_lengths(run_end_encoded_array::get_acc_lengths_ptr(*p_acc_lengths_array))
    {
    }

    run_end_encoded_array::run_end_encoded_array(const self_type& rhs)
        : run_end_encoded_array(rhs.m_proxy)
    {
    }

    auto run_end_encoded_array::operator=(const self_type& rhs) -> self_type&
    {
        if (this != &rhs)
        {
            m_proxy = rhs.m_proxy;
            m_encoded_length = rhs.m_encoded_length;
            p_acc_lengths_array = array_factory(m_proxy.children()[0].view());
            p_encoded_values_array = array_factory(m_proxy.children()[1].view());
            m_acc_lengths = run_end_encoded_array::get_acc_lengths_ptr(*p_acc_lengths_array);
        }
        return *this;
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

    auto run_end_encoded_array::get_run_length(std::uint64_t run_index) const -> std::uint64_t
    {
        auto ret = std::visit(
            [run_index](auto&& acc_lengths_ptr) -> std::uint64_t
            {
                if (run_index == 0)
                {
                    return static_cast<std::uint64_t>(acc_lengths_ptr[run_index]);
                }
                else
                {
                    return static_cast<std::uint64_t>(
                        acc_lengths_ptr[run_index] - acc_lengths_ptr[run_index - 1]
                    );
                }
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

    auto run_end_encoded_array::operator[](std::uint64_t i) -> array_traits::const_reference
    {
        return static_cast<const run_end_encoded_array*>(this)->operator[](i);
    }

    auto run_end_encoded_array::begin() -> iterator
    {
        return iterator(this, 0, 0);
    }

    auto run_end_encoded_array::end() -> iterator
    {
        return iterator(this, size(), 0);
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
        return const_iterator(this, size(), 0);
    }

    auto run_end_encoded_array::front() const -> array_traits::const_reference
    {
        return operator[](0);
    }

    auto run_end_encoded_array::back() const -> array_traits::const_reference
    {
        return operator[](size() - 1);
    }

    bool operator==(const run_end_encoded_array& lhs, const run_end_encoded_array& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }

    template <class T>
    concept usable_array = (mpl::is_type_instance_of_v<T, primitive_array>
                            || mpl::is_type_instance_of_v<T, primitive_array_impl>)
                           && (std::same_as<typename T::inner_value_type, std::int16_t>
                               || std::same_as<typename T::inner_value_type, std::int32_t>
                               || std::same_as<typename T::inner_value_type, std::int64_t>);

    auto run_end_encoded_array::get_acc_lengths_ptr(const array_wrapper& ar) -> acc_length_ptr_variant_type
    {
        return visit(
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
            },
            ar
        );
    }

    auto run_end_encoded_array::operator[](std::uint64_t i) const -> array_traits::const_reference
    {
        return visit(
            [i, this](const auto& acc_lengths_ptr) -> array_traits::const_reference
            {
                const auto it = std::upper_bound(acc_lengths_ptr, acc_lengths_ptr + this->m_encoded_length, i);
                // std::lower_bound returns an iterator, so we need to convert it to an index
                const auto index = static_cast<std::size_t>(std::distance(acc_lengths_ptr, it));
                return array_element(*p_encoded_values_array, static_cast<std::size_t>(index));
            },
            m_acc_lengths
        );
    }

    std::pair<std::int64_t, std::int64_t>
    run_end_encoded_array::extract_length_and_null_count(const array& acc_lengths_arr, const array& encoded_values_arr)
    {
        SPARROW_ASSERT_TRUE(acc_lengths_arr.size() == encoded_values_arr.size());

        // get the raw null count
        std::uint64_t raw_null_count = detail::array_access::get_arrow_proxy(acc_lengths_arr).null_count();
        auto raw_size = acc_lengths_arr.size();
        // visit the acc_lengths array
        std::int64_t length = 0;
        std::int64_t null_count = 0;
        acc_lengths_arr.visit(
            [&](const auto& acc_lengths_array)
            {
                if constexpr (usable_array<std::decay_t<decltype(acc_lengths_array)>>)
                {
                    if (acc_lengths_array.size() == 0)
                    {
                        return;
                    }
                    auto acc_length_data = acc_lengths_array.data();
                    // get the length of the array (ie last element in the acc_lengths array)
                    length = acc_length_data[raw_size - 1];

                    if (raw_null_count == 0)
                    {
                        return;
                    }
                    for (std::size_t i = 0; i < raw_size; ++i)
                    {
                        // check if the value is null
                        if (!encoded_values_arr[i].has_value())
                        {
                            // how often is this value repeated?
                            const auto run_length = i == 0 ? acc_length_data[i]
                                                           : acc_length_data[i] - acc_length_data[i - 1];
                            null_count += run_length;
                            raw_null_count -= 1;
                            if (raw_null_count == 0)
                            {
                                return;
                            }
                        }
                    }
                }
                else
                {
                    throw std::invalid_argument("array type not supported");
                }
            }
        );
        return {null_count, length};
    };
}

#if defined(__cpp_lib_format)

auto std::formatter<sparrow::run_end_encoded_array>::format(
    const sparrow::run_end_encoded_array& ar,
    std::format_context& ctx
) const -> decltype(ctx.out())
{
    std::format_to(ctx.out(), "Run end encoded [size={}] <", ar.size());

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
    std::ostream& operator<<(std::ostream& os, const sparrow::run_end_encoded_array& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

#endif
