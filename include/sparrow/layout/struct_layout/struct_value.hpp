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

#include "sparrow/config/config.hpp"
#include "sparrow/array/data_traits.hpp"
#include "sparrow/layout/array_base.hpp"
#include "sparrow/utils/memory.hpp"

namespace sparrow
{
    class SPARROW_API struct_value
    {
    public:

        using value_type = array_traits::value_type;
        using const_reference = array_traits::const_reference;
        using size_type = std::size_t;

        struct_value(const std::vector<cloning_ptr<array_base>>& children, size_type index);
        size_type size() const;
        const_reference operator[](size_type i) const;

    private:
    
        const std::vector<cloning_ptr<array_base>>&  m_children;
        size_type m_index;
    };
}

