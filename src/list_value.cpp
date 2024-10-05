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

#include "sparrow_v01/layout/list_layout/list_value.hpp"
#include "sparrow_v01/layout/dispatch.hpp"

namespace sparrow
{
    list_value2::list_value2(const array_base* flat_array, size_type index_begin, size_type index_end)
        : p_flat_array(flat_array)
        , m_index_begin(index_begin)
        , m_index_end(index_end)
    {
    }

    auto list_value2::size() const -> size_type
    {
        return m_index_end - m_index_begin;
    }

    auto list_value2::operator[](size_type i) const -> const_reference
    {
        return array_element(*p_flat_array, m_index_begin + i);
    }
}
