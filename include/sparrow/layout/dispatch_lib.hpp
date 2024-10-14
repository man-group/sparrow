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

#include <type_traits>

#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/null_array.hpp"
#include "sparrow/layout/primitive_array.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/types/data_traits.hpp"

namespace sparrow
{
    SPARROW_API std::size_t array_size(const array_wrapper& ar);
    SPARROW_API array_traits::const_reference array_element(const array_wrapper& ar, std::size_t index);
}