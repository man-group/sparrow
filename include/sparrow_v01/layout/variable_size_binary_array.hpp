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

#include <concepts>
#include <cstdint>

#include "sparrow/array/data_type.hpp"
#include "sparrow/buffer/buffer_adaptor.hpp"
#include "sparrow/layout/layout_iterator.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/nullable.hpp"

#include "sparrow_v01/layout/array_base.hpp"

namespace sparrow
{
    template <std::ranges::sized_range T, class CR, layout_offset OT = std::int32_t>
    class variable_size_binary_array;

    template <class L>
    class variable_size_binary_reference;

    template <class L, bool is_const>
    class variable_size_binary_value_iterator;

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    struct array_inner_types<variable_size_binary_array<T, CR, OT>> : array_inner_types_base
    {
        using array_type = variable_size_binary_array<T, CR, OT>;
        using base_type = array_crtp_base<array_type>;

        using inner_value_type = T;
        using inner_reference = variable_size_binary_reference<array_type>;
        using inner_const_reference = CR;
        using offset_type = OT;

        using data_value_type = typename T::value_type;
        using offset_iterator = OT*;
        using const_offset_iterator = const OT*;
        using data_iterator = data_value_type*;
        using const_data_iterator = const data_value_type*;

        using value_iterator = variable_size_binary_value_iterator<array_type, false>;
        using const_value_iterator = variable_size_binary_value_iterator<array_type, true>;

        using iterator = layout_iterator<array_type, false>;
        using const_iterator = layout_iterator<array_type, true>;

        using size_type = typename base_type::size_type;
    };

    /**
     * Iterator over the data values of a variable size binary layout.
     *
     * @tparam L the layout type
     * @tparam is_const a boolean flag specifying whether this iterator is const.
     */
    template <class L, bool is_const>
    class variable_size_binary_value_iterator : public iterator_base<
                                                    variable_size_binary_value_iterator<L, is_const>,
                                                    mpl::constify_t<typename array_inner_types<L>::inner_value_type, is_const>,
                                                    std::contiguous_iterator_tag,
                                                    impl::get_inner_reference_t<array_inner_types<L>, is_const>>
    {
    public:

        using self_type = variable_size_binary_value_iterator<L, is_const>;
        using base_type = iterator_base<
            self_type,
            mpl::constify_t<typename array_inner_types<L>::inner_value_type, is_const>,
            std::contiguous_iterator_tag,
            impl::get_inner_reference_t<array_inner_types<L>, is_const>>;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;
        using size_type = typename array_inner_types<L>::size_type;
        using layout_type = mpl::constify_t<L, is_const>;

        variable_size_binary_value_iterator() noexcept = default;
        variable_size_binary_value_iterator(layout_type* layout, size_type index);

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
        using value_type = typename L::inner_value_type;
        using reference = typename L::inner_reference;
        using const_reference = typename L::inner_const_reference;
        using size_type = typename L::size_type;
        using difference_type = std::ptrdiff_t;
        using iterator = typename L::value_iterator;
        using const_iterator = typename L::const_value_iterator;
        using offset_type = typename L::offset_type;

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

        // iterator begin();
        // iterator end();

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
        using offset_type = typename inner_types::offset_type;
        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_reference = typename base_type::bitmap_reference;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;
        using value_type = nullable<inner_value_type>;
        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using offset_iterator = typename inner_types::offset_iterator;
        using const_offset_iterator = typename inner_types::const_offset_iterator;
        using size_type = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using iterator_tag = std::contiguous_iterator_tag;
        using data_iterator = typename inner_types::data_iterator;
        using const_data_iterator = typename inner_types::const_data_iterator;
        using data_value_type = typename inner_types::data_value_type;

        using value_iterator = typename inner_types::value_iterator;
        using const_value_iterator = typename inner_types::const_value_iterator;

        explicit variable_size_binary_array(arrow_proxy);
        ~variable_size_binary_array() override = default;

        using base_type::size;

        // reference operator[](size_type i);
        const_reference operator[](size_type i) const;

    private:

        static constexpr size_t OFFSET_BUFFER_INDEX = 1;
        static constexpr size_t DATA_BUFFER_INDEX = 2;

        using base_type::bitmap_begin;
        using base_type::bitmap_end;
        using base_type::has_value;
        using base_type::storage;

        offset_iterator offset(size_type i);
        offset_iterator offset_end();
        data_iterator data(size_type i);

        const_offset_iterator offset(size_type i) const;
        const_offset_iterator offset_end() const;
        const_data_iterator data(size_type i) const;

        // template <std::ranges::sized_range U>
        //     requires mpl::convertible_ranges<U, T>
        // void assign(U&& rhs, size_type index);

        // inner_reference value(size_type i);
        inner_const_reference value(size_type i) const;

        // value_iterator value_begin();
        // value_iterator value_end();

        const_value_iterator value_cbegin() const;
        const_value_iterator value_cend() const;

        variable_size_binary_array(const variable_size_binary_array&) = default;
        variable_size_binary_array* clone_impl() const override;

        friend class array_crtp_base<self_type>;
        friend class variable_size_binary_reference<self_type>;
        friend class variable_size_binary_value_iterator<self_type, true>;
    };

    /******************************************************
     * variable_size_binary_value_iterator implementation *
     ******************************************************/

    template <class L, bool is_const>
    variable_size_binary_value_iterator<L, is_const>::variable_size_binary_value_iterator(
        layout_type* layout,
        size_type index
    )
        : p_layout(layout)
        , m_index(static_cast<difference_type>(index))
    {
    }

    template <class L, bool is_const>
    auto variable_size_binary_value_iterator<L, is_const>::dereference() const -> reference
    {
        if constexpr (is_const)
        {
            return p_layout->value(static_cast<size_type>(m_index));
        }
        else
        {
            return reference(p_layout, static_cast<size_type>(m_index));
        }
    }

    template <class L, bool is_const>
    void variable_size_binary_value_iterator<L, is_const>::increment()
    {
        ++m_index;
    }

    template <class L, bool is_const>
    void variable_size_binary_value_iterator<L, is_const>::decrement()
    {
        --m_index;
    }

    template <class L, bool is_const>
    void variable_size_binary_value_iterator<L, is_const>::advance(difference_type n)
    {
        m_index += n;
    }

    template <class L, bool is_const>
    auto
    variable_size_binary_value_iterator<L, is_const>::distance_to(const self_type& rhs) const -> difference_type
    {
        return rhs.m_index - m_index;
    }

    template <class L, bool is_const>
    bool variable_size_binary_value_iterator<L, is_const>::equal(const self_type& rhs) const
    {
        return (p_layout == rhs.p_layout) && (m_index == rhs.m_index);
    }

    template <class L, bool is_const>
    bool variable_size_binary_value_iterator<L, is_const>::less_than(const self_type& rhs) const
    {
        return (p_layout == rhs.p_layout) && (m_index < rhs.m_index);
    }

    /*************************************************
     * variable_size_binary_reference implementation *
     *************************************************/

    template <class L>
    variable_size_binary_reference<L>::variable_size_binary_reference(L* layout, size_type index)
        : p_layout(layout)
        , m_index(index)
    {
    }

    template <class L>
    template <std::ranges::sized_range T>
        requires mpl::convertible_ranges<T, typename L::inner_value_type>
    auto variable_size_binary_reference<L>::operator=(T&& rhs) -> self_type&
    {
        p_layout->assign(std::forward<T>(rhs), m_index);
        return *this;
    }

    template <class L>
    template <class U>
        requires std::assignable_from<U&, const char*>
    auto variable_size_binary_reference<L>::operator=(const char* rhs) -> self_type&
    {
        return *this = std::string_view(rhs);
    }

    template <class L>
    auto variable_size_binary_reference<L>::size() const -> size_type
    {
        return static_cast<size_type>(offset(m_index + 1) - offset(m_index));
    }

    // template <class L>
    // auto variable_size_binary_reference<L>::begin() -> iterator
    // {
    //     return p_layout->data(uoffset(m_index));
    // }

    // template <class L>
    // auto variable_size_binary_reference<L>::end() -> iterator
    // {
    //     return p_layout->data(uoffset(m_index + 1));
    // }

    template <class L>
    auto variable_size_binary_reference<L>::begin() const -> const_iterator
    {
        return cbegin();
    }

    template <class L>
    auto variable_size_binary_reference<L>::end() const -> const_iterator
    {
        return cend();
    }

    template <class L>
    auto variable_size_binary_reference<L>::cbegin() const -> const_iterator
    {
        return p_layout->data(uoffset(m_index));
    }

    template <class L>
    auto variable_size_binary_reference<L>::cend() const -> const_iterator
    {
        return p_layout->data(uoffset(m_index + 1));
    }

    template <class L>
    template <std::ranges::input_range T>
        requires mpl::convertible_ranges<T, typename L::inner_value_type>
    bool variable_size_binary_reference<L>::operator==(const T& rhs) const
    {
        return std::equal(cbegin(), cend(), std::cbegin(rhs), std::cend(rhs));
    }

    template <class L>
    template <class U>
        requires std::assignable_from<U&, const char*>
    bool variable_size_binary_reference<L>::operator==(const char* rhs) const
    {
        return operator==(std::string_view(rhs));
    }

    template <class L>
    template <std::ranges::input_range T>
        requires mpl::convertible_ranges<T, typename L::inner_value_type>
    auto variable_size_binary_reference<L>::operator<=>(const T& rhs) const
    {
        return lexicographical_compare_three_way(*this, rhs);
    }

    template <class L>
    template <class U>
        requires std::assignable_from<U&, const char*>
    auto variable_size_binary_reference<L>::operator<=>(const char* rhs) const
    {
        return operator<=>(std::string_view(rhs));
    }

    template <class L>
    auto variable_size_binary_reference<L>::offset(size_type index) const -> offset_type
    {
        return *(p_layout->offset(index));
    }

    template <class L>
    auto variable_size_binary_reference<L>::uoffset(size_type index) const -> size_type
    {
        return static_cast<size_type>(offset(index));
    }

    /*********************************************
     * variable_size_binary_array implementation *
     *********************************************/

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    variable_size_binary_array<T, CR, OT>::variable_size_binary_array(arrow_proxy proxy)
        : array_base(proxy.data_type())
        , base_type(std::move(proxy))
    {
        const auto type = storage().data_type();
        SPARROW_ASSERT_TRUE(type == data_type::STRING || type == data_type::BINARY);  // TODO: Add
                                                                                      // data_type::LARGE_STRING
                                                                                      // and
                                                                                      // data_type::LARGE_BINARY
        SPARROW_ASSERT_TRUE(
            ((type == data_type::STRING || type == data_type::BINARY) && std::same_as<OT, int32_t>)
        );
    }

    // template <std::ranges::sized_range T, class CR, layout_offset OT>
    // auto variable_size_binary_array<T, CR, OT>::operator[](size_type i) -> reference
    // {
    //     SPARROW_ASSERT_TRUE(i < size());
    //     return reference(value(i), has_value(i));
    // }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return const_reference(value(i), has_value(i));
    }

    // template <std::ranges::sized_range T, class CR, layout_offset OT>
    // auto variable_size_binary_array<T, CR, OT>::data() -> pointer
    // {
    //     return storage().buffers()[DATA_BUFFER_INDEX].template data<inner_value_type>()
    //            + static_cast<size_type>(storage().offset());
    // }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::data(size_type i) -> data_iterator
    {
        SPARROW_ASSERT_FALSE(storage().buffers()[DATA_BUFFER_INDEX].size() == 0u);
        return storage().buffers()[DATA_BUFFER_INDEX].template data<data_value_type>() + i;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::data(size_type i) const -> const_data_iterator
    {
        SPARROW_ASSERT_FALSE(storage().buffers()[DATA_BUFFER_INDEX].size() == 0u);
        return storage().buffers()[DATA_BUFFER_INDEX].template data<const data_value_type>() + i;
    }

    // template <std::ranges::sized_range T, class CR, layout_offset OT>
    // template <std::ranges::sized_range U>
    //     requires mpl::convertible_ranges<U, T>
    // void variable_size_binary_array<T, CR, OT>::assign(U&& rhs, size_type index)
    // {
    //     auto& data_buffer = storage().buffers()[1];
    //     const auto offset_beg = *offset(index);
    //     const auto offset_end = *offset(index + 1);
    //     const auto initial_value_length = offset_end - offset_beg;
    //     const auto new_value_length = static_cast<difference_type>(std::ranges::size(rhs));
    //     const auto shift_val = new_value_length - initial_value_length;

    //     if (shift_val != 0)
    //     {
    //         const auto shift_val_abs = static_cast<size_type>(std::abs(shift_val));
    //         const auto layout_data_length = size();
    //         const auto new_data_buffer_size = shift_val < 0 ? data_buffer.size() - shift_val_abs
    //                                                         : data_buffer.size() + shift_val_abs;
    //         data_buffer.resize(new_data_buffer_size);
    //         // Move elements to make space for the new value
    //         std::move_backward(data_buffer.begin() + offset_end, data_buffer.end() - shift_val,
    //         data_buffer.end());
    //         // Adjust offsets for subsequent elements
    //         std::for_each(
    //             std::execution::unseq,
    //             offset(index + 1),
    //             offset(layout_data_length + 1),
    //             [shift_val](auto& offset)
    //             {
    //                 offset += shift_val;
    //             }
    //         );
    //     }
    //     // Copy the new value into the buffer
    //     std::copy(std::ranges::begin(rhs), std::ranges::end(rhs), data_buffer.begin() + offset_beg);
    // }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::offset(size_type i) -> offset_iterator
    {
        SPARROW_ASSERT_TRUE(i < size() + storage().offset());
        return storage().buffers()[OFFSET_BUFFER_INDEX].template data<OT>()
               + static_cast<size_type>(storage().offset()) + i;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::offset(size_type i) const -> const_offset_iterator
    {
        SPARROW_ASSERT_TRUE(i < size() + storage().offset());
        return storage().buffers()[OFFSET_BUFFER_INDEX].template data<OT>()
               + static_cast<size_type>(storage().offset()) + i;
    }

    // template <std::ranges::sized_range T, class CR, layout_offset OT>
    // auto variable_size_binary_array<T, CR, OT>::value(size_type i) -> inner_reference
    // {
    //     SPARROW_ASSERT_TRUE(i < size());
    //     return storage().buffers()[OFFSET_BUFFER_INDEX].template data<OT>()
    //            + static_cast<size_type>(storage().offset()) + i;
    // }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::value(size_type i) const -> inner_const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        const OT offset_begin = *offset(i);
        SPARROW_ASSERT_TRUE(offset_begin >= 0);
        const OT offset_end = *offset(i + 1);
        SPARROW_ASSERT_TRUE(offset_end >= 0);
        const const_data_iterator pointer_begin = data(static_cast<size_type>(offset_begin));
        const const_data_iterator pointer_end = data(static_cast<size_type>(offset_end));
        return inner_const_reference(pointer_begin, pointer_end);
    }

    // template <std::ranges::sized_range T, class CR, layout_offset OT>
    // auto variable_size_binary_array<T, CR, OT>::value_begin() -> value_iterator
    // {
    //     return value_iterator{data()};
    // }

    // template <std::ranges::sized_range T, class CR, layout_offset OT>
    // auto variable_size_binary_array<T, CR, OT>::value_end() -> value_iterator
    // {
    //     return sparrow::next(value_begin(), size());
    // }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator{this, 0};
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
