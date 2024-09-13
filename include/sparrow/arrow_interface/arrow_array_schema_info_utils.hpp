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
#include <ranges>

#include "sparrow/array/data_type.hpp"
#include "sparrow/buffer/buffer_adaptor.hpp"
#include "sparrow/buffer/buffer_view.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/utils/contracts.hpp"

namespace sparrow
{
    /// @returns `true` if the number of buffers in an `ArrowArray` for a given data type is valid, `false`
    /// otherwise.
    constexpr bool validate_buffers_count(data_type data_type, int64_t n_buffers)
    {
        const std::size_t expected_buffer_count = get_expected_buffer_count(data_type);
        return static_cast<std::size_t>(n_buffers) == expected_buffer_count;
    }

    /// @returns The the expected number of children for a given data type.
    constexpr std::size_t get_expected_children_count(data_type data_type)
    {
        switch (data_type)
        {
            case data_type::NA:
            case data_type::RUN_ENCODED:
            case data_type::BOOL:
            case data_type::UINT8:
            case data_type::INT8:
            case data_type::UINT16:
            case data_type::INT16:
            case data_type::UINT32:
            case data_type::INT32:
            case data_type::FLOAT:
            case data_type::UINT64:
            case data_type::INT64:
            case data_type::DOUBLE:
            case data_type::HALF_FLOAT:
            case data_type::TIMESTAMP:
            case data_type::FIXED_SIZE_BINARY:
            case data_type::DECIMAL:
            case data_type::FIXED_WIDTH_BINARY:
            case data_type::STRING:
            case data_type::BINARY:
                return 0;
            case data_type::LIST:
            case data_type::LARGE_LIST:
            case data_type::LIST_VIEW:
            case data_type::LARGE_LIST_VIEW:
            case data_type::FIXED_SIZED_LIST:
            case data_type::STRUCT:
            case data_type::MAP:
            case data_type::SPARSE_UNION:
                return 1;
            case data_type::DENSE_UNION:
                return 2;
        }
        mpl::unreachable();
    }

    /// @returns `true` if  the format of an `ArrowArray` for a given data type is valid, `false` otherwise.
    inline bool validate_format_with_arrow_array(data_type data_type, const ArrowArray& array)
    {
        const bool buffers_count_valid = validate_buffers_count(data_type, array.n_buffers);
        const bool children_count_valid = static_cast<std::size_t>(array.n_children)
                                          == get_expected_children_count(data_type);
        return buffers_count_valid && children_count_valid;
    }

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

    constexpr std::array<buffer_type, 2> buffers_types_validity_data{buffer_type::VALIDITY, buffer_type::DATA};
    constexpr std::array<buffer_type, 2> buffers_types_validity_offset32{
        buffer_type::VALIDITY,
        buffer_type::OFFSETS_32BIT
    };
    constexpr std::array<buffer_type, 2> buffers_types_validity_offset64{
        buffer_type::VALIDITY,
        buffer_type::OFFSETS_64BIT
    };
    constexpr std::array<buffer_type, 3> buffers_types_validity_offsets32_data{
        buffer_type::VALIDITY,
        buffer_type::OFFSETS_32BIT,
        buffer_type::DATA
    };
    constexpr std::array<buffer_type, 3> buffers_types_validity_offsets64_data{
        buffer_type::VALIDITY,
        buffer_type::OFFSETS_64BIT,
        buffer_type::DATA
    };
    constexpr std::array<buffer_type, 3> buffers_types_validity_offsets32_sizes32{
        buffer_type::VALIDITY,
        buffer_type::OFFSETS_32BIT,
        buffer_type::SIZES_32BIT
    };
    constexpr std::array<buffer_type, 3> buffers_types_validity_offsets64_sizes64{
        buffer_type::VALIDITY,
        buffer_type::OFFSETS_64BIT,
        buffer_type::SIZES_64BIT
    };
    constexpr std::array<buffer_type, 1> buffers_types_validity{buffer_type::VALIDITY};
    constexpr std::array<buffer_type, 1> buffers_types_types_ids{buffer_type::TYPE_IDS};
    constexpr std::array<buffer_type, 2> buffers_types_types_ids_offset32{
        buffer_type::TYPE_IDS,
        buffer_type::OFFSETS_32BIT
    };
    constexpr std::array<buffer_type, 0> buffers_types_empty{};

    /// @returns A vector of buffer types for a given data type.
    /// This information helps how interpret and parse each buffer in an ArrowArray.
    constexpr std::span<const buffer_type> get_buffer_types_from_data_type(data_type data_type)
    {
        switch (data_type)
        {
            case data_type::BOOL:
            case data_type::UINT8:
            case data_type::INT8:
            case data_type::UINT16:
            case data_type::INT16:
            case data_type::UINT32:
            case data_type::INT32:
            case data_type::FLOAT:
            case data_type::UINT64:
            case data_type::INT64:
            case data_type::DOUBLE:
            case data_type::HALF_FLOAT:
            case data_type::TIMESTAMP:
            case data_type::FIXED_SIZE_BINARY:
            case data_type::DECIMAL:
            case data_type::FIXED_WIDTH_BINARY:
                return buffers_types_validity_data;
            case data_type::BINARY:
            case data_type::STRING:
                return buffers_types_validity_offsets32_data;
            case data_type::LIST:
                return buffers_types_validity_offset32;
            case data_type::LARGE_LIST:
                return buffers_types_validity_offset64;
            case data_type::LIST_VIEW:
                return buffers_types_validity_offsets32_sizes32;
            case data_type::LARGE_LIST_VIEW:
                return buffers_types_validity_offsets64_sizes64;
            case data_type::FIXED_SIZED_LIST:
            case data_type::STRUCT:
                return buffers_types_validity;
            case data_type::SPARSE_UNION:
                return buffers_types_types_ids;
            case data_type::DENSE_UNION:
                return buffers_types_types_ids_offset32;
            case data_type::NA:
            case data_type::MAP:
            case data_type::RUN_ENCODED:
                return buffers_types_empty;
        }
        mpl::unreachable();
    }

    constexpr size_t get_buffer_type_index(data_type data_type, buffer_type buffer_type)
    {
        const auto buffer_types = get_buffer_types_from_data_type(data_type);
        const auto it = std::find(buffer_types.begin(), buffer_types.end(), buffer_type);
        if (it == buffer_types.end())
        {
            throw std::runtime_error("Unsupported buffer type");
        }
        return static_cast<size_t>(std::distance(buffer_types.begin(), it));
    }

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

    // template <std::integral T, std::ranges::input_range offset_buffer>
    // constexpr size_t
    // binary_bytes_count(buffer_type offset_buffer_type, T size, const offset_buffer& offset_buf)
    // {
    //     SPARROW_ASSERT_TRUE(
    //         offset_buffer_type == buffer_type::OFFSETS_32BIT || offset_buffer_type ==
    //         buffer_type::OFFSETS_64BIT
    //     );
    //     if (offset_buf.empty())
    //     {
    //         return 0;
    //     }
    //     if (offset_buffer_type == buffer_type::OFFSETS_32BIT)
    //     {
    //         return make_buffer_adaptor<int32_t>(offset_buf).back();
    //     }
    //     else
    //     {
    //         return make_buffer_adaptor<int64_t>(offset_buf).back();
    //     }
    // }

    /// @returns The number of bytes required according to the provided buffer type, length, offset and data
    /// type.

    inline std::size_t compute_buffer_size(
        buffer_type bt,
        size_t length,
        size_t offset,
        data_type dt,
        const std::vector<sparrow::buffer_view<uint8_t>>& previous_buffers,
        buffer_type previous_buffer_type
    )
    {
        constexpr double bit_per_byte = 8.;
        switch (bt)
        {
            case buffer_type::VALIDITY:
                return static_cast<std::size_t>(std::ceil(static_cast<double>(length + offset) / bit_per_byte));
            case buffer_type::DATA:
                if (bt == buffer_type::DATA && (dt == data_type::STRING || dt == data_type::BINARY))
                {
                    SPARROW_ASSERT_TRUE(previous_buffer_type == buffer_type::OFFSETS_32BIT
                                        || previous_buffer_type == buffer_type::OFFSETS_64BIT);
                    if (previous_buffers.back().empty())
                    {
                        return 0;
                    }
                    else if (previous_buffer_type == buffer_type::OFFSETS_32BIT)
                    {
                        const auto offset_buf = make_buffer_adaptor<int32_t>(previous_buffers.back());
                        [[maybe_unused]] const std::vector<uint32_t> offsets(offset_buf.begin(), offset_buf.end());
                        return static_cast<std::size_t>(offset_buf.back());
                    }
                    else
                    {
                        const auto offset_buf = make_buffer_adaptor<int64_t>(previous_buffers.back());
                        return static_cast<std::size_t>(offset_buf.back());
                    }
                }
                return primitive_bytes_count(dt, length + offset);
            case buffer_type::OFFSETS_32BIT:
            case buffer_type::SIZES_32BIT:
                return get_offset_element_count(dt, length, offset) * sizeof(std::int32_t);
            case buffer_type::OFFSETS_64BIT:
            case buffer_type::SIZES_64BIT:
                return get_offset_element_count(dt, length, offset) * sizeof(std::int64_t);
            case buffer_type::VIEWS:
                // TODO: Implement
                SPARROW_ASSERT(false, "Not implemented");
                return 0;
            case buffer_type::TYPE_IDS:
                return length + offset;
        }
        mpl::unreachable();
    }

}
