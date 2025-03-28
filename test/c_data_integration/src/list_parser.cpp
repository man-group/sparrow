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

#include "sparrow/c_data_integration/list_parser.hpp"

#include "sparrow/c_data_integration/constant.hpp"
#include "sparrow/c_data_integration/json_parser.hpp"
#include "sparrow/c_data_integration/utils.hpp"

namespace sparrow::c_data_integration
{
    sparrow::array
    list_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(schema, "list");
        const std::string name = schema.at("name").get<std::string>();
        auto validity = utils::get_validity(array);
        auto offsets = array.at(OFFSET).get<std::vector<int32_t>>();
        auto metadata = utils::get_metadata(schema);
        sparrow::list_array ar{
            std::move(get_children_arrays(array, schema, root)[0]),
            offsets,
            std::move(validity),
            name,
            std::move(metadata)
        };
        return sparrow::array{std::move(ar)};
    }

    sparrow::array
    large_list_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(schema, "largelist");
        const std::string name = schema.at("name").get<std::string>();
        auto validity = utils::get_validity(array);
        const auto& offsets_str = array.at(OFFSET).get<std::vector<std::string>>();
        auto offsets = utils::from_strings_to_Is<uint64_t>(offsets_str);
        auto metadata = utils::get_metadata(schema);
        sparrow::big_list_array ar{
            std::move(get_children_arrays(array, schema, root)[0]),
            offsets,
            std::move(validity),
            name,
            std::move(metadata)
        };
        return sparrow::array{std::move(ar)};
    }
}