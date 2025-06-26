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

#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/iterator.hpp"

namespace sparrow
{
    template <class FUNCTOR>
    class functor_index_iterator : public iterator_base<
                                       functor_index_iterator<FUNCTOR>,             // Derived
                                       std::invoke_result_t<FUNCTOR, std::size_t>,  // Element
                                       std::random_access_iterator_tag,
                                       std::invoke_result_t<FUNCTOR, std::size_t>  // Reference
                                       >
    {
    public:

        using result_type = std::invoke_result_t<FUNCTOR, std::size_t>;
        using self_type = functor_index_iterator<FUNCTOR>;
        using difference_type = std::ptrdiff_t;
        using size_type = std::size_t;
        using functor_type = FUNCTOR;

        constexpr functor_index_iterator() = default;

        constexpr functor_index_iterator(FUNCTOR functor, size_type index)
            : m_functor(std::move(functor))
            , m_index(index)
        {
        }

    private:

        [[nodiscard]] result_type dereference() const
        {
            return m_functor(m_index);
        }

        constexpr void increment()
        {
            ++m_index;
        }

        constexpr void decrement()
        {
            --m_index;
        }

        constexpr void advance(difference_type n)
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

        [[nodiscard]] constexpr difference_type distance_to(const self_type& rhs) const
        {
            return static_cast<difference_type>(rhs.m_index) - static_cast<difference_type>(m_index);
        }

        [[nodiscard]] constexpr bool equal(const self_type& rhs) const
        {
            return m_index == rhs.m_index;
        }

        [[nodiscard]] constexpr bool less_than(const self_type& rhs) const
        {
            return m_index < rhs.m_index;
        }

        FUNCTOR m_functor = FUNCTOR{};
        size_type m_index = 0;

        friend class iterator_access;
    };
}
