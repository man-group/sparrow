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

#include <filesystem>

#include "sparrow/c_interface.hpp"

#include "c_data_integration.hpp"
#include "doctest/doctest.h"

const std::filesystem::path json_files_path = JSON_FILES_PATH;

const std::vector<std::filesystem::path> json_to_test = {
    "generated/custom-metadata.json",
    "generated/datetime.json",
    "generated/decimal128.json",
    "generated/dictionary-nested.json",
    "generated/dictionary-unsigned.json",
    "generated/dictionary.json",
    "generated/extension.json",
    "generated/map.json",
    "generated/nested.json",
    "generated/non_canonical_map.json",
    "generated/null-trivial.json",
    "generated/null.json",
    "generated/primitive-empty.json",
    "generated/primitive-no-batches.json",
    "generated/primitive.json",
    "generated/recursive-nested.json",
    "generated/unions.json",
};

TEST_SUITE("c_data_integration")
{
    TEST_CASE("ExportSchemaFromJson")
    {
        for (const auto& json : json_to_test)
        {
            SUBCASE(json.string().c_str())
            {
                ArrowSchema schema;
                const auto result = nanoarrow_CDataIntegration_ExportSchemaFromJson(
                    (json_files_path / json).string().c_str(),
                    &schema
                );
                CHECK_EQ(result, nullptr);
            }
        }
    }
}
