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
        const std::string schema_name = schema->name
                                            ? schema->name
                                            : (schema_from_json->name ? schema_from_json->name : "nullptr");
        const std::string prefix_with_name = prefix + " [" + schema_name + "]";
        if (schema == nullptr || schema_from_json == nullptr)
        {
            return prefix_with_name + " is null";
        }
        std::vector<std::string> differences;
        if (std::strcmp(schema->format, schema_from_json->format) != 0)
        {
            differences.push_back(
                prefix_with_name + " format mismatch: pointer=" + schema->format
                + " vs json=" + schema_from_json->format
            );
        }
        if ((schema->name != nullptr) || (schema_from_json->name != nullptr))
        {
            if ((schema->name != nullptr) != (schema_from_json->name != nullptr))
            {
                differences.push_back(
                    prefix_with_name
                    + " name mismatch: pointer=" + std::string(schema->name ? schema->name : "nullptr")
                    + " vs json=" + std::string(schema_from_json->name ? schema_from_json->name : "nullptr")
                );
            }
            else if (schema->name != nullptr && schema_from_json->name != nullptr)
            {
                // Compare the names
                // Note: std::strcmp returns 0 if the strings are equal
                // and a non-zero value if they are not equal
                if (std::strcmp(schema->name, schema_from_json->name) != 0)
                {
                    differences.push_back(
                        prefix_with_name + " name mismatch: pointer=" + std::string(schema->name)
                        + " vs json=" + std::string(schema_from_json->name)
                    );
                }
            }
        }
        if (schema->flags != schema_from_json->flags)
        {
            differences.push_back(
                prefix_with_name + " flags mismatch: pointer=" + std::to_string(schema->flags)
                + " vs json=" + std::to_string(schema_from_json->flags)
            );
        }
        if (schema->n_children != schema_from_json->n_children)
        {
            differences.push_back(
                prefix_with_name + " children count mismatch: pointer=" + std::to_string(schema->n_children)
                + " vs json=" + std::to_string(schema_from_json->n_children)
            );
        }
        else
        {
            for (int64_t i = 0; i < schema->n_children; ++i)
            {
                const auto child_schema = schema->children[i];
                const auto child_schema_from_json = schema_from_json->children[i];
                const auto child_prefix = prefix_with_name + " child [" + std::to_string(i) + "]";
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
                prefix_with_name + " dictionary mismatch: pointer=" + std::to_string(schema_has_dict)
                + " vs json=" + std::to_string(schema_from_json_has_dict)
            );
        }
        if (schema_from_json_has_dict && schema_has_dict)
        {
            const auto dict_schema = schema->dictionary;
            const auto dict_schema_from_json = schema_from_json->dictionary;
            const auto error = compare_schemas(prefix_with_name + " dictionary", dict_schema, dict_schema_from_json);
            if (error.has_value())
            {
                differences.push_back(*error);
            }
        }

        if (!differences.empty())
        {
            std::string result = prefix_with_name + " differences:\n";
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
        const std::string schema_name = schema_from_json->name ? schema_from_json->name : "nullptr";
        const std::string prefix_with_name = prefix + " [" + schema_name + "]";
        if (array == nullptr || array_from_json == nullptr)
        {
            return prefix_with_name + " is null";
        }
        std::vector<std::string> differences;
        if (array->length != array_from_json->length)
        {
            differences.push_back(
                prefix_with_name + " length mismatch: pointer=" + std::to_string(array->length)
                + " vs json=" + std::to_string(array_from_json->length)
            );
        }
        if (array->null_count != array_from_json->null_count)
        {
            differences.push_back(
                prefix_with_name + " null count mismatch: pointer=" + std::to_string(array->null_count)
                + " vs json=" + std::to_string(array_from_json->null_count)
            );
        }
        if (array->n_buffers != array_from_json->n_buffers)
        {
            differences.push_back(
                prefix_with_name + " buffers count mismatch: pointer=" + std::to_string(array->n_buffers)
                + " vs json=" + std::to_string(array_from_json->n_buffers)
            );
        }
        else
        {
            sparrow::arrow_proxy from_json{array_from_json, schema_from_json};
            sparrow::arrow_proxy from{array, schema_from_json};
            for (size_t i = 0; i < static_cast<size_t>(from_json.n_buffers()); ++i)
            {
                const size_t from_json_buffer_size = from_json.buffers()[i].size();
                const size_t from_buffer_size = from.buffers()[i].size();

                if (from_json_buffer_size != from_buffer_size)
                {
                    differences.push_back(
                        prefix_with_name + " buffer [" + std::to_string(i) + "] size mismatch: pointer="
                        + std::to_string(from_buffer_size) + " vs json=" + std::to_string(from_json_buffer_size)
                    );
                    continue;
                }
                for (size_t y = 0; y < from_json_buffer_size; ++y)
                {
                    if (from_json.buffers()[i][y] != from.buffers()[i][y])
                    {
                        differences.push_back(
                            prefix_with_name + " buffer [" + std::to_string(i) + "] mismatch ["
                            + std::to_string(y) + "]: pointer=" + std::to_string(from.buffers()[i][y])
                            + " vs json=" + std::to_string(from_json.buffers()[i][y])
                        );
                    }
                }
            }
        }
        if (array->n_children != array_from_json->n_children)
        {
            differences.push_back(
                prefix_with_name + " children count mismatch: pointer=" + std::to_string(array->n_children)
                + " vs json=" + std::to_string(array_from_json->n_children)
            );
        }
        else
        {
            for (int64_t i = 0; i < array->n_children; ++i)
            {
                const auto child_array = array->children[i];
                const auto child_array_from_json = array_from_json->children[i];
                const auto child_prefix = prefix_with_name + " child [" + std::to_string(i) + "]";
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
            std::string result = prefix_with_name + " differences:\n";
            for (const auto& diff : differences)
            {
                result += "- " + diff + "\n";
            }
            return result;
        }
        return std::nullopt;
    }
}
