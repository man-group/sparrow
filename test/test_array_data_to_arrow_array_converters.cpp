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
    TEST_CASE("to_vector_of_arrow_array_shared_ptr")
    {
        SUBCASE("move")
        {
            std::vector<sparrow::array_data> array_data_vec{
                sparrow::test::make_test_array_data<uint8_t>(10, 1, {1, 3, 5, 7, 9}),
                sparrow::test::make_test_array_data<uint8_t>(5, 0, {1}),
            };
            const auto arrow_array_vec = sparrow::to_vector_of_arrow_array_shared_ptr(std::move(array_data_vec));
            CHECK_EQ(arrow_array_vec.size(), 2);

            CHECK_EQ(arrow_array_vec[0]->length, 10);
            CHECK_EQ(arrow_array_vec[0]->null_count, 5);
            CHECK_EQ(arrow_array_vec[0]->offset, 1);
            CHECK_EQ(arrow_array_vec[0]->n_buffers, 2);
            CHECK_EQ(arrow_array_vec[0]->n_children, 1);
            CHECK_NE(arrow_array_vec[0]->children, nullptr);
            CHECK_EQ(arrow_array_vec[0]->dictionary, nullptr);

            CHECK_EQ(arrow_array_vec[1]->length, 5);
            CHECK_EQ(arrow_array_vec[1]->null_count, 1);
            CHECK_EQ(arrow_array_vec[1]->offset, 0);
            CHECK_EQ(arrow_array_vec[1]->n_buffers, 2);
            CHECK_EQ(arrow_array_vec[1]->n_children, 1);
            CHECK_NE(arrow_array_vec[1]->children, nullptr);
            CHECK_EQ(arrow_array_vec[1]->dictionary, nullptr);
        }

        SUBCASE("copy")
        {
            const std::vector<sparrow::array_data> array_data_vec{
                sparrow::test::make_test_array_data<uint8_t>(10, 1, {1, 3, 5, 7, 9}),
                sparrow::test::make_test_array_data<uint8_t>(5, 0, {1}),
            };

            const auto arrow_array_vec = sparrow::to_vector_of_arrow_array_shared_ptr(array_data_vec);
            CHECK_EQ(arrow_array_vec.size(), 2);

            CHECK_EQ(arrow_array_vec[0]->length, 10);
            CHECK_EQ(arrow_array_vec[0]->null_count, 5);
            CHECK_EQ(arrow_array_vec[0]->offset, 1);
            CHECK_EQ(arrow_array_vec[0]->n_buffers, 2);
            CHECK_EQ(arrow_array_vec[0]->n_children, 1);
            CHECK_NE(arrow_array_vec[0]->children, nullptr);
            CHECK_EQ(arrow_array_vec[0]->dictionary, nullptr);

            CHECK_EQ(arrow_array_vec[1]->length, 5);
            CHECK_EQ(arrow_array_vec[1]->null_count, 1);
            CHECK_EQ(arrow_array_vec[1]->offset, 0);
            CHECK_EQ(arrow_array_vec[1]->n_buffers, 2);
            CHECK_EQ(arrow_array_vec[1]->n_children, 1);
            CHECK_NE(arrow_array_vec[1]->children, nullptr);
            CHECK_EQ(arrow_array_vec[1]->dictionary, nullptr);
        }
    }

    TEST_CASE("arrow_array_buffer_from_array_data")
    {
        SUBCASE("move")
        {
            auto array_data = sparrow::test::make_test_array_data<uint8_t>(10, 1, {1, 3, 5, 7, 9});
            const auto buffers = sparrow::arrow_array_buffer_from_array_data(std::move(array_data));
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
            const auto buffers = sparrow::arrow_array_buffer_from_array_data(array_data);
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

    TEST_CASE("to_arrow_array_unique_ptr")
    {
        SUBCASE("move")
        {
            auto array_data = sparrow::test::make_test_array_data<uint8_t>(10, 1, {1, 3, 5, 7, 9});
            const auto arrow_array = sparrow::to_arrow_array_unique_ptr(std::move(array_data));
            CHECK_EQ(arrow_array->length, 10);
            CHECK_EQ(arrow_array->null_count, 5);
            CHECK_EQ(arrow_array->offset, 1);
            CHECK_EQ(arrow_array->n_buffers, 2);
            CHECK_EQ(arrow_array->n_children, 1);
            CHECK_NE(arrow_array->children, nullptr);
            CHECK_EQ(arrow_array->dictionary, nullptr);
        }

        SUBCASE("copy")
        {
            const auto array_data = sparrow::test::make_test_array_data<uint8_t>(10, 1, {1, 3, 5, 7, 9});
            const auto arrow_array = sparrow::to_arrow_array_unique_ptr(array_data);
            CHECK_EQ(arrow_array->length, 10);
            CHECK_EQ(arrow_array->null_count, 5);
            CHECK_EQ(arrow_array->offset, 1);
            CHECK_EQ(arrow_array->n_buffers, 2);
            CHECK_EQ(arrow_array->n_children, 1);
            CHECK_NE(arrow_array->children, nullptr);
            CHECK_EQ(arrow_array->dictionary, nullptr);
        }
    }
}
