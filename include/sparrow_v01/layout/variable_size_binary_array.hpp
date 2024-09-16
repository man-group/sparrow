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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or mplied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "sparrow/layout/layout_iterator.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/nullable.hpp"

#include "sparrow_v01/layout/array_base.hpp"

namespace sparrow
{
    template <std::ranges::sized_range T, class CR, layout_offset OT = std::int64_t>
    class variable_size_binary_array;

    /**
     * @class vs_binary_value_iterator
     *
     * @brief Iterator over the data values of a variable size binary
     * layout.
     *
     * @tparam L the layout type
     * @tparam is_const a boolean flag specifying whether this iterator is const.
     */
    template <class L, bool is_const>
    class vs_binary_value_iterator : public iterator_base<
                                         vs_binary_value_iterator<L, is_const>,
                                         mpl::constify_t<typename L::inner_value_type, is_const>,
                                         std::contiguous_iterator_tag,
                                         impl::get_inner_reference_t<L, is_const>>
    {
    public:

        using self_type = vs_binary_value_iterator<L, is_const>;
        using base_type = iterator_base<
            self_type,
            mpl::constify_t<typename L::inner_value_type, is_const>,
            std::contiguous_iterator_tag,
            impl::get_inner_reference_t<L, is_const>>;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;
        using size_type = typename L::size_type;
        using layout_type = mpl::constify_t<L, is_const>;

        vs_binary_value_iterator() noexcept = default;
        vs_binary_value_iterator(layout_type* layout, size_type index);

    private:

        reference dereference() const;
        void increment();
        void decrement();
        void advance(difference_type n);
        difference_type distance_to(const self_type& rhs) const;
        bool equal(const self_type& rhs) const;
        bool less_than(const self_type& rhs) const;

        layout_type* p_layout = nullptr;
        difference_type m_index;

        friend class iterator_access;
    };
      
    /**
     * Implementation of reference to inner type used for layout L
     *
     * @tparam L the layout type
     */
    template <class L>
    class variable_size_binary_reference
    {
    public:

        using self_type = variable_size_binary_reference<L>;
        using data_storage_type = typename L::data_storage_type;
        using value_type = typename L::inner_value_type;
        using reference = typename L::inner_reference;
        using const_reference = typename L::inner_const_reference;
        using size_type = typename L::size_type;
        using difference_type = std::ptrdiff_t;
        using iterator = typename L::data_iterator;
        using const_iterator = typename L::const_data_iterator;
        using offset_type = typename L::offset_type;
        using buffer_type = data_storage_type::buffer_type;

        variable_size_binary_reference(L* layout, size_type index);
        variable_size_binary_reference(const variable_size_binary_reference&) = default;
        variable_size_binary_reference(variable_size_binary_reference&&) = default;

        template <std::ranges::sized_range T>
            requires mpl::convertible_ranges<T, typename L::inner_value_type>
        self_type& operator=(T&& rhs);

        // This is to avoid const char* from begin caught by the previous
        // operator= overload. It would convert const char* to const char[N],
        // including the null-terminating char.
        template <class U = typename L::inner_value_type>
            requires std::assignable_from<U&, const char*>
        self_type& operator=(const char* rhs);

        size_type size() const;

        iterator begin();
        iterator end();

        const_iterator begin() const;
        const_iterator end() const;
        const_iterator cbegin() const;
        const_iterator cend() const;

        template <std::ranges::input_range T>
            requires mpl::convertible_ranges<T, typename L::inner_value_type>
        bool operator==(const T& rhs) const;

        template <class U = typename L::inner_value_type>
            requires std::assignable_from<U&, const char*>
        bool operator==(const char* rhs) const;

        template <std::ranges::input_range T>
            requires mpl::convertible_ranges<T, typename L::inner_value_type>
        auto operator<=>(const T& rhs) const;

        template <class U = typename L::inner_value_type>
            requires std::assignable_from<U&, const char*>
        auto operator<=>(const char* rhs) const;

    private:

        offset_type offset(size_type index) const;
        size_type uoffset(size_type index) const;

        L* p_layout = nullptr;
        size_type m_index = size_type(0);
    };

        template <std::ranges::sized_range T, class CR, layout_offset OT>
    struct array_inner_types<variable_size_binary_array<T, CR, OT>> : array_inner_types_base
    {
        using array_type = variable_size_binary_array<T, CR, OT>;

        using inner_value_type = T;
        using inner_reference = variable_size_binary_reference<array_type>;
        using inner_const_reference = CR;
        using pointer = inner_value_type*;
        using const_pointer = const inner_value_type*;

        using value_iterator = vs_binary_value_iterator<array_type, false>;
        using const_value_iterator = vs_binary_value_iterator<array_type, true>;

        using iterator = layout_iterator<array_type, false>;
        using const_iterator = layout_iterator<array_type, true>;
    };

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    class variable_size_binary_array final : public array_base,
                                             public array_crtp_base<variable_size_binary_array<T, CR, OT>>
    {
    public:

        using self_type = variable_size_binary_array<T, CR, OT>;
        using base_type = array_crtp_base<self_type>;
        using inner_types = array_inner_types<self_type>;
        using inner_value_type = typename inner_types::inner_value_type;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;
        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_reference = typename base_type::bitmap_reference;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;
        using value_type = nullable<inner_value_type>;
        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using pointer = typename inner_types::pointer;
        using const_pointer = typename inner_types::const_pointer;
        using offset_iterator = OT*;
        using const_offset_iterator = const OT*;
        using size_type = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using iterator_tag = std::contiguous_iterator_tag;

        using value_iterator = typename base_type::value_iterator;
        using const_value_iterator = typename base_type::const_value_iterator;

        explicit variable_size_binary_array(arrow_proxy);
        virtual ~variable_size_binary_array() = default;

        using base_type::size;

        reference operator[](size_type i);
        const_reference operator[](size_type i) const;

    private:

        using base_type::bitmap_begin;
        using base_type::bitmap_end;
        using base_type::has_value;
        using base_type::storage;

        offset_iterator offset(size_type i);
        offset_iterator offset_end();
        pointer data();
        const_pointer data() const;

        inner_reference value(size_type i);
        inner_const_reference value(size_type i) const;

        value_iterator value_begin();
        value_iterator value_end();

        const_value_iterator value_cbegin() const;
        const_value_iterator value_cend() const;

        variable_size_binary_array(const variable_size_binary_array&) = default;
        variable_size_binary_array* clone_impl() const override;

        friend class array_crtp_base<self_type>;
    };

    /*********************************************
     * variable_size_binary_array implementation *
     *********************************************/

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    variable_size_binary_array<T, CR, OT>::variable_size_binary_array(arrow_proxy proxy)
        : array_base()
        , base_type(std::move(proxy))
    {
        const data_type type = storage().data_type();
        SPARROW_ASSERT_TRUE(type == data_type::STRING || type == data_type::BINARY);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::operator[](size_type i) -> reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return reference(value(i), has_value(i));
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return reference(value(i), has_value(i));
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::data() -> pointer
    {
        return storage().buffers()[1u].template data<inner_value_type>();
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::data() const -> const_pointer
    {
        return storage().buffers()[1u].template data<const inner_value_type>();
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::offset(size_type i) -> offset_iterator
    {
        storage().buffers()[]
        return buffer_at(storage(), 0u).template data<OT>() + sparrow::offset(storage()) + i;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::value(size_type i) -> inner_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        const long long offset_i = *offset(i);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::value(size_type i) const -> inner_const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return data()[i + static_cast<size_type>(storage().offset())];
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::value_begin() -> value_iterator
    {
        return value_iterator{data() + storage().offset()};
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::value_end() -> value_iterator
    {
        return sparrow::next(value_begin(), size());
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator{data() + storage().offset()};
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::value_cend() const -> const_value_iterator
    {
        return sparrow::next(value_cbegin(), size());
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    variable_size_binary_array<T, CR, OT>* variable_size_binary_array<T, CR, OT>::clone_impl() const
    {
        return new variable_size_binary_array<T, CR, OT>(*this);
    }
}
