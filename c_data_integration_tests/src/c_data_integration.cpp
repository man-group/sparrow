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

#include "sparrow/c_data_integration/c_data_integration.hpp"

#include <cstdint>
#include <fstream>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

#include <sparrow/array.hpp>
#include <sparrow/record_batch.hpp>

#include "sparrow/json_reader/comparison.hpp"
#include "sparrow/json_reader/json_parser.hpp"


static std::string global_error;

const char* external_CDataIntegration_ExportSchemaFromJson(const char* json_path, ArrowSchema* out)
{
    try
    {
        std::ifstream json_file(json_path);
        std::istream& json_stream = json_file;
        const nlohmann::json data = nlohmann::json::parse(json_stream);
        sparrow::record_batch record_batch = sparrow::json_reader::build_record_batch_from_json(data, 0);
        sparrow::struct_array struct_array = record_batch.extract_struct_array();
        auto [array, schema] = sparrow::extract_arrow_structures(std::move(struct_array));
        array.release(&array);
        *out = schema;
    }
    catch (const std::exception& e)
    {
        global_error = e.what();
        return global_error.c_str();
    }
    return nullptr;
}

const char* external_CDataIntegration_ImportSchemaAndCompareToJson(const char* json_path, ArrowSchema* schema)
{
    if (schema == nullptr)
    {
        return "Schema is null";
    }
    try
    {
        std::ifstream json_file(json_path);
        std::istream& json_stream = json_file;
        const nlohmann::json data = nlohmann::json::parse(json_stream);
        sparrow::record_batch record_batch = sparrow::json_reader::build_record_batch_from_json(data, 0);

        sparrow::struct_array struct_array = record_batch.extract_struct_array();
        auto [array_from_json, schema_from_json] = sparrow::extract_arrow_structures(std::move(struct_array));
        array_from_json.release(&array_from_json);
        const std::optional<std::string> result = sparrow::json_reader::compare_schemas(
            "Batch Schema",
            schema,
            &schema_from_json
        );
        schema_from_json.release(&schema_from_json);
        if (result.has_value())
        {
            global_error = result.value();
            return global_error.c_str();
        }
    }
    catch (const std::exception& e)
    {
        global_error = e.what();
        return global_error.c_str();
    }
    return nullptr;
}

const char* external_CDataIntegration_ExportBatchFromJson(const char* json_path, int num_batch, ArrowArray* out)
{
    try
    {
        std::ifstream json_file(json_path);
        std::istream& json_stream = json_file;
        const nlohmann::json data = nlohmann::json::parse(json_stream);
        sparrow::record_batch record_batch = sparrow::json_reader::build_record_batch_from_json(
            data,
            static_cast<size_t>(num_batch)
        );
        sparrow::struct_array struct_array = record_batch.extract_struct_array();
        auto [array_from_json, schema_from_json] = sparrow::extract_arrow_structures(std::move(struct_array));
        schema_from_json.release(&schema_from_json);
        *out = array_from_json;
    }
    catch (const std::exception& e)
    {
        global_error = e.what();
        return global_error.c_str();
    }
    return nullptr;
}

const char*
external_CDataIntegration_ImportBatchAndCompareToJson(const char* json_path, int num_batch, ArrowArray* batch)
{
    if (batch == nullptr)
    {
        return "Batch is null";
    }
    try
    {
        std::ifstream json_file(json_path);
        std::istream& json_stream = json_file;
        const nlohmann::json data = nlohmann::json::parse(json_stream);
        sparrow::record_batch record_batch = sparrow::json_reader::build_record_batch_from_json(
            data,
            static_cast<size_t>(num_batch)
        );
        sparrow::struct_array struct_array = record_batch.extract_struct_array();
        auto [array_from_json, schema_from_json] = sparrow::extract_arrow_structures(std::move(struct_array));
        const std::string schema_name = schema_from_json.name != nullptr ? std::string(schema_from_json.name)
                                                                         : "N/A";
        const std::optional<std::string> result = sparrow::json_reader::compare_arrays(
            "Batch " + schema_name,
            batch,
            &array_from_json,
            &schema_from_json
        );
        array_from_json.release(&array_from_json);
        schema_from_json.release(&schema_from_json);
        if (result.has_value())
        {
            global_error = result.value();
            return global_error.c_str();
        }
    }
    catch (const std::exception& e)
    {
        global_error = e.what();
        return global_error.c_str();
    }
    return nullptr;
}

int64_t external_BytesAllocated()
{
    return 0;
}
