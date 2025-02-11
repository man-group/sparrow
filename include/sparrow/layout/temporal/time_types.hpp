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

        explicit time_seconds(int32_t seconds)
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

        explicit time_milliseconds(int32_t milliseconds)
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

        explicit time_microseconds(int64_t microseconds)
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

        explicit time_nanoseconds(int64_t nanoseconds)
            : std::chrono::duration<int64_t, std::nano>(nanoseconds)
        {
        }
    };
}

#if defined(__cpp_lib_format)

template <typename T>
    requires std::same_as<T, sparrow::chrono::time_seconds>
             || std::same_as<T, sparrow::chrono::time_milliseconds>
             || std::same_as<T, sparrow::chrono::time_microseconds>
             || std::same_as<T, sparrow::chrono::time_nanoseconds>
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
