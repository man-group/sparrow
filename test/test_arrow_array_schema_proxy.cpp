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


#include <string_view>

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/c_interface.hpp"

#include "arrow_array_schema_creation.hpp"
#include "doctest/doctest.h"


TEST_SUITE("ArrowArrowSchemaProxy")
{
    TEST_CASE("constructors")
    {
        SUBCASE("move")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            const bool is_schema_release_null = schema.release == nullptr;
            const bool is_array_release_null = array.release == nullptr;
            CHECK(is_schema_release_null);
            CHECK(is_array_release_null);
        }

        SUBCASE("pointer")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(&array, &schema);
            const bool is_schema_release_null = schema.release == nullptr;
            const bool is_array_release_null = array.release == nullptr;
            CHECK_FALSE(is_schema_release_null);
            CHECK_FALSE(is_array_release_null);
        }
    }

    TEST_CASE("destructor")
    {
        SUBCASE("move")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            {
                sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            }
        }

        SUBCASE("move pointer")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            {
                sparrow::arrow_proxy proxy(std::move(array), &schema);
            }
            const bool is_schema_release_null = schema.release == nullptr;
            CHECK_FALSE(is_schema_release_null);
        }

        SUBCASE("pointer")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            {
                sparrow::arrow_proxy proxy(&array, &schema);
            }
            const bool is_schema_release_null = schema.release == nullptr;
            const bool is_array_release_null = array.release == nullptr;
            CHECK_FALSE(is_schema_release_null);
            CHECK_FALSE(is_array_release_null);
        }
    }

    TEST_CASE("move semantics")
    {
        SUBCASE("move constructor")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            auto proxy2{std::move(proxy)};
            CHECK_EQ(proxy2.format(), "C");
        }

        SUBCASE("move assignment")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            sparrow::arrow_proxy proxy{std::move(array), std::move(schema)};

            auto [schema2, array2] = make_sparrow_arrow_schema_and_array();
            const sparrow::arrow_proxy proxy2{std::move(array2), std::move(schema2)};
            proxy = std::move(proxy2);
            CHECK_EQ(proxy.format(), "C");
        }
    }

    TEST_CASE("copy semantics")
    {
        SUBCASE("copy constructor")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            const sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            auto proxy2(proxy);
            CHECK_EQ(proxy2.format(), "C");
            proxy2.set_format("L");
            CHECK_EQ(proxy.format(), "C");
        }

        SUBCASE("copy assignment")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            const sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            sparrow::arrow_proxy proxy2 = proxy;
            CHECK_EQ(proxy.format(), "C");
            CHECK_EQ(proxy2.format(), "C");
            proxy2.set_format("L");
            CHECK_EQ(proxy.format(), "C");
        }
    }

    TEST_CASE("format")
    {
        auto [schema, array] = make_sparrow_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
        CHECK_EQ(proxy.format(), "C");
    }

    TEST_CASE("set_format")
    {
        SUBCASE("on sparrow c structure")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            constexpr std::string_view format = "U";
            proxy.set_format(format);
            CHECK_EQ(proxy.format(), format);
        }

        SUBCASE("on external c structure")
        {
            auto [schema, array] = make_external_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            CHECK_THROWS_AS(proxy.set_format("U"), std::runtime_error);
        }
    }

    TEST_CASE("name")
    {
        auto [schema, array] = make_sparrow_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
        CHECK_EQ(proxy.name(), "test");
    }

    TEST_CASE("set_name")
    {
        SUBCASE("on sparrow c structure")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            proxy.set_name("new name");
            CHECK_EQ(proxy.name(), "new name");
        }

        SUBCASE("on external c structure")
        {
            auto [schema, array] = make_external_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            CHECK_THROWS_AS(proxy.set_name("new name"), std::runtime_error);
        }
    }

    TEST_CASE("metadata")
    {
        auto [schema, array] = make_sparrow_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
        CHECK_EQ(proxy.metadata(), "test metadata");
    }

    TEST_CASE("set_metadata")
    {
        SUBCASE("on sparrow c structure")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            proxy.set_metadata("new metadata");
            CHECK_EQ(proxy.metadata(), "new metadata");
        }

        SUBCASE("on external c structure")
        {
            auto [schema, array] = make_external_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            CHECK_THROWS_AS(proxy.set_metadata("new metadata"), std::runtime_error);
        }
    }

    TEST_CASE("flags")
    {
        auto [schema, array] = make_sparrow_arrow_schema_and_array();
        schema.flags |= static_cast<int64_t>(sparrow::ArrowFlag::MAP_KEYS_SORTED)
                        | static_cast<int64_t>(sparrow::ArrowFlag::NULLABLE);
        const sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
        const auto flags = proxy.flags();
        REQUIRE_EQ(flags.size(), 2);
        CHECK_EQ(flags[0], sparrow::ArrowFlag::NULLABLE);
        CHECK_EQ(flags[1], sparrow::ArrowFlag::MAP_KEYS_SORTED);
    }

    TEST_CASE("set_flags")
    {
        SUBCASE("on sparrow c structure")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            proxy.set_flags({sparrow::ArrowFlag::DICTIONARY_ORDERED, sparrow::ArrowFlag::NULLABLE});
            const auto flags = proxy.flags();
            REQUIRE_EQ(flags.size(), 2);
            CHECK_EQ(flags[0], sparrow::ArrowFlag::DICTIONARY_ORDERED);
            CHECK_EQ(flags[1], sparrow::ArrowFlag::NULLABLE);
        }

        SUBCASE("on external c structure")
        {
            auto [schema, array] = make_external_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            CHECK_THROWS_AS(
                proxy.set_flags({sparrow::ArrowFlag::DICTIONARY_ORDERED, sparrow::ArrowFlag::NULLABLE}),
                std::runtime_error
            );
        }
    }

    TEST_CASE("length")
    {
        auto [schema, array] = make_sparrow_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
        CHECK_EQ(proxy.length(), 10);
    }

    TEST_CASE("set_length")
    {
        SUBCASE("on sparrow c structure")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            proxy.set_length(20);
            CHECK_EQ(proxy.length(), 20);
        }

        SUBCASE("on external c structure")
        {
            auto [schema, array] = make_external_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            CHECK_THROWS_AS(proxy.set_length(20), std::runtime_error);
        }
    }

    TEST_CASE("null_count")
    {
        auto [schema, array] = make_sparrow_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
        CHECK_EQ(proxy.null_count(), 2);
    }

    TEST_CASE("set_null_count")
    {
        SUBCASE("on sparrow c structure")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            proxy.set_null_count(5);
            CHECK_EQ(proxy.null_count(), 5);
        }

        SUBCASE("on external c structure")
        {
            auto [schema, array] = make_external_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            CHECK_THROWS_AS(proxy.set_null_count(5), std::runtime_error);
        }
    }

    TEST_CASE("offset")
    {
        auto [schema, array] = make_sparrow_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
        CHECK_EQ(proxy.offset(), 0);
    }

    TEST_CASE("set_offset")
    {
        SUBCASE("on sparrow c structure")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            proxy.set_offset(5);
            CHECK_EQ(proxy.offset(), 5);
        }

        SUBCASE("on external c structure")
        {
            auto [schema, array] = make_external_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            CHECK_THROWS_AS(proxy.set_offset(5), std::runtime_error);
        }
    }

    TEST_CASE("n_buffers")
    {
        auto [schema, array] = make_sparrow_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
        CHECK_EQ(proxy.n_buffers(), 2);
    }

    TEST_CASE("set_n_buffers")
    {
        SUBCASE("on sparrow c structure")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            CHECK_EQ(proxy.n_children(), 0);
            proxy.set_n_buffers(3);
            CHECK_EQ(proxy.n_buffers(), 3);
        }

        SUBCASE("on external c structure")
        {
            auto [schema, array] = make_external_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            CHECK_THROWS_AS(proxy.set_n_buffers(3), std::runtime_error);
        }
    }

    TEST_CASE("n_children")
    {
        auto [schema, array] = make_sparrow_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
        CHECK_EQ(proxy.n_children(), 0);
    }

    TEST_CASE("buffers")
    {
        auto [schema, array] = make_sparrow_arrow_schema_and_array();
        sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
        auto buffers = proxy.buffers();
        REQUIRE_EQ(buffers.size(), 2);
        CHECK_EQ(buffers[0].size(), 2);
        const sparrow::dynamic_bitset_view<uint8_t> bitmap(buffers[0].data(), 10);
        CHECK(bitmap.test(0));
        CHECK(bitmap.test(1));
        CHECK_FALSE(bitmap.test(2));
        CHECK_FALSE(bitmap.test(3));
        CHECK(bitmap.test(4));
        CHECK(bitmap.test(5));
        CHECK(bitmap.test(6));
        CHECK(bitmap.test(7));
        CHECK(bitmap.test(8));
        CHECK(bitmap.test(9));
        CHECK_EQ(buffers[1].size(), 10);
        for (std::size_t i = 0; i < 10; ++i)
        {
            CHECK_EQ(buffers[1][i], i);
        }
    }

    TEST_CASE("set_buffer")
    {
        SUBCASE("on sparrow c structure")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            auto buffer = sparrow::buffer<uint8_t>(10);
            for (auto& element : buffer)
            {
                element = 9;
            }
            proxy.set_buffer(1, std::move(buffer));
            auto buffers = proxy.buffers();
            REQUIRE_EQ(buffers.size(), 2);
            CHECK_EQ(buffers[0].size(), 2);
            CHECK_EQ(buffers[1].size(), 10);
            auto& buffer_view = buffers[1];
            for (auto element : buffer_view)
            {
                CHECK_EQ(element, 9);
            }
        }

        SUBCASE("on external c structure")
        {
            auto [schema, array] = make_external_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            auto buffer = sparrow::buffer<uint8_t>({1, 2, 3});
            CHECK_THROWS_AS(proxy.set_buffer(1, std::move(buffer)), std::runtime_error);
        }
    }

    TEST_CASE("children")
    {
        auto [schema, array] = make_sparrow_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
        const auto children = proxy.children();
        CHECK_EQ(children.size(), 0);
    }

    TEST_CASE("add_children")
    {
        SUBCASE("on sparrow c structure")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            auto array_schema_pair = make_sparrow_arrow_schema_and_array();
            std::array<sparrow::arrow_array_and_schema_pointers,1> array_child_ptr{{{&array_schema_pair.second ,&array_schema_pair.first}}};
            proxy.add_children(array_child_ptr);
            const auto children = proxy.children();
            CHECK_EQ(children.size(), 1);
            CHECK_EQ(children[0].format(), "C");
        }
    }

    TEST_CASE("pop_children")
    {
        SUBCASE("on sparrow c structure")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));

            auto array_schema_pair  = make_sparrow_arrow_schema_and_array();
            std::array<sparrow::arrow_array_and_schema_pointers,1> array_child_ptr{{{&array_schema_pair.second ,&array_schema_pair.first}}};
            proxy.add_children(array_child_ptr);
            proxy.pop_children(1);
            const auto& children = proxy.children();
            CHECK_EQ(children.size(), 0);
            CHECK_EQ(proxy.n_children(), 0);
        }
    }

    TEST_CASE("dictionary")
    {
        auto [schema, array] = make_sparrow_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
        CHECK_FALSE(proxy.dictionary());
    }

    TEST_CASE("set_dictionary")
    {
        SUBCASE("on sparrow c structure")
        {
            auto [schema, array] = make_sparrow_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            auto array_schema_pair  = make_sparrow_arrow_schema_and_array();
            proxy.set_dictionary(&array_schema_pair.second, &array_schema_pair.first);
            const auto& dictionary = proxy.dictionary();
            REQUIRE(dictionary);
            CHECK_EQ(dictionary->format(), "C");
        }

        SUBCASE("on external c structure")
        {
            auto [schema, array] = make_external_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            auto [schema_dict, array_dict] = make_external_arrow_schema_and_array();
            CHECK_THROWS_AS(proxy.set_dictionary(&array_dict, &schema_dict), std::runtime_error);
        }
    }

    TEST_CASE("is_created_with_sparrow")
    {
        auto [schema, array] = make_sparrow_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
        CHECK(proxy.is_created_with_sparrow());

        auto [schema_ext, array_ext] = make_external_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy_ext(std::move(array_ext), std::move(schema_ext));
        CHECK_FALSE(proxy_ext.is_created_with_sparrow());
    }

    TEST_CASE("private_data")
    {
        auto [schema, array] = make_sparrow_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
        CHECK_NE(proxy.private_data(), nullptr);

        auto [schema_ext, array_ext] = make_external_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy_ext(std::move(array_ext), std::move(schema_ext));
        CHECK_EQ(proxy_ext.private_data(), nullptr);
    }
}
