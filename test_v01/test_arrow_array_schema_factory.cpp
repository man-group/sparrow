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

#include <array>
#include <cstdint>

#include "sparrow/array/data_type.hpp"
#include "sparrow/buffer/buffer_adaptor.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"

#include "doctest/doctest.h"
#include "sparrow_v01/arrow_interface/arrow_array_schema_factory.hpp"
#include "sparrow_v01/utils/offsets.hpp"

namespace sparrow
{
    TEST_SUITE("ArrowArray and ArrowSchema factory")
    {
        TEST_CASE("make_dictionary_encoded_arrow_schema")
        {
            ArrowSchema schema = make_dictionary_encoded_arrow_schema(data_type::STRING, data_type::UINT32);
            CHECK_EQ(std::string_view(schema.format), "I");
            CHECK_EQ(std::string_view(schema.name), "dictionary keys");
            CHECK_EQ(schema.metadata, nullptr);
            CHECK_EQ(schema.flags, 0);
            CHECK_EQ(schema.n_children, 0);
            CHECK_EQ(schema.children, nullptr);
            CHECK_EQ(schema.release, release_arrow_schema);
            REQUIRE_NE(schema.dictionary, nullptr);
            ArrowSchema* dictionary = schema.dictionary;
            CHECK_EQ(std::string_view(dictionary->format), "u");
            CHECK_EQ(std::string_view(dictionary->name), "dictionary values");
            CHECK_EQ(dictionary->metadata, nullptr);
            CHECK_EQ(dictionary->flags, 0);
            CHECK_EQ(dictionary->n_children, 0);
            CHECK_EQ(dictionary->children, nullptr);
            CHECK_EQ(dictionary->dictionary, nullptr);
            schema.release(&schema);
        }

        TEST_CASE("make_bitmap_buffer")
        {
            constexpr std::array<uint8_t, 4> nulls{0, 1, 5, 12};
            buffer<uint8_t> bitmap = make_bitmap_buffer(15, nulls);
            REQUIRE_EQ(bitmap.size(), 2);
            CHECK_EQ(bitmap[0], 0b11011100);
            CHECK_EQ(bitmap[1], 0b01101111);
        }

        TEST_CASE("make_variable_size_binary_arrow_array")
        {
            using offset_type = int32_t;
            constexpr int64_t offset = 1;
            const std::array<std::string, 5> strings{"in_the_offset", "hello", "world", "!", "null"};
            constexpr std::array<uint8_t, 1> nulls{4};
            ArrowArray array = make_variable_size_binary_arrow_array<offset_type>(strings, nulls, offset);
            CHECK_EQ(array.length, 4);
            CHECK_EQ(array.null_count, 1);
            CHECK_EQ(array.offset, offset);
            CHECK_EQ(array.n_buffers, 3);
            CHECK_EQ(array.n_children, 0);
            REQUIRE_NE(array.buffers, nullptr);
            const dynamic_bitset_view<uint8_t> bitmap{
                static_cast<uint8_t*>(const_cast<void*>(array.buffers[0])),
                static_cast<size_t>(array.length + offset)
            };
            REQUIRE_EQ(bitmap.size(), 5);
            CHECK_EQ(bitmap[0], true);
            CHECK_EQ(bitmap[1], true);
            CHECK_EQ(bitmap[2], true);
            CHECK_EQ(bitmap[3], true);
            CHECK_EQ(bitmap[4], false);
            const auto offsets = static_cast<uint8_t*>(const_cast<void*>(array.buffers[1]));
            sparrow::buffer_view<uint8_t> offsets_buffer_view{offsets, (strings.size() + 1) * sizeof(offset_type)};
            const auto offsets_buffer_adaptor = make_buffer_adaptor<offset_type>(offsets_buffer_view);
            REQUIRE_EQ(offsets_buffer_adaptor.size(), strings.size() + 1);
            CHECK_EQ(offsets_buffer_adaptor[0], 0);
            CHECK_EQ(offsets_buffer_adaptor[1], 13);
            CHECK_EQ(offsets_buffer_adaptor[2], 18);
            CHECK_EQ(offsets_buffer_adaptor[3], 23);
            CHECK_EQ(offsets_buffer_adaptor[4], 24);
            CHECK_EQ(offsets_buffer_adaptor[5], 28);

            const auto values = static_cast<char*>(const_cast<void*>(array.buffers[2]));
            sparrow::buffer_view<char> values_buffer_view{values, 28};
            std::string_view values_buffer_string_view{values_buffer_view.data(), values_buffer_view.size()};
            CHECK_EQ(values_buffer_string_view, "in_the_offsethelloworld!null");

            CHECK_EQ(array.children, nullptr);
            CHECK_EQ(array.dictionary, nullptr);
            array.release(&array);
        }

        TEST_CASE("make_primitive_arrow_array")
        {
            const std::array<uint32_t, 5> values{1, 2, 3, 4, 5};
            constexpr std::array<uint8_t, 1> nulls{2};
            constexpr int64_t offset = 1;
            ArrowArray array = make_primitive_arrow_array(values, nulls, offset);
            CHECK_EQ(array.length, 4);
            CHECK_EQ(array.null_count, 1);
            CHECK_EQ(array.offset, offset);
            CHECK_EQ(array.n_buffers, 2);
            CHECK_EQ(array.n_children, 0);
            REQUIRE_NE(array.buffers, nullptr);
            const dynamic_bitset_view<uint8_t> bitmap{
                static_cast<uint8_t*>(const_cast<void*>(array.buffers[0])),
                static_cast<size_t>(array.length + offset)
            };
            REQUIRE_EQ(bitmap.size(), 5);
            CHECK_EQ(bitmap[0], true);
            CHECK_EQ(bitmap[1], true);
            CHECK_EQ(bitmap[2], false);
            CHECK_EQ(bitmap[3], true);
            const auto values_buffer = static_cast<uint32_t*>(const_cast<void*>(array.buffers[1]));
            sparrow::buffer_view<uint32_t> values_buffer_view{values_buffer, values.size()};
            const auto values_buffer_adaptor = make_buffer_adaptor<uint32_t>(values_buffer_view);
            REQUIRE_EQ(values_buffer_adaptor.size(), 5);
            CHECK_EQ(values_buffer_adaptor[0], 1);
            CHECK_EQ(values_buffer_adaptor[1], 2);
            CHECK_EQ(values_buffer_adaptor[2], 3);
            CHECK_EQ(values_buffer_adaptor[3], 4);
            CHECK_EQ(values_buffer_adaptor[4], 5);

            CHECK_EQ(array.children, nullptr);
            CHECK_EQ(array.dictionary, nullptr);
            array.release(&array);
        }
    }
}
