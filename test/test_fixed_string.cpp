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

#include <ios>
#include <ostream>
#include <string_view>
#include <type_traits>

#include "sparrow/utils/fixed_string.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    TEST_SUITE("fixed_string")
    {
        TEST_CASE("basic construction")
        {
            SUBCASE("empty string")
            {
                constexpr fixed_string<1> empty("");
                CHECK_EQ(empty.value[0], '\0');
                CHECK_EQ(std::string_view(empty), "");
            }

            SUBCASE("single character")
            {
                constexpr fixed_string<2> single("a");
                CHECK_EQ(single.value[0], 'a');
                CHECK_EQ(single.value[1], '\0');
                CHECK_EQ(std::string_view(single), "a");
            }

            SUBCASE("short string")
            {
                constexpr fixed_string<6> hello("hello");
                CHECK_EQ(std::string_view(hello), "hello");
            }

            SUBCASE("longer string")
            {
                constexpr fixed_string<27> alphabet("abcdefghijklmnopqrstuvwxyz");
                CHECK_EQ(std::string_view(alphabet), "abcdefghijklmnopqrstuvwxyz");
            }
        }

        TEST_CASE("constexpr evaluation")
        {
            SUBCASE("compile-time construction")
            {
                constexpr fixed_string<11> extension("arrow.json");
                static_assert(extension.value[0] == 'a');
                static_assert(extension.value[9] == 'n');
                static_assert(extension.value[10] == '\0');
            }
        }

        TEST_CASE("string_view conversion")
        {
            SUBCASE("implicit conversion")
            {
                constexpr fixed_string<8> name("sparrow");
                std::string_view sv = name;
                CHECK_EQ(sv, "sparrow");
                CHECK_EQ(sv.size(), 7);
            }

            SUBCASE("explicit conversion")
            {
                constexpr fixed_string<5> data("data");
                auto sv = static_cast<std::string_view>(data);
                CHECK_EQ(sv, "data");
            }

            SUBCASE("length excludes null terminator")
            {
                constexpr fixed_string<11> extension("arrow.uuid");
                std::string_view sv = extension;
                CHECK_EQ(sv.size(), 10);
                CHECK_EQ(sv.length(), 10);
            }
        }

        TEST_CASE("size calculation")
        {
            SUBCASE("N includes null terminator")
            {
                fixed_string<1> empty("");
                CHECK_EQ(sizeof(empty.value), 1);

                fixed_string<6> hello("hello");
                CHECK_EQ(sizeof(hello.value), 6);
                CHECK_EQ(std::string_view(hello).size(), 5);
            }
        }

        template <fixed_string Str>
        struct holder
        {
            static constexpr std::string_view value = Str;
        };

        TEST_CASE("as template parameter")
        {
            SUBCASE("can be used as non-type template parameter")
            {
                // This tests that fixed_string is a structural type
                // suitable for use as a non-type template parameter

                using test_type = holder<"test">;
                CHECK_EQ(test_type::value, "test");

                using extension_type = holder<"arrow.json">;
                CHECK_EQ(extension_type::value, "arrow.json");
            }

            SUBCASE("different strings create different types")
            {
                using holder1 = holder<"first">;
                using holder2 = holder<"second">;

                CHECK_EQ(holder1::value, "first");
                CHECK_EQ(holder2::value, "second");

                // Different strings produce different types
                static_assert(!std::is_same_v<holder1, holder2>);
            }
        }
    }
}
