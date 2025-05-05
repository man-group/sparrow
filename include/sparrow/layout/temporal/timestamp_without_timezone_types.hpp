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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or mplied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <chrono>

namespace sparrow
{
    /**
     * A zoned time value without timezone, in seconds.
     */
    struct zoned_time_without_timezone_seconds : public std::chrono::duration<int32_t>
    {
        zoned_time_without_timezone_seconds() = default;

        explicit zoned_time_without_timezone_seconds(int32_t seconds)
            : std::chrono::duration<int32_t>(seconds)
        {
        }
    };

    /**
     * A zoned time value without timezone, in milliseconds.
     */
    struct zoned_time_without_timezone_milliseconds : public std::chrono::duration<int32_t, std::milli>
    {
        zoned_time_without_timezone_milliseconds() = default;

        explicit zoned_time_without_timezone_milliseconds(int32_t milliseconds)
            : std::chrono::duration<int32_t, std::milli>(milliseconds)
        {
        }
    };

    /**
     * A zoned time value without timezone, in microseconds.
     */
    struct zoned_time_without_timezone_microseconds : public std::chrono::duration<int64_t, std::micro>
    {
        zoned_time_without_timezone_microseconds() = default;

        explicit zoned_time_without_timezone_microseconds(int64_t microseconds)
            : std::chrono::duration<int64_t, std::micro>(microseconds)
        {
        }
    };

    /**
     * A zoned time value without timezone, in nanoseconds.
     */
    struct zoned_time_without_timezone_nanoseconds : public std::chrono::duration<int64_t, std::nano>
    {
        zoned_time_without_timezone_nanoseconds() = default;

        explicit zoned_time_without_timezone_nanoseconds(int64_t nanoseconds)
            : std::chrono::duration<int64_t, std::nano>(nanoseconds)
        {
        }
    };
}


#if defined(__cpp_lib_format)

#    include <format>

template <typename T>
    requires std::same_as<T, sparrow::zoned_time_without_timezone_seconds>
             || std::same_as<T, sparrow::zoned_time_without_timezone_milliseconds>
             || std::same_as<T, sparrow::zoned_time_without_timezone_microseconds>
             || std::same_as<T, sparrow::zoned_time_without_timezone_nanoseconds>
struct std::formatter<T>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const T& time, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", time.count());
    }
};

#endif
