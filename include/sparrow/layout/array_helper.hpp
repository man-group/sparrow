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

// N.B. This file is temporary, its goal is to make it easier to not include
// dispatch.hpp in other headers to avoid bloating the header dependencies.
// On the long run these free functions should probably become methods of
// the array facade.

#include "sparrow/config/config.hpp"
#include "sparrow/layout/array_wrapper.hpp"

namespace sparrow
{
    SPARROW_API
    std::size_t array_size(const array_wrapper& ar);

    SPARROW_API
    bool array_has_value(const array_wrapper& ar, std::size_t index);

    SPARROW_API
    array_traits::const_reference array_element(const array_wrapper& ar, std::size_t index);

    SPARROW_API
    array_traits::inner_value_type array_default_element_value(const array_wrapper& ar);
}
