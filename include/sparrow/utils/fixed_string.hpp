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
#include <cstddef>
#include <string_view>

namespace sparrow
{
    // Helper template to pass string literals as template parameters
    template <std::size_t N>
    struct fixed_string
    {
        constexpr fixed_string(const char (&str)[N])
        {
            std::copy_n(str, N, value);
        }

        constexpr operator std::string_view() const
        {
            return std::string_view(value, N - 1);
        }

        char value[N];
    };
}