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

#include "sparrow/c_data_integration/utils.hpp"

#include <string_view>

#include "sparrow/c_data_integration/constant.hpp"

namespace sparrow::c_data_integration::utils
{

    const nlohmann::json& get_child(const nlohmann::json& schema_or_array, const std::string& name)
    {
        auto it = std::find_if(
            schema_or_array.at("children").begin(),
            schema_or_array.at("children").end(),
            [&name](const nlohmann::json& child)
            {
                return child.at("name").get<std::string>() == name;
            }
        );

        if (it == schema_or_array.at("children").end())
        {
            throw std::runtime_error("Child not found: " + name);
        }

        return *it;
    }

    std::vector<std::pair<const nlohmann::json&, const nlohmann::json&>>
    get_children(const nlohmann::json& array, const nlohmann::json& schema)
    {
        const auto& schema_children = schema.at("children");
        std::vector<std::pair<const nlohmann::json&, const nlohmann::json&>> children;
        children.reserve(schema_children.size());

        for (const auto& child_schema : schema_children)
        {
            const std::string& name = child_schema.at("name").get<std::string>();
            children.emplace_back(get_child(array, name), child_schema);
        }

        return children;
    }

    std::vector<bool> get_validity(const nlohmann::json& array)
    {
        if (!array.contains(VALIDITY))
        {
            throw std::runtime_error("Validity not found in array");
        }
        auto validity_ints = array.at(VALIDITY).get<std::vector<int>>();
        auto validity_range = validity_ints
                              | std::views::transform(
                                  [](int value)
                                  {
                                      return value != 0;
                                  }
                              );
        return std::vector<bool>(validity_range.begin(), validity_range.end());
    }

    void check_type(const nlohmann::json& schema, const std::string& type)
    {
        const std::string schema_type = schema.at("type").at("name").get<std::string>();
        if (schema_type != type)
        {
            throw std::runtime_error("Not expected type: " + schema_type + ", expected: " + type);
        }
    }

    std::optional<std::vector<sparrow::metadata_pair>> get_metadata(const nlohmann::json& schema)
    {
        std::vector<sparrow::metadata_pair> metadata;
        const auto metadata_json = schema.find("metadata");
        if (metadata_json == schema.end())
        {
            return std::nullopt;
        }
        for (const auto& [_, pair] : metadata_json->items())
        {
            auto key = pair.at("key").get<std::string>();
            auto value = pair.at("value").get<std::string>();
            metadata.emplace_back(std::move(key), std::move(value));
        }
        return metadata;
    }


}  // namespace sparrow::c_data_integration::utils