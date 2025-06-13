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

#include "sparrow/c_data_integration/fixedsizelist_parser.hpp"

#include "sparrow/c_data_integration/json_parser.hpp"
#include "sparrow/c_data_integration/utils.hpp"

namespace sparrow::c_data_integration
{

    sparrow::array fixed_size_list_array_from_json(
        const nlohmann::json& array,
        const nlohmann::json& schema,
        const nlohmann::json& root
    )
    {
        utils::check_type(schema, "fixedsizelist");
        const std::string name = schema.at("name").get<std::string>();
        const size_t list_size = schema.at("type").at("listSize").get<size_t>();
        bool nullable = schema.at("nullable").get<bool>();
        auto metadata = utils::get_metadata(schema);
        if (nullable)
        {
            auto validity = utils::get_validity(array);
            sparrow::fixed_sized_list_array ar{
                list_size,
                std::move(get_children_arrays(array, schema, root)[0]),
                std::move(validity),
                name,
                std::move(metadata)
            };

            return sparrow::array{sparrow::fixed_sized_list_array{
                list_size,
                std::move(get_children_arrays(array, schema, root)[0]),
                std::move(validity),
                name,
                std::move(metadata)
            }};
        }
        else
        {
            return sparrow::array{sparrow::fixed_sized_list_array{
                list_size,
                std::move(get_children_arrays(array, schema, root)[0]),
                false,
                name,
                std::move(metadata)
            }};
        }
    }
}
