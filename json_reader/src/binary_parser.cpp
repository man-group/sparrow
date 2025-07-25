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

#include "sparrow/json_reader/binary_parser.hpp"

#include "sparrow/json_reader/constant.hpp"
#include "sparrow/json_reader/utils.hpp"

namespace sparrow::json_reader
{
    sparrow::array
    binary_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json&)
    {
        utils::check_type(schema, "binary");
        const std::string name = schema.at("name").get<std::string>();
        auto data_str = array.at(DATA).get<std::vector<std::string>>();
        std::vector<std::vector<std::byte>> data = utils::hex_strings_to_bytes(data_str);
        const bool nullable = schema.at("nullable").get<bool>();
        auto metadata = utils::get_metadata(schema);
        if (nullable)
        {
            auto validity = utils::get_validity(array);
            sparrow::binary_array ar{std::move(data), std::move(validity), name, std::move(metadata)};
            ar.zero_null_values();
            return sparrow::array{std::move(ar)};
        }
        else
        {
            return sparrow::array{sparrow::binary_array{std::move(data), false, name, std::move(metadata)}};
        }
    }

    sparrow::array
    large_binary_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json&)
    {
        utils::check_type(schema, "largebinary");
        const std::string name = schema.at("name").get<std::string>();
        auto data_str = array.at(DATA).get<std::vector<std::string>>();
        std::vector<std::vector<std::byte>> data = utils::hex_strings_to_bytes(data_str);
        const bool nullable = schema.at("nullable").get<bool>();
        auto metadata = utils::get_metadata(schema);
        if (nullable)
        {
            auto validity = utils::get_validity(array);
            return sparrow::array{
                sparrow::big_binary_array{std::move(data), std::move(validity), name, std::move(metadata)}
            };
        }
        else
        {
            return sparrow::array{sparrow::big_binary_array{std::move(data), false, name, std::move(metadata)}};
        }
    }
}