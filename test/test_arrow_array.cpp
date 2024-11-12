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
#include <memory>
#include <ranges>

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_info_utils.hpp"
#include "sparrow/c_interface.hpp"

#include "arrow_array_schema_creation.hpp"
#include "doctest/doctest.h"

using buffer_type = sparrow::buffer<uint8_t>;
const buffer_type buffer_dummy({0, 1, 2, 3, 4});
using buffers_type = std::vector<buffer_type>;
const buffers_type buffers_dummy{buffer_dummy, buffer_dummy, buffer_dummy};

TEST_SUITE("C Data Interface")
{
    TEST_CASE("ArrowArray")
    {
        SUBCASE("release")
        {
            auto children = new ArrowArray*[2];
            children[0] = new ArrowArray();
            children[1] = new ArrowArray();
            ArrowArray* dictionary = new ArrowArray();
            auto array = sparrow::make_arrow_array(
                1,
                0,
                0,
                buffers_dummy,
                2,
                children,
                dictionary
            );

            array.release(&array);
            CHECK_EQ(array.buffers, nullptr);
            CHECK_EQ(array.children, nullptr);
            const bool is_release_nullptr = array.release == nullptr;
            CHECK(is_release_nullptr);
            CHECK_EQ(array.private_data, nullptr);
        }

        SUBCASE("release wo/ children and dictionary")
        {
            auto array = sparrow::make_arrow_array(1, 0, 0, buffers_dummy, 0, nullptr, nullptr);
            array.release(&array);
            CHECK_EQ(array.buffers, nullptr);
            CHECK_EQ(array.children, nullptr);
            const bool is_release_nullptr = array.release == nullptr;
            CHECK(is_release_nullptr);
            CHECK_EQ(array.private_data, nullptr);
        }

        SUBCASE("deep_copy")
        {
            auto [array, schema] = make_sparrow_arrow_schema_and_array();
            auto array_copy = sparrow::copy_array(array, schema);
            CHECK_EQ(array.length, array_copy.length);
            CHECK_EQ(array.null_count, array_copy.null_count);
            CHECK_EQ(array.offset, array_copy.offset);
            CHECK_EQ(array.n_buffers, array_copy.n_buffers);
            CHECK_EQ(array.n_children, array_copy.n_children);
            CHECK_NE(array.buffers, array_copy.buffers);
            CHECK_NE(array.private_data, array_copy.private_data);
            for (size_t i = 0; i < static_cast<size_t>(array.n_buffers); ++i)
            {
                CHECK_NE(array.buffers[i], array_copy.buffers[i]);
            }
            auto array_buffers = reinterpret_cast<const int8_t**>(array.buffers);
            auto array_copy_buffers = reinterpret_cast<const int8_t**>(array_copy.buffers);

            for (size_t i = 0; i < static_cast<size_t>(array.length); ++i)
            {
                CHECK_EQ(array_buffers[1][i], array_copy_buffers[1][i]);
            }
        }

        SUBCASE("validate_format_with_arrow_array")
        {
            auto [array, schema] = make_sparrow_arrow_schema_and_array();
            CHECK(sparrow::validate_format_with_arrow_array(sparrow::data_type::INT8, array));
            // CHECK_FALSE(sparrow::validate_format_with_arrow_array(sparrow::data_type::FIXED_SIZED_LIST,
            // array));
        }

        SUBCASE("compute_buffer_size")
        {
            CHECK_EQ(
                sparrow::compute_buffer_size(
                    sparrow::buffer_type::DATA,
                    10,
                    0,
                    sparrow::data_type::INT8,
                    {},
                    sparrow::buffer_type::VALIDITY
                ),
                10
            );

            CHECK_EQ(
                sparrow::compute_buffer_size(
                    sparrow::buffer_type::DATA,
                    10,
                    5,
                    sparrow::data_type::INT8,
                    {},
                    sparrow::buffer_type::VALIDITY
                ),
                15
            );

            CHECK_EQ(
                sparrow::compute_buffer_size(
                    sparrow::buffer_type::DATA,
                    10,
                    5,
                    sparrow::data_type::INT16,
                    {},
                    sparrow::buffer_type::VALIDITY
                ),
                30
            );

            CHECK_EQ(
                sparrow::compute_buffer_size(
                    sparrow::buffer_type::DATA,
                    10,
                    5,
                    sparrow::data_type::INT32,
                    {},
                    sparrow::buffer_type::VALIDITY
                ),
                60
            );

            CHECK_EQ(
                sparrow::compute_buffer_size(
                    sparrow::buffer_type::VALIDITY,
                    10,
                    5,
                    sparrow::data_type::UINT8,
                    {},
                    sparrow::buffer_type::VALIDITY
                ),
                2
            );
        }
    }
}

TEST_SUITE("arrow_array_private_data")
{
    TEST_CASE("buffers")
    {
        const auto buffers = buffers_dummy;
        sparrow::arrow_array_private_data private_data(buffers);

        auto buffers_ptrs = private_data.buffers_ptrs<int8_t>();
        for (size_t i = 0; i < buffers.size(); ++i)
        {
            for (size_t j = 0; j < buffers[i].size(); ++j)
            {
                CHECK_EQ(buffers_ptrs[i][j], buffers[i][j]);
            }
        }
    }
}
