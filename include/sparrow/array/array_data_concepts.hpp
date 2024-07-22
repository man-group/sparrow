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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "sparrow/layout/dictionary_encoded_layout.hpp"
#include "sparrow/layout/fixed_size_layout.hpp"
#include "sparrow/layout/null_layout.hpp"
#include "sparrow/layout/variable_size_binary_layout.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    /**
     * Concept to check if a layout is a supported layout.
     *
     * A layout is considered supported if it is an instance of `fixed_size_layout`,
     * `variable_size_binary_layout`, or `dictionary_encoded_layout`.
     *
     * @tparam Layout The layout type to check.
     */
    template <class Layout>
    concept arrow_layout = mpl::is_type_instance_of_v<Layout, null_layout>
                           || mpl::is_type_instance_of_v<Layout, fixed_size_layout>
                           || mpl::is_type_instance_of_v<Layout, variable_size_binary_layout>
                           || mpl::is_type_instance_of_v<Layout, dictionary_encoded_layout>;

    /**
     * Concept to check if a type is a range of arrow base type extended.
     *
     * This concept checks if a type is a range and if its value type satisfies the
     * `is_arrow_base_type_extended` concept.
     *
     * @tparam R The range to check.
     */
    template <class R>
    concept range_of_arrow_base_type_extended = std::ranges::range<R>
                                                && is_arrow_base_type_extended<std::ranges::range_value_t<R>>;

    /**
     * Concept to check if a type is a range for array data.
     *
     * This concept checks if a type `R` satisfies the requirements of being a range for array data.
     * A type `R` is considered a range for array data if it satisfies either of the following conditions:
     * - It satisfies the `range_of_arrow_base_type_extended` concept.
     * - The unwrapped and decayed `range_value_t` of `R` satisfies the `range_of_arrow_base_type_extended`
     * concept.
     *
     * @tparam R The range to be checked.
     */
    template <class R>
    concept range_for_array_data = range_of_arrow_base_type_extended<R>
                                   || range_of_arrow_base_type_extended<
                                       std::unwrap_ref_decay_t<std::ranges::range_value_t<R>>>;

    /**
     * Concept for a constant range that can be used with array data.
     *
     * This concept requires that the range is both a constant range and a range that can be used with array
     * data.
     *
     * @tparam R The type of the range.
     */
    template <class R>
    concept constant_range_for_array_data = mpl::constant_range<R> && range_for_array_data<R>;

}  // namespace sparrow
