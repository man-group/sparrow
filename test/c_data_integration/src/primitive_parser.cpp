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
    sparrow::array
    primitive_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(array, schema, "int");
        const uint8_t bit_width = schema.at("type").at("bitWidth").get<uint8_t>();
        const bool is_signed = schema.at("type").at("isSigned").get<bool>();
        const std::string name = schema.at("name").get<std::string>();
        auto validity = utils::get_validity(array);
        auto metadata = utils::get_metadata(schema);
        if (is_signed)
        {
            switch (bit_width)
            {
                case 8:
                {
                    sparrow::primitive_array<int8_t> ar{
                        array.at(DATA).get<std::vector<int8_t>>(),
                        std::move(validity),
                        name,
                        std::move(metadata)
                    };
                    return sparrow::array{std::move(ar)};
                }
                case 16:
                {
                    sparrow::primitive_array<int16_t> ar{
                        array.at(DATA).get<std::vector<int16_t>>(),
                        std::move(validity),
                        name,
                        std::move(metadata)
                    };
                    return sparrow::array{std::move(ar)};
                }
                case 32:
                {
                    sparrow::primitive_array<int32_t> ar{
                        array.at(DATA).get<std::vector<int32_t>>(),
                        std::move(validity),
                        name,
                        std::move(metadata)
                    };
                    return sparrow::array{std::move(ar)};
                }
                case 64:
                {
                    const auto& data_str = array.at(DATA).get<std::vector<std::string>>();
                    auto data = utils::from_strings_to_Is<int64_t>(data_str);
                    sparrow::primitive_array<int64_t> ar{data, std::move(validity), name, std::move(metadata)};
                    return sparrow::array{std::move(ar)};
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
                    sparrow::primitive_array<uint8_t> ar{
                        array.at(DATA).get<std::vector<uint8_t>>(),
                        std::move(validity),
                        name,
                        std::move(metadata)
                    };
                    return sparrow::array{std::move(ar)};
                }
                case 16:
                {
                    sparrow::primitive_array<uint16_t> ar{
                        array.at(DATA).get<std::vector<uint16_t>>(),
                        std::move(validity),
                        name,
                        std::move(metadata)
                    };
                    return sparrow::array{std::move(ar)};
                }
                case 32:
                {
                    sparrow::primitive_array<uint32_t> ar{
                        array.at(DATA).get<std::vector<uint32_t>>(),
                        std::move(validity),
                        name,
                        std::move(metadata)
                    };
                    return sparrow::array{std::move(ar)};
                }
                case 64:
                {
                    const auto& data_str = array.at(DATA).get<std::vector<std::string>>();
                    auto data = utils::from_strings_to_Is<uint64_t>(data_str);
                    sparrow::primitive_array<uint64_t> ar{data, std::move(validity), name, std::move(metadata)};
                    return sparrow::array{std::move(ar)};
                }
            }
        }
        throw std::runtime_error("Invalid bit width or signedness");
    }

    sparrow::array
    floating_point_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(array, schema, "floatingpoint");
        const std::string precision = schema.at("type").at("precision").get<std::string>();
        const std::string name = schema.at("name").get<std::string>();
        auto validity = utils::get_validity(array);
        auto metadata = utils::get_metadata(schema);
        if (precision == "HALF")
        {
            sparrow::primitive_array<sparrow::float16_t> ar{
                array.at(DATA).get<std::vector<float>>(),
                std::move(validity),
                name,
                std::move(metadata)
            };
            return sparrow::array{std::move(ar)};
        }
        else if (precision == "SINGLE")
        {
            sparrow::primitive_array<sparrow::float32_t> ar{
                array.at(DATA).get<std::vector<sparrow::float32_t>>(),
                std::move(validity),
                name,
                std::move(metadata)
            };
            return sparrow::array{std::move(ar)};
        }
        else if (precision == "DOUBLE")
        {
            sparrow::primitive_array<sparrow::float64_t> ar{
                array.at(DATA).get<std::vector<sparrow::float64_t>>(),
                std::move(validity),
                name,
                std::move(metadata)
            };
            return sparrow::array{std::move(ar)};
        }

        throw std::runtime_error("Invalid precision");
    }
}