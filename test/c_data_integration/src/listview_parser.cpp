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

#include "sparrow/c_data_integration/listview_parser.hpp"

namespace sparrow::c_data_integration
{
    sparrow::array
    list_view_array_from_json(const nlohmann::json&, const nlohmann::json&, const nlohmann::json&)
    {
        // check_type(array, schema, "listview");
        // const std::string name = schema.at("name").get<std::string>();
        // auto validity = get_validity(array);
        // auto metadata = get_metadata(schema);
        // sparrow::list_view_array ar{
        //     std::move(get_children_arrays(array, schema)[0]),
        //     std::move(validity),
        //     name,
        //     std::move(metadata)
        // };
        // return sparrow::array{std::move(ar)};
        throw std::runtime_error("list_view_array_from_json not implemented");
    }

    sparrow::array
    large_list_view_array_from_json(const nlohmann::json&, const nlohmann::json&, const nlohmann::json&)
    {
        // check_type(array, schema, "largelistview");
        // const std::string name = schema.at("name").get<std::string>();
        // const auto children_json = get_children(array, schema);
        // auto validity = get_validity(array);
        // auto metadata = get_metadata(schema);
        // sparrow::big_list_view_array ar{
        //     std::move(get_children_arrays(array, schema)[0]),
        //     std::move(validity),
        //     name,
        //     std::move(metadata)
        // };
        // return sparrow::array{std::move(ar)};
        throw std::runtime_error("large_list_view_array_from_json not implemented");
    }
}