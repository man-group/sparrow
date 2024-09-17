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
#include "sparrow/c_interface.hpp"

#include "arrow_array_schema_creation.hpp"
#include "doctest/doctest.h"



using buffer_type = sparrow::buffer<uint8_t>;
const buffer_type buffer_dummy({0, 1, 2, 3, 4});
using buffers_type = std::vector<buffer_type>;
const buffers_type buffers_dummy{buffer_dummy, buffer_dummy, buffer_dummy};

template <std::ranges::input_range B>
void check_common(
    const sparrow::arrow_array_unique_ptr& array,
    const B& buffers,
    std::vector<ArrowArray*> children_ptr,
    ArrowArray* dictionary_pointer
)
{
    CHECK_EQ(array->length, 1);
    CHECK_EQ(array->null_count, 0);
    CHECK_EQ(array->offset, 0);
    CHECK_EQ(array->n_buffers, buffers.size());
    const int8_t** buffer_ptr = reinterpret_cast<const int8_t**>(array->buffers);
    for (size_t i = 0; i < buffers.size(); ++i)
    {
        REQUIRE_NE(buffer_ptr[i], nullptr);
        for (size_t j = 0; j < buffers[i].size(); ++j)
        {
            CHECK_EQ(buffer_ptr[i][j], buffers[i][j]);
        }
    }
    CHECK_EQ(array->n_children, children_ptr.size());
    if (children_ptr.empty())
    {
        CHECK_EQ(array->children, nullptr);
    }
    for (size_t i = 0; i < children_ptr.size(); ++i)
    {
        CHECK_EQ(array->children[i], children_ptr[i]);
    }
    CHECK_EQ(array->dictionary, dictionary_pointer);
    const bool is_release_arrow_array = array->release == &sparrow::release_arrow_array;
    CHECK(is_release_arrow_array);
    CHECK_NE(array->private_data, nullptr);
}

TEST_SUITE("C Data Interface")
{
    TEST_CASE("ArrowArray")
    {
        SUBCASE("arrow_array_unique_ptr")
        {
            SUBCASE("sparrow::default_arrow_array_unique_ptr")
            {
                const sparrow::arrow_array_unique_ptr array = sparrow::default_arrow_array_unique_ptr();
                CHECK_EQ(array->length, 0);
                CHECK_EQ(array->null_count, 0);
                CHECK_EQ(array->offset, 0);
                CHECK_EQ(array->n_buffers, 0);
                CHECK_EQ(array->n_children, 0);
                CHECK_EQ(array->buffers, nullptr);
                CHECK_EQ(array->children, nullptr);
                const bool is_release_nullptr = array->release == nullptr;
                CHECK(is_release_nullptr);
                CHECK_EQ(array->private_data, nullptr);
            }

            SUBCASE("default")
            {
                const sparrow::arrow_array_unique_ptr array;
                CHECK_EQ(array, nullptr);
            }

            SUBCASE("move")
            {
                sparrow::arrow_array_unique_ptr array = sparrow::default_arrow_array_unique_ptr();
                const auto array_2 = std::move(array);
                CHECK_EQ(array, nullptr);
                CHECK_NE(array_2, nullptr);
            }

            SUBCASE("nullptr")
            {
                const sparrow::arrow_array_unique_ptr array(nullptr);
                CHECK_EQ(array, nullptr);
            }
        }

        SUBCASE("arrow_array_shared_ptr")
        {
            SUBCASE("constructors")
            {
                SUBCASE("default")
                {
                    const sparrow::arrow_array_shared_ptr array;
                    CHECK_FALSE(array);
                    const auto deleter = array.get_deleter();
                    const bool is_release_arrow_array_custom_deleter = deleter
                                                                       == &sparrow::arrow_array_custom_deleter;
                    CHECK(is_release_arrow_array_custom_deleter);
                }

                SUBCASE("from arrow_array_unique_ptr")
                {
                    sparrow::arrow_array_unique_ptr array = sparrow::default_arrow_array_unique_ptr();
                    array->length = 99;
                    array->null_count = 42;
                    const sparrow::arrow_array_shared_ptr shared_array(std::move(array));
                    CHECK_EQ(shared_array->length, 99);
                    CHECK_EQ(shared_array->null_count, 42);
                    const auto del_p = shared_array.get_deleter();
                    const auto is_release_arrow_array_custom_deleter = *del_p
                                                                       == &sparrow::arrow_array_custom_deleter;
                    CHECK(is_release_arrow_array_custom_deleter);
                }

                SUBCASE("move")
                {
                    sparrow::arrow_array_unique_ptr array = sparrow::default_arrow_array_unique_ptr();
                    array->length = 99;
                    array->null_count = 42;
                    sparrow::arrow_array_shared_ptr shared_array(std::move(array));
                    sparrow::arrow_array_shared_ptr shared_array_2(std::move(shared_array));
                    CHECK_EQ(shared_array_2->length, 99);
                    CHECK_EQ(shared_array_2->null_count, 42);
                    const auto del_p = shared_array_2.get_deleter();
                    const bool is_release_arrow_array_custom_deleter = *del_p
                                                                       == &sparrow::arrow_array_custom_deleter;
                    CHECK(is_release_arrow_array_custom_deleter);
                }

                SUBCASE("copy")
                {
                    sparrow::arrow_array_unique_ptr array = sparrow::default_arrow_array_unique_ptr();
                    array->length = 99;
                    array->null_count = 42;
                    const sparrow::arrow_array_shared_ptr shared_array(std::move(array));
                    const sparrow::arrow_array_shared_ptr shared_array_2(shared_array);
                    CHECK_EQ(shared_array_2->length, 99);
                    CHECK_EQ(shared_array_2->null_count, 42);
                    const auto del_p = shared_array_2.get_deleter();
                    const bool is_release_arrow_array_custom_deleter = *del_p
                                                                       == &sparrow::arrow_array_custom_deleter;
                    CHECK(is_release_arrow_array_custom_deleter);
                }

                SUBCASE("nullptr")
                {
                    const sparrow::arrow_array_shared_ptr shared_array(nullptr);
                    CHECK_FALSE(shared_array);
                    const auto del_p = shared_array.get_deleter();
                    const bool is_release_arrow_array_custom_deleter = *del_p
                                                                       == &sparrow::arrow_array_custom_deleter;
                    CHECK(is_release_arrow_array_custom_deleter);
                }
            }

            SUBCASE("operator=")
            {
                SUBCASE("move")
                {
                    sparrow::arrow_array_unique_ptr array = sparrow::default_arrow_array_unique_ptr();
                    array->length = 99;
                    array->null_count = 42;
                    sparrow::arrow_array_shared_ptr shared_array(std::move(array));
                    const sparrow::arrow_array_shared_ptr shared_array_2 = std::move(shared_array);
                    CHECK_EQ(shared_array_2->length, 99);
                    CHECK_EQ(shared_array_2->null_count, 42);
                    // obtain pointer to the deleter:
                    const auto del_p = shared_array_2.get_deleter();
                    const bool is_release_arrow_array_custom_deleter = *del_p
                                                                       == &sparrow::arrow_array_custom_deleter;
                    CHECK(is_release_arrow_array_custom_deleter);
                }

                SUBCASE("copy")
                {
                    sparrow::arrow_array_unique_ptr array = sparrow::default_arrow_array_unique_ptr();
                    array->length = 99;
                    array->null_count = 42;
                    const sparrow::arrow_array_shared_ptr shared_array(std::move(array));

                    sparrow::arrow_array_shared_ptr shared_array_2 = shared_array;
                    CHECK_EQ(shared_array_2->length, 99);
                    CHECK_EQ(shared_array_2->null_count, 42);
                    const auto del_p = shared_array_2.get_deleter();
                    const bool is_release_arrow_array_custom_deleter = *del_p
                                                                       == &sparrow::arrow_array_custom_deleter;
                    CHECK(is_release_arrow_array_custom_deleter);
                }
            }
        }

        SUBCASE("make_array_constructor")
        {
            SUBCASE("w/ buffers, shared_ptr children and shared_ptr dictionary")
            {
                std::vector<ArrowArray*> children;
                children.emplace_back(sparrow::default_arrow_array_unique_ptr().release());
                children.emplace_back(sparrow::default_arrow_array_unique_ptr().release());
                const auto children_1_ptr = children[0];
                const auto children_2_ptr = children[1];

                sparrow::arrow_array_shared_ptr dictionary(sparrow::default_arrow_array_unique_ptr());
                const auto dictionary_pointer = dictionary.get();

                const auto array = sparrow::make_arrow_array_unique_ptr(
                    1,
                    0,
                    0,
                    buffers_dummy,
                    children.size(),
                    children.data(),
                    dictionary_pointer
                );

                check_common(array, buffers_dummy, {children_1_ptr, children_2_ptr}, dictionary_pointer);
            }

            SUBCASE("w/ buffers, wo/ children and dictionary")
            {
                const auto array = sparrow::make_arrow_array_unique_ptr(1, 0, 0, buffers_dummy, 0, nullptr, nullptr);
                check_common(array, buffers_dummy, {}, nullptr);
            }
        }

        SUBCASE("release")
        {
            std::vector<ArrowArray*> children;
            children.emplace_back(sparrow::default_arrow_array_unique_ptr().release());
            children.emplace_back(sparrow::default_arrow_array_unique_ptr().release());
            sparrow::arrow_array_unique_ptr dictionary(sparrow::default_arrow_array_unique_ptr());
            auto array = sparrow::make_arrow_array_unique_ptr(
                1,
                0,
                0,
                buffers_dummy,
                0,
                children.data(),
                dictionary.release()
            );

            array->release(array.get());

            CHECK_EQ(array->length, 0);
            CHECK_EQ(array->null_count, 0);
            CHECK_EQ(array->offset, 0);
            CHECK_EQ(array->n_buffers, 0);
            CHECK_EQ(array->n_children, 0);
            CHECK_EQ(array->buffers, nullptr);
            CHECK_EQ(array->children, nullptr);
            const bool is_release_nullptr = array->release == nullptr;
            CHECK(is_release_nullptr);
            CHECK_EQ(array->private_data, nullptr);
        }

        SUBCASE("release wo/ children and dictionary")
        {
            auto array = sparrow::make_arrow_array_unique_ptr(1, 0, 0, buffers_dummy, 0, nullptr, nullptr);

            array->release(array.get());

            CHECK_EQ(array->length, 0);
            CHECK_EQ(array->null_count, 0);
            CHECK_EQ(array->offset, 0);
            CHECK_EQ(array->n_buffers, 0);
            CHECK_EQ(array->n_children, 0);
            CHECK_EQ(array->buffers, nullptr);
            CHECK_EQ(array->children, nullptr);
            const bool is_release_nullptr = array->release == nullptr;
            CHECK(is_release_nullptr);
            CHECK_EQ(array->private_data, nullptr);
        }

        SUBCASE("deep_copy")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            auto array_copy = sparrow::deep_copy_array(array, schema);
            CHECK_EQ(array.length , array_copy.length);
            CHECK_EQ(array.null_count , array_copy.null_count);
            CHECK_EQ(array.offset , array_copy.offset);
            CHECK_EQ(array.n_buffers , array_copy.n_buffers);
            CHECK_EQ(array.n_children , array_copy.n_children);
            CHECK_NE(array.buffers , array_copy.buffers);
            CHECK_NE(array.private_data , array_copy.private_data);
            for(size_t i = 0; i < static_cast<size_t>(array.n_buffers); ++i)
            {
                CHECK_NE(array.buffers[i] , array_copy.buffers[i]);
            }
            auto array_buffers = reinterpret_cast<const int8_t**>(array.buffers);
            auto array_copy_buffers = reinterpret_cast<const int8_t**>(array_copy.buffers);

            for(size_t i = 0; i < static_cast<size_t>(array.length); ++i)
            {
                CHECK_EQ(array_buffers[1][i], array_copy_buffers[1][i]);
            }
        }

        SUBCASE("validate_format_with_arrow_array")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            CHECK(sparrow::validate_format_with_arrow_array(sparrow::data_type::INT8, array));
            CHECK_FALSE(sparrow::validate_format_with_arrow_array(sparrow::data_type::FIXED_SIZED_LIST, array));
        }

        SUBCASE("compute_buffer_size")
        {
            CHECK_EQ(sparrow::compute_buffer_size(
                sparrow::buffer_type::DATA,
                10,
                0,
                sparrow::data_type::INT8
            ), 10);

            CHECK_EQ(sparrow::compute_buffer_size(
                sparrow::buffer_type::DATA,
                10,
                5,
                sparrow::data_type::INT8
            ), 15);

            CHECK_EQ(sparrow::compute_buffer_size(
                sparrow::buffer_type::DATA,
                10,
                5,
                sparrow::data_type::INT16
            ), 30);

            CHECK_EQ(sparrow::compute_buffer_size(
                sparrow::buffer_type::DATA,
                10,
                5,
                sparrow::data_type::INT32
            ), 60);

            CHECK_EQ(sparrow::compute_buffer_size(
                sparrow::buffer_type::VALIDITY,
                10,
                5,
                sparrow::data_type::UINT8
            ), 2);

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
