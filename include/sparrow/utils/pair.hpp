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

#include <utility>

#if defined(__cpp_lib_format)
#    include <format>
#    include <ostream>
#endif

#if defined(__cpp_lib_format) && __cplusplus < 202300L

template <typename T, typename U>
struct std::formatter<std::pair<T, U>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const std::pair<T, U>& p, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "({}, {})", p.first, p.second);
    }
};

namespace sparrow
{
    template <typename T, typename U>
    std::ostream& operator<<(std::ostream& os, const std::pair<T, U>& value)
    {
        os << std::format("{}", value);
        return os;
    }
}
#endif