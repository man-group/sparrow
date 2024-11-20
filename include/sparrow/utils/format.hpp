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

#include <algorithm>
#include <format>
#include <numeric>
#include <ranges>
#include <string>
#include <variant>
#include <vector>

#include "sparrow/utils/contracts.hpp"

namespace std
{
    template <class... T>
    struct formatter<std::variant<T...>>
    {
        constexpr auto parse(format_parse_context& ctx)
        {
            auto pos = ctx.begin();
            while (pos != ctx.end() && *pos != '}')
            {
                m_format_string.push_back(*pos);
                ++pos;
            }
            m_format_string.push_back('}');
            return pos;
        }

        auto format(const std::variant<T...>& v, std::format_context& ctx) const
        {
            return std::visit(
                [&ctx, this](const auto& value)
                {
                    return std::vformat_to(ctx.out(), m_format_string, std::make_format_args(value));
                },
                v
            );
        }

        std::string m_format_string = "{:";
    };
}

namespace sparrow
{
    template <typename R>
    concept RangeOfRanges = std::ranges::range<R> && std::ranges::range<std::ranges::range_value_t<R>>;

    template <typename T>
    concept Format = requires(const T& t) { std::format(t, 1); };

    template <typename T>
    concept RangeOfFormats = std::ranges::range<T> && Format<std::ranges::range_value_t<T>>;

    constexpr size_t max_width(const std::ranges::input_range auto& data)
    {
        size_t max_width = 0;
        for (const auto& value : data)
        {
            max_width = std::max(max_width, std::format("{}", value).size());
        }
        return max_width;
    }

    template <RangeOfRanges Columns>
    constexpr std::vector<size_t> columns_widths(const Columns& columns)
    {
        std::vector<size_t> widths;
        widths.reserve(std::ranges::size(columns));
        for (const auto& col : columns)
        {
            widths.push_back(max_width(col));
        }
        return widths;
    }

    template <typename OutputIt, std::ranges::input_range Widths, std::ranges::input_range Values>
        requires(std::same_as<std::ranges::range_value_t<Widths>, size_t>)
    constexpr void
    to_row(OutputIt out, const Widths& widths, const Values& values, std::string_view separator = "|")
    {
        SPARROW_ASSERT_TRUE(std::ranges::size(widths) == std::ranges::size(values))
        if (std::ranges::size(values) == 0)
        {
            return;
        }
        auto value_it = values.begin();
        auto width_it = widths.begin();
        for (size_t i = 0; i < std::ranges::size(values); ++i)
        {
            const std::string fmt = std::format("{}{{:>{}}}", separator, *width_it);
            const auto& value = *value_it;
            std::vformat_to(out, fmt, std::make_format_args(value));
            ++value_it;
            ++width_it;
        }
        std::format_to(out, "{}", separator);
    }

    template <typename OutputIt>
    constexpr void
    horizontal_separator(OutputIt out, const std::vector<size_t>& widths, std::string_view separator = "-")
    {
        if (std::ranges::size(widths) == 0)
        {
            return;
        }
        const size_t count = std::ranges::size(widths) + 1 + std::reduce(widths.begin(), widths.end());
        std::format_to(out, "{}", std::string(count, separator[0]));
    }

    template <std::ranges::input_range Headers, RangeOfRanges Columns, typename OutputIt>
        requires(std::convertible_to<std::ranges::range_value_t<Headers>, std::string>)
    constexpr void to_table_with_columns(OutputIt out, const Headers& headers, const Columns& columns)
    {
        const size_t column_count = std::ranges::size(columns);
        SPARROW_ASSERT_TRUE(std::ranges::size(headers) == column_count);
        if (column_count == 0)
        {
            return;
        }

        // columns lenght must be the same
        if (column_count > 0)
        {
            for (auto it = columns.begin() + 1; it != columns.end(); ++it)
            {
                SPARROW_ASSERT_TRUE(std::ranges::size(columns[0]) == std::ranges::size(*it));
            }
        }

        std::vector<size_t> widths = columns_widths(columns);

        // max with names
        for (size_t i = 0; i < std::ranges::size(headers); ++i)
        {
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wsign-conversion"
#endif
#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
            widths[i] = std::max(widths[i], static_cast<decltype(std::ranges::size(headers[i]) std::ranges::size(headers[i]));
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#    pragma clang diagnostic pop
#endif
        }
        to_row(out, widths, headers);
        std::format_to(out, "{}", '\n');
        horizontal_separator(out, widths);
        std::format_to(out, "{}", '\n');

        // print data
        for (size_t i = 0; i < std::ranges::size(columns[0]); ++i)
        {
            const auto row_range = std::views::transform(
                columns,
                [i](const auto& column)
                {
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wsign-conversion"
#endif
#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
                    return column[i];
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#    pragma clang diagnostic pop
#endif
                }
            );
            to_row(out, widths, row_range);
            std::format_to(out, "{}", '\n');
        }

        horizontal_separator(out, widths);
    }
}
