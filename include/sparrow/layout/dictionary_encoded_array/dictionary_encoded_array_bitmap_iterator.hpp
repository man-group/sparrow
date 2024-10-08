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

#include "sparrow/utils/iterator.hpp"

#pragma once

namespace sparrow
{
    /**
     * Validity iterator which check the validity of the index, if it's a valid index, it check the value at
     * the index in the sublayout.
     */
    template <typename index_array_type, typename value_array_bitmap_range_type>
    class validity_iterator : public iterator_base<
                                  validity_iterator<index_array_type, value_array_bitmap_range_type>,
                                  bool,
                                  std::random_access_iterator_tag,
                                  bool>
    {
    public:

        using self_type = validity_iterator<index_array_type, value_array_bitmap_range_type>;
        using base_type = iterator_base<self_type, bool, std::random_access_iterator_tag, bool>;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;
        using size_type = size_t;

        validity_iterator() noexcept = default;
        validity_iterator(
            const index_array_type& index_bitmap,
            const value_array_bitmap_range_type& value_bitmap,
            size_type index
        );

    private:

        constexpr reference dereference() const noexcept;
        constexpr void increment();
        constexpr void decrement();
        constexpr void advance(difference_type n);
        constexpr difference_type distance_to(const self_type& rhs) const noexcept;
        constexpr bool equal(const self_type& rhs) const noexcept;
        constexpr bool less_than(const self_type& rhs) const noexcept;

        const index_array_type* m_keys_array;
        value_array_bitmap_range_type m_value_array_bitmap;
        size_type m_index;

        friend class iterator_access;
    };

    /************************************
     * validity_iterator implementation *
     ************************************/

    template <typename index_array_type, typename value_array_bitmap_type>
    validity_iterator<index_array_type, value_array_bitmap_type>::validity_iterator(
        const index_array_type& index_array,
        const value_array_bitmap_type& value_bitmap,
        size_type index
    )
        : m_keys_array(&index_array)
        , m_value_array_bitmap(value_bitmap)
        , m_index(index)
    {
        SPARROW_ASSERT_TRUE(m_index < m_keys_array->size());
    }

    template <typename index_array_type, typename value_array_bitmap_type>
    constexpr auto
    validity_iterator<index_array_type, value_array_bitmap_type>::dereference() const noexcept -> reference
    {
        const auto key = (*m_keys_array)[m_index];
        if(!key.has_value())
        {
            return false;
        }
        const auto key_value = key.value();
        const bool value_validity = m_value_array_bitmap[static_cast<difference_type>(key_value)];
        return value_validity;
    }

    template <typename index_array_type, typename value_array_bitmap_type>
    constexpr void validity_iterator<index_array_type, value_array_bitmap_type>::increment()
    {
        ++m_index;
    }

    template <typename index_array_type, typename value_array_bitmap_type>
    constexpr void validity_iterator<index_array_type, value_array_bitmap_type>::decrement()
    {
        --m_index;
    }

    template <typename index_array_type, typename value_array_bitmap_type>
    constexpr void validity_iterator<index_array_type, value_array_bitmap_type>::advance(difference_type n)
    {
        if (n >= 0)
        {
            m_index += static_cast<size_type>(n);
        }
        else
        {
            SPARROW_ASSERT_TRUE(std::abs(n) <= static_cast<difference_type>(m_index));
            m_index -= static_cast<size_type>(-n);
        }
    }

    template <typename index_array_type, typename value_array_bitmap_type>
    constexpr auto validity_iterator<index_array_type, value_array_bitmap_type>::distance_to(const self_type& rhs
    ) const noexcept -> difference_type
    {
        return rhs.m_index - m_index;
    }

    template <typename index_array_type, typename value_array_bitmap_type>
    constexpr bool
    validity_iterator<index_array_type, value_array_bitmap_type>::equal(const self_type& rhs) const noexcept
    {
        return m_index == rhs.m_index;
    }

    template <typename index_array_type, typename value_array_bitmap_type>
    constexpr bool
    validity_iterator<index_array_type, value_array_bitmap_type>::less_than(const self_type& rhs) const noexcept
    {
        return m_index < rhs.m_index;
    }
}
