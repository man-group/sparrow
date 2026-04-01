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
        : base_type(array_ptr, index)
    {
    }

    auto array_const_iterator::dereference() const -> array_traits::const_reference
    {
        SPARROW_ASSERT_TRUE(p_object != nullptr);
        SPARROW_ASSERT_TRUE(m_index < p_object->size());
        return (*p_object)[m_index];
    }
}
