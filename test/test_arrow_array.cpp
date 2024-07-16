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

#include "sparrow/arrow_array.hpp"
#include "sparrow/buffer.hpp"
#include "sparrow/mp_utils.hpp"

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
    ArrowArray* dictionary_ptr
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
    CHECK_EQ(array->dictionary, dictionary_ptr);
    CHECK_EQ(array->release, sparrow::delete_array);
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
                CHECK_EQ(array->release, nullptr);
                CHECK_EQ(array->private_data, nullptr);
            }

            SUBCASE("default")
            {
                const sparrow::arrow_array_unique_ptr array;
                CHECK_EQ(array, nullptr);
            }

            SUBCASE("nullptr")
            {
                const sparrow::arrow_array_unique_ptr array(nullptr);
                CHECK_EQ(array, nullptr);
            }
        }

        SUBCASE("arrow_array_shared_ptr")
        {
            SUBCASE("default")
            {
                const sparrow::arrow_array_shared_ptr array;
                CHECK_EQ(array, nullptr);
                auto deleter = std::get_deleter<void (*)(ArrowArray*)>(array);
                CHECK_EQ(*deleter, &sparrow::arrow_array_custom_deleter);
            }

            SUBCASE("nullptr")
            {
                const sparrow::arrow_array_shared_ptr array(nullptr);
                CHECK_EQ(array, nullptr);
                auto deleter = std::get_deleter<void (*)(ArrowArray*)>(array);
                CHECK_EQ(*deleter, &sparrow::arrow_array_custom_deleter);
            }

            SUBCASE("arrow_array_unique_ptr")
            {
                sparrow::arrow_array_unique_ptr array = sparrow::default_arrow_array_unique_ptr();
                array->length = 99;
                array->null_count = 42;
                const sparrow::arrow_array_shared_ptr shared_array(std::move(array));
                CHECK_EQ(shared_array->length, 99);
                CHECK_EQ(shared_array->null_count, 42);
            }
        }

        SUBCASE("make_array_constructor")
        {
            SUBCASE("w/ buffers, shared_ptr children and shared_ptr dictionary")
            {
                std::vector<sparrow::arrow_array_shared_ptr> children;
                children.emplace_back(sparrow::default_arrow_array_unique_ptr());
                children.emplace_back(sparrow::default_arrow_array_unique_ptr());
                const auto children_1_ptr = children[0].get();
                const auto children_2_ptr = children[1].get();

                sparrow::arrow_array_shared_ptr dictionary(sparrow::default_arrow_array_unique_ptr());
                const auto dictionary_ptr = dictionary.get();

                const auto array = sparrow::make_arrow_array_unique_ptr(
                    1,
                    0,
                    0,
                    buffers_dummy,
                    std::move(children),
                    std::move(dictionary)
                );

                CHECK(children.empty());
                CHECK_EQ(dictionary, nullptr);
                check_common(array, buffers_dummy, {children_1_ptr, children_2_ptr}, dictionary_ptr);
            }

            SUBCASE("w/ buffers, wo/ children and dictionary")
            {
                const auto array = sparrow::make_arrow_array_unique_ptr(1, 0, 0, buffers_dummy, std::vector<sparrow::arrow_array_shared_ptr>{}, nullptr);

                check_common(array, buffers_dummy, {}, nullptr);
            }
        }

        SUBCASE("release")
        {
            std::vector<sparrow::arrow_array_shared_ptr> children;
            children.emplace_back(sparrow::default_arrow_array_unique_ptr());
            children.emplace_back(sparrow::default_arrow_array_unique_ptr());
            sparrow::arrow_array_unique_ptr dictionary(sparrow::default_arrow_array_unique_ptr());
            auto array = sparrow::make_arrow_array_unique_ptr(
                1,
                0,
                0,
                buffers_dummy,
                std::move(children),
                std::move(dictionary)
            );

            array->release(array.get());

            CHECK_EQ(array->length, 0);
            CHECK_EQ(array->null_count, 0);
            CHECK_EQ(array->offset, 0);
            CHECK_EQ(array->n_buffers, 0);
            CHECK_EQ(array->n_children, 0);
            CHECK_EQ(array->buffers, nullptr);
            CHECK_EQ(array->children, nullptr);
            CHECK_EQ(array->release, nullptr);
            CHECK_EQ(array->private_data, nullptr);
        }

        SUBCASE("release wo/ children and dictionary")
        {
            auto array = sparrow::make_arrow_array_unique_ptr(1, 0, 0, buffers_dummy, std::vector<sparrow::arrow_array_shared_ptr>{}, nullptr);

            array->release(array.get());

            CHECK_EQ(array->length, 0);
            CHECK_EQ(array->null_count, 0);
            CHECK_EQ(array->offset, 0);
            CHECK_EQ(array->n_buffers, 0);
            CHECK_EQ(array->n_children, 0);
            CHECK_EQ(array->buffers, nullptr);
            CHECK_EQ(array->children, nullptr);
            CHECK_EQ(array->release, nullptr);
            CHECK_EQ(array->private_data, nullptr);
        }
    }
}

TEST_SUITE("arrow_array_private_data")
{
    TEST_CASE("buffers")
    {
        const auto buffers = buffers_dummy;
        auto children = std::vector<sparrow::arrow_array_shared_ptr>{};
        children.emplace_back(sparrow::default_arrow_array_unique_ptr());
        children.back()->null_count = 99;
        auto dictionary = sparrow::arrow_array_shared_ptr{sparrow::default_arrow_array_unique_ptr()};
        dictionary->null_count = 11;
        CHECK_EQ(children.back().use_count(), 1);
        CHECK_EQ(dictionary.use_count(), 1);
        sparrow::arrow_array_private_data private_data(buffers, children, dictionary);
        CHECK_EQ(children.back().use_count(), 2);
        CHECK_EQ(dictionary.use_count(), 2);

        auto buffers_ptrs = private_data.buffers_ptrs<int8_t>();
        for (size_t i = 0; i < buffers.size(); ++i)
        {
            for (size_t j = 0; j < buffers[i].size(); ++j)
            {
                CHECK_EQ(buffers_ptrs[i][j], buffers[i][j]);
            }
        }

        auto children_ptrs = private_data.children_ptrs();
        for (size_t i = 0; i < children.size(); ++i)
        {
            CHECK_EQ(children_ptrs[i], children[i].get());
        }

        CHECK_EQ(private_data.dictionary_ptr(), dictionary.get());

        auto children_2 = private_data.children();
        CHECK_EQ(children_2.back().get(), children.back().get());
    }
}
