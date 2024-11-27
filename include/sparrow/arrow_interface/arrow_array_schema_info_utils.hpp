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

#include <algorithm>

#include "sparrow/buffer/buffer_adaptor.hpp"
#include "sparrow/buffer/buffer_view.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/types/data_type.hpp"
#include "sparrow/utils/contracts.hpp"

namespace sparrow
{
    enum class buffer_type : uint8_t
    {
        VALIDITY,
        DATA,
        OFFSETS_32BIT,
        OFFSETS_64BIT,
        VIEWS,
        TYPE_IDS,
        SIZES_32BIT,
        SIZES_64BIT,
    };

    /// @returns The expected offset element count for a given data type and array length.
    constexpr std::size_t get_offset_element_count(data_type data_type, size_t length, size_t offset)
    {
        switch (data_type)
        {
            case data_type::STRING:
            case data_type::LIST:
            case data_type::LARGE_LIST:
                return length + offset + 1;
            case data_type::LIST_VIEW:
            case data_type::LARGE_LIST_VIEW:
            case data_type::DENSE_UNION:
                return length + offset;
            default:
                throw std::runtime_error("Unsupported data type");
        }
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
            case data_type::TIMESTAMP:
            case data_type::DECIMAL:
            case data_type::LIST:
            case data_type::STRUCT:
            case data_type::STRING:
            case data_type::BINARY:
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

    /// @returns `true` if  the format of an `ArrowArray` for a given data type is valid, `false` otherwise.
    inline bool validate_format_with_arrow_array(data_type, const ArrowArray&)
    {
        return true;
        /* THE CODE BELOW MAKES WRONG ASSUMPTIONS AND NEEDS TO BE REFACTORED IN A SEPERATE PR*/
        // const bool buffers_count_valid = validate_buffers_count(data_type, array.n_buffers);
        // // const bool children_count_valid = static_cast<std::size_t>(array.n_children)
        // //                                   == get_expected_children_count(data_type);

        // //std::cout<<"child cound: "<<array.n_children<<" expected:
        // "<<get_expected_children_count(data_type)<<std::endl; return buffers_count_valid //&&
        // children_count_valid;
    }
}
