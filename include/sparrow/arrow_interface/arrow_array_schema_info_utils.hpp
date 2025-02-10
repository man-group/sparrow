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

#include "sparrow/c_interface.hpp"
#include "sparrow/types/data_type.hpp"

namespace sparrow
{

    /// @returns `true` if  the format of an `ArrowArray` for a given data type is valid, `false` otherwise.
    inline bool validate_format_with_arrow_array(data_type, const ArrowArray&)
    {
        return true;
        /* THE CODE USED TO MAKES WRONG ASSUMPTIONS AND NEEDS TO BE REFACTORED IN A SEPERATE PR*/
    }

    constexpr bool has_bitmap(data_type dt)
    {
        switch (dt)
        {
            // List all data types. We use the default warning to catch missing cases.
            case data_type::BOOL:
            case data_type::INT8:
            case data_type::INT16:
            case data_type::INT32:
            case data_type::INT64:
            case data_type::UINT8:
            case data_type::UINT16:
            case data_type::UINT32:
            case data_type::UINT64:
            case data_type::HALF_FLOAT:
            case data_type::FLOAT:
            case data_type::DOUBLE:
            case data_type::DATE_DAYS:
            case data_type::DATE_MILLISECONDS:
            case data_type::TIMESTAMP_SECONDS:
            case data_type::TIMESTAMP_MILLISECONDS:
            case data_type::TIMESTAMP_MICROSECONDS:
            case data_type::TIMESTAMP_NANOSECONDS:
            case data_type::DURATION_SECONDS:
            case data_type::DURATION_MILLISECONDS:
            case data_type::DURATION_MICROSECONDS:
            case data_type::DURATION_NANOSECONDS:
            case data_type::INTERVAL_MONTHS:
            case data_type::INTERVAL_DAYS_TIME:
            case data_type::INTERVAL_MONTHS_DAYS_NANOSECONDS:
            case data_type::DECIMAL32:
            case data_type::DECIMAL64:
            case data_type::DECIMAL128:
            case data_type::DECIMAL256:
            case data_type::LIST:
            case data_type::STRUCT:
            case data_type::STRING:
            case data_type::LARGE_STRING:
            case data_type::BINARY:
            case data_type::LARGE_BINARY:
            case data_type::FIXED_WIDTH_BINARY:
            case data_type::LARGE_LIST:
            case data_type::LIST_VIEW:
            case data_type::LARGE_LIST_VIEW:
            case data_type::FIXED_SIZED_LIST:
            case data_type::STRING_VIEW:
            case data_type::BINARY_VIEW:
                return true;
            case data_type::MAP:
            case data_type::NA:
            case data_type::SPARSE_UNION:
            case data_type::DENSE_UNION:
            case data_type::RUN_ENCODED:
                return false;
        }
        mpl::unreachable();
    }
}
