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

#include <cstddef>

#include "sparrow/config/config.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/utils/iterator.hpp"

namespace sparrow
{
    class array;

    class SPARROW_API array_const_iterator
        : public pointer_index_iterator_base<
              array_const_iterator,
              const array,
              array_traits::value_type,
              array_traits::const_reference,
              std::random_access_iterator_tag>
    {
    public:

        using difference_type = std::ptrdiff_t;
        using size_type = std::size_t;

        array_const_iterator() = default;

    private:

        array_const_iterator(const array* array_ptr, size_type index);

        friend class array;
    };
}
