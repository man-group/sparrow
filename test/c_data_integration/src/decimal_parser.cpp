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

#include "sparrow/c_data_integration/decimal_parser.hpp"

#include "sparrow/c_data_integration/constant.hpp"
#include "sparrow/c_data_integration/utils.hpp"

namespace sparrow::c_data_integration
{
    sparrow::array
    decimal_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(array, schema, "decimal");
        const std::string name = schema.at("name").get<std::string>();
        const uint32_t precision = schema.at("type").at("precision").get<uint32_t>();
        const uint32_t scale = schema.at("type").at("scale").get<uint32_t>();
        const uint32_t byte_width = schema.at("type").at("bitWidth").get<uint32_t>();
        auto data_str = array.at(DATA).get<std::vector<std::string>>();
        auto metadata = utils::get_metadata(schema);
        auto validity = utils::get_validity(array);

        if (byte_width == 32)
        {
            auto data = data_str
                        | std::views::transform(
                            [](const std::string& value) -> int32_t
                            {
                                return std::stoi(value);
                            }
                        );
            return sparrow::array{
                sparrow::decimal_32_array{data, std::move(validity), precision, scale, name, std::move(metadata)}
            };
        }
        else if (byte_width == 64)
        {
            auto data = utils::from_strings_to_Is<int64_t>(data_str);
            return sparrow::array{
                sparrow::decimal_64_array{data, std::move(validity), precision, scale, name, std::move(metadata)}
            };
        }
#ifndef SPARROW_USE_LARGE_INT_PLACEHOLDERS
        else if (byte_width == 128)
        {
            auto data = data_str
                        | std::views::transform(
                            [](const std::string& value_str) -> sparrow::int128_t
                            {
                                return sparrow::stobigint<sparrow::int128_t>(value_str);
                            }
                        );
            return sparrow::array{
                sparrow::decimal_128_array{data, std::move(validity), precision, scale, name, std::move(metadata)}
            };
        }
        else if (byte_width == 256)
        {
            auto data = data_str
                        | std::views::transform(
                            [](const std::string& value_str)
                            {
                                return sparrow::stobigint<sparrow::int256_t>(value_str);
                            }
                        );
            return sparrow::array{
                sparrow::decimal_256_array{data, std::move(validity), precision, scale, name, std::move(metadata)}
            };
        }
#endif
        else
        {
            throw std::runtime_error("Invalid byte width for decimal type");
        }
    }
}