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

#include <string>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <filesystem>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include "sparrow/c_data_integration/c_data_integration.hpp"
#include "sparrow/c_data_integration/json_parser.hpp"
#include "sparrow/c_interface.hpp"

#include "better_junit_reporter.hpp"


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
    json_files_path / "run_end_encoded.json",
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

TEST_SUITE("c_data_integration")
{
    TEST_CASE("ExportSchemaFromJson")
    {
        for (const auto& json_path : jsons_to_test)
        {
            SUBCASE(json_path.filename().string().c_str())
            {
                if (!std::filesystem::exists(json_path))
                {
                    throw std::runtime_error("File does not exist");
                }
                ArrowSchema schema;
                const auto error = external_CDataIntegration_ExportSchemaFromJson(
                    json_path.string().c_str(),
                    &schema
                );
                if (error != nullptr)
                {
                    CHECK_EQ(std::string_view(error), std::string_view());
                }
            }
        }
    }

    TEST_CASE("ImportSchemaAndCompareToJson")
    {
        for (const auto& json_path : jsons_to_test)
        {
            SUBCASE(json_path.filename().string().c_str())
            {
                ArrowSchema schema;
                auto error = external_CDataIntegration_ExportSchemaFromJson(json_path.string().c_str(), &schema);
                if (error != nullptr)
                {
                    CHECK_EQ(std::string_view(error), std::string_view());
                }
                error = external_CDataIntegration_ImportSchemaAndCompareToJson(json_path.string().c_str(), &schema);
                if (error != nullptr)
                {
                    CHECK_EQ(std::string_view(error), std::string_view());
                }
            }
        }
    }

    TEST_CASE("ExportBatchFromJson")
    {
        for (const auto& json_path : jsons_to_test)
        {
            SUBCASE(json_path.filename().string().c_str())
            {
                for (size_t i = 0; i < get_number_of_batches(json_path); ++i)
                {
                    ArrowArray array;
                    const auto error = external_CDataIntegration_ExportBatchFromJson(
                        json_path.string().c_str(),
                        static_cast<int>(i),
                        &array
                    );
                    if (error != nullptr)
                    {
                        CHECK_EQ(std::string_view(error), std::string_view());
                    }
                }
            }
        }
    }

    TEST_CASE("ImportBatchAndCompareToJson")
    {
        for (const auto& json_path : jsons_to_test)
        {
            SUBCASE(json_path.filename().string().c_str())
            {
                for (size_t i = 0; i < get_number_of_batches(json_path); ++i)
                {
                    ArrowArray array;
                    auto error = external_CDataIntegration_ExportBatchFromJson(
                        json_path.string().c_str(),
                        static_cast<int>(i),
                        &array
                    );
                    if (error != nullptr)
                    {
                        CHECK_EQ(std::string_view(error), std::string_view());
                    }
                    error = external_CDataIntegration_ImportBatchAndCompareToJson(
                        json_path.string().c_str(),
                        static_cast<int>(i),
                        &array
                    );
                    if (error != nullptr)
                    {
                        CHECK_EQ(std::string_view(error), std::string_view());
                    }
                }
            }
        }
    }
}
