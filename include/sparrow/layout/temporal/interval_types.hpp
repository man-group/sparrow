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
#if defined(__cpp_lib_format)
#    include <format>
#endif

#include "sparrow/layout/temporal/temporal_types.hpp"

namespace sparrow
{

// We pack the structures to ensure that they are the same size on all platforms
#pragma pack(push, 1)

    struct days_time_interval
    {
        chrono::days days;
        std::chrono::duration<int32_t, std::milli> time;
    };

#pragma pack(pop)

    inline bool operator==(const days_time_interval& lhs, const days_time_interval& rhs)
    {
        return lhs.days == rhs.days && lhs.time == rhs.time;
    }

#pragma pack(push, 1)

    struct month_day_nanoseconds_interval
    {
        chrono::months months;
        chrono::days days;
        std::chrono::duration<int64_t, std::nano> nanoseconds;
    };

#pragma pack(pop)

    inline bool operator==(const month_day_nanoseconds_interval& lhs, const month_day_nanoseconds_interval& rhs)
    {
        return lhs.months == rhs.months && lhs.days == rhs.days && lhs.nanoseconds == rhs.nanoseconds;
    }
}

namespace std
{
#if defined(__cpp_lib_format)
    template <>
    struct formatter<sparrow::days_time_interval>
    {
        constexpr auto parse(std::format_parse_context& ctx)
        {
            return ctx.begin();  // Simple implementation
        }

        auto format(const sparrow::days_time_interval& interval, std::format_context& ctx) const
        {
            std::ostringstream oss;
            oss << interval.days.count() << " days/" << interval.time.count() << " ms";
            const std::string interval_str = oss.str();
            return std::format_to(ctx.out(), "{}", interval_str);
        }
    };

    template <>
    struct formatter<sparrow::month_day_nanoseconds_interval>
    {
        constexpr auto parse(std::format_parse_context& ctx)
        {
            return ctx.begin();  // Simple implementation
        }

        auto format(const sparrow::month_day_nanoseconds_interval& interval, std::format_context& ctx) const
        {
            std::ostringstream oss;
            oss << interval.months.count() << " months/" << interval.days.count() << " days/"
                << interval.nanoseconds.count() << " ns";
            const std::string interval_str = oss.str();
            return std::format_to(ctx.out(), "{}", interval_str);
        }
    };
#endif
}
