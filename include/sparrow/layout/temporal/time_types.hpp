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

#pragma once

#include <chrono>
#include <cstdint>

#if defined(__cpp_lib_format)
#    include <format>
#endif

namespace sparrow::chrono
{
    /**
     * A duration representing time elapsed since midnight.
     */
    struct time_seconds : public std::chrono::duration<int32_t>
    {
        time_seconds() = default;

        time_seconds(int32_t seconds)
            : std::chrono::duration<int32_t>(seconds)
        {
        }
    };

    /**
     * A duration representing time elapsed since midnight, in milliseconds.
     */
    struct time_milliseconds : public std::chrono::duration<int32_t, std::milli>
    {
        time_milliseconds() = default;

        time_milliseconds(int32_t milliseconds)
            : std::chrono::duration<int32_t, std::milli>(milliseconds)
        {
        }
    };

    /**
     * A duration representing time elapsed since midnight, in microseconds.
     */
    struct time_microseconds : public std::chrono::duration<int64_t, std::micro>
    {
        time_microseconds() = default;

        time_microseconds(int64_t microseconds)
            : std::chrono::duration<int64_t, std::micro>(microseconds)
        {
        }
    };

    /**
     * A duration representing time elapsed since midnight, in nanoseconds.
     */
    struct time_nanoseconds : public std::chrono::duration<int64_t, std::nano>
    {
        time_nanoseconds() = default;

        time_nanoseconds(int64_t nanoseconds)
            : std::chrono::duration<int64_t, std::nano>(nanoseconds)
        {
        }
    };
}

#if defined(__cpp_lib_format)

template <>
struct std::formatter<sparrow::chrono::time_seconds>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::chrono::time_seconds& time, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", time.count());
    }
};

template <>
struct std::formatter<sparrow::chrono::time_milliseconds>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::chrono::time_milliseconds& time, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", time.count());
    }
};

template <>
struct std::formatter<sparrow::chrono::time_microseconds>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::chrono::time_microseconds& time, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", time.count());
    }
};

template <>
struct std::formatter<sparrow::chrono::time_nanoseconds>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::chrono::time_nanoseconds& time, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", time.count());
    }
};
#endif
