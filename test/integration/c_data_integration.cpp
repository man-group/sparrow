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

#include "c_data_integration.hpp"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "sparrow/array.hpp"
#include "sparrow/layout/fixed_width_binary_layout/fixed_width_binary_array.hpp"
#include "sparrow/layout/null_array.hpp"
#include "sparrow/layout/primitive_layout/primitive_array.hpp"
#include "sparrow/layout/run_end_encoded_layout/run_end_encoded_array.hpp"
#include "sparrow/layout/struct_layout/struct_array.hpp"
#include "sparrow/layout/temporal/date_array.hpp"
#include "sparrow/layout/temporal/date_types.hpp"
#include "sparrow/layout/temporal/duration_array.hpp"
#include "sparrow/layout/temporal/interval_array.hpp"
#include "sparrow/layout/temporal/timestamp_array.hpp"
#include "sparrow/layout/union_array.hpp"
#include "sparrow/layout/variable_size_binary_view_array.hpp"
#include "sparrow/record_batch.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/metadata.hpp"


sparrow::array build_array_from_json(const nlohmann::json& array, const nlohmann::json& schema);

static constexpr std::string_view VALIDITY = "VALIDITY";
static constexpr std::string_view DATA = "DATA";
static constexpr std::string_view OFFSET = "OFFSET";
static constexpr std::string_view TYPE_ID = "TYPE_ID";

std::vector<std::vector<std::byte>> hexStringsToBytes(const std::vector<std::string>& hexStrings)
{
    std::vector<std::vector<std::byte>> result;
    result.reserve(hexStrings.size());

    for (const auto& hexStr : hexStrings)
    {
        std::vector<std::byte> bytes;

        // If the string is empty, add an empty vector
        if (hexStr.empty())
        {
            result.push_back(std::move(bytes));
            continue;
        }

        // Reserve memory for the expected number of bytes (half the hex string length)
        bytes.reserve((hexStr.length() + 1) / 2);

        // Process hex string two characters at a time
        for (size_t i = 0; i < hexStr.length(); i += 2)
        {
            std::string_view byteStr = (i + 1 < hexStr.length()) ? std::string_view(hexStr).substr(i, 2)
                                                                 : std::string_view(hexStr).substr(i, 1);

            // Parse the hex byte
            unsigned int byteValue;
            auto [ptr, ec] = std::from_chars(byteStr.data(), byteStr.data() + byteStr.size(), byteValue, 16);
            if (ec == std::errc{})
            {
                bytes.push_back(static_cast<std::byte>(byteValue));
            }
        }

        result.push_back(std::move(bytes));
    }

    return result;
}

const nlohmann::json& get_child(const nlohmann::json& schema_or_array, const std::string& name)
{
    auto it = std::find_if(
        schema_or_array.at("children").begin(),
        schema_or_array.at("children").end(),
        [&name](const nlohmann::json& child)
        {
            return child.at("name").get<std::string>() == name;
        }
    );

    if (it == schema_or_array.at("children").end())
    {
        throw std::runtime_error("Child not found: " + name);
    }

    return *it;
}

std::vector<std::pair<const nlohmann::json&, const nlohmann::json&>>
get_children(const nlohmann::json& array, const nlohmann::json& schema)
{
    const auto& schema_children = schema.at("children");
    std::vector<std::pair<const nlohmann::json&, const nlohmann::json&>> children;
    children.reserve(schema_children.size());

    for (const auto& child_schema : schema_children)
    {
        const std::string& name = child_schema.at("name").get<std::string>();
        children.emplace_back(get_child(array, name), child_schema);
    }

    return children;
}

std::vector<bool> get_validity(const nlohmann::json& array)
{
    auto validity_range = array.at(VALIDITY).get<std::vector<int>>()
                          | std::views::transform(
                              [](int value)
                              {
                                  return value != 0;
                              }
                          );
    return std::vector<bool>(validity_range.begin(), validity_range.end());
}

std::vector<sparrow::array> get_children_arrays(const nlohmann::json& array, const nlohmann::json& schema)
{
    const auto children_json = get_children(array, schema);
    std::vector<sparrow::array> children;
    children.reserve(children_json.size());
    for (const auto& [child_array, child_schema] : children_json)
    {
        children.push_back(build_array_from_json(child_array, child_schema));
    }
    return children;
}

void check_type(const nlohmann::json& array, const nlohmann::json& schema, const std::string& type)
{
    const std::string schema_type = schema.at("type").at("name").get<std::string>();
    if (schema_type != type)
    {
        throw std::runtime_error("Invalid type");
    }
}

std::optional<std::vector<sparrow::metadata_pair>> get_metadata(const nlohmann::json& schema)
{
    std::vector<sparrow::metadata_pair> metadata;
    const auto metadata_json = schema.find("metadata");
    if (metadata_json == schema.end())
    {
        return std::nullopt;
    }
    for (const auto& [_, pair] : metadata_json->items())
    {
        auto key = pair.at("key").get<std::string>();
        auto value = pair.at("value").get<std::string>();
        metadata.emplace_back(std::move(key), std::move(value));
    }
    return metadata;
}

sparrow::array null_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "null");
    const std::string name = schema.at("name").get<std::string>();
    const std::size_t count = array.at("count").get<std::size_t>();
    auto metadata = get_metadata(schema);
    sparrow::null_array ar{count, name, std::move(metadata)};
    return sparrow::array{std::move(ar)};
}

sparrow::array struct_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "struct");
    const std::string name = schema.at("name").get<std::string>();
    auto validity = get_validity(array);
    auto metadata = get_metadata(schema);
    sparrow::struct_array ar{get_children_arrays(array, schema), std::move(validity), name, std::move(metadata)};
    return sparrow::array{std::move(ar)};
}

sparrow::array list_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "list");
    const std::string name = schema.at("name").get<std::string>();
    auto validity = get_validity(array);
    auto offsets = array.at(OFFSET).get<std::vector<int32_t>>();
    auto metadata = get_metadata(schema);
    sparrow::list_array ar{
        std::move(get_children_arrays(array, schema)[0]),
        offsets,
        std::move(validity),
        name,
        std::move(metadata)
    };
    return sparrow::array{std::move(ar)};
}

sparrow::array large_list_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "largelist");
    const std::string name = schema.at("name").get<std::string>();
    auto validity = get_validity(array);
    auto offsets = array.at(OFFSET).get<std::vector<std::string>>()
                   | std::views::transform(
                       [](const std::string& offset)
                       {
                           return std::stoull(offset);
                       }
                   );
    auto metadata = get_metadata(schema);
    sparrow::big_list_array ar{
        std::move(get_children_arrays(array, schema)[0]),
        offsets,
        std::move(validity),
        name,
        std::move(metadata)
    };
    return sparrow::array{std::move(ar)};
}

sparrow::array list_view_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
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

sparrow::array large_list_view_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
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

sparrow::array primitive_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "int");
    const uint8_t bit_width = schema.at("type").at("bitWidth").get<uint8_t>();
    const bool is_signed = schema.at("type").at("isSigned").get<bool>();
    const std::string name = schema.at("name").get<std::string>();
    auto validity = get_validity(array);
    auto metadata = get_metadata(schema);
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
                auto data = array.at(DATA).get<std::vector<std::string>>()
                            | std::views::transform(
                                [](const std::string& str)
                                {
                                    return std::stoll(std::string(str));
                                }
                            );
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
                auto data = array.at(DATA).get<std::vector<std::string>>()
                            | std::views::transform(
                                [](const std::string& str)
                                {
                                    return std::stoull(str);
                                }
                            );
                sparrow::primitive_array<uint64_t> ar{data, std::move(validity), name, std::move(metadata)};
                return sparrow::array{std::move(ar)};
            }
        }
    }
    throw std::runtime_error("Invalid bit width or signedness");
}

sparrow::array bool_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "bool");
    const std::string name = schema.at("name").get<std::string>();
    auto validity = get_validity(array);
    auto metadata = get_metadata(schema);
    sparrow::primitive_array<bool> ar{
        array.at(DATA).get<std::vector<bool>>(),
        std::move(validity),
        name,
        std::move(metadata)
    };
    return sparrow::array{std::move(ar)};
}

sparrow::array floating_point_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "floatingpoint");
    const std::string precision = schema.at("type").at("precision").get<std::string>();
    const std::string name = schema.at("name").get<std::string>();
    auto validity = get_validity(array);
    auto metadata = get_metadata(schema);
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

sparrow::array decimal_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "decimal");
    const std::string name = schema.at("name").get<std::string>();
    const uint32_t precision = schema.at("type").at("precision").get<uint32_t>();
    const uint32_t scale = schema.at("type").at("scale").get<uint32_t>();
    const uint32_t byte_width = schema.at("type").at("bitWidth").get<uint32_t>();
    auto data_str = array.at(DATA).get<std::vector<std::string>>();

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
            sparrow::decimal_32_array{data, get_validity(array), precision, scale, name, get_metadata(schema)}
        };
    }
    else if (byte_width == 64)
    {
        auto data = data_str
                    | std::views::transform(
                        [](const std::string& value) -> int64_t
                        {
                            return std::stoll(value);
                        }
                    );
        return sparrow::array{
            sparrow::decimal_64_array{data, get_validity(array), precision, scale, name, get_metadata(schema)}
        };
    }
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
            sparrow::decimal_128_array{data, get_validity(array), precision, scale, name, get_metadata(schema)}
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
            sparrow::decimal_256_array{data, get_validity(array), precision, scale, name, get_metadata(schema)}
        };
    }
    else
    {
        throw std::runtime_error("Invalid byte width for decimal type");
    }
}

sparrow::array fixedsizebinary_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "fixedsizebinary");
    const std::string name = schema.at("name").get<std::string>();
    const std::size_t byte_width = schema.at("type").at("byteWidth").get<std::size_t>();
    auto data_str = array.at(DATA).get<std::vector<std::string>>();
    auto data = hexStringsToBytes(data_str);
    auto metadata = get_metadata(schema);
    if (data.empty())
    {
        sparrow::u8_buffer<std::byte> data_buffer(0);
        sparrow::fixed_width_binary_array
            ar{std::move(data_buffer), byte_width, std::array<bool, 0>{}, name, std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
    else
    {
        auto validity = get_validity(array);
        sparrow::fixed_width_binary_array ar{std::move(data), std::move(validity), name, std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
}

sparrow::array string_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "utf8");
    const std::string name = schema.at("name").get<std::string>();
    auto data = array.at(DATA).get<std::vector<std::string>>();
    auto validity = get_validity(array);
    auto metadata = get_metadata(schema);
    return sparrow::array{sparrow::string_array{std::move(data), std::move(validity), name, std::move(metadata)}
    };
}

sparrow::array big_string_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "largeutf8");
    const std::string name = schema.at("name").get<std::string>();
    auto data = array.at(DATA).get<std::vector<std::string>>();
    auto validity = get_validity(array);
    auto metadata = get_metadata(schema);
    return sparrow::array{
        sparrow::big_string_array{std::move(data), std::move(validity), name, std::move(metadata)}
    };
}

sparrow::array string_view_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "utf8_view");
    const std::string name = schema.at("name").get<std::string>();
    auto data = array.at(DATA).get<std::vector<std::string>>();
    auto validity = get_validity(array);
    auto metadata = get_metadata(schema);
    return sparrow::array{
        sparrow::string_view_array{std::move(data), std::move(validity), name, std::move(metadata)}
    };
}

sparrow::array date_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "date");
    const std::string name = schema.at("name").get<std::string>();
    const std::string unit = schema.at("type").at("unit").get<std::string>();
    auto validity = get_validity(array);
    auto metadata = get_metadata(schema);
    if (unit == "DAY")
    {
        auto date_days_values = array.at(DATA).get<std::vector<int32_t>>()
                                | std::views::transform(
                                    [](int32_t value)
                                    {
                                        return sparrow::date_days{sparrow::date_days::duration{value}};
                                    }
                                );
        sparrow::date_days_array ar{date_days_values, std::move(validity), name, std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
    else if (unit == "MILLISECOND")
    {
        auto data = array.at(DATA).get<std::vector<std::string>>()
                    | std::views::transform(
                        [](const std::string& value)
                        {
                            return std::stoll(value);
                        }
                    )
                    | std::views::transform(
                        [](int64_t value)
                        {
                            return sparrow::date_milliseconds{sparrow::date_milliseconds::duration{value}};
                        }
                    );
        sparrow::date_milliseconds_array ar{data, std::move(validity), name, std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
    else
    {
        throw std::runtime_error("Invalid unit");
    }
}

sparrow::array time_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "time");
    const std::string name = schema.at("name").get<std::string>();
    const std::string unit = schema.at("type").at("unit").get<std::string>();

    auto validity = get_validity(array);
    auto metadata = get_metadata(schema);
    if (unit == "SECOND")
    {
        auto values = array.at(DATA).get<std::vector<int32_t>>()
                      | std::views::transform(
                          [](int32_t value)
                          {
                              return sparrow::chrono::time_seconds{value};
                          }
                      );
        sparrow::time_seconds_array ar{values, std::move(validity), name, std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }

    else if (unit == "MILLISECOND")
    {
        auto values = array.at(DATA).get<std::vector<int32_t>>()
                      | std::views::transform(
                          [](int32_t value)
                          {
                              return sparrow::chrono::time_milliseconds{value};
                          }
                      );
        sparrow::time_milliseconds_array ar{values, std::move(validity), name, std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
    else if (unit == "MICROSECOND")
    {
        auto values = array.at(DATA).get<std::vector<std::string>>()
                      | std::views::transform(
                          [](const std::string& value)
                          {
                              return std::stoll(value);
                          }
                      )
                      | std::views::transform(
                          [](int64_t value)
                          {
                              return sparrow::chrono::time_microseconds{value};
                          }
                      );
        sparrow::time_microseconds_array ar{values, std::move(validity), name, std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
    else if (unit == "NANOSECOND")
    {
        auto values = array.at(DATA).get<std::vector<std::string>>()
                      | std::views::transform(
                          [](const std::string& value)
                          {
                              return std::stoll(value);
                          }
                      )
                      | std::views::transform(
                          [](int64_t value)
                          {
                              return sparrow::chrono::time_nanoseconds{value};
                          }
                      );
        sparrow::time_nanoseconds_array ar{values, std::move(validity), name, std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
    else
    {
        throw std::runtime_error("Invalid unit");
    }
}

sparrow::array timestamp_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "timestamp");
    const std::string name = schema.at("name").get<std::string>();
    const std::string unit = schema.at("type").at("unit").get<std::string>();
    std::optional<std::string> timezone;
    if (schema.at("type").contains("timezone"))
    {
        timezone = schema.at("type").at("timezone").get<std::string>();
    }
    const date::time_zone* tz = timezone ? date::locate_zone(*timezone) : nullptr;
    auto data = array.at(DATA).get<std::vector<std::string>>();
    auto data_int = data
                    | std::views::transform(
                        [](const std::string& value) -> int64_t
                        {
                            return std::stoll(value);
                        }
                    );
    auto validity = get_validity(array);
    auto metadata = get_metadata(schema);
    if (unit == "SECOND")
    {
        auto values = data_int
                      | std::views::transform(
                          [tz](int64_t value)
                          {
                              using duration = sparrow::timestamp_second::duration;
                              const date::sys_time<duration> sys_time{duration{value}};
                              return sparrow::timestamp_second{tz, sys_time};
                          }
                      );
        sparrow::timestamp_seconds_array ar{tz, values, std::move(validity), name, std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
    else if (unit == "MILLISECOND")
    {
        auto values = data_int
                      | std::views::transform(
                          [tz](int64_t value)
                          {
                              using duration = sparrow::timestamp_millisecond::duration;
                              const date::sys_time<duration> sys_time{duration{value}};
                              return sparrow::timestamp_millisecond{tz, sys_time};
                          }
                      );
        sparrow::timestamp_milliseconds_array ar{tz, values, std::move(validity), name, std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
    else if (unit == "MICROSECOND")
    {
        auto values = data_int
                      | std::views::transform(
                          [tz](int64_t value)
                          {
                              using duration = sparrow::timestamp_microsecond::duration;
                              const date::sys_time<duration> sys_time{duration{value}};
                              return sparrow::timestamp_microsecond{tz, sys_time};
                          }
                      );
        sparrow::timestamp_microseconds_array ar{tz, values, std::move(validity), name, std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
    else if (unit == "NANOSECOND")
    {
        auto values = data_int
                      | std::views::transform(
                          [tz](int64_t value)
                          {
                              using duration = sparrow::timestamp_nanosecond::duration;
                              const date::sys_time<duration> sys_time{duration{value}};
                              return sparrow::timestamp_nanosecond{tz, sys_time};
                          }
                      );
        sparrow::timestamp_nanoseconds_array ar{tz, values, std::move(validity), name, std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
    else
    {
        throw std::runtime_error("Invalid unit");
    }
}

sparrow::array fixed_size_list_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "fixedsizelist");
    const std::string name = schema.at("name").get<std::string>();
    const size_t list_size = schema.at("type").at("listSize").get<size_t>();
    auto validity = get_validity(array);
    auto metadata = get_metadata(schema);
    sparrow::fixed_sized_list_array ar{
        list_size,
        std::move(get_children_arrays(array, schema)[0]),
        std::move(validity),
        name,
        std::move(metadata)
    };
    return sparrow::array{std::move(ar)};
}

sparrow::array duration_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "duration");
    const std::string name = schema.at("name").get<std::string>();
    const std::string unit = schema.at("type").at("unit").get<std::string>();
    auto data = array.at(DATA).get<std::vector<int64_t>>();
    auto validity = get_validity(array);
    auto metadata = get_metadata(schema);
    if (unit == "SECOND")
    {
        auto values = data
                      | std::views::transform(
                          [](int64_t value)
                          {
                              return std::chrono::seconds{value};
                          }
                      );
        sparrow::duration_seconds_array ar{values, std::move(validity), std::move(name), std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
    else if (unit == "MILLISECOND")
    {
        auto values = data
                      | std::views::transform(
                          [](int64_t value)
                          {
                              return std::chrono::milliseconds{value};
                          }
                      );
        sparrow::duration_milliseconds_array ar{values, std::move(validity), std::move(name), std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
    else if (unit == "MICROSECOND")
    {
        auto values = data
                      | std::views::transform(
                          [](int64_t value)
                          {
                              return std::chrono::microseconds{value};
                          }
                      );
        sparrow::duration_microseconds_array ar{values, std::move(validity), std::move(name), std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
    else if (unit == "NANOSECOND")
    {
        auto values = data
                      | std::views::transform(
                          [](int64_t value)
                          {
                              return std::chrono::nanoseconds{value};
                          }
                      );
        sparrow::duration_nanoseconds_array ar{values, std::move(validity), std::move(name), std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
    else
    {
        throw std::runtime_error("Invalid unit");
    }
}

sparrow::array interval_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "interval");
    const std::string name = schema.at("name").get<std::string>();
    const std::string unit = schema.at("type").at("unit").get<std::string>();

    auto validity = get_validity(array);
    auto metadata = get_metadata(schema);
    if (unit == "YEAR_MONTH")
    {
        auto values = array.at(DATA).get<std::vector<int32_t>>()
                      | std::views::transform(
                          [](int64_t value)
                          {
                              return std::chrono::months{value};
                          }
                      );
        sparrow::months_interval_array ar{values, std::move(validity), std::move(name), std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
    else if (unit == "DAY_TIME")
    {
        auto values = array.at(DATA)
                      | std::views::transform(
                          [](const nlohmann::json& value) -> sparrow::days_time_interval
                          {
                              return sparrow::days_time_interval{
                                  .days = std::chrono::days{value.at("days").get<int32_t>()},
                                  .time = std::chrono::milliseconds{value.at("milliseconds").get<int32_t>()}
                              };
                          }
                      );
        sparrow::days_time_interval_array ar{values, std::move(validity), std::move(name), std::move(metadata)};
        return sparrow::array{std::move(ar)};
    }
    else
    {
        throw std::runtime_error("Invalid unit");
    }
}

sparrow::array binary_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "binary");
    const std::string name = schema.at("name").get<std::string>();
    auto data_str = array.at(DATA).get<std::vector<std::string>>();
    // each element of data are strings which are hexadecimal representation of bytes
    // convert them to bytes
    auto data = hexStringsToBytes(data_str);
    auto validity = get_validity(array);
    auto metadata = get_metadata(schema);
    return sparrow::array{sparrow::binary_array{std::move(data), std::move(validity), name, std::move(metadata)}
    };
}

sparrow::array sparse_union_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "union");
    const std::string mode = schema.at("type").at("mode").get<std::string>();
    if (mode != "SPARSE")
    {
        throw std::runtime_error("Invalid mode");
    }
    const std::string name = schema.at("name").get<std::string>();
    auto metadata = get_metadata(schema);
    auto type_ids_values = schema.at("type").at("typeIds").get<std::vector<uint8_t>>();
    const sparrow::sparse_union_array::type_id_buffer_type type_ids{std::move(type_ids_values)};
    auto children = get_children_arrays(array, schema);
    return sparrow::array{sparrow::sparse_union_array{std::move(children), std::vector<std::uint8_t>{}}};
}

sparrow::array dense_union_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "union");
    const std::string mode = schema.at("type").at("mode").get<std::string>();
    if (mode != "DENSE")
    {
        throw std::runtime_error("Invalid mode");
    }
    const std::string name = schema.at("name").get<std::string>();
    auto metadata = get_metadata(schema);
    sparrow::dense_union_array::offset_buffer_type offsets{array.at(OFFSET).get<std::vector<uint32_t>>()};
    sparrow::u8_buffer<std::uint8_t> child_index_to_type_id{
        schema.at("type").at("typeIds").get<std::vector<uint8_t>>()
    };
    std::vector<std::uint8_t> type_ids = array.at(TYPE_ID).get<std::vector<std::uint8_t>>();
    auto children = get_children_arrays(array, schema);
    return sparrow::array{sparrow::dense_union_array{
        std::move(children),
        std::move(type_ids),
        std::move(offsets),
        std::move(child_index_to_type_id)
    }};
    // throw std::runtime_error("Not implemented");
}

sparrow::array union_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    check_type(array, schema, "union");
    const std::string mode = schema.at("type").at("mode").get<std::string>();
    if (mode == "DENSE")
    {
        return dense_union_array_from_json(array, schema);
    }
    else if (mode == "SPARSE")
    {
        return sparse_union_array_from_json(array, schema);
    }
    else
    {
        throw std::runtime_error("Invalid mode: " + mode);
    }
}

// sparrow::array runendencoded_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
// {
//     check_type(array, schema, "runendencoded");
//     return sparrow::array{sparrow::run_end_encoded_array{}};
// }

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
            const bool nullable = field.at("nullable").get<bool>();
            const auto type = field.at("type");

            const auto dictionary_it = field.find("dictionary");
            if (dictionary_it != field.end())
            {
                SPARROW_ASSERT_TRUE(dictionary_it->is_object());
                const auto id_it = field.find("type");
            }

            const auto children_it = field.find("children");
            if (children_it != field.end())
            {
                SPARROW_ASSERT_TRUE(children_it->is_array());
                read_schema_from_json(*children_it);
            }
        }
    }
}

// Unordered map witk key = type name and value = function
using array_builder_function = std::function<sparrow::array(const nlohmann::json&, const nlohmann::json&)>;
const std::unordered_map<std::string, array_builder_function> array_builders{
    {"null", null_array_from_json},
    {"struct", struct_array_from_json},
    {"list", list_array_from_json},
    {"largelist", large_list_array_from_json},
    {"listview", list_view_array_from_json},
    {"largelistview", large_list_view_array_from_json},
    {"union", union_array_from_json},
    {"int", primitive_array_from_json},
    {"floatingpoint", floating_point_from_json},
    {"utf8", string_array_from_json},
    {"largeutf8", big_string_array_from_json},
    {"binary", binary_array_from_json},
    // {"largebinary", binary_array_from_json},
    {"utf8view", string_view_from_json},
    // {"binaryview", binary_array_from_json},
    {"fixedsizebinary", fixedsizebinary_from_json},
    {"bool", bool_array_from_json},
    {"decimal", decimal_from_json},
    {"date", date_array_from_json},
    {"time", time_array_from_json},
    {"timestamp", timestamp_array_from_json},
    {"interval", interval_array_from_json},
    {"duration", duration_array_from_json},
    {"sparse_union", sparse_union_array_from_json},
    {"fixedsizelist", fixed_size_list_array_from_json},
    // {"runendencoded", runendencoded_array_from_json},
};

sparrow::array build_array_from_json(const nlohmann::json& array, const nlohmann::json& schema)
{
    const std::string type = schema.at("type").at("name").get<std::string>();
    const auto builder_it = array_builders.find(type);
    if (builder_it == array_builders.end())
    {
        throw std::runtime_error("Invalid type");
    }
    return builder_it->second(array, schema);
}

sparrow::record_batch build_record_batch_from_json(const nlohmann::json& data, size_t num_batches)
{
    const auto& schemas = data.at("schema").at("fields");
    std::unordered_map<std::string, const nlohmann::json> schema_map;
    for (const auto& schema : schemas)
    {
        const std::string name = schema.at("name").get<std::string>();
        schema_map.try_emplace(name, schema);
    }
    const auto& batches = data.at("batches");
    const auto& batch = batches.at(num_batches);

    const auto& columns = batch.at("columns");
    std::vector<sparrow::array> arrays;
    arrays.reserve(columns.size());
    for (const auto& column : columns)
    {
        const auto column_name = column.at("name").get<std::string>();
        const auto& schema = schema_map.at(column_name);
        arrays.emplace_back(build_array_from_json(column, schema));
    }
    const auto names = columns
                       | std::views::transform(
                           [](const nlohmann::json& column)
                           {
                               return column.at("name").get<std::string>();
                           }
                       );
    return sparrow::record_batch{names, std::move(arrays)};
}

static std::string global_error;

const char* nanoarrow_CDataIntegration_ExportSchemaFromJson(const char* json_path, ArrowSchema* out)
{
    try
    {
        std::ifstream json_file(json_path);
        const nlohmann::json data = nlohmann::json::parse(json_file);
        sparrow::record_batch record_batch = build_record_batch_from_json(data, 0);
        sparrow::struct_array struct_array = record_batch.extract_struct_array();
        auto [array, schema] = sparrow::extract_arrow_structures(std::move(struct_array));
        array.release(&array);
        *out = schema;
    }
    catch (const std::exception& e)
    {
        global_error = e.what();
        return global_error.c_str();
    }
    return nullptr;
}
