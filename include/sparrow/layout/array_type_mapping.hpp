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

#include <array>

#include "sparrow/types/data_type.hpp"

/**
 * @file array_type_mapping.hpp
 * @brief Single source of truth for data_type to array class mapping using C++20
 *
 * This header defines compile-time mapping between data_type enum values and their
 * corresponding array classes using template specialization. This eliminates code
 * duplication across factory registration, dispatch, and dictionary encoding.
 */

namespace sparrow
{
    // Primary template - undefined for unsupported types
    // Specializations are defined in array_registry.hpp after all array types are included
    template <data_type DT>
    struct array_type_map;

    // Helper alias
    template <data_type DT>
    using array_type_t = typename array_type_map<DT>::type;

    // Dictionary encoding type map (for integer types)
    template <data_type DT>
    struct dictionary_key_type;

    template <data_type DT>
    using dictionary_key_t = typename dictionary_key_type<DT>::type;

    // Timestamp types with/without timezone
    template <data_type DT>
    struct timestamp_type_map;

    // List of all supported data types - single source of truth
    inline constexpr std::array all_data_types = {
        data_type::NA,
        data_type::BOOL,
        data_type::UINT8,
        data_type::INT8,
        data_type::UINT16,
        data_type::INT16,
        data_type::UINT32,
        data_type::INT32,
        data_type::UINT64,
        data_type::INT64,
        data_type::HALF_FLOAT,
        data_type::FLOAT,
        data_type::DOUBLE,
        data_type::STRING,
        data_type::STRING_VIEW,
        data_type::LARGE_STRING,
        data_type::BINARY,
        data_type::BINARY_VIEW,
        data_type::LARGE_BINARY,
        data_type::LIST,
        data_type::LARGE_LIST,
        data_type::LIST_VIEW,
        data_type::LARGE_LIST_VIEW,
        data_type::FIXED_SIZED_LIST,
        data_type::STRUCT,
        data_type::MAP,
        data_type::RUN_ENCODED,
        data_type::DENSE_UNION,
        data_type::SPARSE_UNION,
        data_type::DECIMAL32,
        data_type::DECIMAL64,
        data_type::DECIMAL128,
        data_type::DECIMAL256,
        data_type::FIXED_WIDTH_BINARY,
        data_type::DATE_DAYS,
        data_type::DATE_MILLISECONDS,
        data_type::TIMESTAMP_SECONDS,
        data_type::TIMESTAMP_MILLISECONDS,
        data_type::TIMESTAMP_MICROSECONDS,
        data_type::TIMESTAMP_NANOSECONDS,
        data_type::DURATION_SECONDS,
        data_type::DURATION_MILLISECONDS,
        data_type::DURATION_MICROSECONDS,
        data_type::DURATION_NANOSECONDS,
        data_type::INTERVAL_MONTHS,
        data_type::INTERVAL_DAYS_TIME,
        data_type::INTERVAL_MONTHS_DAYS_NANOSECONDS,
        data_type::TIME_SECONDS,
        data_type::TIME_MILLISECONDS,
        data_type::TIME_MICROSECONDS,
        data_type::TIME_NANOSECONDS
    };

}  // namespace sparrow
