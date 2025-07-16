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
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "sparrow/c_data_integration/utils.hpp"

#include "doctest/doctest.h"

namespace sparrow::c_data_integration::utils
{
    TEST_SUITE("utils")
    {
        TEST_CASE("hex_string_to_bytes")
        {
            SUBCASE("empty string")
            {
                const auto result = hex_string_to_bytes("");
                CHECK(result.empty());
            }

            SUBCASE("simple hex conversion")
            {
                const auto result = hex_string_to_bytes("48656c6c6f");
                const std::vector<std::byte> expected =
                    {std::byte{0x48}, std::byte{0x65}, std::byte{0x6c}, std::byte{0x6c}, std::byte{0x6f}};
                CHECK(result == expected);
            }

            SUBCASE("single byte")
            {
                const auto result = hex_string_to_bytes("FF");
                const std::vector<std::byte> expected = {std::byte{0xFF}};
                CHECK(result == expected);
            }

            SUBCASE("zero byte")
            {
                const auto result = hex_string_to_bytes("00");
                const std::vector<std::byte> expected = {std::byte{0x00}};
                CHECK(result == expected);
            }

            SUBCASE("multiple bytes")
            {
                const auto result = hex_string_to_bytes("DEADBEEF");
                const std::vector<std::byte> expected =
                    {std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF}};
                CHECK(result == expected);
            }

            SUBCASE("odd length string - only processes complete byte pairs")
            {
                const auto result = hex_string_to_bytes("ABC");
                const std::vector<std::byte> expected = {std::byte{0xAB}, std::byte{0x0C}};
                CHECK(result == expected);
            }

            SUBCASE("invalid hex characters - skips invalid bytes")
            {
                const auto result = hex_string_to_bytes("XY");
                CHECK(result.empty());
            }
        }

        TEST_CASE("hex_strings_to_bytes")
        {
            SUBCASE("empty vector")
            {
                const auto result = hex_strings_to_bytes({});
                CHECK(result.empty());
            }

            SUBCASE("single hex string")
            {
                const auto result = hex_strings_to_bytes({"48656c6c6f"});
                CHECK(result.size() == 1);
                const std::vector<std::byte> expected =
                    {std::byte{0x48}, std::byte{0x65}, std::byte{0x6c}, std::byte{0x6c}, std::byte{0x6f}};
                CHECK(result[0] == expected);
            }

            SUBCASE("multiple hex strings")
            {
                const auto result = hex_strings_to_bytes({"FF", "00", "DEAD"});
                CHECK(result.size() == 3);

                const std::vector<std::byte> expected1 = {std::byte{0xFF}};
                const std::vector<std::byte> expected2 = {std::byte{0x00}};
                const std::vector<std::byte> expected3 = {std::byte{0xDE}, std::byte{0xAD}};

                CHECK(result[0] == expected1);
                CHECK(result[1] == expected2);
                CHECK(result[2] == expected3);
            }

            SUBCASE("mix of valid and empty strings")
            {
                const auto result = hex_strings_to_bytes({"", "FF", ""});
                CHECK(result.size() == 3);
                CHECK(result[0].empty());
                CHECK(result[1] == std::vector<std::byte>{std::byte{0xFF}});
                CHECK(result[2].empty());
            }
        }

        TEST_CASE("get_children_with_same_name")
        {
            SUBCASE("single matching child")
            {
                const nlohmann::json schema = nlohmann::json::parse(R"({
                    "children": [
                        {"name": "field1", "type": "int32"},
                        {"name": "field2", "type": "string"}
                    ]
                })");

                const auto result = get_children_with_same_name(schema, "field1");
                CHECK(result.size() == 1);
                CHECK(result[0]["name"].get<std::string>() == "field1");
            }

            SUBCASE("multiple matching children")
            {
                const nlohmann::json schema = nlohmann::json::parse(R"({
                    "children": [
                        {"name": "field1", "type": "int32"},
                        {"name": "field1", "type": "string"},
                        {"name": "field2", "type": "float"}
                    ]
                })");

                const auto result = get_children_with_same_name(schema, "field1");
                CHECK(result.size() == 2);
                CHECK(result[0]["name"].get<std::string>() == "field1");
                CHECK(result[1]["name"].get<std::string>() == "field1");
            }

            SUBCASE("no matching children - throws exception")
            {
                const nlohmann::json schema = nlohmann::json::parse(R"({
                    "children": [
                        {"name": "field1", "type": "int32"},
                        {"name": "field2", "type": "string"}
                    ]
                })");

                CHECK_THROWS_AS(get_children_with_same_name(schema, "nonexistent"), std::runtime_error);
            }

            SUBCASE("empty children array - throws exception")
            {
                const nlohmann::json schema = nlohmann::json::parse(R"({
                    "children": []
                })");

                CHECK_THROWS_AS(get_children_with_same_name(schema, "field1"), std::runtime_error);
            }
        }

        TEST_CASE("get_validity")
        {
            SUBCASE("valid validity array with mixed values")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "VALIDITY": [1, 0, 1, 1, 0]
                })");

                const auto result = get_validity(array);
                const std::vector<bool> expected = {true, false, true, true, false};
                CHECK(result == expected);
            }

            SUBCASE("all valid")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "VALIDITY": [1, 1, 1]
                })");

                const auto result = get_validity(array);
                const std::vector<bool> expected = {true, true, true};
                CHECK(result == expected);
            }

            SUBCASE("all invalid")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "VALIDITY": [0, 0, 0]
                })");

                const auto result = get_validity(array);
                const std::vector<bool> expected = {false, false, false};
                CHECK(result == expected);
            }

            SUBCASE("empty validity array")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "VALIDITY": []
                })");

                const auto result = get_validity(array);
                CHECK(result.empty());
            }

            SUBCASE("missing validity field - throws exception")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "DATA": [1, 2, 3]
                })");

                CHECK_THROWS_AS(get_validity(array), std::runtime_error);
            }

            SUBCASE("non-zero values treated as valid")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "VALIDITY": [2, 0, -1, 100]
                })");

                const auto result = get_validity(array);
                const std::vector<bool> expected = {true, false, true, true};
                CHECK(result == expected);
            }
        }

        TEST_CASE("get_offsets")
        {
            SUBCASE("unsigned integer array")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "OFFSET": [0, 3, 7, 10]
                })");

                const auto result = get_offsets(array);
                const std::vector<size_t> expected = {0, 3, 7, 10};
                CHECK(result == expected);
            }

            SUBCASE("string array with valid numbers")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "OFFSET": ["0", "3", "7", "10"]
                })");

                const auto result = get_offsets(array);
                const std::vector<size_t> expected = {0, 3, 7, 10};
                CHECK(result == expected);
            }

            SUBCASE("empty offset array")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "OFFSET": []
                })");

                const auto result = get_offsets(array);
                CHECK(result.empty());
            }

            SUBCASE("missing offset field - throws exception")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "DATA": [1, 2, 3]
                })");

                CHECK_THROWS_AS(get_offsets(array), std::runtime_error);
            }

            SUBCASE("offset is not an array - throws exception")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "OFFSET": "not_an_array"
                })");

                CHECK_THROWS_AS(get_offsets(array), std::runtime_error);
            }

            SUBCASE("string array with invalid number - throws exception")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "OFFSET": ["0", "invalid", "7"]
                })");

                CHECK_THROWS_AS(get_offsets(array), std::runtime_error);
            }

            SUBCASE("unsupported array element type - throws exception")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "OFFSET": [true, false]
                })");

                CHECK_THROWS_AS(get_offsets(array), std::runtime_error);
            }
        }

        TEST_CASE("get_sizes")
        {
            SUBCASE("unsigned integer array")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "SIZE": [3, 4, 3, 2]
                })");

                const auto result = get_sizes(array);
                const std::vector<size_t> expected = {3, 4, 3, 2};
                CHECK(result == expected);
            }

            SUBCASE("string array with valid numbers")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "SIZE": ["3", "4", "3", "2"]
                })");

                const auto result = get_sizes(array);
                const std::vector<size_t> expected = {3, 4, 3, 2};
                CHECK(result == expected);
            }

            SUBCASE("empty size array")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "SIZE": []
                })");

                const auto result = get_sizes(array);
                CHECK(result.empty());
            }

            SUBCASE("missing size field - throws exception")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "DATA": [1, 2, 3]
                })");

                CHECK_THROWS_AS(get_sizes(array), std::runtime_error);
            }

            SUBCASE("size is not an array - throws exception")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "SIZE": "not_an_array"
                })");

                CHECK_THROWS_AS(get_sizes(array), std::runtime_error);
            }

            SUBCASE("string array with invalid number - throws exception")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "SIZE": ["3", "invalid", "2"]
                })");

                CHECK_THROWS_AS(get_sizes(array), std::runtime_error);
            }

            SUBCASE("unsupported array element type - throws exception")
            {
                const nlohmann::json array = nlohmann::json::parse(R"({
                    "SIZE": [null, null]
                })");

                CHECK_THROWS_AS(get_sizes(array), std::runtime_error);
            }
        }

        TEST_CASE("check_type")
        {
            SUBCASE("matching type")
            {
                const nlohmann::json schema = nlohmann::json::parse(R"({
                    "type": {
                        "name": "int32"
                    }
                })");

                CHECK_NOTHROW(check_type(schema, "int32"));
            }

            SUBCASE("non-matching type - throws exception")
            {
                const nlohmann::json schema = nlohmann::json::parse(R"({
                    "type": {
                        "name": "int32"
                    }
                })");

                CHECK_THROWS_AS(check_type(schema, "string"), std::runtime_error);
            }

            SUBCASE("complex type name")
            {
                const nlohmann::json schema = nlohmann::json::parse(R"({
                    "type": {
                        "name": "list<int32>"
                    }
                })");

                CHECK_NOTHROW(check_type(schema, "list<int32>"));
                CHECK_THROWS_AS(check_type(schema, "list<string>"), std::runtime_error);
            }
        }

        TEST_CASE("get_metadata")
        {
            SUBCASE("valid metadata")
            {
                const nlohmann::json schema = nlohmann::json::parse(R"({
                    "metadata": [
                        {"key": "encoding", "value": "utf-8"},
                        {"key": "timezone", "value": "UTC"}
                    ]
                })");

                const auto result = get_metadata(schema);
                REQUIRE(result.has_value());
                CHECK(result->size() == 2);

                bool found_encoding = false, found_timezone = false;
                for (const auto& pair : *result)
                {
                    if (pair.first == "encoding" && pair.second == "utf-8")
                    {
                        found_encoding = true;
                    }
                    if (pair.first == "timezone" && pair.second == "UTC")
                    {
                        found_timezone = true;
                    }
                }
                CHECK(found_encoding);
                CHECK(found_timezone);
            }

            SUBCASE("empty metadata")
            {
                const nlohmann::json schema = nlohmann::json::parse(R"({
                    "metadata": []
                })");

                const auto result = get_metadata(schema);
                REQUIRE(result.has_value());
                CHECK(result->empty());
            }

            SUBCASE("no metadata field")
            {
                const nlohmann::json schema = nlohmann::json::parse(R"({
                    "type": {"name": "int32"}
                })");

                const auto result = get_metadata(schema);
                CHECK_FALSE(result.has_value());
            }

            SUBCASE("single metadata entry")
            {
                const nlohmann::json schema = nlohmann::json::parse(R"({
                    "metadata": [
                        {"key": "version", "value": "1.0"}
                    ]
                })");

                const auto result = get_metadata(schema);
                REQUIRE(result.has_value());
                CHECK(result->size() == 1);
                CHECK((*result)[0].first == "version");
                CHECK((*result)[0].second == "1.0");
            }
        }

        TEST_CASE("from_strings_to_Is template function")
        {
            SUBCASE("int64_t conversion")
            {
                const std::vector<std::string> data = {"123", "-456", "0"};
                const auto result_range = from_strings_to_Is<int64_t>(data);
                const std::vector<int64_t> result(result_range.begin(), result_range.end());

                const std::vector<int64_t> expected = {123, -456, 0};
                CHECK(result == expected);
            }

            SUBCASE("uint64_t conversion")
            {
                const std::vector<std::string> data = {"123", "456", "0"};
                const auto result_range = from_strings_to_Is<uint64_t>(data);
                const std::vector<uint64_t> result(result_range.begin(), result_range.end());

                const std::vector<uint64_t> expected = {123, 456, 0};
                CHECK(result == expected);
            }

            SUBCASE("empty vector")
            {
                const std::vector<std::string> data = {};
                const auto result_range = from_strings_to_Is<int64_t>(data);
                const std::vector<int64_t> result(result_range.begin(), result_range.end());

                CHECK(result.empty());
            }

            SUBCASE("large numbers")
            {
                const std::vector<std::string> data = {"9223372036854775807", "-9223372036854775808"};
                const auto result_range = from_strings_to_Is<int64_t>(data);
                const std::vector<int64_t> result(result_range.begin(), result_range.end());

                const std::vector<int64_t> expected = {INT64_MAX, INT64_MIN};
                CHECK(result == expected);
            }
        }
    }
}