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
#include "sparrow_v01/utils/buffers.hpp"

namespace sparrow
{
    TEST_SUITE("number_of_bytes")
    {
        TEST_CASE("empty range")
        {
            constexpr std::array<std::string_view, 0> strings{};
            CHECK_EQ(number_of_bytes(strings), 0);
        }

        TEST_CASE("single element")
        {
            const std::array<std::string, 1> strings{"hello"};
            CHECK_EQ(number_of_bytes(strings), 5);
        }

        TEST_CASE("multiple elements")
        {
            const std::array<std::string, 3> strings{"hello", "world", "!"};
            CHECK_EQ(number_of_bytes(strings), 11);
        }

        TEST_CASE("empty string")
        {
            const std::array<std::string, 1> strings{""};
            CHECK_EQ(number_of_bytes(strings), 0);
        }

        TEST_CASE("empty strings")
        {
            const std::array<std::string, 3> strings{"", "", ""};
            CHECK_EQ(number_of_bytes(strings), 0);
        }

        TEST_CASE("empty and non-empty strings")
        {
            const std::array<std::string, 3> strings{"", "world", ""};
            CHECK_EQ(number_of_bytes(strings), 5);
        }
    }

    TEST_SUITE("strings_to_buffer")
    {
        TEST_CASE("const reference")
        {
            SUBCASE("empty range")
            {
                const std::array<std::string, 0> strings{};
                const buffer<uint8_t> buffer = strings_to_buffer(strings);
                CHECK_EQ(buffer.size(), 0);
            }

            SUBCASE("single element")
            {
                const std::array<std::string, 1> strings{"hello"};
                const buffer<uint8_t> buffer = strings_to_buffer(strings);
                REQUIRE_EQ(buffer.size(), 5);
                CHECK_EQ(buffer[0], 'h');
                CHECK_EQ(buffer[1], 'e');
                CHECK_EQ(buffer[2], 'l');
                CHECK_EQ(buffer[3], 'l');
                CHECK_EQ(buffer[4], 'o');
            }

            SUBCASE("multiple elements")
            {
                const std::array<std::string, 3> strings{"hello", "world", "!"};
                const buffer<uint8_t> buffer = strings_to_buffer(strings);
                REQUIRE_EQ(buffer.size(), 11);
                CHECK_EQ(buffer[0], 'h');
                CHECK_EQ(buffer[1], 'e');
                CHECK_EQ(buffer[2], 'l');
                CHECK_EQ(buffer[3], 'l');
                CHECK_EQ(buffer[4], 'o');
                CHECK_EQ(buffer[5], 'w');
                CHECK_EQ(buffer[6], 'o');
                CHECK_EQ(buffer[7], 'r');
                CHECK_EQ(buffer[8], 'l');
                CHECK_EQ(buffer[9], 'd');
                CHECK_EQ(buffer[10], '!');
            }

            SUBCASE("empty string")
            {
                const std::array<std::string, 1> strings{""};
                const buffer<uint8_t> buffer = strings_to_buffer(strings);
                CHECK_EQ(buffer.size(), 0);
            }

            SUBCASE("empty strings")
            {
                const std::array<std::string, 3> strings{"", "", ""};
                const buffer<uint8_t> buffer = strings_to_buffer(strings);
                CHECK_EQ(buffer.size(), 0);
            }
        }

        TEST_CASE("move")
        {
            SUBCASE("empty range")
            {
                std::array<std::string, 0> strings{};
                buffer<uint8_t> buffer = strings_to_buffer(std::move(strings));
                CHECK_EQ(buffer.size(), 0);
            }

            SUBCASE("single element")
            {
                std::array<std::string, 1> strings{"hello"};
                buffer<uint8_t> buffer = strings_to_buffer(std::move(strings));
                REQUIRE_EQ(buffer.size(), 5);
                CHECK_EQ(buffer[0], 'h');
                CHECK_EQ(buffer[1], 'e');
                CHECK_EQ(buffer[2], 'l');
                CHECK_EQ(buffer[3], 'l');
                CHECK_EQ(buffer[4], 'o');
            }

            SUBCASE("multiple elements")
            {
                std::array<std::string, 3> strings{"hello", "world", "!"};
                buffer<uint8_t> buffer = strings_to_buffer(std::move(strings));
                REQUIRE_EQ(buffer.size(), 11);
                CHECK_EQ(buffer[0], 'h');
                CHECK_EQ(buffer[1], 'e');
                CHECK_EQ(buffer[2], 'l');
                CHECK_EQ(buffer[3], 'l');
                CHECK_EQ(buffer[4], 'o');
                CHECK_EQ(buffer[5], 'w');
                CHECK_EQ(buffer[6], 'o');
                CHECK_EQ(buffer[7], 'r');
                CHECK_EQ(buffer[8], 'l');
                CHECK_EQ(buffer[9], 'd');
                CHECK_EQ(buffer[10], '!');
            }

            SUBCASE("empty string")
            {
                std::array<std::string, 1> strings{""};
                buffer<uint8_t> buffer = strings_to_buffer(std::move(strings));
                CHECK_EQ(buffer.size(), 0);
            }

            SUBCASE("empty strings")
            {
                std::array<std::string, 3> strings{"", "", ""};
                buffer<uint8_t> buffer = strings_to_buffer(std::move(strings));
                CHECK_EQ(buffer.size(), 0);
            }
        }
    }
}
