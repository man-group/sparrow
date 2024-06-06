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

#include "sparrow/dictionary_encoded_layout.hpp"
#include "sparrow/fixed_size_layout.hpp"
#include "sparrow/mp_utils.hpp"
#include "sparrow/variable_size_binary_layout.hpp"

namespace sparrow
{
    template <class Layout>
    concept is_a_supported_layout = mpl::is_type_instance_of_v<Layout, fixed_size_layout>
                                    || mpl::is_type_instance_of_v<Layout, variable_size_binary_layout>
                                    || mpl::is_type_instance_of_v<Layout, dictionary_encoded_layout>;

    template <typename R>
    concept range_of_arrow_base_type_extended = std::ranges::range<R>
                                                && is_arrow_base_type_extended<std::ranges::range_value_t<R>>;

    template <class R>
    concept range_for_array_data = range_of_arrow_base_type_extended<R>
                                   || range_of_arrow_base_type_extended<
                                       std::unwrap_ref_decay_t<std::ranges::range_value_t<R>>>;

    template <class R>
    concept constant_range_for_array_data = mpl::constant_range<R> && range_for_array_data<R>;

}  // namespace sparrow
