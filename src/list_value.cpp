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

#include "sparrow/layout/list_layout/list_value.hpp"
#include "sparrow/layout/dispatch.hpp"

#include <iostream>

namespace sparrow
{
    list_value::list_value(const array_wrapper* flat_array, size_type index_begin, size_type index_end)
        : p_flat_array(flat_array)
        , m_index_begin(index_begin)
        , m_index_end(index_end)
    {
        SPARROW_ASSERT_TRUE(index_begin<=index_end);
    }

    auto list_value::size() const -> size_type
    {
        return m_index_end - m_index_begin;
    }

    auto list_value::operator[](size_type i) const -> const_reference
    {
        return array_element(*p_flat_array, m_index_begin + i);
    }

    bool operator==(const list_value& lhs, const list_value& rhs)
    {
        bool res = lhs.size() == rhs.size();
        for (std::size_t i = 0; res && i < lhs.size(); ++i)
        {
            res = lhs[i] == rhs[i];
        }
        return res;
        // TODO: refactor with the following when list_value is a range
        // return std::ranges::equal(lhs, rhs);
    }
}
