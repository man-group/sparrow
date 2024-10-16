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
#include "sparrow/layout/dictionary_encoded_array.hpp"
#include "sparrow/layout/primitive_array.hpp"
#include "sparrow/layout/variable_size_binary_array.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/layout/run_end_encoded_layout/run_end_encoded_array.hpp"
#include "sparrow/layout/list_layout/list_array.hpp"
#include "sparrow/layout/struct_layout/struct_array.hpp"
#include "sparrow/layout/union_array.hpp"
#include "sparrow/types/data_traits.hpp"

namespace sparrow
{
    template <class F>
    using visit_result_t = std::invoke_result_t<F, null_array>;

    template <class F>
    visit_result_t<F> visit(F&& func, const array_wrapper& ar)
    {
        if (ar.is_dictionary())
        {
            switch(ar.data_type())
            {
            case data_type::UINT8:
                return func(unwrap_array<dictionary_encoded_array<std::uint8_t>>(ar));
            case data_type::INT8:
                return func(unwrap_array<dictionary_encoded_array<std::int8_t>>(ar));
            case data_type::UINT16:
                return func(unwrap_array<dictionary_encoded_array<std::uint16_t>>(ar));
            case data_type::INT16:
                return func(unwrap_array<dictionary_encoded_array<std::int16_t>>(ar));
            case data_type::UINT32:
                return func(unwrap_array<dictionary_encoded_array<std::uint32_t>>(ar));
            case data_type::INT32:
                return func(unwrap_array<dictionary_encoded_array<std::int32_t>>(ar));
            case data_type::UINT64:
                return func(unwrap_array<dictionary_encoded_array<std::uint64_t>>(ar));
            case data_type::INT64:
                return func(unwrap_array<dictionary_encoded_array<std::int64_t>>(ar));
            default:
                throw std::runtime_error("data datype of dictionary encoded array must be an integer");
            }
        }
        else
        {
            switch(ar.data_type())
            {
            case data_type::NA:
                return func(unwrap_array<null_array>(ar));;
            case data_type::BOOL:
                return func(unwrap_array<primitive_array<bool>>(ar));
            case data_type::UINT8:
                return func(unwrap_array<primitive_array<std::uint8_t>>(ar));
            case data_type::INT8:
                return func(unwrap_array<primitive_array<std::int8_t>>(ar));
            case data_type::UINT16:
                return func(unwrap_array<primitive_array<std::uint16_t>>(ar));
            case data_type::INT16:
                return func(unwrap_array<primitive_array<std::int16_t>>(ar));
            case data_type::UINT32:
                return func(unwrap_array<primitive_array<std::uint32_t>>(ar));
            case data_type::INT32:
                return func(unwrap_array<primitive_array<std::int32_t>>(ar));
            case data_type::UINT64:
                return func(unwrap_array<primitive_array<std::uint64_t>>(ar));
            case data_type::INT64:
                return func(unwrap_array<primitive_array<std::int64_t>>(ar));
            case data_type::HALF_FLOAT:
                return func(unwrap_array<primitive_array<float16_t>>(ar));
            case data_type::FLOAT:
                return func(unwrap_array<primitive_array<float32_t>>(ar));
            case data_type::DOUBLE:
                return func(unwrap_array<primitive_array<float64_t>>(ar));
            case data_type::STRING:
                return func(unwrap_array<variable_size_binary_array<std::string, std::string_view>>(ar));
            case data_type::RUN_ENCODED:
                return func(unwrap_array<run_end_encoded_array>(ar));
            case data_type::LIST:
                return func(unwrap_array<list_array>(ar));
            case data_type::LARGE_LIST:
                return func(unwrap_array<big_list_array>(ar));
            case data_type::LIST_VIEW:
                return func(unwrap_array<list_view_array>(ar));
            case data_type::LARGE_LIST_VIEW:
                return func(unwrap_array<big_list_view_array>(ar));
            case data_type::FIXED_SIZED_LIST:
                return func(unwrap_array<fixed_sized_list_array>(ar));
            case data_type::STRUCT:
                return func(unwrap_array<struct_array>(ar));
            case data_type::DENSE_UNION:
                return func(unwrap_array<dense_union_array>(ar));
            case data_type::SPARSE_UNION:
                return func(unwrap_array<sparse_union_array>(ar));
            default:
                throw std::invalid_argument("array type not supported");
            }
        }
    }
}

