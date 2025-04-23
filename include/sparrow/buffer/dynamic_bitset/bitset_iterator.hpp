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
        constexpr bitset_iterator(bitset_type* bitset, size_type index);

    private:

        constexpr reference dereference() const noexcept;
        constexpr void increment();
        constexpr void decrement();
        constexpr void advance(difference_type n);
        [[nodiscard]] constexpr difference_type distance_to(const self_type& rhs) const noexcept;
        [[nodiscard]] constexpr bool equal(const self_type& rhs) const noexcept;
        [[nodiscard]] constexpr bool less_than(const self_type& rhs) const noexcept;

        [[nodiscard]] static constexpr difference_type as_signed(size_type i);
        [[nodiscard]] static constexpr size_type as_unsigned(difference_type i);

        bitset_type* p_bitset = nullptr;
        size_type m_index;

        friend class iterator_access;
    };

    template <class B, bool is_const>
    constexpr bitset_iterator<B, is_const>::bitset_iterator(bitset_type* bitset, size_type index)
        : p_bitset(bitset)
        , m_index(index)
    {
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
            return p_bitset->test(m_index);
        }
        else
        {
            return bitset_reference<B>(*p_bitset, m_index);
        }
    }

    template <class B, bool is_const>
    constexpr void bitset_iterator<B, is_const>::increment()
    {
        ++m_index;
    }

    template <class B, bool is_const>
    constexpr void bitset_iterator<B, is_const>::decrement()
    {
        --m_index;
    }

    template <class B, bool is_const>
    constexpr void bitset_iterator<B, is_const>::advance(difference_type n)
    {
        if (n < 0 && static_cast<size_type>(-n) > m_index) {
            // Prevent underflow by clamping m_index to 0
            m_index = 0;
        } else {
            m_index = as_unsigned(as_signed(m_index) + n);
        }
    }

    template <class B, bool is_const>
    constexpr auto bitset_iterator<B, is_const>::distance_to(const self_type& rhs) const noexcept
        -> difference_type
    {
        SPARROW_ASSERT_TRUE(p_bitset == rhs.p_bitset);
        return as_signed(rhs.m_index) - as_signed(m_index);
    }

    template <class B, bool is_const>
    constexpr bool bitset_iterator<B, is_const>::equal(const self_type& rhs) const noexcept
    {
        SPARROW_ASSERT_TRUE(p_bitset == rhs.p_bitset);
        return m_index == rhs.m_index;
    }

    template <class B, bool is_const>
    constexpr bool bitset_iterator<B, is_const>::less_than(const self_type& rhs) const noexcept
    {
        SPARROW_ASSERT_TRUE(p_bitset == rhs.p_bitset);
        return m_index < rhs.m_index;
    }

    template <class B, bool is_const>
    constexpr auto bitset_iterator<B, is_const>::as_signed(size_type i) -> difference_type
    {
        return static_cast<difference_type>(i);
    }

    template <class B, bool is_const>
    constexpr auto bitset_iterator<B, is_const>::as_unsigned(difference_type i) -> size_type
    {
        return static_cast<size_type>(i);
    }
}
