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

#include "sparrow/json_reader/json_parser.hpp"

#include <sparrow/layout/array_access.hpp>

#include "sparrow/json_reader/binary_parser.hpp"
#include "sparrow/json_reader/binaryview_parser.hpp"
#include "sparrow/json_reader/bool_parser.hpp"
#include "sparrow/json_reader/constant.hpp"
#include "sparrow/json_reader/decimal_parser.hpp"
#include "sparrow/json_reader/fixedsizebinary_parser.hpp"
#include "sparrow/json_reader/fixedsizelist_parser.hpp"
#include "sparrow/json_reader/list_parser.hpp"
#include "sparrow/json_reader/listview_parser.hpp"
#include "sparrow/json_reader/map_parser.hpp"
#include "sparrow/json_reader/null_parser.hpp"
#include "sparrow/json_reader/primitive_parser.hpp"
#include "sparrow/json_reader/run_end_encoded_parser.hpp"
#include "sparrow/json_reader/string_parser.hpp"
#include "sparrow/json_reader/struct_parser.hpp"
#include "sparrow/json_reader/temporal_parser.hpp"
#include "sparrow/json_reader/union_parser.hpp"
#include "sparrow/json_reader/utils.hpp"

namespace sparrow::json_reader
{
    // Unordered map witk key = type name and value = function
    using array_builder_function = std::function<
        sparrow::array(const nlohmann::json&, const nlohmann::json&, const nlohmann::json&)>;
    const std::unordered_map<std::string, array_builder_function> array_builders{
        {"binary", binary_array_from_json},
        {"binaryview", binaryview_array_from_json},
        {"bool", bool_array_from_json},
        {"date", date_array_from_json},
        {"decimal", decimal_from_json},
        {"dictionary", dictionary_encode_array_from_json},
        {"duration", duration_array_from_json},
        {"fixedsizebinary", fixedsizebinary_from_json},
        {"fixedsizelist", fixed_size_list_array_from_json},
        {"floatingpoint", floating_point_from_json},
        {"int", primitive_array_from_json},
        {"interval", interval_array_from_json},
        {"largebinary", large_binary_array_from_json},
        {"largelist", large_list_array_from_json},
        {"largelistview", large_list_view_array_from_json},
        {"largeutf8", big_string_array_from_json},
        {"list", list_array_from_json},
        {"listview", list_view_array_from_json},
        {"map", map_array_from_json},
        {"null", null_array_from_json},
        {"runendencoded", runendencoded_array_from_json},
        {"struct", struct_array_from_json},
        {"time", time_array_from_json},
        {"timestamp", timestamp_array_from_json},
        {"union", union_array_from_json},
        {"utf8", string_array_from_json},
        {"utf8view", utf8view_array_from_json},
    };

    std::vector<sparrow::array>
    get_children_arrays(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        std::vector<sparrow::array> children;
        for (const auto& child_schema : schema.at("children"))
        {
            const std::string& name = child_schema.at("name").get<std::string>();
            auto children_with_same_name = utils::get_children_with_same_name(array, name);
            size_t children_left = children_with_same_name.size();
            for (const auto& child_with_same_name : children_with_same_name)
            {
                try
                {
                    sparrow::array child = build_array_from_json(child_with_same_name, child_schema, root);
                    children.push_back(std::move(child));
                    break;
                }
                catch (const std::exception& e)
                {
                    if (children_left == 1)
                    {
                        throw std::runtime_error("Failed to build array for child '" + name + "': " + e.what());
                    }
                }
                children_left--;
            }
        }
        return children;
    }

    sparrow::array dictionary_encode_array_from_json(
        const nlohmann::json& array,
        const nlohmann::json& schema,
        const nlohmann::json& root
    )
    {
        const auto& dictionary = schema.at("dictionary");
        const size_t dictionary_id = dictionary.at("id").get<size_t>();
        const auto get_dictionary_array = [&]()
        {
            for (const auto& dictionary_element : root.at("dictionaries"))
            {
                if (dictionary_element.at("id").get<size_t>() == dictionary_id)
                {
                    nlohmann::json altered_schema = schema;
                    if (altered_schema.contains("dictionary"))
                    {
                        altered_schema.erase("metadata");
                    }
                    return build_array_from_json(dictionary_element.at("data").at("columns")[0], altered_schema, root, false);
                }
            }
            throw std::runtime_error("Dictionary not found");
        };

        sparrow::array dictionary_array = get_dictionary_array();

        const auto& index_type = dictionary.at("indexType");
        const std::string name = schema.at("name").get<std::string>();
        const bool index_is_signed = index_type.at("isSigned").get<bool>();
        const size_t index_bit_width = index_type.at("bitWidth").get<size_t>();

        auto index_validity = utils::get_validity(array);
        auto index_metadata = utils::get_metadata(schema);

        auto create_dictionary = [&](auto&& keys)
        {
            using key_element_type = std::decay_t<decltype(keys[0])>;
            return sparrow::array{sparrow::dictionary_encoded_array<key_element_type>{
                std::forward<std::vector<key_element_type>>(keys),
                std::move(dictionary_array),
                std::move(index_validity),
                name,
                std::move(index_metadata)
            }};
        };

        if (index_is_signed)
        {
            switch (index_bit_width)
            {
                case 8:
                    return create_dictionary(array.at(DATA).get<std::vector<int8_t>>());
                case 16:
                    return create_dictionary(array.at(DATA).get<std::vector<int16_t>>());
                case 32:
                    return create_dictionary(array.at(DATA).get<std::vector<int32_t>>());
                case 64:
                    return create_dictionary(array.at(DATA).get<std::vector<int64_t>>());
            }
        }
        else
        {
            switch (index_bit_width)
            {
                case 8:
                    return create_dictionary(array.at(DATA).get<std::vector<uint8_t>>());
                case 16:
                    return create_dictionary(array.at(DATA).get<std::vector<uint16_t>>());
                case 32:
                    return create_dictionary(array.at(DATA).get<std::vector<uint32_t>>());
                case 64:
                    return create_dictionary(array.at(DATA).get<std::vector<uint64_t>>());
            }
        }
        throw std::runtime_error("Invalid bit width or signedness");
    }

    sparrow::array build_array_from_json(
        const nlohmann::json& array,
        const nlohmann::json& schema,
        const nlohmann::json& root,
        bool check_dictionary
    )
    {
        std::string type = schema.at("type").at("name").get<std::string>();
        const bool is_dictionary = schema.contains("dictionary") && check_dictionary;
        if (is_dictionary)
        {
            type = "dictionary";
        }
        const auto builder_it = array_builders.find(type);
        if (builder_it == array_builders.end())
        {
            throw std::runtime_error("Unsupported type: " + type);
        }
        sparrow::array ar = builder_it->second(array, schema, root);
        return ar;
    }

    nlohmann::json
    generate_empty_columns_batch(const std::vector<std::pair<std::string, nlohmann::json>>& schemas)
    {
        nlohmann::json batch = nlohmann::json::object();
        nlohmann::json empty_columns = nlohmann::json::array();
        for (const auto& [name, schema] : schemas)
        {
            nlohmann::json empty_column = nlohmann::json::object();
            empty_column["name"] = name;
            empty_column["count"] = 0;
            empty_column[DATA] = nlohmann::json::array();
            empty_column[VALIDITY] = nlohmann::json::array();
            empty_columns.push_back(empty_column);
        }
        batch["columns"] = empty_columns;
        batch["count"] = schemas.size();
        return batch;
    }

    sparrow::record_batch build_record_batch_from_json(const nlohmann::json& root, size_t num_batches)
    {
        const auto& schemas = root.at("schema").at("fields");
        std::vector<std::pair<std::string, nlohmann::json>> schema_map;
        for (const auto& schema : schemas)
        {
            const std::string name = schema.at("name").get<std::string>();
            schema_map.emplace_back(name, schema);
        }
        auto batches = root.at("batches");
        if (batches.empty())
        {
            auto empty_columns = generate_empty_columns_batch(schema_map);
            batches.push_back(empty_columns);
        }
        if (num_batches >= batches.size())
        {
            throw std::runtime_error(
                "Invalid batch number: index " + std::to_string(num_batches) + " out of "
                + std::to_string(batches.size()) + " batches"
            );
        }
        const auto& batch = batches.at(num_batches);

        const auto& columns = batch.at("columns");
        std::vector<sparrow::array> arrays;
        arrays.reserve(columns.size());
        for (const auto& column : columns)
        {
            const auto column_name = column.at("name").get<std::string>();
            auto schema_with_name_it = std::ranges::find_if(
                schema_map,
                [&column_name](const auto& pair)
                {
                    return pair.first == column_name;
                }
            );
            if (schema_with_name_it == schema_map.end())
            {
                throw std::runtime_error("Column '" + column_name + "' not found in schema");
            }
            const auto& schema = schema_with_name_it->second;

            arrays.emplace_back(build_array_from_json(column, schema, root));
            schema_map.erase(schema_with_name_it);  // Remove processed schema
        }

        const auto names = columns
                           | std::views::transform(
                               [](const nlohmann::json& column)
                               {
                                   return column.at("name").get<std::string>();
                               }
                           );
        std::optional<std::vector<sparrow::metadata_pair>> metadata;
        if (root.at("schema").contains("metadata"))
        {
            metadata = utils::get_metadata(root.at("schema"));
        }
        return sparrow::record_batch{names, std::move(arrays), "", std::move(metadata)};
    }

}  // namespace sparrow::json_reader::json_parser