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
#include "sparrow/layout/temporal/interval_array.hpp"
#include "sparrow/layout/temporal/timestamp_array.hpp"
#include "sparrow/layout/temporal/timestamp_without_timezone_array.hpp"

namespace sparrow::c_data_integration
{
    template <typename T, typename DATA_RANGE>
    sparrow::array get_array(
        const nlohmann::json& array,
        const nlohmann::json& schema,
        DATA_RANGE&& data,
        std::string name,
        std::optional<std::vector<sparrow::metadata_pair>>&& metadata
    )
    {
        const bool nullable = schema.at("nullable").get<bool>();
        if (nullable)
        {
            auto validity = utils::get_validity(array);
            return sparrow::array{
                T{std::forward<DATA_RANGE>(data), std::move(validity), name, std::move(metadata)}
            };
        }
        else
        {
            return sparrow::array{T{std::forward<DATA_RANGE>(data), false, name, std::move(metadata)}};
        }
    }

    template <typename T, typename DATA_RANGE>
    sparrow::array get_array(
        const date::time_zone* tz,
        const nlohmann::json& array,
        const nlohmann::json& schema,
        DATA_RANGE&& data,
        std::string name,
        std::optional<std::vector<sparrow::metadata_pair>>&& metadata
    )
    {
        const bool nullable = schema.at("nullable").get<bool>();
        if (nullable)
        {
            auto validity = utils::get_validity(array);
            return sparrow::array{
                T{tz, std::forward<DATA_RANGE>(data), std::move(validity), name, std::move(metadata)}
            };
        }
        else
        {
            return sparrow::array{T{tz, std::forward<DATA_RANGE>(data), false, name, std::move(metadata)}};
        }
    }

    sparrow::array
    date_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json&)
    {
        utils::check_type(schema, "date");
        const std::string name = schema.at("name").get<std::string>();
        const std::string unit = schema.at("type").at("unit").get<std::string>();
        auto metadata = utils::get_metadata(schema);
        if (unit == "DAY")
        {
            auto date_days_values = std::ranges::views::all(array.at(DATA).get<std::vector<int32_t>>())
                                    | std::views::transform(
                                        [](int32_t value)
                                        {
                                            return sparrow::date_days{sparrow::date_days::duration{value}};
                                        }
                                    );
            return get_array<sparrow::date_days_array>(array, schema, date_days_values, name, std::move(metadata));
        }
        else if (unit == "MILLISECOND")
        {
            auto data = std::ranges::views::all(array.at(DATA).get<std::vector<std::string>>())
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
            return get_array<sparrow::date_milliseconds_array>(array, schema, data, name, std::move(metadata));
        }
        else
        {
            throw std::runtime_error("Invalid unit: " + unit);
        }
    }

    sparrow::array
    time_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json&)
    {
        utils::check_type(schema, "time");
        const std::string name = schema.at("name").get<std::string>();
        const std::string unit = schema.at("type").at("unit").get<std::string>();
        auto metadata = utils::get_metadata(schema);
        if (unit == "SECOND")
        {
            auto values = std::ranges::views::all(array.at(DATA).get<std::vector<int32_t>>())
                          | std::views::transform(
                              [](int32_t value)
                              {
                                  return sparrow::chrono::time_seconds{value};
                              }
                          );
            return get_array<sparrow::time_seconds_array>(array, schema, values, name, std::move(metadata));
        }

        else if (unit == "MILLISECOND")
        {
            auto values = std::ranges::views::all(array.at(DATA).get<std::vector<int32_t>>())
                          | std::views::transform(
                              [](int32_t value)
                              {
                                  return sparrow::chrono::time_milliseconds{value};
                              }
                          );
            return get_array<sparrow::time_milliseconds_array>(array, schema, values, name, std::move(metadata));
        }
        else if (unit == "MICROSECOND")
        {
            auto values_str = array.at(DATA).get<std::vector<std::string>>();
            auto values = utils::from_strings_to_Is<int64_t>(values_str)
                          | std::views::transform(
                              [](int64_t value)
                              {
                                  return sparrow::chrono::time_microseconds{value};
                              }
                          );
            return get_array<sparrow::time_microseconds_array>(array, schema, values, name, std::move(metadata));
        }
        else if (unit == "NANOSECOND")
        {
            auto values_str = array.at(DATA).get<std::vector<std::string>>();
            auto values = utils::from_strings_to_Is<int64_t>(values_str)
                          | std::views::transform(
                              [](int64_t value)
                              {
                                  return sparrow::chrono::time_nanoseconds{value};
                              }
                          );
            return get_array<sparrow::time_nanoseconds_array>(array, schema, values, name, std::move(metadata));
        }
        else
        {
            throw std::runtime_error("Invalid unit: " + unit);
        }
    }

    sparrow::array
    timestamp_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json&)
    {
        utils::check_type(schema, "timestamp");
        const std::string name = schema.at("name").get<std::string>();
        const std::string unit = schema.at("type").at("unit").get<std::string>();
        std::optional<std::string> timezone;
        if (schema.at("type").contains("timezone"))
        {
            timezone = schema.at("type").at("timezone").get<std::string>();
        }

        const auto& data_str = array.at(DATA).get<std::vector<std::string>>();
        auto data_int = utils::from_strings_to_Is<int64_t>(data_str);
        auto metadata = utils::get_metadata(schema);
        if (unit == "SECOND")
        {
            if (timezone.has_value())
            {
                const date::time_zone* tz = date::locate_zone(*timezone);
                auto values = data_int
                              | std::views::transform(
                                  [tz](int64_t value)
                                  {
                                      using duration = sparrow::timestamp_second::duration;
                                      const date::sys_time<duration> sys_time{duration{value}};
                                      return sparrow::timestamp_second{tz, sys_time};
                                  }
                              );
                return get_array<sparrow::timestamp_seconds_array>(
                    tz,
                    array,
                    schema,
                    values,
                    name,
                    std::move(metadata)
                );
            }
            else
            {
                auto values = data_int
                              | std::views::transform(
                                  [](int64_t value)
                                  {
                                      return zoned_time_without_timezone_seconds{value};
                                  }
                              );

                return get_array<sparrow::timestamp_without_timezone_seconds_array>(
                    array,
                    schema,
                    values,
                    name,
                    std::move(metadata)
                );
            }
        }
        else if (unit == "MILLISECOND")
        {
            if (timezone.has_value())
            {
                const date::time_zone* tz = date::locate_zone(*timezone);
                auto values = data_int
                              | std::views::transform(
                                  [tz](int64_t value)
                                  {
                                      using duration = sparrow::timestamp_millisecond::duration;
                                      const date::sys_time<duration> sys_time{duration{value}};
                                      return sparrow::timestamp_millisecond{tz, sys_time};
                                  }
                              );
                return get_array<sparrow::timestamp_milliseconds_array>(
                    tz,
                    array,
                    schema,
                    values,
                    name,
                    std::move(metadata)
                );
            }
            else
            {
                auto values = data_int
                              | std::views::transform(
                                  [](int64_t value)
                                  {
                                      return zoned_time_without_timezone_milliseconds{value};
                                  }
                              );
                return get_array<sparrow::timestamp_without_timezone_milliseconds_array>(
                    array,
                    schema,
                    values,
                    name,
                    std::move(metadata)
                );
            }
        }
        else if (unit == "MICROSECOND")
        {
            if (timezone.has_value())
            {
                const date::time_zone* tz = date::locate_zone(*timezone);
                auto values = data_int
                              | std::views::transform(
                                  [tz](int64_t value)
                                  {
                                      using duration = sparrow::timestamp_microsecond::duration;
                                      const date::sys_time<duration> sys_time{duration{value}};
                                      return sparrow::timestamp_microsecond{tz, sys_time};
                                  }
                              );
                return get_array<sparrow::timestamp_microseconds_array>(
                    tz,
                    array,
                    schema,
                    values,
                    name,
                    std::move(metadata)
                );
            }
            else
            {
                auto values = data_int
                              | std::views::transform(
                                  [](int64_t value)
                                  {
                                      return zoned_time_without_timezone_microseconds{value};
                                  }
                              );
                return get_array<sparrow::timestamp_without_timezone_microseconds_array>(
                    array,
                    schema,
                    values,
                    name,
                    std::move(metadata)
                );
            }
        }
        else if (unit == "NANOSECOND")
        {
            if (timezone.has_value())
            {
                const date::time_zone* tz = date::locate_zone(*timezone);
                auto values = data_int
                              | std::views::transform(
                                  [tz](int64_t value)
                                  {
                                      using duration = sparrow::timestamp_nanosecond::duration;
                                      const date::sys_time<duration> sys_time{duration{value}};
                                      return sparrow::timestamp_nanosecond{tz, sys_time};
                                  }
                              );
                return get_array<sparrow::timestamp_nanoseconds_array>(
                    tz,
                    array,
                    schema,
                    values,
                    name,
                    std::move(metadata)
                );
            }
            else
            {
                auto values = data_int
                              | std::views::transform(
                                  [](int64_t value)
                                  {
                                      return zoned_time_without_timezone_nanoseconds{value};
                                  }
                              );
                return get_array<sparrow::timestamp_without_timezone_nanoseconds_array>(
                    array,
                    schema,
                    values,
                    name,
                    std::move(metadata)
                );
            }
        }
        else
        {
            throw std::runtime_error("Invalid unit: " + unit);
        }
    }

    sparrow::array
    duration_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json&)
    {
        utils::check_type(schema, "duration");
        const std::string name = schema.at("name").get<std::string>();
        const std::string unit = schema.at("type").at("unit").get<std::string>();
        auto data_str = array.at(DATA).get<std::vector<std::string>>();
        auto data = utils::from_strings_to_Is<int64_t>(data_str);
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
            return get_array<sparrow::duration_seconds_array>(array, schema, values, name, std::move(metadata));
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
            return get_array<sparrow::duration_milliseconds_array>(array, schema, values, name, std::move(metadata));
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
            return get_array<sparrow::duration_microseconds_array>(array, schema, values, name, std::move(metadata));
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
            return get_array<sparrow::duration_nanoseconds_array>(array, schema, values, name, std::move(metadata));
        }
        else
        {
            throw std::runtime_error("Invalid unit: " + unit);
        }
    }

    sparrow::array
    interval_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json&)
    {
        utils::check_type(schema, "interval");
        const std::string name = schema.at("name").get<std::string>();
        const std::string unit = schema.at("type").at("unit").get<std::string>();
        auto metadata = utils::get_metadata(schema);

        if (unit == "YEAR_MONTH")
        {
            auto data = array.at(DATA).get<std::vector<int32_t>>();
            auto values = data
                          | std::views::transform(
                              [](int64_t value)
                              {
                                  return std::chrono::months{value};
                              }
                          );
            return get_array<sparrow::months_interval_array>(array, schema, values, name, std::move(metadata));
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
            return get_array<sparrow::days_time_interval_array>(array, schema, values, name, std::move(metadata));
        }
        else if (unit == "MONTH_DAY_NANO")
        {
            auto values = array.at(DATA)
                          | std::views::transform(
                              [](const nlohmann::json& value) -> sparrow::month_day_nanoseconds_interval
                              {
                                  return sparrow::month_day_nanoseconds_interval{
                                      .months = std::chrono::months{value.at("months").get<int32_t>()},
                                      .days = std::chrono::days{value.at("days").get<int32_t>()},
                                      .nanoseconds = std::chrono::nanoseconds{value.at("nanoseconds")
                                                                                  .get<int64_t>()}
                                  };
                              }
                          );
            return get_array<sparrow::month_day_nanoseconds_interval_array>(
                array,
                schema,
                values,
                name,
                std::move(metadata)
            );
        }
        else
        {
            throw std::runtime_error("Invalid unit: " + unit);
        }
    }
}
