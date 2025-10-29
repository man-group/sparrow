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

#include "sparrow/primitive_array.hpp"
#include "sparrow/utils/extension.hpp"

namespace sparrow
{
    /**
     * @brief Bool8 array using 8-bit storage for boolean values.
     *
     * Bool8 represents a boolean value using 1 byte (8 bits) to store each value
     * instead of only 1 bit as in the original Arrow Boolean type. Although less
     * compact than the original representation, Bool8 may have better zero-copy
     * compatibility with various systems that also store booleans using 1 byte.
     *
     * The Bool8 extension type is defined as:
     * - Extension name: "arrow.bool8"
     * - Storage type: Int8
     * - false is denoted by the value 0
     * - true can be specified using any non-zero value (preferably 1)
     * - Extension metadata: empty string
     *
     */
    using bool8_array = primitive_array<int8_t, simple_extension<"arrow.bool8">, bool>;

    
}

#if defined(__cpp_lib_format)
#    include <format>


// Formatter specialization for bool8_array
template <>
struct std::formatter<sparrow::bool8_array>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const sparrow::bool8_array& ar, std::format_context& ctx) const
    {
        std::format_to(ctx.out(), "Bool8 array [{}]: [", ar.size());
        for (std::size_t i = 0; i < ar.size(); ++i)
        {
            if (i > 0)
            {
                std::format_to(ctx.out(), ", ");
            }
            const auto elem = ar[i];
            if (elem.has_value())
            {
                std::format_to(ctx.out(), "{}", static_cast<bool>(elem.value()) ? "true" : "false");
            }
            else
            {
                std::format_to(ctx.out(), "null");
            }
        }
        return std::format_to(ctx.out(), "]");
    }
};

namespace sparrow
{
    inline std::ostream& operator<<(std::ostream& os, const bool8_array& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

#endif

