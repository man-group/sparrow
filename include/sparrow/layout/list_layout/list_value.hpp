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
#include "sparrow_v01/layout/array_base.hpp"

namespace sparrow
{
    class SPARROW_API list_value2
    {
    public:

        using value_type = array_traits::value_type;
        using const_reference = array_traits::const_reference;
        using size_type = std::size_t;

        list_value2(const array_base* flat_array, size_type index_begin, size_type index_end);

        size_type size() const;
        const_reference operator[](size_type i) const;

    private:

        const array_base* p_flat_array;
        size_type m_index_begin;
        size_type m_index_end;
    };
}

