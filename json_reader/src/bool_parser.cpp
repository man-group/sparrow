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

#include "sparrow/json_reader/bool_parser.hpp"

#include "sparrow/json_reader/constant.hpp"
#include "sparrow/json_reader/utils.hpp"

namespace sparrow::json_reader
{
    sparrow::array
    bool_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json&)
    {
        utils::check_type(schema, "bool");
        std::string name = schema.at("name").get<std::string>();
        const bool nullable = schema.at("nullable").get<bool>();
        auto metadata = utils::get_metadata(schema);
        if (nullable)
        {
            auto validity = utils::get_validity(array);
            auto primitive_array = sparrow::primitive_array<bool>{
                array.at(DATA).get<std::vector<bool>>(),
                std::move(validity),
                name,
                std::move(metadata)
            };
            primitive_array.zero_null_values();
            return sparrow::array{std::move(primitive_array)};
        }
        else
        {
            return sparrow::array{sparrow::primitive_array<bool>{
                array.at(DATA).get<std::vector<bool>>(),
                false,
                name,
                std::move(metadata)
            }};
        }
    }
}
