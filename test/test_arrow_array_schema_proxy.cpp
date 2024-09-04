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
#include <utility>

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/buffer/buffer_adaptor.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/c_interface.hpp"

#include "doctest/doctest.h"
#include "external_array_data_creation.hpp"

std::pair<ArrowSchema, ArrowArray> make_default_arrow_schema_and_array()
{
    std::pair<ArrowSchema, ArrowArray> pair;
    constexpr size_t size = 10;
    constexpr size_t offset = 1;
    sparrow::test::fill_schema_and_array<uint32_t>(pair.first, pair.second, size, offset, {2, 3});
    return pair;
}

TEST_SUITE("ArrowArrowSchemaProxy")
{
    TEST_CASE("constructors")
    {
        SUBCASE("move")
        {
            auto [schema, array] = make_default_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
        }

        SUBCASE("move pointer")
        {
            auto [schema, array] = make_default_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(std::move(array), &schema);
        }

        SUBCASE("pointer")
        {
            auto [schema, array] = make_default_arrow_schema_and_array();
            sparrow::arrow_proxy proxy(&array, &schema);
        }
    }

    TEST_CASE("destructor")
    {
        SUBCASE("move")
        {
            auto [schema, array] = make_default_arrow_schema_and_array();
            {
                sparrow::arrow_proxy proxy(std::move(array), std::move(schema));
            }
        }

        SUBCASE("move pointer")
        {
            auto [schema, array] = make_default_arrow_schema_and_array();
            {
                sparrow::arrow_proxy proxy(std::move(array), &schema);
            }
            CHECK_NE(schema.release, nullptr);
        }

        SUBCASE("pointer")
        {
            auto [schema, array] = make_default_arrow_schema_and_array();
            {
                sparrow::arrow_proxy proxy(&array, &schema);
            }
            CHECK_NE(schema.release, nullptr);
            CHECK_NE(schema.release, nullptr);
        }
    }

    TEST_CASE("format")
    {
        auto [schema, array] = make_default_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(&array, &schema);
        CHECK_EQ(proxy.format(), "I");
    }

    TEST_CASE("name")
    {
        auto [schema, array] = make_default_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(&array, &schema);
        CHECK_EQ(proxy.name(), "test");
    }

    TEST_CASE("metadata")
    {
        auto [schema, array] = make_default_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(&array, &schema);
        CHECK_EQ(proxy.metadata(), "test metadata");
    }

    TEST_CASE("flags")
    {
        auto [schema, array] = make_default_arrow_schema_and_array();
        schema.flags |= static_cast<int64_t>(sparrow::ArrowFlag::MAP_KEYS_SORTED)
                        | static_cast<int64_t>(sparrow::ArrowFlag::NULLABLE);
        const sparrow::arrow_proxy proxy(&array, &schema);
        const auto flags = proxy.flags();
        REQUIRE_EQ(flags.size(), 2);
        CHECK_EQ(flags[0], sparrow::ArrowFlag::NULLABLE);
        CHECK_EQ(flags[1], sparrow::ArrowFlag::MAP_KEYS_SORTED);
    }

    TEST_CASE("length")
    {
        auto [schema, array] = make_default_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(&array, &schema);
        CHECK_EQ(proxy.length(), 10);
    }

    TEST_CASE("null_count")
    {
        auto [schema, array] = make_default_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(&array, &schema);
        CHECK_EQ(proxy.null_count(), 2);
    }

    TEST_CASE("offset")
    {
        auto [schema, array] = make_default_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(&array, &schema);
        CHECK_EQ(proxy.offset(), 1);
    }

    TEST_CASE("n_buffers")
    {
        auto [schema, array] = make_default_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(&array, &schema);
        CHECK_EQ(proxy.n_buffers(), 2);
    }

    TEST_CASE("n_children")
    {
        auto [schema, array] = make_default_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(&array, &schema);
        CHECK_EQ(proxy.n_children(), 0);
    }

    TEST_CASE("buffers")
    {
        auto [schema, array] = make_default_arrow_schema_and_array();
        sparrow::arrow_proxy proxy(&array, &schema);
        auto buffers = proxy.buffers();
        REQUIRE_EQ(buffers.size(), 2);
        CHECK_EQ(buffers[0].size(), 2);
        sparrow::dynamic_bitset<uint8_t> bitmap(buffers[0].data(), 10);
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
        CHECK_EQ(buffers[1].size(), sizeof(uint32_t) * 10);
        const auto values = sparrow::make_buffer_adaptor<const uint32_t>(buffers[1]);
        REQUIRE_EQ(values.size(), 10);
        for (std::size_t i = 0; i < 10; ++i)
        {
            CHECK_EQ(values[i], i);
        }
    }

    TEST_CASE("children")
    {
        auto [schema, array] = make_default_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(&array, &schema);
        const auto children = proxy.children();
        CHECK_EQ(children.size(), 0);
    }

    TEST_CASE("dictionary")
    {
        auto [schema, array] = make_default_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(&array, &schema);
        CHECK_FALSE(proxy.dictionary().has_value());
    }

    TEST_CASE("release")
    {
        auto [schema, array] = make_default_arrow_schema_and_array();
        sparrow::arrow_proxy proxy(&array, &schema);
        proxy.release();
        const bool array_release_is_null = array.release == nullptr;
        CHECK(array_release_is_null);
        const bool schema_release_is_null = schema.release == nullptr;
        CHECK(schema_release_is_null);
    }

    TEST_CASE("is_released")
    {
        auto [schema, array] = make_default_arrow_schema_and_array();
        sparrow::arrow_proxy proxy(&array, &schema);
        CHECK_FALSE(proxy.is_released());
        proxy.release();
        CHECK(proxy.is_released());
    }

    TEST_CASE("is_created_with_sparrow")
    {
        auto [schema, array] = make_default_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(&array, &schema);
        CHECK_FALSE(proxy.is_created_with_sparrow());
    }

    TEST_CASE("private_data")
    {
        auto [schema, array] = make_default_arrow_schema_and_array();
        const sparrow::arrow_proxy proxy(&array, &schema);
        CHECK_EQ(proxy.private_data(), nullptr);
    }
}
