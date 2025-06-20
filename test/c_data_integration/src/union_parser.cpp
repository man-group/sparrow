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

#include "sparrow/c_data_integration/union_parser.hpp"

#include "sparrow/c_data_integration/constant.hpp"
#include "sparrow/c_data_integration/json_parser.hpp"
#include "sparrow/c_data_integration/utils.hpp"

namespace sparrow::c_data_integration
{
    sparrow::array
    sparse_union_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(schema, "union");
        const std::string mode = schema.at("type").at("mode").get<std::string>();
        if (mode != "SPARSE")
        {
            throw std::runtime_error("Invalid mode");
        }
        const std::string name = schema.at("name").get<std::string>();
        auto metadata = utils::get_metadata(schema);
        auto type_ids_values = schema.at("type").at("typeIds").get<std::vector<uint8_t>>();
        std::vector<std::uint8_t> type_ids = array.at(TYPE_ID).get<std::vector<std::uint8_t>>();
        sparrow::sparse_union_array::type_id_buffer_type type_ids_buffer{std::move(type_ids)};
        auto children = get_children_arrays(array, schema, root);
        auto type_mapping = type_ids_values;
        return sparrow::array{sparrow::sparse_union_array{
            std::move(children),
            std::move(type_ids_buffer),
            std::make_optional(std::move(type_mapping)),
            name,
            std::move(metadata)
        }};
    }

    sparrow::array
    dense_union_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(schema, "union");
        const std::string mode = schema.at("type").at("mode").get<std::string>();
        if (mode != "DENSE")
        {
            throw std::runtime_error("Invalid mode");
        }
        const std::string name = schema.at("name").get<std::string>();
        auto metadata = utils::get_metadata(schema);
        sparrow::dense_union_array::offset_buffer_type offsets{array.at(OFFSET).get<std::vector<uint32_t>>()};
        sparrow::u8_buffer<std::uint8_t> child_index_to_type_id{
            schema.at("type").at("typeIds").get<std::vector<uint8_t>>()
        };
        std::vector<std::uint8_t> type_ids = array.at(TYPE_ID).get<std::vector<std::uint8_t>>();
        auto children = get_children_arrays(array, schema, root);
        return sparrow::array{sparrow::dense_union_array{
            std::move(children),
            std::move(type_ids),
            std::move(offsets),
            std::make_optional(std::move(child_index_to_type_id)),
            name,
            std::move(metadata)
        }};
    }

    sparrow::array
    union_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(schema, "union");
        const std::string mode = schema.at("type").at("mode").get<std::string>();
        if (mode == "DENSE")
        {
            return dense_union_array_from_json(array, schema, root);
        }
        else if (mode == "SPARSE")
        {
            return sparse_union_array_from_json(array, schema, root);
        }
        else
        {
            throw std::runtime_error("Invalid mode: " + mode);
        }
    }
}