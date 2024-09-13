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
#include <cstdint>

#include "sparrow/buffer/dynamic_bitset.hpp"

#include "doctest/doctest.h"
#include "sparrow_v01/utils/bitmap_offset.hpp"

namespace sparrow
{
    constexpr std::array<uint8_t, 4> nulls{0, 1, 5, 12};

    sparrow::dynamic_bitset<uint8_t> create_bitset()
    {
        sparrow::dynamic_bitset<uint8_t> bitset{15, true};
        for (const auto null : nulls)
        {
            bitset.set(null, false);
        }
        return bitset;
    }

    TEST_SUITE("Bitmap with offset")
    {
        TEST_CASE("constructor")
        {
            SUBCASE("move")
            {
                auto bitset = create_bitset();
                CHECK_NOTHROW(bitmap_offset bitmap{std::move(bitset), 2});
            }

            SUBCASE("reference")
            {
                auto bitset = create_bitset();
                CHECK_NOTHROW(bitmap_offset bitmap{bitset, 2});
            }
        }

        TEST_CASE("size")
        {
            const auto bitset = create_bitset();
            bitmap_offset bitmap{bitset, 2};
            CHECK_EQ(bitmap.size(), 13);
        }

        TEST_CASE("empty")
        {
            const auto bitset = create_bitset();
            bitmap_offset bitmap{bitset, 2};
            CHECK_FALSE(bitmap.empty());
        }

        TEST_CASE("null_count")
        {
            const auto bitset = create_bitset();
            bitmap_offset bitmap{bitset, 2};
            CHECK_EQ(bitmap.null_count(), 2);
        }

        TEST_CASE("test")
        {
            const auto bitset = create_bitset();
            bitmap_offset bitmap{bitset, 2};
            CHECK(bitmap.test(0));
            CHECK(bitmap.test(1));
            CHECK(bitmap.test(2));
            CHECK_FALSE(bitmap.test(3));
            CHECK(bitmap.test(4));
            CHECK(bitmap.test(5));
            CHECK(bitmap.test(6));
            CHECK(bitmap.test(7));
            CHECK(bitmap.test(8));
            CHECK(bitmap.test(9));
            CHECK_FALSE(bitmap.test(10));
            CHECK(bitmap.test(11));
            CHECK(bitmap.test(12));
        }

        TEST_CASE("at")
        {
            SUBCASE("mutable")
            {
                auto bitset = create_bitset();
                bitmap_offset bitmap{bitset, 2};
                CHECK(bitmap.at(0));
                CHECK(bitmap.at(1));
                CHECK(bitmap.at(2));
                CHECK_FALSE(bitmap.at(3));
                CHECK(bitmap.at(4));
                CHECK(bitmap.at(5));
                CHECK(bitmap.at(6));
                CHECK(bitmap.at(7));
                CHECK(bitmap.at(8));
                CHECK(bitmap.at(9));
                CHECK_FALSE(bitmap.at(10));
                CHECK(bitmap.at(11));
                CHECK(bitmap.at(12));
            }

            SUBCASE("const")
            {
                const auto bitset = create_bitset();
                const bitmap_offset bitmap{bitset, 2};
                CHECK(bitmap.at(0));
                CHECK(bitmap.at(1));
                CHECK(bitmap.at(2));
                CHECK_FALSE(bitmap.at(3));
                CHECK(bitmap.at(4));
                CHECK(bitmap.at(5));
                CHECK(bitmap.at(6));
                CHECK(bitmap.at(7));
                CHECK(bitmap.at(8));
                CHECK(bitmap.at(9));
                CHECK_FALSE(bitmap.at(10));
                CHECK(bitmap.at(11));
                CHECK(bitmap.at(12));
            }
        }

        TEST_CASE("operator[]")
        {
            SUBCASE("mutable")
            {
                auto bitset = create_bitset();
                bitmap_offset bitmap{bitset, 2};
                CHECK(bitmap[0]);
                CHECK(bitmap[1]);
                CHECK(bitmap[2]);
                CHECK_FALSE(bitmap[3]);
                CHECK(bitmap[4]);
                CHECK(bitmap[5]);
                CHECK(bitmap[6]);
                CHECK(bitmap[7]);
                CHECK(bitmap[8]);
                CHECK(bitmap[9]);
                CHECK_FALSE(bitmap[10]);
                CHECK(bitmap[11]);
                CHECK(bitmap[12]);
            }

            SUBCASE("const")
            {
                const auto bitset = create_bitset();
                const bitmap_offset bitmap{bitset, 2};
                CHECK(bitmap[0]);
                CHECK(bitmap[1]);
                CHECK(bitmap[2]);
                CHECK_FALSE(bitmap[3]);
                CHECK(bitmap[4]);
                CHECK(bitmap[5]);
                CHECK(bitmap[6]);
                CHECK(bitmap[7]);
                CHECK(bitmap[8]);
                CHECK(bitmap[9]);
                CHECK_FALSE(bitmap[10]);
                CHECK(bitmap[11]);
                CHECK(bitmap[12]);
            }
        }

        // TEST_CASE("begin")
        // {
        //     const auto bitset = create_bitset();
        //     bitmap_offset bitmap{bitset, 2};
        //     auto citer = bitmap.begin();
        //     CHECK(*citer);
        //     citer++;
        //     CHECK(*citer);
        //     citer++;
        //     CHECK_FALSE(*citer);
        //     citer++;
        //     CHECK(*citer);
        //     citer++;
        //     CHECK_FALSE(*citer);
        //     citer++;
        //     CHECK(*citer);
        //     citer++;
        // }
    }
}
