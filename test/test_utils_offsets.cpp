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

#include <array>

#include "doctest/doctest.h"
#include "sparrow/utils/offsets.hpp"

namespace sparrow
{
    TEST_SUITE("make_offset_buffer")
    {
        TEST_CASE("empty range")
        {
            std::array<std::string_view, 0> strings{};
            const auto offsets = make_offset_buffer<int32_t>(strings);
            REQUIRE_EQ(offsets.size(), 1);
            CHECK_EQ(offsets[0], 0);
        }

        TEST_CASE("single element")
        {
            std::array<std::string, 1> strings{"hello"};
            const auto offsets = make_offset_buffer<int32_t>(strings);
            REQUIRE_EQ(offsets.size(), 2);
            CHECK_EQ(offsets[0], 0);
            CHECK_EQ(offsets[1], 5);
        }

        TEST_CASE("multiple elements")
        {
            std::array<std::string, 3> strings{"hello", "world", "!"};
            const auto offsets = make_offset_buffer<int32_t>(strings);
            REQUIRE_EQ(offsets.size(), 4);
            CHECK_EQ(offsets[0], 0);
            CHECK_EQ(offsets[1], 5);
            CHECK_EQ(offsets[2], 10);
            CHECK_EQ(offsets[3], 11);
        }

        TEST_CASE("very long string")
        {
            std::array<std::string, 1> strings;
            strings[0] = std::string(9999ULL, 'p');
            const auto offsets = make_offset_buffer<int32_t>(strings);
            REQUIRE_EQ(offsets.size(), 2);
            CHECK_EQ(offsets[0], 0);
            CHECK_EQ(offsets[1], 9999);
        }
    }
}
