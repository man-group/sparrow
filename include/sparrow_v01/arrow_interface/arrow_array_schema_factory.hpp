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

#include <cstdint>
#include "sparrow/array/data_type.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/buffer.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/c_interface.hpp"

#include "sparrow_v01/utils/buffers.hpp"
#include "sparrow_v01/utils/offsets.hpp"


namespace sparrow
{
    inline ArrowSchema make_dictionary_encoded_arrow_schema(data_type value_data_type, data_type keys_data_type)
    {
        ArrowSchema* values_schema = make_arrow_schema_unique_ptr(
                                         data_type_to_format(value_data_type),
                                         "dictionary values",
                                         std::nullopt,
                                         std::nullopt,
                                         0,
                                         nullptr,
                                         nullptr
        )
                                         .release();

        ArrowSchema keys_schema = make_arrow_schema(
            data_type_to_format(keys_data_type),
            "dictionary keys",
            std::nullopt,
            std::nullopt,
            0,
            nullptr,
            values_schema
        );

        return keys_schema;
    }

    template <std::ranges::range R>
        requires(std::integral<std::ranges::range_value_t<R>>)
    buffer<uint8_t> make_bitmap_buffer(size_t count, R&& nulls)
    {
        if (!std::ranges::empty(nulls))
        {
            SPARROW_ASSERT_TRUE(*std::ranges::max_element(nulls) < count);
        }
        dynamic_bitset<uint8_t> bitmap(count, true);
        for (const auto i : nulls)
        {
            bitmap.set(i, false);
        }
        return bitmap.buffer();
    };

    template <layout_offset offset_type, std::ranges::sized_range Values, std::ranges::sized_range Nulls>
        requires std::integral<std::ranges::range_value_t<Nulls>>
    constexpr ArrowArray make_variable_size_binary_arrow_array(Values&& range, Nulls&& nulls, int64_t offset)
    {
        const auto length = static_cast<int64_t>(std::ranges::size(range)) - offset;
        const auto null_count = static_cast<int64_t>(std::ranges::size(nulls));
        std::vector<buffer<uint8_t>> value_buffers{
            make_bitmap_buffer(std::ranges::size(range), std::move(nulls)),
            range_to_buffer(make_offset_buffer<offset_type>(range)),
            strings_to_buffer(std::move(range))
        };
        return make_arrow_array(length, null_count, offset, std::move(value_buffers), 0, nullptr, nullptr);
    }

    template <std::ranges::sized_range Values, std::ranges::sized_range Nulls>
        requires std::is_arithmetic_v<std::ranges::range_value_t<Values>>
                 && std::integral<std::ranges::range_value_t<Nulls>>
    constexpr ArrowArray make_primitive_arrow_array(Values&& range, Nulls&& nulls, int64_t offset)
    {
        const int64_t length = static_cast<int64_t>(std::ranges::size(range)) - offset;
        const auto null_count = static_cast<int64_t>(std::ranges::size(nulls));
        std::vector<buffer<uint8_t>> value_buffers{
            make_bitmap_buffer(std::ranges::size(range), std::move(nulls)),
            range_to_buffer(std::move(range))
        };
        return make_arrow_array(length, null_count, offset, std::move(value_buffers), 0, nullptr, nullptr);
    }

    //  constexpr ArrowArray make_primitive_arrow_array(data_type dt)
    // {
    //     ArrowSchema schema = make_arrow_schema(
    //         data_type_to_format(dt),
    //         "dictionary keys",
    //         std::nullopt,
    //         std::nullopt,
    //         0,
    //         nullptr,
    //         values_schema
    //     );

    //     return schema;}


    template <
        std::ranges::sized_range Keys,
        std::ranges::sized_range KeyNulls,
        std::ranges::sized_range Values,
        std::ranges::sized_range ValuesNulls>
        requires std::integral<std::ranges::range_value_t<Keys>>
                 && std::integral<std::ranges::range_value_t<KeyNulls>>
    ArrowArray make_dictionary_encoded_arrow_array(
        Keys&& keys,
        KeyNulls&& keys_nulls,
        int64_t keys_offset,
        Values&& values,
        ValuesNulls&& values_nulls,
        int64_t values_offset
    )
    {
        ArrowArray keys_arrow_array = make_primitive_arrow_array(keys, keys_nulls, keys_offset);
        keys_arrow_array.dictionary = new ArrowArray;
        *keys_arrow_array.dictionary = make_variable_size_binary_arrow_array<int32_t>(
            std::move(values),
            std::move(values_nulls),
            values_offset
        );
        return keys_arrow_array;
    }

}
