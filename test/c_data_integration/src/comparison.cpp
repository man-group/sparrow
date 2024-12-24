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

#include "sparrow/c_data_integration/comparison.hpp"

#include <cstring>
#include <vector>

#include "sparrow/arrow_array_schema_proxy.hpp"

namespace sparrow::c_data_integration
{
    std::optional<std::string>
    compare_schemas(const std::string& prefix, const ArrowSchema* schema, const ArrowSchema* schema_from_json)
    {
        if (schema == nullptr || schema_from_json == nullptr)
        {
            return prefix + " is null";
        }
        std::vector<std::string> differences;
        if (std::strcmp(schema->format, schema_from_json->format) != 0)
        {
            differences.push_back(
                prefix + " format mismatch: " + schema->format + " vs " + schema_from_json->format
            );
        }
        if (schema->name != nullptr && schema_from_json->name == nullptr)
        {
            if (std::strcmp(schema->name, schema_from_json->name) != 0)
            {
                differences.push_back(
                    prefix + " name mismatch: " + schema->name + " vs " + schema_from_json->name
                );
            }
        }
        if (schema->metadata != nullptr && schema_from_json->metadata == nullptr)
        {
            if (std::strcmp(schema->metadata, schema_from_json->metadata) != 0)
            {
                differences.push_back(
                    prefix + " metadata mismatch: " + schema->metadata + " vs " + schema_from_json->metadata
                );
            }
        }
        if (schema->flags != schema_from_json->flags)
        {
            differences.push_back(
                prefix + " flags mismatch: " + std::to_string(schema->flags) + " vs "
                + std::to_string(schema_from_json->flags)
            );
        }
        if (schema->n_children != schema_from_json->n_children)
        {
            differences.push_back(
                prefix + " children count mismatch: " + std::to_string(schema->n_children) + " vs "
                + std::to_string(schema_from_json->n_children)
            );
        }
        else
        {
            for (int64_t i = 0; i < schema->n_children; ++i)
            {
                const auto child_schema = schema->children[i];
                const auto child_schema_from_json = schema_from_json->children[i];
                const auto child_prefix = prefix + " child [" + std::to_string(i) + "]";
                const auto error = compare_schemas(child_prefix, child_schema, child_schema_from_json);
                if (error.has_value())
                {
                    differences.push_back(*error);
                }
            }
        }
        const bool schema_from_json_has_dict = schema_from_json->dictionary != nullptr;
        const bool schema_has_dict = schema->dictionary != nullptr;
        if (schema_from_json_has_dict != schema_has_dict)
        {
            differences.push_back(
                prefix + " dictionary mismatch: " + std::to_string(schema_from_json_has_dict) + " vs "
                + std::to_string(schema_has_dict)
            );
        }
        if (schema_from_json_has_dict && schema_has_dict)
        {
            const auto dict_schema = schema->dictionary;
            const auto dict_schema_from_json = schema_from_json->dictionary;
            const auto error = compare_schemas(prefix + " dictionary", dict_schema, dict_schema_from_json);
            if (error.has_value())
            {
                differences.push_back(*error);
            }
        }

        if (!differences.empty())
        {
            std::string result = prefix + " differences:\n";
            for (const auto& diff : differences)
            {
                result += "- " + diff + "\n";
            }
            return result;
        }
        return std::nullopt;
    }

    std::optional<std::string>
    compare_arrays(const std::string& prefix, ArrowArray* array, ArrowArray* array_from_json, ArrowSchema* schema_from_json)
    {
        if (array == nullptr || array_from_json == nullptr)
        {
            return prefix + " is null";
        }
        std::vector<std::string> differences;
        if (array->length != array_from_json->length)
        {
            differences.push_back(
                prefix + " length mismatch: " + std::to_string(array->length) + " vs "
                + std::to_string(array_from_json->length)
            );
        }
        if (array->null_count != array_from_json->null_count)
        {
            differences.push_back(
                prefix + " null count mismatch: " + std::to_string(array->null_count) + " vs "
                + std::to_string(array_from_json->null_count)
            );
        }
        if (array->n_buffers != array_from_json->n_buffers)
        {
            differences.push_back(
                prefix + " buffers count mismatch: " + std::to_string(array->n_buffers) + " vs "
                + std::to_string(array_from_json->n_buffers)
            );
        }
        else
        {
            sparrow::arrow_proxy from_json{array_from_json, schema_from_json};
            sparrow::arrow_proxy from{array, schema_from_json};
            for (size_t i = 0; i < static_cast<size_t>(array->n_buffers); ++i)
            {
                for (size_t y = 0; y < from_json.n_buffers(); ++i)
                {
                    if (from_json.buffers()[i][y] != from.buffers()[i][y])
                    {
                        differences.push_back(
                            prefix + " buffer [" + std::to_string(i) + "] mismatch [" + std::to_string(y)
                            + "]:" + std::to_string(from_json.buffers()[i][y]) + " vs "
                            + std::to_string(from.buffers()[i][y])
                        );
                    }
                }
            }
        }
        if (array->n_children != array_from_json->n_children)
        {
            differences.push_back(
                prefix + " children count mismatch: " + std::to_string(array->n_children) + " vs "
                + std::to_string(array_from_json->n_children)
            );
        }
        else
        {
            for (int64_t i = 0; i < array->n_children; ++i)
            {
                const auto child_array = array->children[i];
                const auto child_array_from_json = array_from_json->children[i];
                const auto child_prefix = prefix + " child [" + std::to_string(i) + "]";
                const auto error = compare_arrays(
                    child_prefix,
                    child_array,
                    child_array_from_json,
                    schema_from_json->children[i]
                );
                if (error.has_value())
                {
                    differences.push_back(*error);
                }
            }
        }

        if (!differences.empty())
        {
            std::string result = prefix + " differences:\n";
            for (const auto& diff : differences)
            {
                result += "- " + diff + "\n";
            }
            return result;
        }
        return std::nullopt;
    }
}
