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

#include <charconv>
#include <string_view>

#include "sparrow/c_data_integration/constant.hpp"

namespace sparrow::c_data_integration::utils
{
    std::vector<std::vector<std::byte>> hex_strings_to_bytes(const std::vector<std::string>& hexStrings)
    {
        std::vector<std::vector<std::byte>> result;
        result.reserve(hexStrings.size());

        for (const auto& hexStr : hexStrings)
        {
            std::vector<std::byte> bytes;

            if (hexStr.empty())
            {
                result.push_back(std::move(bytes));
                continue;
            }

            bytes.reserve(hexStr.size() / 2);
            for (size_t i = 0; i < hexStr.size(); i += 2)
            {
                const std::string_view byte_str = std::string_view(hexStr).substr(i, 2);
                unsigned char byte_value = 0;
                auto [ptr, ec] = std::from_chars(byte_str.data(), byte_str.data() + 2, byte_value, 16);
                if (ec == std::errc{})
                {
                    bytes.push_back(static_cast<std::byte>(byte_value));
                }
            }
            result.push_back(std::move(bytes));
        }

        return result;
    }

    std::vector<nlohmann::json>
    get_children_with_same_name(const nlohmann::json& schema_or_array, const std::string& name)
    {
        std::vector<nlohmann::json> matches;
        const auto& children = schema_or_array.at("children");
        std::ranges::for_each(
            children,
            [&](const auto& child)
            {
                if (child.at("name").template get<std::string>() == name)
                {
                    matches.push_back(child);
                }
            }
        );
        if (matches.empty())
        {
            throw std::runtime_error("Child not found: " + name);
        }
        return matches;
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

    std::vector<size_t> get_offsets(const nlohmann::json& array)
    {
        if (!array.contains(OFFSET))
        {
            throw std::runtime_error("Offset not found in array");
        }
        if (!array.at(OFFSET).is_array())
        {
            throw std::runtime_error("Offset is not an array");
        }
        if (array.at(OFFSET).empty())
        {
            return std::vector<size_t>{};
        }
        // check element type
        if (array.at(OFFSET).front().is_number_unsigned())
        {
            return array.at(OFFSET).get<std::vector<size_t>>();
        }
        if (array.at(OFFSET).front().is_string())
        {
            const auto& strings = array.at(OFFSET).get<std::vector<std::string>>();
            auto offsets = strings
                           | std::views::transform(
                               [](const std::string& str)
                               {
                                   size_t value;
                                   auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
                                   if (ec != std::errc{})
                                   {
                                       throw std::runtime_error("Invalid offset value: " + str);
                                   }
                                   return value;
                               }
                           );
            return std::vector<size_t>(offsets.begin(), offsets.end());
        }
        throw std::runtime_error("Offset is not an array of unsigned integers or strings");
    }

    std::vector<size_t> get_sizes(const nlohmann::json& array)
    {
        if (!array.contains(SIZE))
        {
            throw std::runtime_error("Size not found in array");
        }
        if (!array.at(SIZE).is_array())
        {
            throw std::runtime_error("Size is not an array");
        }
        if (array.at(SIZE).empty())
        {
            return std::vector<size_t>{};
        }
        // check element type
        if (array.at(SIZE).front().is_number_unsigned())
        {
            return array.at(SIZE).get<std::vector<size_t>>();
        }
        if (array.at(SIZE).front().is_string())
        {
            const auto& strings = array.at(SIZE).get<std::vector<std::string>>();
            auto sizes = strings
                         | std::views::transform(
                             [](const std::string& str)
                             {
                                 size_t value;
                                 auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
                                 if (ec != std::errc{})
                                 {
                                     throw std::runtime_error("Invalid size value: " + str);
                                 }
                                 return value;
                             }
                         );
            return std::vector<size_t>(sizes.begin(), sizes.end());
        }

        throw std::runtime_error("Size is not an array of unsigned integers or strings");
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
