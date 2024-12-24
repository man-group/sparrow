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

#include "sparrow/c_data_integration/temporal_parser.hpp"

#include "sparrow/c_data_integration/constant.hpp"
#include "sparrow/c_data_integration/utils.hpp"

namespace sparrow::c_data_integration
{

    sparrow::array
    date_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(array, schema, "date");
        const std::string name = schema.at("name").get<std::string>();
        const std::string unit = schema.at("type").at("unit").get<std::string>();
        auto validity = utils::get_validity(array);
        auto metadata = utils::get_metadata(schema);
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

    sparrow::array
    time_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(array, schema, "time");
        const std::string name = schema.at("name").get<std::string>();
        const std::string unit = schema.at("type").at("unit").get<std::string>();

        auto validity = utils::get_validity(array);
        auto metadata = utils::get_metadata(schema);
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

    sparrow::array
    timestamp_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(array, schema, "timestamp");
        const std::string name = schema.at("name").get<std::string>();
        const std::string unit = schema.at("type").at("unit").get<std::string>();
        std::optional<std::string> timezone;
        if (schema.at("type").contains("timezone"))
        {
            timezone = schema.at("type").at("timezone").get<std::string>();
        }
        const date::time_zone* tz = timezone ? date::locate_zone(*timezone) : nullptr;
        const auto& data_str = array.at(DATA).get<std::vector<std::string>>();
        auto data_int = utils::from_strings_to_Is<int64_t>(data_str);
        auto validity = utils::get_validity(array);
        auto metadata = utils::get_metadata(schema);
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

    sparrow::array
    duration_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(array, schema, "duration");
        const std::string name = schema.at("name").get<std::string>();
        const std::string unit = schema.at("type").at("unit").get<std::string>();
        auto data = array.at(DATA).get<std::vector<int64_t>>();
        auto validity = utils::get_validity(array);
        auto metadata = utils::get_metadata(schema);
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
            sparrow::duration_milliseconds_array ar{
                values,
                std::move(validity),
                std::move(name),
                std::move(metadata)
            };
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
            sparrow::duration_microseconds_array ar{
                values,
                std::move(validity),
                std::move(name),
                std::move(metadata)
            };
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
            sparrow::duration_nanoseconds_array ar{
                values,
                std::move(validity),
                std::move(name),
                std::move(metadata)
            };
            return sparrow::array{std::move(ar)};
        }
        else
        {
            throw std::runtime_error("Invalid unit");
        }
    }

    sparrow::array
    interval_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(array, schema, "interval");
        const std::string name = schema.at("name").get<std::string>();
        const std::string unit = schema.at("type").at("unit").get<std::string>();

        auto validity = utils::get_validity(array);
        auto metadata = utils::get_metadata(schema);
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
}