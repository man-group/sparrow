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

#include "sparrow/layout/struct_layout/struct_value.hpp"
#include "sparrow/layout/dispatch.hpp"

namespace sparrow
{
    struct_value::struct_value(  const std::vector<child_ptr>& children, size_type index)
        : m_children(children)
        , m_index(index)
    {
    }

    auto struct_value::size() const -> size_type
    {
        return m_children.size();
    }

    auto struct_value::operator[](size_type i) const -> const_reference
    {
        return array_element(*(m_children[i].get()), m_index);
    }
}
