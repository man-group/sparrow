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

#include "sparrow/c_interface.hpp"
#include "sparrow/config/config.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/types/data_traits.hpp"

namespace sparrow
{
    class array
    {
    public:

        using size_type = std::size_t;
        using value_type = array_traits::value_type;
        using const_reference = array_traits::const_reference;
 
        SPARROW_API array() = default;

        SPARROW_API array(ArrowArray&& array, ArrowSchema&& schema);
        SPARROW_API array(ArrowArray&& array, ArrowSchema* schema);
        SPARROW_API array(ArrowArray* array, ArrowSchema* schema);
        
        SPARROW_API size_type size() const;
        SPARROW_API const_reference operator[](size_type) const;

    private:

        cloning_ptr<array_wrapper> p_array = nullptr;
    };
}

