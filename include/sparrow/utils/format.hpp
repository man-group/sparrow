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
#include <cstddef>
#include <cstdint>
#include <format>
#include <numeric>
#include <ranges>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "sparrow/utils/contracts.hpp"

namespace sparrow::detail
{
    struct sequence_format_spec
    {
        char fill = ' ';
        char align = '>';  // '<', '>', '^'
        std::size_t width = 0;

        // Parse:  [[fill]align] [width]
        // Grammar subset:  (fill? align?) width?
        template <class It>
        constexpr It parse(It it, It end)
        {
            if (it == end || *it == '}')
            {
                return it;
            }

            // Detect [fill][align] or [align]
            auto next = it;
            if (next != end)
            {
                ++next;
                if (next != end && (*next == '<' || *next == '>' || *next == '^') && *it != '<' && *it != '>'
                    && *it != '^')
                {
                    fill = *it;
                    align = *next;
                    it = ++next;
                }
                else if (*it == '<' || *it == '>' || *it == '^')
                {
                    align = *it;
                    ++it;
                }
            }

            // Parse width
            std::size_t w = 0;
            bool has_w = false;
            while (it != end && *it >= '0' && *it <= '9')
            {
                has_w = true;
                w = w * 10 + static_cast<unsigned>(*it - '0');
                ++it;
            }
            if (has_w)
            {
                width = w;
            }

            // Ignore (silently) everything until '}' (keeps constexpr friendliness)
            while (it != end && *it != '}')
            {
                ++it;
            }

            return it;
        }

        std::string apply_alignment(std::string inner) const
        {
            if (width <= inner.size())
            {
                return inner;
            }

            const std::size_t pad = width - inner.size();
            switch (align)
            {
                case '<':
                    return inner + std::string(pad, fill);
                case '^':
                {
                    std::size_t left = pad / 2;
                    std::size_t right = pad - left;
                    return std::string(left, fill) + inner + std::string(right, fill);
                }
                case '>':
                default:
                    return std::string(pad, fill) + inner;
            }
        }

        template <class Seq>
        std::string build_core(const Seq& seq) const
        {
            std::string core;
            core.push_back('<');
            bool first = true;
            for (auto&& elem : seq)
            {
                if (!first)
                {
                    core.append(", ");
                }
                std::format_to(std::back_inserter(core), "{}", elem);
                first = false;
            }
            core.push_back('>');
            return core;
        }
    };
}  // namespace sparrow::detail

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

    template <>
    struct formatter<std::byte>
    {
        constexpr auto parse(format_parse_context& ctx)
        {
            return m_underlying_formatter.parse(ctx);
        }

        auto format(std::byte b, std::format_context& ctx) const
        {
            return std::format_to(ctx.out(), "{:#04x}", std::to_integer<unsigned>(b));
        }

    private:

        // Store the parsed format specification
        std::formatter<unsigned> m_underlying_formatter;
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

    constexpr size_t size_of_utf8(const std::string_view str)
    {
        size_t size = 0;
        for (const char c : str)
        {
            if ((c & 0xC0) != 0x80)
            {
                ++size;
            }
        }
        return size;
    }

    constexpr size_t max_width(const std::ranges::input_range auto& data)
    {
        size_t max_width = 0;
        for (const auto& value : data)
        {
            if constexpr (std::is_same_v<std::remove_cvref_t<std::decay_t<decltype(value)>>, std::string>
                          || std::is_same_v<std::remove_cvref_t<std::decay_t<decltype(value)>>, std::string_view>
                          || std::is_same_v<std::remove_cvref_t<std::decay_t<decltype(value)>>, const char*>
                          || std::is_same_v<std::remove_cvref_t<std::decay_t<decltype(value)>>, char*>)
            {
                max_width = std::max(max_width, size_of_utf8(value));
            }
            else
            {
                max_width = std::max(max_width, std::format("{}", value).size());
            }
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

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wsign-conversion"
#endif
#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
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
            widths[i] = std::max(widths[i], std::ranges::size(headers[i]));
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
                    return column[i];
                }
            );
            to_row(out, widths, row_range);
            std::format_to(out, "{}", '\n');
        }

        horizontal_separator(out, widths);
    }
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#    pragma clang diagnostic pop
#endif
}
