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

#include "sparrow/json_reader/listview_parser.hpp"

#include "sparrow/json_reader/json_parser.hpp"
#include "sparrow/json_reader/utils.hpp"

namespace sparrow::json_reader
{
    sparrow::array
    list_view_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(schema, "listview");
        const std::string name = schema.at("name").get<std::string>();
        auto validity = utils::get_validity(array);
        auto offsets = utils::get_offsets(array);
        auto sizes = utils::get_sizes(array);
        auto metadata = utils::get_metadata(schema);
        std::vector<sparrow::array> arrays = get_children_arrays(array, schema, root);
        sparrow::list_view_array ar{
            std::move(arrays[0]),
            std::move(offsets),
            std::move(sizes),
            std::move(validity),
            name,
            std::move(metadata)
        };
        return sparrow::array{std::move(ar)};
    }

    sparrow::array large_list_view_array_from_json(
        const nlohmann::json& array,
        const nlohmann::json& schema,
        const nlohmann::json& root
    )
    {
        utils::check_type(schema, "largelistview");
        const std::string name = schema.at("name").get<std::string>();
        auto validity = utils::get_validity(array);
        auto offsets = utils::get_offsets(array);
        auto sizes = utils::get_sizes(array);
        auto metadata = utils::get_metadata(schema);
        std::vector<sparrow::array> arrays = get_children_arrays(array, schema, root);
        sparrow::big_list_view_array ar{
            std::move(arrays[0]),
            std::move(offsets),
            std::move(sizes),
            std::move(validity),
            name,
            std::move(metadata)
        };
        return sparrow::array{std::move(ar)};
    }
}