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

#include "sparrow/c_data_integration/json_parser.hpp"

#include <sparrow/layout/array_access.hpp>

#include "sparrow/c_data_integration/binary_parser.hpp"
#include "sparrow/c_data_integration/bool_parser.hpp"
#include "sparrow/c_data_integration/constant.hpp"
#include "sparrow/c_data_integration/decimal_parser.hpp"
#include "sparrow/c_data_integration/fixedsizebinary_parser.hpp"
#include "sparrow/c_data_integration/fixedsizelist_parser.hpp"
#include "sparrow/c_data_integration/list_parser.hpp"
#include "sparrow/c_data_integration/null_parser.hpp"
#include "sparrow/c_data_integration/primitive_parser.hpp"
#include "sparrow/c_data_integration/run_end_encoded_parser.hpp"
#include "sparrow/c_data_integration/string_parser.hpp"
#include "sparrow/c_data_integration/stringview_parser.hpp"
#include "sparrow/c_data_integration/struct_parser.hpp"
#include "sparrow/c_data_integration/temporal_parser.hpp"
#include "sparrow/c_data_integration/utils.hpp"

namespace sparrow::c_data_integration
{

    // Unordered map witk key = type name and value = function
    using array_builder_function = std::function<
        sparrow::array(const nlohmann::json&, const nlohmann::json&, const nlohmann::json&)>;
    const std::unordered_map<std::string, array_builder_function> array_builders{
        {"binary", binary_array_from_json},
        {"bool", bool_array_from_json},
        {"decimal", decimal_from_json},
        {"fixedsizebinary", fixedsizebinary_from_json},
        {"fixedsizelist", fixed_size_list_array_from_json},
        {"floatingpoint", floating_point_from_json},
        {"int", primitive_array_from_json},
        {"largebinary", large_binary_array_from_json},
        {"largelist", large_list_array_from_json},
        {"largeutf8", big_string_array_from_json},
        {"list", list_array_from_json},
        {"null", null_array_from_json},
        {"struct", struct_array_from_json},
        {"utf8", string_array_from_json},
        {"utf8view", string_view_from_json},
        {"date", date_array_from_json},
        {"time", time_array_from_json},
        {"timestamp", timestamp_array_from_json},
        {"interval", interval_array_from_json},
        {"duration", duration_array_from_json},
        {"runendencoded", runendencoded_array_from_json},
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
        const std::string name = schema.at("name").get<std::string>();
        const auto& dictionary = schema.at("dictionary");
        const size_t dictionary_id = dictionary.at("id").get<size_t>();
        const auto get_dictionary_array = [&]()
        {
            for (const auto& dictionary_element : root.at("dictionaries"))
            {
                if (dictionary_element.at("id").get<size_t>() == dictionary_id)
                {
                    return build_array_from_json(dictionary_element.at("data").at("columns")[0], schema, root, false);
                }
            }
            throw std::runtime_error("Dictionary not found");
        };

        sparrow::array dictionary_array = get_dictionary_array();

        const auto& index_type = dictionary.at("indexType");
        const std::string index_name = index_type.at("name").get<std::string>();
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
                index_name,
                std::forward<std::optional<std::vector<sparrow::metadata_pair>>>(index_metadata)
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

    void read_schema_from_json(const nlohmann::json& data)
    {
        SPARROW_ASSERT_TRUE(data.is_object());
        const auto fields_it = data.find("fields");
        if (fields_it != data.end())
        {
            SPARROW_ASSERT_TRUE(fields_it->is_array());
            for (const auto& field : *fields_it)
            {
                SPARROW_ASSERT_TRUE(field.is_object());

                const std::string name = field.at("name").get<std::string>();
                [[maybe_unused]] const bool nullable = field.at("nullable").get<bool>();
                const auto type = field.at("type");

                // TODO: support dictionary
                // const auto dictionary_it = field.find("dictionary");
                // if (dictionary_it != field.end())
                // {
                //     SPARROW_ASSERT_TRUE(dictionary_it->is_object());
                //     const auto id_it = field.find("type");
                // }

                const auto children_it = field.find("children");
                if (children_it != field.end())
                {
                    SPARROW_ASSERT_TRUE(children_it->is_array());
                    read_schema_from_json(*children_it);
                }
            }
        }
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

    sparrow::record_batch build_record_batch_from_json(const nlohmann::json& root, size_t num_batches)
    {
        const auto& schemas = root.at("schema").at("fields");
        std::unordered_multimap<std::string, const nlohmann::json> schema_map;
        for (const auto& schema : schemas)
        {
            const std::string name = schema.at("name").get<std::string>();
            schema_map.emplace(name, schema);
        }
        const auto& batches = root.at("batches");
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
            const auto schemas_iterators = schema_map.equal_range(column_name);
            const auto num_schemas = std::distance(schemas_iterators.first, schemas_iterators.second);
            size_t inc = 0;
            for (auto& [_, schema] : std::ranges::subrange(schemas_iterators.first, schemas_iterators.second))
            {
                try
                {
                    arrays.emplace_back(build_array_from_json(column, schema, root));
                    break;
                }
                catch (const std::exception& e)
                {
                    if (inc == num_schemas - 1)
                    {
                        throw std::runtime_error(
                            "Failed to build array for column '" + column_name + "': " + e.what()
                        );
                    }
                }
                ++inc;
            }
        }
        const auto names = columns
                           | std::views::transform(
                               [](const nlohmann::json& column)
                               {
                                   return column.at("name").get<std::string>();
                               }
                           );
        return sparrow::record_batch{names, std::move(arrays), ""};
    }

}  // namespace sparrow::c_data_integration::json_parser