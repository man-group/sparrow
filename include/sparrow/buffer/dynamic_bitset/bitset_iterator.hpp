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
    class bitset_iterator : public pointer_index_iterator_base<
                                bitset_iterator<B, is_const>,
                                mpl::constify_t<B, is_const>,
                                mpl::constify_t<typename B::value_type, is_const>,
                                std::conditional_t<is_const, bool, bitset_reference<B>>,
                                std::random_access_iterator_tag>
    {
    public:

        using self_type = bitset_iterator<B, is_const>;
        using base_type = pointer_index_iterator_base<
            self_type,
            mpl::constify_t<B, is_const>,
            mpl::constify_t<typename B::value_type, is_const>,
            std::conditional_t<is_const, bool, bitset_reference<B>>,
            std::random_access_iterator_tag>;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;

        using block_type = mpl::constify_t<typename B::block_type, is_const>;
        using bitset_type = mpl::constify_t<B, is_const>;
        using size_type = typename B::size_type;

        constexpr bitset_iterator() noexcept = default;
        constexpr bitset_iterator(bitset_type* bitset, size_type index);

    private:

        constexpr reference dereference() const noexcept;
        constexpr void advance(difference_type n) noexcept;

        friend class iterator_access;
    };

    template <class B, bool is_const>
    constexpr bitset_iterator<B, is_const>::bitset_iterator(bitset_type* bitset, size_type index)
        : base_type(bitset, index)
    {
    }

    template <class B, bool is_const>
    constexpr auto bitset_iterator<B, is_const>::dereference() const noexcept -> reference
    {
        if constexpr (is_const)
        {
            return this->p_object->test(this->m_index);
        }
        else
        {
            return bitset_reference<B>(*this->p_object, this->m_index);
        }
    }

    template <class B, bool is_const>
    constexpr void bitset_iterator<B, is_const>::advance(difference_type n) noexcept
    {
        if (n < 0 && static_cast<size_type>(-n) > this->m_index)
        {
            // Prevent underflow by clamping m_index to 0
            this->m_index = 0;
        }
        else
        {
            this->m_index = static_cast<size_type>(static_cast<difference_type>(this->m_index) + n);
        }
    }
}
