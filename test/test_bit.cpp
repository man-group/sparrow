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

#include <cstdint>

#include "sparrow/utils/bit.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    TEST_SUITE("bit")
    {
        TEST_CASE("byteswap")
        {
            SUBCASE("uint8_t")
            {
                constexpr uint8_t x = 0x12u;
                CHECK_EQ(byteswap(x), 0x12u);
            }

            SUBCASE("uint16_t")
            {
                constexpr uint16_t x = 0x1234u;
                CHECK_EQ(byteswap(x), 0x3412u);
            }

            SUBCASE("uint32_t")
            {
                constexpr uint32_t x = 0x12345678u;
                CHECK_EQ(byteswap(x), 0x78563412u);
            }

            SUBCASE("uint64_t")
            {
                constexpr uint64_t x = 0x123456789abcdef0ull;
                CHECK_EQ(byteswap(x), 0xf0debc9a78563412ull);
            }
        }

        TEST_CASE("to_native_endian")
        {
            SUBCASE("uint8_t")
            {
                constexpr uint8_t x = 0x12u;
                CHECK_EQ(to_native_endian<std::endian::little>(x), 0x12u);
                CHECK_EQ(to_native_endian<std::endian::big>(x), 0x12u);
            }

            SUBCASE("uint16_t")
            {
                constexpr uint16_t x = 0x1234u;
                if constexpr (std::endian::native == std::endian::little)
                {
                    CHECK_EQ(to_native_endian<std::endian::little>(x), 0x1234u);
                    CHECK_EQ(to_native_endian<std::endian::big>(x), 0x3412u);
                }
                else
                {
                    CHECK_EQ(to_native_endian<std::endian::little>(x), 0x3412u);
                    CHECK_EQ(to_native_endian<std::endian::big>(x), 0x1234u);
                }
            }

            SUBCASE("uint32_t")
            {
                constexpr uint32_t x = 0x12345678u;
                if constexpr (std::endian::native == std::endian::little)
                {
                    CHECK_EQ(to_native_endian<std::endian::little>(x), 0x12345678u);
                    CHECK_EQ(to_native_endian<std::endian::big>(x), 0x78563412u);
                }
                else
                {
                    CHECK_EQ(to_native_endian<std::endian::little>(x), 0x78563412u);
                    CHECK_EQ(to_native_endian<std::endian::big>(x), 0x12345678u);
                }
            }

            SUBCASE("uint64_t")
            {
                constexpr uint64_t x = 0x123456789abcdef0ull;
                if constexpr (std::endian::native == std::endian::little)
                {
                    CHECK_EQ(to_native_endian<std::endian::little>(x), 0x123456789abcdef0ull);
                    CHECK_EQ(to_native_endian<std::endian::big>(x), 0xf0debc9a78563412ull);
                }
                else
                {
                    CHECK_EQ(to_native_endian<std::endian::little>(x), 0xf0debc9a78563412ull);
                    CHECK_EQ(to_native_endian<std::endian::big>(x), 0x123456789abcdef0ull);
                }
            }
        }
    }
}