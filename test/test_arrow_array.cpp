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
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/c_interface.hpp"

#include "arrow_array_schema_creation.hpp"
#include "doctest/doctest.h"

void check_equal(const ArrowArray& lhs, const ArrowArray& rhs)
{
    CHECK_EQ(lhs.length, rhs.length);
    CHECK_EQ(lhs.null_count, rhs.null_count);
    CHECK_EQ(lhs.offset, rhs.offset);
    CHECK_EQ(lhs.n_buffers, rhs.n_buffers);
    CHECK_EQ(lhs.n_children, rhs.n_children);
    CHECK_NE(lhs.buffers, rhs.buffers);
    CHECK_NE(lhs.private_data, rhs.private_data);
    for (size_t i = 0; i < static_cast<size_t>(lhs.n_buffers); ++i)
    {
        CHECK_NE(lhs.buffers[i], rhs.buffers[i]);
    }
    auto lhs_buffers = reinterpret_cast<const int8_t**>(lhs.buffers);
    auto rhs_buffers = reinterpret_cast<const int8_t**>(rhs.buffers);

    for (size_t i = 0; i < static_cast<size_t>(lhs.length); ++i)
    {
        CHECK_EQ(lhs_buffers[1][i], rhs_buffers[1][i]);
    }
}

void check_empty(const ArrowArray& arr)
{
    CHECK_EQ(arr.length, 0);
    CHECK_EQ(arr.null_count, 0);
    CHECK_EQ(arr.offset, 0);
    CHECK_EQ(arr.n_buffers, 0);
    CHECK_EQ(arr.buffers, nullptr);
    CHECK_EQ(arr.n_children, 0);
    CHECK_EQ(arr.children, nullptr);
    CHECK_EQ(arr.dictionary, nullptr);
}

TEST_SUITE("C Data Interface")
{
    TEST_CASE("ArrowArray")
    {
        SUBCASE("release")
        {
            ArrowArray array = test::make_arrow_array(true);
            array.release(&array);
            CHECK_EQ(array.buffers, nullptr);
            CHECK_EQ(array.children, nullptr);
            const bool is_release_nullptr = array.release == nullptr;
            CHECK(is_release_nullptr);
            CHECK_EQ(array.private_data, nullptr);
        }

        SUBCASE("release wo/ children and dictionary")
        {
            auto array = test::make_arrow_array(false);
            array.release(&array);
            CHECK_EQ(array.buffers, nullptr);
            CHECK_EQ(array.children, nullptr);
            const bool is_release_nullptr = array.release == nullptr;
            CHECK(is_release_nullptr);
            CHECK_EQ(array.private_data, nullptr);
        }

        SUBCASE("deep_copy")
        {
            auto [array, schema] = test::make_arrow_schema_and_array(true);
            auto array_copy = sparrow::copy_array(array, schema);
            check_equal(array, array_copy);
            array.release(&array);
            array_copy.release(&array_copy);
            schema.release(&schema);
        }

        SUBCASE("swap")
        {
            auto [array0, schema0] = test::make_arrow_schema_and_array(true);
            auto array0_bkup = sparrow::copy_array(array0, schema0);

            auto [array1, schema1] = test::make_arrow_schema_and_array(false);
            auto array1_bkup = sparrow::copy_array(array1, schema1);

            sparrow::swap(array0, array1);
            check_equal(array0, array1_bkup);
            check_equal(array1, array0_bkup);

            array0.release(&array0);
            schema0.release(&schema0);
            array1.release(&array1);
            schema1.release(&schema1);
            array0_bkup.release(&array0_bkup);
            array1_bkup.release(&array1_bkup);
        }

        SUBCASE("move_array")
        {
            auto [src_array, src_schema] = test::make_arrow_schema_and_array(true);
            auto control = sparrow::copy_array(src_array, src_schema);

            auto dst_array = sparrow::move_array(std::move(src_array));
            check_empty(src_array);
            check_equal(dst_array, control);

            auto dst2_array = sparrow::move_array(dst_array);
            check_empty(dst_array);
            check_equal(dst2_array, control);

            dst2_array.release(&dst2_array);
            src_schema.release(&src_schema);
            control.release(&control);
        }

        SUBCASE("validate_format_with_arrow_array")
        {
            auto [array, schema] = test::make_arrow_schema_and_array(false);
            CHECK(sparrow::validate_format_with_arrow_array(sparrow::data_type::INT8, array));
            array.release(&array);
            schema.release(&schema);
            // CHECK_FALSE(sparrow::validate_format_with_arrow_array(sparrow::data_type::FIXED_SIZED_LIST,
            // array));
        }

#if defined(__cpp_lib_format)
        SUBCASE("formatting")
        {
            auto [array, schema] = test::make_arrow_schema_and_array(false);
            [[maybe_unused]] const auto format = std::format("{}", array);
            array.release(&array);
            schema.release(&schema);
            // We don't check the result has it show the address of the object, which is not the same at each
            // run of the test
        }
#endif
    }
}

TEST_SUITE("arrow_array_private_data")
{
    TEST_CASE("buffers")
    {
        const auto buffers = test::detail::get_test_buffer_list0();
        sparrow::arrow_array_private_data private_data(buffers, sparrow::repeat_view<bool>(true, 0), true);

        auto buffers_ptrs = private_data.buffers_ptrs<uint8_t>();
        for (size_t i = 0; i < buffers.size(); ++i)
        {
            for (size_t j = 0; j < buffers[i].size(); ++j)
            {
                CHECK_EQ(buffers_ptrs[i][j], buffers[i][j]);
            }
        }
    }
}
