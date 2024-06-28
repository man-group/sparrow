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

#include <memory>
#include <ranges>

#include "sparrow/arrow_array.hpp"
#include "sparrow/buffer.hpp"
#include "sparrow/mp_utils.hpp"

#include "doctest/doctest.h"

using buffer_type = sparrow::buffer<int32_t>;
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
    const int32_t** buffer_ptr = reinterpret_cast<const int32_t**>(array->buffers);
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
    CHECK_EQ(array->release, sparrow::delete_array<int>);
    CHECK_NE(array->private_data, nullptr);
}

TEST_SUITE("C Data Interface")
{
    TEST_CASE("ArrowArray")
    {
        SUBCASE("arrow_array_unique_ptr")
        {
            SUBCASE("sparrow::default_arrow_array")
            {
                static_assert(std::same_as<
                              sparrow::mpl::get_deleter_type_t<sparrow::arrow_array_unique_ptr>,
                              sparrow::arrow_array_custom_deleter_struct>);
                const sparrow::arrow_array_unique_ptr array = sparrow::default_arrow_array();
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
                sparrow::arrow_array_unique_ptr array = sparrow::default_arrow_array();
                array->length = 99;
                array->null_count = 42;
                const sparrow::arrow_array_shared_ptr shared_array(std::move(array));
                CHECK_EQ(shared_array->length, 99);
                CHECK_EQ(shared_array->null_count, 42);
            }
        }

        SUBCASE("make_array_constructor")
        {
            SUBCASE("w/ buffers, unique_ptr children and unique_ptr dictionary")
            {
                std::vector<sparrow::arrow_array_unique_ptr> children;
                children.emplace_back(sparrow::default_arrow_array());
                children.emplace_back(sparrow::default_arrow_array());
                const auto children_1_ptr = children[0].get();
                const auto children_2_ptr = children[1].get();

                sparrow::arrow_array_unique_ptr dictionary(sparrow::default_arrow_array());
                const auto dictionary_ptr = dictionary.get();

                const auto array = sparrow::make_arrow_array<int32_t>(
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

            SUBCASE("w/ shared_ptr buffers, children and shared_ptr dictionary")
            {
                std::vector<sparrow::arrow_array_shared_ptr> children;
                children.emplace_back(sparrow::default_arrow_array());
                children.emplace_back(sparrow::default_arrow_array());
                const auto children_1_ptr = children[0].get();
                const auto children_2_ptr = children[1].get();

                sparrow::arrow_array_shared_ptr dictionary(sparrow::default_arrow_array());
                const auto dictionary_ptr = dictionary.get();

                std::vector<std::shared_ptr<buffer_type>> buffers;
                for (const auto& buffer : buffers_dummy)
                {
                    buffers.emplace_back(std::make_shared<buffer_type>(buffer));
                }

                const auto array = sparrow::make_arrow_array<int32_t>(1, 0, 0, buffers, children, dictionary);

                CHECK_EQ(children.at(0).use_count(), 2);
                CHECK_EQ(children.at(1).use_count(), 2);
                CHECK_EQ(dictionary.use_count(), 2);
                check_common(array, buffers_dummy, {children_1_ptr, children_2_ptr}, dictionary_ptr);
            }

            SUBCASE("w/ pointers buffers, children and dictionary")
            {
                std::vector<sparrow::arrow_array_unique_ptr> children;
                children.emplace_back(sparrow::default_arrow_array());
                children.emplace_back(sparrow::default_arrow_array());
                const auto children_1_ptr = children[0].get();
                const auto children_2_ptr = children[1].get();
                std::vector<ArrowArray*> children_ptr{children_1_ptr, children_2_ptr};

                sparrow::arrow_array_unique_ptr dictionary(sparrow::default_arrow_array());

                auto buffers = buffers_dummy;

                std::vector<int32_t*> buffers_ptr;
                for (auto& buffer : buffers)
                {
                    buffers_ptr.push_back(buffer.data());
                }

                const auto array = sparrow::make_arrow_array<int32_t>(
                    1,
                    0,
                    0,
                    static_cast<int64_t>(buffers_ptr.size()),
                    buffers_ptr.data(),
                    static_cast<int64_t>(children_ptr.size()),
                    children_ptr.data(),
                    dictionary.get()
                );

                check_common(array, buffers_dummy, children_ptr, dictionary.get());
            }

            SUBCASE("w/ std::tuple for buffers and children")
            {
                std::tuple<sparrow::buffer<int>, std::vector<int64_t>> buffers_tuple{buffer_dummy, {0, 1, 2}};

                std::tuple<sparrow::arrow_array_unique_ptr, sparrow::arrow_array_shared_ptr> children_tuple{
                    sparrow::default_arrow_array(),
                    sparrow::default_arrow_array()
                };

                const auto array = sparrow::make_arrow_array<int>(
                    1,
                    0,
                    0,
                    std::move(buffers_tuple),
                    std::move(children_tuple),
                    nullptr
                );
            }

            SUBCASE("w/ buffers, wo/ children and dictionary")
            {
                const auto array = sparrow::make_arrow_array<int>(1, 0, 0, buffers_dummy, nullptr, nullptr);

                check_common(array, buffers_dummy, {}, nullptr);
            }
        }

        SUBCASE("release")
        {
            std::vector<sparrow::arrow_array_unique_ptr> children;
            children.emplace_back(sparrow::default_arrow_array());
            children.emplace_back(sparrow::default_arrow_array());
            sparrow::arrow_array_unique_ptr dictionary(sparrow::default_arrow_array());
            auto array = sparrow::make_arrow_array<int>(
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
            auto array = sparrow::make_arrow_array<int>(1, 0, 0, buffers_dummy, nullptr, nullptr);

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
        children.emplace_back(sparrow::default_arrow_array());
        children.back()->null_count = 99;
        auto dictionary = sparrow::arrow_array_shared_ptr{sparrow::default_arrow_array()};
        dictionary->null_count = 11;
        CHECK_EQ(children.back().use_count(), 1);
        CHECK_EQ(dictionary.use_count(), 1);
        sparrow::arrow_array_private_data<int> private_data(buffers, children, dictionary);
        CHECK_EQ(children.back().use_count(), 2);
        CHECK_EQ(dictionary.use_count(), 2);

        auto buffers_ptrs = private_data.buffers_ptrs();
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

        auto lol = std::move(private_data.children());
        CHECK_EQ(lol.get_pointers_vec().back(), children.back().get());
    }
}
