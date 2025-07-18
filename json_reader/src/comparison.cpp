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

#include "sparrow/json_reader/comparison.hpp"

#include <cstring>
#include <string>
#include <vector>

#include "sparrow/array.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/utils/metadata.hpp"

namespace sparrow::json_reader
{
    std::optional<std::string>
    compare_schemas(const std::string& prefix, const ArrowSchema* schema, const ArrowSchema* schema_from_json)
    {
        if (schema == nullptr || schema_from_json == nullptr)
        {
            return prefix + " schema is null";
        }

        const std::string schema_name = schema->name
                                            ? schema->name
                                            : (schema_from_json->name ? schema_from_json->name : "nullptr");
        const std::string prefix_with_name = prefix + " [" + schema_name + "]";

        std::vector<std::string> differences;

        // Check format strings are not null before comparing
        if (schema->format == nullptr || schema_from_json->format == nullptr)
        {
            differences.push_back(
                prefix_with_name + " format is null: pointer=" + (schema->format ? schema->format : "nullptr")
                + " vs json=" + (schema_from_json->format ? schema_from_json->format : "nullptr")
            );
        }
        else if (std::strcmp(schema->format, schema_from_json->format) != 0)
        {
            differences.push_back(
                prefix_with_name + " format mismatch: pointer=" + schema->format
                + " vs json=" + schema_from_json->format
            );
        }

        // if ((schema->name != nullptr) || (schema_from_json->name != nullptr))
        // {
        //     if ((schema->name != nullptr) != (schema_from_json->name != nullptr))
        //     {
        //         differences.push_back(
        //             prefix_with_name
        //             + " name mismatch: pointer=" + std::string(schema->name ? schema->name : "nullptr")
        //             + " vs json=" + std::string(schema_from_json->name ? schema_from_json->name :
        //             "nullptr")
        //         );
        //     }
        //     else if (schema->name != nullptr && schema_from_json->name != nullptr)
        //     {
        //         if (std::strcmp(schema->name, schema_from_json->name) != 0)
        //         {
        //             differences.push_back(
        //                 prefix_with_name + " name mismatch: pointer=" + std::string(schema->name)
        //                 + " vs json=" + std::string(schema_from_json->name)
        //             );
        //         }
        //     }
        // }

        auto metadata_to_string = [](const char* metadata) -> std::string
        {
            if (metadata == nullptr)
            {
                return "nullptr";
            }
            sparrow::key_value_view metadata_view(metadata);
            constexpr std::string_view opening_bracket = "(";
            constexpr std::string_view closing_bracket = ")";
            constexpr std::string_view separator = ": ";
            std::string result;
            for (const auto& pair : metadata_view)
            {
                result += opening_bracket;
                result += std::string(pair.first);
                result += separator;
                result += std::string(pair.second);
                result += closing_bracket;
            }
            return result;
        };

        // check metadata
        if (schema->metadata != nullptr || schema_from_json->metadata != nullptr)
        {
            const std::string pointer_metadata = metadata_to_string(schema->metadata);
            const std::string json_metadata = metadata_to_string(schema_from_json->metadata);
            if ((schema->metadata != nullptr) != (schema_from_json->metadata != nullptr))
            {
                differences.push_back(
                    prefix_with_name + " metadata mismatch: pointer=" + pointer_metadata
                    + " vs json=" + json_metadata
                );
            }
            else if (schema->metadata != nullptr && schema_from_json->metadata != nullptr)
            {
                if (pointer_metadata != json_metadata)
                {
                    // If metadata is not equal, we need to show the differences
                    differences.push_back(
                        prefix_with_name + " metadata mismatch: \npointer=" + pointer_metadata
                        + "vs json=" + json_metadata
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
        if (array->null_count != array_from_json->null_count)
        {
            return prefix_with_name + " null count mismatch: pointer=" + std::to_string(array->null_count)
                   + " vs json=" + std::to_string(array_from_json->null_count);
        }

        std::vector<std::string> differences;
        const sparrow::array array_from_ptr(array, schema_from_json);
        const sparrow::array array_from_json_ptr(array_from_json, schema_from_json);

        // check is same layout
        if (array_from_ptr.data_type() != array_from_json_ptr.data_type())
        {
            differences.push_back(
                prefix_with_name + " layout format mismatch: pointer="
                + std::string(data_type_to_format(array_from_ptr.data_type()))
                + " vs json=" + std::string(data_type_to_format(array_from_json_ptr.data_type()))
            );
        }

        if (array_from_ptr.size() != array_from_json_ptr.size())
        {
            differences.push_back(
                prefix_with_name + " size mismatch: pointer=" + std::to_string(array_from_ptr.size())
                + " vs json=" + std::to_string(array_from_json_ptr.size())
            );
        }

        array_from_ptr.visit(
            [&](const auto& typed_array_from_ptr) -> bool
            {
                return array_from_json_ptr.visit(
                    [&](const auto& typed_array_from_json_ptr) -> bool
                    {
                        if constexpr (!std::same_as<decltype(typed_array_from_ptr), decltype(typed_array_from_json_ptr)>)
                        {
                            throw std::runtime_error("Cannot compare arrays of different types");
                        }
                        else
                        {
                            for (size_t i = 0; i < typed_array_from_ptr.size(); ++i)
                            {
                                if (typed_array_from_ptr[i] != typed_array_from_json_ptr[i])
                                {
                                    std::string diff = "Array values mismatch at index " + std::to_string(i);
#if defined(__cpp_lib_format)
                                    diff += ": pointer=" + std::format("{}", typed_array_from_ptr[i])
                                            + " vs json=" + std::format("{}", typed_array_from_json_ptr[i]);
#endif
                                    differences.push_back(prefix + " " + diff);
                                }
                            }
                        }

                        return true;
                    }
                );
            }
        );

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
