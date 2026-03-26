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

#include "sparrow/array_iterator.hpp"

#include "sparrow/array_api.hpp"
#include "sparrow/utils/contracts.hpp"

namespace sparrow
{
    array_const_iterator::array_const_iterator(const array* array_ptr, size_type index)
        : p_array(array_ptr)
        , m_index(index)
    {
    }

    auto array_const_iterator::dereference() const -> array_traits::const_reference
    {
        SPARROW_ASSERT_TRUE(p_array != nullptr);
        SPARROW_ASSERT_TRUE(m_index < p_array->size());
        return (*p_array)[m_index];
    }

    void array_const_iterator::increment()
    {
        ++m_index;
    }

    void array_const_iterator::decrement()
    {
        --m_index;
    }

    void array_const_iterator::advance(difference_type n)
    {
        if (n >= 0)
        {
            m_index += static_cast<size_type>(n);
        }
        else
        {
            SPARROW_ASSERT_TRUE(static_cast<size_type>(-n) <= m_index);
            m_index -= static_cast<size_type>(-n);
        }
    }

    auto array_const_iterator::distance_to(const array_const_iterator& rhs) const -> difference_type
    {
        SPARROW_ASSERT_TRUE(p_array == rhs.p_array);
        return static_cast<difference_type>(rhs.m_index) - static_cast<difference_type>(m_index);
    }

    bool array_const_iterator::equal(const array_const_iterator& rhs) const
    {
        return p_array == rhs.p_array && m_index == rhs.m_index;
    }

    bool array_const_iterator::less_than(const array_const_iterator& rhs) const
    {
        SPARROW_ASSERT_TRUE(p_array == rhs.p_array);
        return m_index < rhs.m_index;
    }
}
