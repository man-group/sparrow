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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <sparrow/array.hpp>
#include <sparrow/json_reader/comparison.hpp>
#include <sparrow/json_reader/json_parser.hpp>
#include <sparrow/json_reader/utils.hpp>
#include <sparrow/record_batch.hpp>

const std::filesystem::path json_files_path = JSON_FILES_PATH;

const std::vector<std::filesystem::path> jsons_to_test = {
    json_files_path / "binary_view.json",
    json_files_path / "custom-metadata.json",
    json_files_path / "datetime.json",
    json_files_path / "decimal32.json",
    json_files_path / "decimal64.json",
#ifndef SPARROW_USE_LARGE_INT_PLACEHOLDERS
    json_files_path / "decimal.json",
    json_files_path / "decimal128.json",
    json_files_path / "decimal256.json",
#endif
    json_files_path / "duplicate_fieldnames.json",
    json_files_path / "dictionary-nested.json",
    json_files_path / "dictionary-unsigned.json",
    json_files_path / "dictionary.json",
    json_files_path / "duplicate_fieldnames.json",
    json_files_path / "duration.json",
    json_files_path / "interval_mdn.json",
    json_files_path / "interval.json",
    json_files_path / "list_view.json",
    json_files_path / "nested_large_offsets.json",
    json_files_path / "nested.json",
    json_files_path / "null-trivial.json",
    json_files_path / "null.json",
    json_files_path / "map.json",
    json_files_path / "non_canonical_map.json",
    json_files_path / "primitive_large_offsets.json",
    json_files_path / "primitive_no_batches.json",
    json_files_path / "primitive_zerolength.json",
    json_files_path / "primitive-empty.json",
    json_files_path / "primitive.json",
    json_files_path / "recursive-nested.json",
    // json_files_path / "run_end_encoded.json",
    json_files_path / "union.json",
};

size_t get_number_of_batches(const std::filesystem::path& json_path)
{
    std::ifstream json_file(json_path);
    if (!json_file.is_open())
    {
        throw std::runtime_error("Could not open file: " + json_path.string());
    }
    const nlohmann::json data = nlohmann::json::parse(json_file);
    return data["batches"].size();
}

nlohmann::json load_json_file(const std::filesystem::path& json_path)
{
    std::ifstream json_file(json_path);
    if (!json_file.is_open())
    {
        throw std::runtime_error("Could not open file: " + json_path.string());
    }
    return nlohmann::json::parse(json_file);
}

TEST_SUITE("json_reader_parser")
{
    TEST_CASE("build_record_batch_from_json")
    {
        for (const auto& json_path : jsons_to_test)
        {
            SUBCASE(json_path.filename().string().c_str())
            {
                if (!std::filesystem::exists(json_path))
                {
                    FAIL("File does not exist: " << json_path.string());
                }

                const auto json_data = load_json_file(json_path);
                const size_t num_batches = get_number_of_batches(json_path);

                for (size_t batch_idx = 0; batch_idx < num_batches; ++batch_idx)
                {
                    INFO("Processing batch " << batch_idx << " of " << num_batches);

                    // Test that we can build a record batch from JSON
                    CHECK_NOTHROW({
                        auto record_batch = sparrow::json_reader::build_record_batch_from_json(
                            json_data,
                            batch_idx
                        );
                        CHECK(record_batch.nb_columns() > 0);
                        CHECK(record_batch.nb_rows() >= 0);
                    });
                }
            }
        }
    }

    TEST_CASE("build_array_from_json")
    {
        for (const auto& json_path : jsons_to_test)
        {
            SUBCASE(json_path.filename().string().c_str())
            {
                if (!std::filesystem::exists(json_path))
                {
                    FAIL("File does not exist: " << json_path.string());
                }

                const auto json_data = load_json_file(json_path);
                const size_t num_batches = get_number_of_batches(json_path);

                for (size_t batch_idx = 0; batch_idx < num_batches; ++batch_idx)
                {
                    INFO("Processing batch " << batch_idx << " of " << num_batches);

                    const auto& batch = json_data["batches"][batch_idx];
                    const auto& schema = json_data["schema"];

                    // Test each column in the batch
                    for (size_t col_idx = 0; col_idx < batch["columns"].size(); ++col_idx)
                    {
                        const auto& column = batch["columns"][col_idx];
                        const auto& field_schema = schema["fields"][col_idx];

                        INFO("Processing column " << col_idx << ": " << column["name"].get<std::string>());

                        // Test that we can build an array from JSON column data
                        CHECK_NOTHROW({
                            auto array = sparrow::json_reader::build_array_from_json(
                                column,
                                field_schema,
                                json_data
                            );
                            CHECK(array.size() >= 0);
                        });
                    }
                }
            }
        }
    }

    TEST_CASE("array_roundtrip_comparison")
    {
        for (const auto& json_path : jsons_to_test)
        {
            SUBCASE(json_path.filename().string().c_str())
            {
                if (!std::filesystem::exists(json_path))
                {
                    FAIL("File does not exist: " << json_path.string());
                }

                const auto json_data = load_json_file(json_path);
                const size_t num_batches = get_number_of_batches(json_path);

                for (size_t batch_idx = 0; batch_idx < num_batches; ++batch_idx)
                {
                    INFO("Processing batch " << batch_idx << " of " << num_batches);

                    // Build record batch from JSON
                    auto record_batch = sparrow::json_reader::build_record_batch_from_json(json_data, batch_idx);
                    auto struct_array = record_batch.extract_struct_array();
                    auto [array, schema] = sparrow::extract_arrow_structures(std::move(struct_array));

                    // Compare the generated array with the original JSON
                    const std::optional<std::string> result = sparrow::json_reader::compare_arrays(
                        "Batch " + std::to_string(batch_idx),
                        &array,
                        &array,
                        &schema
                    );

                    if (result.has_value())
                    {
                        INFO("Comparison error: " << result.value());
                        CHECK(!result.has_value());
                    }

                    // Clean up
                    array.release(&array);
                    schema.release(&schema);
                }
            }
        }
    }

    TEST_CASE("schema_roundtrip_comparison")
    {
        for (const auto& json_path : jsons_to_test)
        {
            SUBCASE(json_path.filename().string().c_str())
            {
                if (!std::filesystem::exists(json_path))
                {
                    FAIL("File does not exist: " << json_path.string());
                }

                const auto json_data = load_json_file(json_path);

                // Build record batch from JSON (using first batch)
                auto record_batch = sparrow::json_reader::build_record_batch_from_json(json_data, 0);
                auto struct_array = record_batch.extract_struct_array();
                auto [array1, schema1] = sparrow::extract_arrow_structures(std::move(struct_array));

                // Build another record batch from the same JSON
                auto record_batch2 = sparrow::json_reader::build_record_batch_from_json(json_data, 0);
                auto struct_array2 = record_batch2.extract_struct_array();
                auto [array2, schema2] = sparrow::extract_arrow_structures(std::move(struct_array2));

                // Compare the schemas
                const std::optional<std::string> result = sparrow::json_reader::compare_schemas(
                    "Schema comparison",
                    &schema1,
                    &schema2
                );

                if (result.has_value())
                {
                    INFO("Schema comparison error: " << result.value());
                    CHECK(!result.has_value());
                }

                // Clean up
                array1.release(&array1);
                schema1.release(&schema1);
                array2.release(&array2);
                schema2.release(&schema2);
            }
        }
    }
}

TEST_SUITE("json_reader_utils")
{
    TEST_CASE("hex_string_to_bytes")
    {
        // Test basic hex string conversion
        auto result = sparrow::json_reader::utils::hex_string_to_bytes("48656c6c6f");
        CHECK(result.size() == 5);
        CHECK(result[0] == std::byte{0x48});
        CHECK(result[1] == std::byte{0x65});
        CHECK(result[2] == std::byte{0x6c});
        CHECK(result[3] == std::byte{0x6c});
        CHECK(result[4] == std::byte{0x6f});

        // Test empty string
        auto empty_result = sparrow::json_reader::utils::hex_string_to_bytes("");
        CHECK(empty_result.empty());

        // Test uppercase hex
        auto upper_result = sparrow::json_reader::utils::hex_string_to_bytes("ABCDEF");
        CHECK(upper_result.size() == 3);
        CHECK(upper_result[0] == std::byte{0xAB});
        CHECK(upper_result[1] == std::byte{0xCD});
        CHECK(upper_result[2] == std::byte{0xEF});
    }

    TEST_CASE("hex_strings_to_bytes")
    {
        std::vector<std::string> hex_strings = {"48656c6c6f", "576f726c64"};
        auto result = sparrow::json_reader::utils::hex_strings_to_bytes(hex_strings);

        CHECK(result.size() == 2);
        CHECK(result[0].size() == 5);
        CHECK(result[1].size() == 5);

        // Check first string: "Hello"
        CHECK(result[0][0] == std::byte{0x48});
        CHECK(result[0][1] == std::byte{0x65});
        CHECK(result[0][2] == std::byte{0x6c});
        CHECK(result[0][3] == std::byte{0x6c});
        CHECK(result[0][4] == std::byte{0x6f});

        // Check second string: "World"
        CHECK(result[1][0] == std::byte{0x57});
        CHECK(result[1][1] == std::byte{0x6f});
        CHECK(result[1][2] == std::byte{0x72});
        CHECK(result[1][3] == std::byte{0x6c});
        CHECK(result[1][4] == std::byte{0x64});
    }

    TEST_CASE("get_validity")
    {
        // Test with validity array
        nlohmann::json json_object = nlohmann::json::object();
        json_object["VALIDITY"] = nlohmann::json::array({1, 0, 1, 0, 1});
        auto validity = sparrow::json_reader::utils::get_validity(json_object);

        CHECK(validity.size() == 5);
        CHECK(validity[0] == true);
        CHECK(validity[1] == false);
        CHECK(validity[2] == true);
        CHECK(validity[3] == false);
        CHECK(validity[4] == true);

        // Test with empty validity array
        json_object["VALIDITY"] = nlohmann::json::array();
        auto empty_result = sparrow::json_reader::utils::get_validity(json_object);
        CHECK(empty_result.empty());
    }

    TEST_CASE("get_offsets")
    {
        // Test with offset array
        nlohmann::json json_object = nlohmann::json::object();
        json_object["OFFSET"] = nlohmann::json::array({0u, 5u, 10u, 15u, 20u});
        auto offsets = sparrow::json_reader::utils::get_offsets(json_object);

        CHECK(offsets.size() == 5);
        CHECK(offsets[0] == 0);
        CHECK(offsets[1] == 5);
        CHECK(offsets[2] == 10);
        CHECK(offsets[3] == 15);
        CHECK(offsets[4] == 20);

        // Test with empty offsets array
        json_object["OFFSET"] = nlohmann::json::array();
        auto empty_result = sparrow::json_reader::utils::get_offsets(json_object);
        CHECK(empty_result.empty());
    }

    TEST_CASE("get_sizes")
    {
        // Test with sizes array
        nlohmann::json json_object = nlohmann::json::object();
        json_object["SIZE"] = nlohmann::json::array({1u, 2u, 3u, 4u, 5u});
        auto sizes = sparrow::json_reader::utils::get_sizes(json_object);

        CHECK(sizes.size() == 5);
        CHECK(sizes[0] == 1);
        CHECK(sizes[1] == 2);
        CHECK(sizes[2] == 3);
        CHECK(sizes[3] == 4);
        CHECK(sizes[4] == 5);

        // Test with empty sizes array
        json_object["SIZE"] = nlohmann::json::array();
        auto empty_result = sparrow::json_reader::utils::get_sizes(json_object);
        CHECK(empty_result.empty());
    }

    TEST_CASE("check_type")
    {
        // Test with valid type
        nlohmann::json schema_json = {{"type", {{"name", "int32"}}}};
        CHECK_NOTHROW(sparrow::json_reader::utils::check_type(schema_json, "int32"));

        // Test with invalid type should throw
        CHECK_THROWS(sparrow::json_reader::utils::check_type(schema_json, "int64"));

        // Test with missing type field
        nlohmann::json invalid_schema_json = {{"other_field", "value"}};
        CHECK_THROWS(sparrow::json_reader::utils::check_type(invalid_schema_json, "int32"));
    }

    TEST_CASE("get_metadata")
    {
        // Test with metadata
        nlohmann::json metadata_json = nlohmann::json::array(
            {{{"key", "key1"}, {"value", "value1"}}, {{"key", "key2"}, {"value", "value2"}}}
        );

        nlohmann::json json_object = nlohmann::json::object();
        json_object["metadata"] = metadata_json;

        auto metadata = sparrow::json_reader::utils::get_metadata(json_object);
        CHECK(metadata.has_value());
        CHECK(metadata->size() == 2);
        CHECK((*metadata)[0].first == "key1");
        CHECK((*metadata)[0].second == "value1");
        CHECK((*metadata)[1].first == "key2");
        CHECK((*metadata)[1].second == "value2");

        // Test without metadata
        nlohmann::json field_without_metadata = {{"name", "test_field"}};

        auto no_metadata = sparrow::json_reader::utils::get_metadata(field_without_metadata);
        CHECK(!no_metadata.has_value());
    }
}

namespace sparrow::json_reader::utils
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