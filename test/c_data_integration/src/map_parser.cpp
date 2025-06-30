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

#include "sparrow/c_data_integration/map_parser.hpp"

#include "sparrow/c_data_integration/constant.hpp"
#include "sparrow/c_data_integration/json_parser.hpp"
#include "sparrow/c_data_integration/utils.hpp"

namespace sparrow::c_data_integration
{
    sparrow::array
    map_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(schema, "map");
        const std::string name = schema.at("name").get<std::string>();
        auto offsets = array.at(OFFSET).get<std::vector<int32_t>>();
        const bool nullable = schema.at("nullable").get<bool>();
        auto metadata = utils::get_metadata(schema);

        std::vector<sparrow::array> children_arrays = get_children_arrays(array, schema, root);
        SPARROW_ASSERT_TRUE(children_arrays.size() == 1);

        // The struct array contains key-value pairs as its children
        // Get the children arrays from the struct directly
        auto struct_array_json = array.at("children")[0];    // Get the struct child data
        auto struct_schema_json = schema.at("children")[0];  // Get the struct child schema
        std::vector<sparrow::array> entries_children = get_children_arrays(
            struct_array_json,
            struct_schema_json,
            root
        );

        SPARROW_ASSERT_TRUE(entries_children.size() == 2);

        // Get the key and value arrays
        auto key_array = std::move(entries_children[0]);
        auto value_array = std::move(entries_children[1]);

        if (nullable)
        {
            auto validity = utils::get_validity(array);
            return sparrow::array{sparrow::map_array{
                std::move(key_array),
                std::move(value_array),
                std::move(offsets),
                std::move(validity),
                name,
                std::move(metadata)
            }};
        }
        else
        {
            return sparrow::array{sparrow::map_array{
                std::move(key_array),
                std::move(value_array),
                std::move(offsets),
                false,
                name,
                std::move(metadata)
            }};
        }
    }
}
