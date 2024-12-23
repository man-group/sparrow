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

#if defined(__cpp_lib_format)
#    include <format>
#endif

#ifndef SPARROW_USE_LARGE_INT_PLACEHOLDERS

// disabe warnings -Wold-style-cast sign-conversion for clang and gcc
#    if defined(__clang__) || defined(__GNUC__)
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wold-style-cast"
#        pragma GCC diagnostic ignored "-Wsign-conversion"
#        pragma GCC diagnostic ignored "-Wshadow"
#        pragma GCC diagnostic ignored "-Wsign-conversion"
#    endif
#    include <sparrow/details/3rdparty/large_integers/int128_t.hpp>
#    include <sparrow/details/3rdparty/large_integers/int256_t.hpp>

#    if defined(__clang__) || defined(__GNUC__)
#        pragma GCC diagnostic pop
#    endif

#endif

#include <cstdint>

namespace sparrow
{
#ifdef SPARROW_USE_LARGE_INT_PLACEHOLDERS
    constexpr bool large_int_placeholders = true;

    struct int128_t
    {
        int128_t() = default;

        std::uint64_t words[2];

        bool operator==(const int128_t& other) const
        {
            return words[0] == other.words[0] && words[1] == other.words[1];
        }

        bool operator!=(const int128_t& other) const
        {
            return !(*this == other);
        }
    };

    struct int256_t
    {
        int256_t() = default;
        std::uint64_t words[4];

        bool operator==(const int256_t& other) const
        {
            return words[0] == other.words[0] && words[1] == other.words[1] && words[2] == other.words[2]
                   && words[3] == other.words[3];
        }

        bool operator!=(const int256_t& other) const
        {
            return !(*this == other);
        }
    };
    template <class T>
    constexpr bool is_int_placeholder_v = std::is_same_v<T, int128_t> || std::is_same_v<T, int256_t>;

#else

    template <class T>
    constexpr bool is_int_placeholder_v = false;
    constexpr bool large_int_placeholders = false;
    using int128_t = primesum::int128_t;
    using int256_t = primesum::int256_t;

    template <class T>
        requires(std::is_same_v<T, int128_t> || std::is_same_v<T, int256_t>)
    inline std::ostream& operator<<(std::ostream& stream, T n)
    {
        std::string str;

        if (n < 0)
        {
            stream << "-";
            n = -n;
        }
        while (n > 0)
        {
            str.push_back(static_cast<char>('0' + std::int8_t(n % 10)));
            n /= 10;
        }

        if (str.empty())
        {
            str = "0";
        }

        stream << std::string(str.rbegin(), str.rend());

        return stream;
    }
#endif
}  // namespace sparrow

#if defined(__cpp_lib_format)

template <>
struct std::formatter<sparrow::int128_t>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::int128_t&, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", "Decimal int128_t TODO");
    }
};

template <>
struct std::formatter<sparrow::int256_t>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::int256_t&, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", "Decimal int256_t TODO");
    }
};

#endif
