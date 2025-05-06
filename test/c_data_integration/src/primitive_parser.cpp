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

#include "sparrow/c_data_integration/primitive_parser.hpp"

#include <sparrow/layout/primitive_layout/primitive_array.hpp>

#include "sparrow/c_data_integration/constant.hpp"
#include "sparrow/c_data_integration/utils.hpp"

namespace sparrow::c_data_integration
{
    template <typename T, typename JSON_T = T>
    sparrow::array get_array(
        const nlohmann::json& array,
        bool nullable,
        std::string_view name,
        std::optional<std::vector<sparrow::metadata_pair>>&& metadata
    )
    {
        auto data = array.at(DATA).get<std::vector<JSON_T>>();
        if (nullable)
        {
            auto validity = utils::get_validity(array);
            return sparrow::array{
                sparrow::primitive_array<T>{std::move(data), std::move(validity), name, std::move(metadata)}
            };
        }
        else
        {
            return sparrow::array{sparrow::primitive_array<T>{std::move(data), false, name, std::move(metadata)}
            };
        }
    }

    sparrow::array
    primitive_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json&)
    {
        utils::check_type(schema, "int");
        const uint8_t bit_width = schema.at("type").at("bitWidth").get<uint8_t>();
        const bool is_signed = schema.at("type").at("isSigned").get<bool>();
        const bool nullable = schema.at("nullable").get<bool>();
        std::string name = schema.at("name").get<std::string>();
        auto metadata = utils::get_metadata(schema);
        if (is_signed)
        {
            switch (bit_width)
            {
                case 8:
                {
                    return get_array<int8_t>(array, nullable, name, std::move(metadata));
                }
                case 16:
                {
                    return get_array<int16_t>(array, nullable, name, std::move(metadata));
                }
                case 32:
                {
                    return get_array<int32_t>(array, nullable, name, std::move(metadata));
                }
                case 64:
                {
                    return get_array<int64_t>(array, nullable, name, std::move(metadata));
                }
                default:
                    throw std::runtime_error("Invalid bit width");
            }
        }
        else
        {
            switch (bit_width)
            {
                case 8:
                {
                    return get_array<uint8_t>(array, nullable, name, std::move(metadata));
                }
                case 16:
                {
                    return get_array<uint16_t>(array, nullable, name, std::move(metadata));
                }
                case 32:
                {
                    return get_array<uint32_t>(array, nullable, name, std::move(metadata));
                }
                case 64:
                {
                    return get_array<uint64_t>(array, nullable, name, std::move(metadata));
                }
            }
        }
        throw std::runtime_error("Invalid bit width or signedness");
    }

    sparrow::array
    floating_point_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json&)
    {
        utils::check_type(schema, "floatingpoint");
        const std::string precision = schema.at("type").at("precision").get<std::string>();
        std::string name = schema.at("name").get<std::string>();
        const bool nullable = schema.at("nullable").get<bool>();
        auto metadata = utils::get_metadata(schema);

        if (precision == "HALF")
        {
            return get_array<sparrow::float16_t, float>(array, nullable, name, std::move(metadata));
        }
        else if (precision == "SINGLE")
        {
            return get_array<sparrow::float32_t>(array, nullable, name, std::move(metadata));
        }
        else if (precision == "DOUBLE")
        {
            return get_array<sparrow::float64_t>(array, nullable, name, std::move(metadata));
        }

        throw std::runtime_error("Invalid precision");
    }
}