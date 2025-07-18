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

#include "sparrow/json_reader/list_parser.hpp"

#include "sparrow/json_reader/constant.hpp"
#include "sparrow/json_reader/json_parser.hpp"
#include "sparrow/json_reader/utils.hpp"

namespace sparrow::json_reader
{
    sparrow::array
    list_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(schema, "list");
        std::string name = schema.at("name").get<std::string>();
        auto offsets = array.at(OFFSET).get<std::vector<int32_t>>();
        const bool nullable = schema.at("nullable").get<bool>();
        auto metadata = utils::get_metadata(schema);
        std::vector<sparrow::array> arrays = get_children_arrays(array, schema, root);
        SPARROW_ASSERT(arrays.size() == 1, "List array must have exactly one child array");
        if (nullable)
        {
            auto validity = utils::get_validity(array);
            return sparrow::array{sparrow::list_array{
                std::move(arrays[0]),
                std::move(offsets),
                std::move(validity),
                name,
                std::move(metadata)
            }};
        }
        else
        {
            return sparrow::array{
                sparrow::list_array{std::move(arrays[0]), std::move(offsets), false, name, std::move(metadata)}
            };
        }
    }

    sparrow::array
    large_list_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(schema, "largelist");
        const std::string name = schema.at("name").get<std::string>();
        const auto& offsets_str = array.at(OFFSET).get<std::vector<std::string>>();
        auto offsets = utils::from_strings_to_Is<uint64_t>(offsets_str);
        const bool nullable = schema.at("nullable").get<bool>();
        auto metadata = utils::get_metadata(schema);
        std::vector<sparrow::array> arrays = get_children_arrays(array, schema, root);
        SPARROW_ASSERT(arrays.size() == 1, "List array must have exactly one child array");
        if (nullable)
        {
            auto validity = utils::get_validity(array);
            return sparrow::array{sparrow::big_list_array{
                std::move(arrays[0]),
                std::move(offsets),
                std::move(validity),
                name,
                std::move(metadata)
            }};
        }
        else
        {
            return sparrow::array{
                sparrow::big_list_array{std::move(arrays[0]), std::move(offsets), false, name, std::move(metadata)}
            };
        }
    }
}