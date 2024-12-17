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

#pragma once

#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    template <class B>
    class bitset_reference;

    /**
     * @class bitset_iterator
     *
     * Iterator used to iterate over the bits of a dynamic
     * bitset as if they were addressable values.
     *
     * @tparam B the dynamic bitset this iterator operates on
     * @tparam is_const a boolean indicating whether this is
     * a const iterator.
     */
    template <class B, bool is_const>
    class bitset_iterator : public iterator_base<
                                bitset_iterator<B, is_const>,
                                mpl::constify_t<typename B::value_type, is_const>,
                                std::random_access_iterator_tag,
                                std::conditional_t<is_const, bool, bitset_reference<B>>>
    {
    public:

        using self_type = bitset_iterator<B, is_const>;
        using base_type = iterator_base<
            self_type,
            mpl::constify_t<typename B::value_type, is_const>,
            std::contiguous_iterator_tag,
            std::conditional_t<is_const, bool, bitset_reference<B>>>;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;

        using block_type = mpl::constify_t<typename B::block_type, is_const>;
        using bitset_type = mpl::constify_t<B, is_const>;
        using size_type = typename B::size_type;

        constexpr bitset_iterator() noexcept = default;
        constexpr bitset_iterator(bitset_type* bitset, block_type* block, size_type index);

    private:

        constexpr reference dereference() const noexcept;
        constexpr void increment();
        constexpr void decrement();
        constexpr void advance(difference_type n);
        constexpr difference_type distance_to(const self_type& rhs) const noexcept;
        constexpr bool equal(const self_type& rhs) const noexcept;
        constexpr bool less_than(const self_type& rhs) const noexcept;

        constexpr bool is_first_bit_of_block(size_type index) const noexcept;
        constexpr difference_type distance_to_begin() const;

        bitset_type* p_bitset = nullptr;
        block_type* p_block = nullptr;
        // m_index is block-local index.
        // Invariant: m_index < bitset_type::s_bits_per_block
        size_type m_index;

        friend class iterator_access;
    };

    template <class B, bool is_const>
    constexpr bitset_iterator<B, is_const>::bitset_iterator(bitset_type* bitset, block_type* block, size_type index)
        : p_bitset(bitset)
        , p_block(block)
        , m_index(index)
    {
        SPARROW_ASSERT_TRUE(m_index < bitset_type::s_bits_per_block);
    }

    template <class B, bool is_const>
    constexpr auto bitset_iterator<B, is_const>::dereference() const noexcept -> reference
    {
        if constexpr (is_const)
        {
            if (p_bitset->data() == nullptr)
            {
                return true;
            }
            return (*p_block) & bitset_type::bit_mask(m_index);
        }
        else
        {
            return bitset_reference<B>(*p_bitset, *p_block, bitset_type::bit_mask(m_index));
        }
    }

    template <class B, bool is_const>
    constexpr void bitset_iterator<B, is_const>::increment()
    {
        ++m_index;
        // If we have reached next block
        if (is_first_bit_of_block(m_index))
        {
            ++p_block;
            m_index = 0u;
        }
        SPARROW_ASSERT_TRUE(m_index < bitset_type::s_bits_per_block);
    }

    template <class B, bool is_const>
    constexpr void bitset_iterator<B, is_const>::decrement()
    {
        // decreasing moves to previous block
        if (is_first_bit_of_block(m_index))
        {
            --p_block;
            m_index = sizeof(block_type);
        }
        else
        {
            --m_index;
        }
        SPARROW_ASSERT_TRUE(m_index < bitset_type::s_bits_per_block);
    }

    template <class B, bool is_const>
    constexpr void bitset_iterator<B, is_const>::advance(difference_type n)
    {
        if (n >= 0)
        {
            if (std::cmp_less(n, bitset_type::s_bits_per_block - m_index))
            {
                m_index += static_cast<size_type>(n);
            }
            else
            {
                const size_type to_next_block = bitset_type::s_bits_per_block - m_index;
                n -= static_cast<difference_type>(to_next_block);
                const size_type block_n = static_cast<size_type>(n) / bitset_type::s_bits_per_block;
                p_block += block_n + 1;
                n -= static_cast<difference_type>(block_n * bitset_type::s_bits_per_block);
                m_index = static_cast<size_type>(n);
            }
        }
        else
        {
            auto mn = static_cast<size_type>(-n);
            if (m_index >= mn)
            {
                m_index -= mn;
            }
            else
            {
                const size_type block_n = mn / bitset_type::s_bits_per_block;
                p_block -= block_n;
                mn -= block_n * bitset_type::s_bits_per_block;
                if (m_index >= mn)
                {
                    m_index -= mn;
                }
                else
                {
                    mn -= m_index;
                    --p_block;
                    m_index = bitset_type::s_bits_per_block - mn;
                }
            }
        }
        SPARROW_ASSERT_TRUE(m_index < bitset_type::s_bits_per_block);
    }

    template <class B, bool is_const>
    constexpr auto bitset_iterator<B, is_const>::distance_to(const self_type& rhs) const noexcept
        -> difference_type
    {
        if (p_block == rhs.p_block)
        {
            return static_cast<difference_type>(rhs.m_index - m_index);
        }
        else
        {
            const auto dist1 = distance_to_begin();
            const auto dist2 = rhs.distance_to_begin();
            return dist2 - dist1;
        }
    }

    template <class B, bool is_const>
    constexpr bool bitset_iterator<B, is_const>::equal(const self_type& rhs) const noexcept
    {
        return p_block == rhs.p_block && m_index == rhs.m_index;
    }

    template <class B, bool is_const>
    constexpr bool bitset_iterator<B, is_const>::less_than(const self_type& rhs) const noexcept
    {
        return (p_block < rhs.p_block) || (p_block == rhs.p_block && m_index < rhs.m_index);
    }

    template <class B, bool is_const>
    constexpr bool bitset_iterator<B, is_const>::is_first_bit_of_block(size_type index) const noexcept
    {
        return index % bitset_type::s_bits_per_block == 0;
    }

    template <class B, bool is_const>
    constexpr auto bitset_iterator<B, is_const>::distance_to_begin() const -> difference_type
    {
        const difference_type distance = p_block - p_bitset->begin().p_block;
        SPARROW_ASSERT_TRUE(distance >= 0);
        return static_cast<difference_type>(bitset_type::s_bits_per_block) * distance
               + static_cast<difference_type>(m_index);
    }
}
