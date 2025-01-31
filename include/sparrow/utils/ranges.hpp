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

#include <algorithm>
#include <ranges>
#include <type_traits>

#if defined(__cpp_lib_format)
#    include <format>
#    include <ostream>
#endif

#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    template <std::ranges::input_range R>
        requires(std::ranges::sized_range<R>)
    std::size_t range_size(R&& r)
    {
        return static_cast<std::size_t>(std::ranges::size(r));
    }

    template <std::ranges::input_range R>
        requires(!std::ranges::sized_range<R>)
    std::size_t range_size(R&& r)
    {
        return static_cast<std::size_t>(std::ranges::distance(r));
    }

    template <std::ranges::range Range>
        requires std::ranges::sized_range<std::ranges::range_value_t<Range>>
    constexpr bool all_same_size(const Range& range)
    {
        // Optimization for std::array and fixed-size std::span
        if constexpr (mpl::std_array<std::ranges::range_value_t<Range>>
                      || mpl::fixed_size_span<std::ranges::range_value_t<Range>>)
        {
            return true;
        }
        else
        {
            if (std::ranges::empty(range))
            {
                return true;
            }

            const std::size_t first_size = range.front().size();
            return std::ranges::all_of(
                range,
                [first_size](const auto& element)
                {
                    return element.size() == first_size;
                }
            );
        }
    }
};

#if defined(__cpp_lib_format) && !defined(__cpp_lib_format_ranges)

template <typename T, std::size_t N>
struct std::formatter<std::array<T, N>>
{
    // Parsing format specifiers
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const std::array<T, N>& array, std::format_context& ctx) const
    {
        auto out = ctx.out();
        *out++ = '<';

        bool first = true;
        for (const auto& elem : array)
        {
            if (!first)
            {
                *out++ = ',';
                *out++ = ' ';
            }
            out = std::format_to(out, "{}", elem);
            first = false;
        }

        *out++ = '>';
        return out;
    }
};
#endif