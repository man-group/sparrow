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

#include "sparrow/json_reader/fixedsizebinary_parser.hpp"

#include "sparrow/json_reader/constant.hpp"
#include "sparrow/json_reader/utils.hpp"

namespace sparrow::json_reader
{
    sparrow::array
    fixedsizebinary_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json&)
    {
        utils::check_type(schema, "fixedsizebinary");
        std::string name = schema.at("name").get<std::string>();
        const std::size_t byte_width = schema.at("type").at("byteWidth").get<std::size_t>();
        auto data_str = array.at(DATA).get<std::vector<std::string>>();
        std::vector<std::vector<std::byte>> data = utils::hex_strings_to_bytes(data_str);
        for (auto& d : data)
        {
            if (d.size() != byte_width)
            {
                throw std::runtime_error("Invalid byte width");
            }
        }
        const bool nullable = schema.at("nullable").get<bool>();
        auto metadata = utils::get_metadata(schema);

        if (data.empty())
        {
            return sparrow::array{
                sparrow::fixed_width_binary_array{byte_width, nullable, std::move(name), std::move(metadata)}
            };
        }

        if (nullable)
        {
            auto validity = utils::get_validity(array);
            sparrow::fixed_width_binary_array fixed_width_binary_array{
                std::move(data),
                std::move(validity),
                name,
                std::move(metadata)
            };
            fixed_width_binary_array.zero_null_values(std::vector<std::byte>(byte_width, std::byte(0)));
            return sparrow::array{std::move(fixed_width_binary_array)};
        }
        else
        {
            return sparrow::array{
                sparrow::fixed_width_binary_array{std::move(data), false, name, std::move(metadata)}
            };
        }
    }
}
