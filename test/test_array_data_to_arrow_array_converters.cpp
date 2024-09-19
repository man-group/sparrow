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

#include <cstdint>

#include "sparrow/arrow_interface/array_data_to_arrow_array_converters.hpp"

#include "array_data_creation.hpp"
#include "doctest/doctest.h"

TEST_SUITE("ArrowArray array_data converters")
{
    TEST_CASE("to_vector_of_buffer")
    {
        SUBCASE("move")
        {
            auto array_data = sparrow::test::make_test_array_data<uint8_t>(10, 1, {1, 3, 5, 7, 9});
            const auto buffers = sparrow::to_vector_of_buffer(std::move(array_data));
            CHECK(array_data.buffers[0].empty());
            REQUIRE_EQ(buffers.size(), 2);
            REQUIRE_EQ(buffers[0].size(), 2);
            CHECK_EQ(buffers[1].size(), 10);
            CHECK_EQ(buffers[0][0], 0b01010101);
            CHECK_EQ(buffers[0][1], 0b00000001);

            for (size_t i = 0; i < buffers[1].size(); ++i)
            {
                CHECK_EQ(buffers[1][i], i);
            }
        }

        SUBCASE("copy")
        {
            const auto array_data = sparrow::test::make_test_array_data<uint8_t>(10, 1, {1, 3, 5, 7, 9});
            const auto buffers = sparrow::to_vector_of_buffer(array_data);
            CHECK_EQ(array_data.buffers[0].size(), 10);
            REQUIRE_EQ(buffers.size(), 2);
            REQUIRE_EQ(buffers[0].size(), 2);
            CHECK_EQ(buffers[1].size(), 10);

            CHECK_EQ(buffers[0][0], 0b01010101);
            CHECK_EQ(buffers[0][1], 0b00000001);

            for (size_t i = 0; i < buffers[1].size(); ++i)
            {
                CHECK_EQ(buffers[1][i], i);
            }
        }
    }
}
