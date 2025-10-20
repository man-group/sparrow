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

#include "sparrow/utils/metadata.hpp"

#include "doctest/doctest.h"
#include "metadata_sample.hpp"

TEST_SUITE("metadata")
{
    TEST_CASE("key_value_view")
    {
        const sparrow::key_value_view key_values(sparrow::metadata_buffer.data());
        CHECK_EQ(key_values.size(), 2);
        auto kv_it = key_values.cbegin();
        auto kv_1 = *kv_it;
        CHECK_EQ(kv_1.first, "key1");
        CHECK_EQ(kv_1.second, "val1");
        ++kv_it;
        auto kv_2 = *kv_it;
        CHECK_EQ(kv_2.first, "key2");
        CHECK_EQ(kv_2.second, "val2");

        // Test iterator end
        kv_it = key_values.cbegin();
        auto kv_end = key_values.cend();
        while (kv_it != kv_end)
        {
            ++kv_it;
        }

        CHECK_EQ(kv_it, kv_end);
    }

    TEST_CASE("get_metadata_from_key_values")
    {
        const auto metadata_result = sparrow::get_metadata_from_key_values(sparrow::metadata_sample);
        CHECK_EQ(sparrow::metadata_buffer, metadata_result);
    }

    TEST_CASE("empty")
    {
        SUBCASE("non-empty view")
        {
            const sparrow::key_value_view key_values(sparrow::metadata_buffer.data());
            CHECK_FALSE(key_values.empty());
        }

        SUBCASE("empty view")
        {
            // Create an empty metadata buffer
            const std::vector<sparrow::metadata_pair> empty_metadata;
            const auto empty_buffer = sparrow::get_metadata_from_key_values(empty_metadata);
            const sparrow::key_value_view empty_key_values(empty_buffer.data());
            CHECK(empty_key_values.empty());
            CHECK_EQ(empty_key_values.size(), 0);
        }
    }

    TEST_CASE("find")
    {
        const sparrow::key_value_view key_values(sparrow::metadata_buffer.data());

        SUBCASE("find existing key - first element")
        {
            auto it = key_values.find("key1");
            REQUIRE(it != key_values.end());
            CHECK_EQ((*it).first, "key1");
            CHECK_EQ((*it).second, "val1");
        }

        SUBCASE("find existing key - last element")
        {
            auto it = key_values.find("key2");
            REQUIRE(it != key_values.end());
            CHECK_EQ((*it).first, "key2");
            CHECK_EQ((*it).second, "val2");
        }

        SUBCASE("find non-existing key")
        {
            auto it = key_values.find("key3");
            CHECK_EQ(it, key_values.end());
        }

        SUBCASE("find with empty string")
        {
            auto it = key_values.find("");
            CHECK_EQ(it, key_values.end());
        }

        SUBCASE("find in empty view")
        {
            const std::vector<sparrow::metadata_pair> empty_metadata;
            const auto empty_buffer = sparrow::get_metadata_from_key_values(empty_metadata);
            const sparrow::key_value_view empty_key_values(empty_buffer.data());

            auto it = empty_key_values.find("key1");
            CHECK_EQ(it, empty_key_values.end());
        }
    }
}
