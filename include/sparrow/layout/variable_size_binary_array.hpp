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
#include <iterator>
#include <numeric>
#include <ranges>
#include <string>

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/buffer/buffer_adaptor.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/layout_iterator.hpp"
#include "sparrow/types/data_type.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/repeat_container.hpp"

namespace sparrow
{
    template <std::ranges::sized_range T, class CR, layout_offset OT = std::int32_t>
    class variable_size_binary_array;

    template <class L>
    class variable_size_binary_reference;

    template <class Layout, iterator_types Iterator_types>
    class variable_size_binary_value_iterator;

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    struct array_inner_types<variable_size_binary_array<T, CR, OT>> : array_inner_types_base
    {
        using array_type = variable_size_binary_array<T, CR, OT>;

        using inner_value_type = T;
        using inner_reference = variable_size_binary_reference<array_type>;
        using inner_const_reference = CR;
        using offset_type = OT;

        using data_value_type = typename T::value_type;

        using offset_iterator = OT*;
        using const_offset_iterator = const OT*;

        using data_iterator = data_value_type*;
        using const_data_iterator = const data_value_type*;

        using iterator_tag = std::random_access_iterator_tag;

        using const_bitmap_iterator = bitmap_type::const_iterator;

        struct iterator_types
        {
            using value_type = inner_value_type;
            using reference = inner_reference;
            using value_iterator = data_iterator;
            using bitmap_iterator = bitmap_type::iterator;
            using iterator_tag = array_inner_types<variable_size_binary_array<T, CR, OT>>::iterator_tag;
        };

        using value_iterator = variable_size_binary_value_iterator<array_type, iterator_types>;

        struct const_iterator_types
        {
            using value_type = inner_value_type;
            using reference = inner_const_reference;
            using value_iterator = const_data_iterator;
            using bitmap_iterator = const_bitmap_iterator;
            using iterator_tag = array_inner_types<variable_size_binary_array<T, CR, OT>>::iterator_tag;
        };

        using const_value_iterator = variable_size_binary_value_iterator<array_type, const_iterator_types>;

        // using iterator = layout_iterator<array_type, false>;
        // using const_iterator = layout_iterator<array_type, true, CR>;
    };

    /**
     * Iterator over the data values of a variable size binary layout.
     *
     * @tparam L the layout type
     * @tparam is_const a boolean flag specifying whether this iterator is const.
     */
    template <class Layout, iterator_types Iterator_types>
    class variable_size_binary_value_iterator
        : public iterator_base<
              variable_size_binary_value_iterator<Layout, Iterator_types>,
              typename Iterator_types::value_type,
              typename Iterator_types::iterator_tag,
              typename Iterator_types::reference>
    {
    public:

        using self_type = variable_size_binary_value_iterator<Layout, Iterator_types>;
        using base_type = iterator_base<
            self_type,
            typename Iterator_types::value_type,
            typename Iterator_types::iterator_tag,
            typename Iterator_types::reference>;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;
        using layout_type = mpl::constify_t<Layout, true>;
        using size_type = size_t;
        using value_type = base_type::value_type;

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
        using iterator = typename L::data_iterator;
        using const_iterator = typename L::const_data_iterator;
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
}

namespace std
{
    template <typename Layout, template <typename> typename TQual, template <typename> typename UQual>
    struct basic_common_reference<sparrow::variable_size_binary_reference<Layout>, std::string, TQual, UQual>
    {
        using type = std::string;
    };

    template <typename Layout, template <typename> typename TQual, template <class> class UQual>
    struct basic_common_reference<std::string, sparrow::variable_size_binary_reference<Layout>, TQual, UQual>
    {
        using type = std::string;
    };
}

namespace sparrow
{

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    class variable_size_binary_array final
        : public mutable_array_bitmap_base<variable_size_binary_array<T, CR, OT>>
    {
    public:

        using self_type = variable_size_binary_array<T, CR, OT>;
        using base_type = mutable_array_bitmap_base<self_type>;

        using inner_types = array_inner_types<self_type>;
        using inner_value_type = typename inner_types::inner_value_type;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;

        using offset_type = typename inner_types::offset_type;

        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_reference = typename base_type::bitmap_reference;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;
        using const_bitmap_range = typename base_type::const_bitmap_range;

        using value_type = nullable<inner_value_type>;
        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;

        using offset_iterator = typename inner_types::offset_iterator;
        using const_offset_iterator = typename inner_types::const_offset_iterator;

        using size_type = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using iterator_tag = typename base_type::iterator_tag;
        using data_iterator = typename inner_types::data_iterator;

        using const_data_iterator = typename inner_types::const_data_iterator;
        using data_value_type = typename inner_types::data_value_type;

        using value_iterator = typename inner_types::value_iterator;
        using const_value_iterator = typename inner_types::const_value_iterator;

        explicit variable_size_binary_array(arrow_proxy);

        using base_type::get_arrow_proxy;
        using base_type::size;

        inner_reference value(size_type i);
        inner_const_reference value(size_type i) const;

        value_iterator value_begin();
        value_iterator value_end();

        const_value_iterator value_cbegin() const;
        const_value_iterator value_cend() const;

    private:

        static constexpr size_t OFFSET_BUFFER_INDEX = 1;
        static constexpr size_t DATA_BUFFER_INDEX = 2;

        offset_iterator offset(size_type i);
        offset_iterator offsets_begin();
        offset_iterator offsets_end();
        data_iterator data(size_type i);

        const_offset_iterator offset(size_type i) const;
        const_offset_iterator offsets_cbegin() const;
        const_offset_iterator offsets_cend() const;
        const_data_iterator data(size_type i) const;

        // Modifiers

        template <std::ranges::sized_range U>
            requires mpl::convertible_ranges<U, T>
        void resize_values(size_type new_length, U value);

        void resize_offsets(size_type new_length, offset_type offset_value);

        template <std::ranges::sized_range U>
            requires mpl::convertible_ranges<U, T>
        value_iterator insert_value(const_value_iterator pos, U value, size_type count);

        offset_iterator insert_offset(const_offset_iterator pos, offset_type size, size_type count);

        template <mpl::iterator_of_type<T> InputIt>
        value_iterator insert_values(const_value_iterator pos, InputIt first, InputIt last);

        template <mpl::iterator_of_type<OT> InputIt>
        offset_iterator insert_offsets(const_offset_iterator pos, InputIt first, InputIt last);

        value_iterator erase_values(const_value_iterator pos, size_type count);

        offset_iterator erase_offsets(const_offset_iterator pos, size_type count);

        template <std::ranges::sized_range U>
            requires mpl::convertible_ranges<U, T>
        void assign(U&& rhs, size_type index);

        friend class variable_size_binary_reference<self_type>;
        friend const_value_iterator;
        friend base_type;
        friend base_type::base_type;
        friend base_type::base_type::base_type;
    };

    /******************************************************
     * variable_size_binary_value_iterator implementation *
     ******************************************************/

    template <class Layout, iterator_types Iterator_types>
    variable_size_binary_value_iterator<Layout, Iterator_types>::variable_size_binary_value_iterator(
        layout_type* layout,
        size_type index
    )
        : p_layout(layout)
        , m_index(static_cast<difference_type>(index))
    {
    }

    template <class Layout, iterator_types Iterator_types>
    auto variable_size_binary_value_iterator<Layout, Iterator_types>::dereference() const -> reference
    {
        if constexpr (std::same_as<reference, typename Layout::inner_const_reference>)
        {
            return p_layout->value(static_cast<size_type>(m_index));
        }
        else
        {
            return reference(const_cast<Layout*>(p_layout), static_cast<size_type>(m_index));
        }
    }

    template <class Layout, iterator_types Iterator_types>
    void variable_size_binary_value_iterator<Layout, Iterator_types>::increment()
    {
        ++m_index;
    }

    template <class Layout, iterator_types Iterator_types>
    void variable_size_binary_value_iterator<Layout, Iterator_types>::decrement()
    {
        --m_index;
    }

    template <class Layout, iterator_types Iterator_types>
    void variable_size_binary_value_iterator<Layout, Iterator_types>::advance(difference_type n)
    {
        m_index += n;
    }

    template <class Layout, iterator_types Iterator_types>
    auto variable_size_binary_value_iterator<Layout, Iterator_types>::distance_to(const self_type& rhs
    ) const -> difference_type
    {
        return rhs.m_index - m_index;
    }

    template <class Layout, iterator_types Iterator_types>
    bool variable_size_binary_value_iterator<Layout, Iterator_types>::equal(const self_type& rhs) const
    {
        return (p_layout == rhs.p_layout) && (m_index == rhs.m_index);
    }

    template <class Layout, iterator_types Iterator_types>
    bool variable_size_binary_value_iterator<Layout, Iterator_types>::less_than(const self_type& rhs) const
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
        p_layout->get_arrow_proxy().update_buffers();
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

    template <class L>
    auto variable_size_binary_reference<L>::begin() -> iterator
    {
        return iterator(p_layout->data(uoffset(m_index)));
    }

    template <class L>
    auto variable_size_binary_reference<L>::end() -> iterator
    {
        return iterator(p_layout->data(uoffset(m_index + 1)));
    }

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
        return const_iterator(p_layout->data(uoffset(m_index)));
    }

    template <class L>
    auto variable_size_binary_reference<L>::cend() const -> const_iterator
    {
        return const_iterator(p_layout->data(uoffset(m_index + 1)));
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
        : base_type(std::move(proxy))
    {
        const auto type = this->get_arrow_proxy().data_type();
        SPARROW_ASSERT_TRUE(type == data_type::STRING || type == data_type::BINARY);  // TODO: Add
                                                                                      // data_type::LARGE_STRING
                                                                                      // and
                                                                                      // data_type::LARGE_BINARY
        SPARROW_ASSERT_TRUE(
            ((type == data_type::STRING || type == data_type::BINARY) && std::same_as<OT, int32_t>)
        );
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::data(size_type i) -> data_iterator
    {
        arrow_proxy& proxy = get_arrow_proxy();
        SPARROW_ASSERT_TRUE(proxy.buffers()[DATA_BUFFER_INDEX].size() >= i);
        return proxy.buffers()[DATA_BUFFER_INDEX].template data<data_value_type>() + i;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::data(size_type i) const -> const_data_iterator
    {
        const arrow_proxy& proxy = this->get_arrow_proxy();
        SPARROW_ASSERT_TRUE(proxy.buffers()[DATA_BUFFER_INDEX].size() >= i);
        return proxy.buffers()[DATA_BUFFER_INDEX].template data<const data_value_type>() + i;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    template <std::ranges::sized_range U>
        requires mpl::convertible_ranges<U, T>
    void variable_size_binary_array<T, CR, OT>::assign(U&& rhs, size_type index)
    {
        const auto offset_beg = *offset(index);
        const auto offset_end = *offset(index + 1);
        const auto initial_value_length = offset_end - offset_beg;
        const auto new_value_length = static_cast<OT>(std::ranges::size(rhs));
        const OT shift_byte_count = new_value_length - initial_value_length;
        auto& data_buffer = this->get_arrow_proxy().get_array_private_data()->buffers()[DATA_BUFFER_INDEX];
        if (shift_byte_count != 0)
        {
            const auto shift_val_abs = static_cast<size_type>(std::abs(shift_byte_count));
            const auto new_data_buffer_size = shift_byte_count < 0 ? data_buffer.size() - shift_val_abs
                                                                   : data_buffer.size() + shift_val_abs;
            data_buffer.resize(new_data_buffer_size);
            // Move elements to make space for the new value
            std::move_backward(
                data_buffer.begin() + offset_end,
                data_buffer.end() - shift_byte_count,
                data_buffer.end()
            );
            // Adjust offsets for subsequent elements
            std::for_each(
                offset(index + 1),
                offset(size() + 1),
                [shift_byte_count](auto& offset)
                {
                    offset += shift_byte_count;
                }
            );
        }
        // Copy the new value into the buffer
        std::copy(std::ranges::begin(rhs), std::ranges::end(rhs), data_buffer.begin() + offset_beg);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::offset(size_type i) -> offset_iterator
    {
        SPARROW_ASSERT_TRUE(i <= size() + this->get_arrow_proxy().offset());
        return get_arrow_proxy().buffers()[OFFSET_BUFFER_INDEX].template data<OT>()
               + static_cast<size_type>(this->get_arrow_proxy().offset()) + i;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::offset(size_type i) const -> const_offset_iterator
    {
        SPARROW_ASSERT_TRUE(i <= this->size() + this->get_arrow_proxy().offset());
        return this->get_arrow_proxy().buffers()[OFFSET_BUFFER_INDEX].template data<OT>()
               + static_cast<size_type>(this->get_arrow_proxy().offset()) + i;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::offsets_begin() -> offset_iterator
    {
        return offset(0);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::offsets_cbegin() const -> const_offset_iterator
    {
        return offset(0);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::offsets_end() -> offset_iterator
    {
        return offset(size() + 1);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::offsets_cend() const -> const_offset_iterator
    {
        return offset(size() + 1);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::value(size_type i) -> inner_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return inner_reference(this, i);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::value(size_type i) const -> inner_const_reference
    {
        SPARROW_ASSERT_TRUE(i < this->size());
        const OT offset_begin = *offset(i);
        SPARROW_ASSERT_TRUE(offset_begin >= 0);
        const OT offset_end = *offset(i + 1);
        SPARROW_ASSERT_TRUE(offset_end >= 0);
        const const_data_iterator pointer_begin = data(static_cast<size_type>(offset_begin));
        const const_data_iterator pointer_end = data(static_cast<size_type>(offset_end));
        return inner_const_reference(pointer_begin, pointer_end);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::value_begin() -> value_iterator
    {
        return value_iterator{this, 0};
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::value_end() -> value_iterator
    {
        return sparrow::next(value_begin(), size());
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator{this, 0};
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::value_cend() const -> const_value_iterator
    {
        return sparrow::next(value_cbegin(), this->size());
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    template <std::ranges::sized_range U>
        requires mpl::convertible_ranges<U, T>
    void variable_size_binary_array<T, CR, OT>::resize_values(size_type new_length, U value)
    {
        const size_t new_size = new_length + static_cast<size_t>(this->get_arrow_proxy().offset());
        auto& buffers = this->get_arrow_proxy().get_array_private_data()->buffers();
        if (new_length < size())
        {
            const auto offset_begin = static_cast<size_t>(*offset(new_length));
            auto& data_buffer = buffers[DATA_BUFFER_INDEX];
            data_buffer.resize(offset_begin);
            auto& offset_buffer = buffers[OFFSET_BUFFER_INDEX];
            auto offset_buffer_adaptor = make_buffer_adaptor<OT>(offset_buffer);
            offset_buffer_adaptor.resize(new_size + 1);
        }
        else if (new_length > size())
        {
            insert_value(value_cend(), value, new_length - size());
        }
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    template <std::ranges::sized_range U>
        requires mpl::convertible_ranges<U, T>
    auto variable_size_binary_array<T, CR, OT>::insert_value(const_value_iterator pos, U value, size_type count)
        -> value_iterator
    {
        const auto idx = static_cast<size_t>(std::distance(value_cbegin(), pos));
        const OT offset_begin = *offset(idx);
        const std::vector<uint8_t> casted_value{value.cbegin(), value.cend()};
        const repeat_view<std::vector<uint8_t>> my_repeat_view{casted_value, count};
        const auto joined_repeated_value_range = std::ranges::views::join(my_repeat_view);
        auto& data_buffer = this->get_arrow_proxy().get_array_private_data()->buffers()[DATA_BUFFER_INDEX];
        const auto pos_to_insert = sparrow::next(data_buffer.cbegin(), offset_begin);
        data_buffer.insert(pos_to_insert, joined_repeated_value_range.begin(), joined_repeated_value_range.end());
        insert_offset(offsets_cbegin() + idx + 1, static_cast<offset_type>(value.size()), count);
        return sparrow::next(value_begin(), idx);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array<T, CR, OT>::insert_offset(
        const_offset_iterator pos,
        offset_type value_size,
        size_type count
    ) -> offset_iterator
    {
        auto& offset_buffer = get_arrow_proxy().get_array_private_data()->buffers()[OFFSET_BUFFER_INDEX];
        const auto idx = static_cast<size_t>(std::distance(offsets_cbegin(), pos));
        auto offset_buffer_adaptor = make_buffer_adaptor<OT>(offset_buffer);
        const offset_type cumulative_size = value_size * static_cast<offset_type>(count);
        // Adjust offsets for subsequent elements
        std::for_each(
            sparrow::next(offset_buffer_adaptor.begin(), idx + 1),
            offset_buffer_adaptor.end(),
            [cumulative_size](auto& offset)
            {
                offset += cumulative_size;
            }
        );
        offset_buffer_adaptor.insert(sparrow::next(offset_buffer_adaptor.cbegin(), idx + 1), count, 0);
        // Put the right values in the new offsets
        for (size_t i = idx + 1; i < idx + 1 + count; ++i)
        {
            offset_buffer_adaptor[i] = offset_buffer_adaptor[i - 1] + value_size;
        }
        return offsets_begin() + idx;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    template <mpl::iterator_of_type<T> InputIt>
    auto
    variable_size_binary_array<T, CR, OT>::insert_values(const_value_iterator pos, InputIt first, InputIt last)
        -> value_iterator
    {
        auto& data_buffer = get_arrow_proxy().get_array_private_data()->buffers()[DATA_BUFFER_INDEX];
        const auto idx = static_cast<size_t>(std::distance(value_cbegin(), pos));
        const OT offset_begin = *offset(idx);
        auto values = std::ranges::subrange(first, last);
        auto joined_range = values | std::ranges::views::join;
        auto casted_joined_range = joined_range
                                   | std::views::transform(
                                       [](char c) -> uint8_t
                                       {
                                           return static_cast<uint8_t>(c);
                                       }
                                   );
        const auto insert_pos = sparrow::next(data_buffer.cbegin(), offset_begin);
        std::vector<uint8_t> casted_values;
        std::ranges::copy(casted_joined_range, std::back_inserter(casted_values));
        data_buffer.insert(insert_pos, casted_values.begin(), casted_values.end());
        const auto sizes_of_each_value = std::ranges::views::transform(
            values,
            [](const T& value) -> offset_type
            {
                return static_cast<offset_type>(value.size());
            }
        );
        insert_offsets(
            offset(idx + 1),
            sizes_of_each_value.begin(),
            sizes_of_each_value.end()
        );
        return sparrow::next(value_begin(), idx);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    template <mpl::iterator_of_type<OT> InputIt>
    auto variable_size_binary_array<T, CR, OT>::insert_offsets(
        const_offset_iterator pos,
        InputIt first_sizes,
        InputIt last_sizes
    ) -> offset_iterator
    {
        SPARROW_ASSERT_TRUE(pos >= offsets_cbegin());
        SPARROW_ASSERT_TRUE(pos <= offsets_cend());
        SPARROW_ASSERT_TRUE(first_sizes <= last_sizes);
        auto& offset_buffer = get_arrow_proxy().get_array_private_data()->buffers()[OFFSET_BUFFER_INDEX];
        auto offset_buffer_adaptor = make_buffer_adaptor<OT>(offset_buffer);
        const auto idx = std::distance(offsets_cbegin(), pos);
        const OT cumulative_sizes = std::reduce(first_sizes, last_sizes, OT(0));
        const auto sizes_count = std::distance(first_sizes, last_sizes);
        offset_buffer_adaptor.resize(offset_buffer_adaptor.size() + static_cast<size_t>(sizes_count));
        // Move the offsets to make space for the new offsets
        std::move_backward(
            offset_buffer_adaptor.begin() + idx,
            offset_buffer_adaptor.end() - sizes_count,
            offset_buffer_adaptor.end()
        );
        // Adjust offsets for subsequent elements
        std::for_each(
            offset_buffer_adaptor.begin() + idx + sizes_count,
            offset_buffer_adaptor.end(),
            [cumulative_sizes](auto& offset)
            {
                offset += cumulative_sizes;
            }
        );
        // Put the right values in the new offsets
        InputIt it = first_sizes;
        for (size_t i = static_cast<size_t>(idx + 1); i < static_cast<size_t>(idx + sizes_count + 1); ++i)
        {
            offset_buffer_adaptor[i] = offset_buffer_adaptor[i - 1] + *it;
            ++it;
        }
        return offset(static_cast<size_t>(idx));
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto
    variable_size_binary_array<T, CR, OT>::erase_values(const_value_iterator pos, size_type count) -> value_iterator
    {
        SPARROW_ASSERT_TRUE(pos >= value_cbegin());
        SPARROW_ASSERT_TRUE(pos <= value_cend());
        const size_t index = static_cast<size_t>(std::distance(value_cbegin(), pos));
        if (count == 0)
        {
            return sparrow::next(value_begin(), index);
        }
        auto& data_buffer = get_arrow_proxy().get_array_private_data()->buffers()[DATA_BUFFER_INDEX];
        const auto offset_begin = *offset(index);
        const auto offset_end = *offset(index + count);
        const size_t difference = static_cast<size_t>(offset_end - offset_begin);
        // move the values after the erased ones
        std::move(data_buffer.begin() + offset_end, data_buffer.end(), data_buffer.begin() + offset_begin);
        data_buffer.resize(data_buffer.size() - difference);
        // adjust the offsets for the subsequent elements
        erase_offsets(offset(index), count);
        return sparrow::next(value_begin(), index);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto
    variable_size_binary_array<T, CR, OT>::erase_offsets(const_offset_iterator pos, size_type count) -> offset_iterator
    {
        SPARROW_ASSERT_TRUE(pos >= offsets_cbegin());
        SPARROW_ASSERT_TRUE(pos <= offsets_cend());
        const size_t index = static_cast<size_t>(std::distance(offsets_cbegin(), pos));
        if (count == 0)
        {
            return offset(index);
        }
        auto& offset_buffer = get_arrow_proxy().get_array_private_data()->buffers()[OFFSET_BUFFER_INDEX];
        auto offset_buffer_adaptor = make_buffer_adaptor<OT>(offset_buffer);
        const OT offset_start_value = *offset(index);
        const OT offset_end_value = *offset(index + count);
        const OT difference = offset_end_value - offset_start_value;
        // move the offsets after the erased ones
        std::move(
            sparrow::next(offset_buffer_adaptor.begin(), index + count + 1),
            offset_buffer_adaptor.end(),
            sparrow::next(offset_buffer_adaptor.begin(), index + 1)
        );
        offset_buffer_adaptor.resize(offset_buffer_adaptor.size() - count);
        // adjust the offsets for the subsequent elements
        std::for_each(
            sparrow::next(offset_buffer_adaptor.begin(), index + 1),
            offset_buffer_adaptor.end(),
            [difference](OT& offset)
            {
                offset -= difference;
            }
        );
        return offset(index);
    }

}
