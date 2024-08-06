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

#include <numeric>

#include "sparrow/array/array_data_factory.hpp"
#include "sparrow/layout/null_layout.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    TEST_SUITE("null layout")
    {
        TEST_CASE("constructors")
        {
            SUBCASE("with array data")
            {
                constexpr std::size_t size = 5u;
                array_data ad = make_array_data_for_null_layout(size);
                const null_layout nl(ad);
                CHECK_EQ(nl.size(), size);
            }

            SUBCASE("copy")
            {
                constexpr std::size_t size = 5u;
                array_data ad = make_array_data_for_null_layout(size);
                null_layout nl(ad);
                const null_layout nl_copy(nl);
                CHECK_EQ(nl_copy.size(), size);
            }

            SUBCASE("move")
            {
                constexpr std::size_t size = 5u;
                array_data ad = make_array_data_for_null_layout(size);
                null_layout nl(ad);
                const null_layout nl_move(std::move(nl));
                CHECK_EQ(nl_move.size(), size);
            }
        }

        TEST_CASE("operator=")
        {
            SUBCASE("copy")
            {
                constexpr std::size_t size = 5u;
                array_data ad = make_array_data_for_null_layout(size);
                null_layout nl(ad);
                const null_layout nl_copy = nl;
                CHECK_EQ(nl_copy.size(), size);
            }

            SUBCASE("move")
            {
                constexpr std::size_t size = 5u;
                array_data ad = make_array_data_for_null_layout(size);
                null_layout nl(ad);
                const null_layout nl_move = std::move(nl);
                CHECK_EQ(nl_move.size(), size);
            }
        }

        TEST_CASE("operator[]")
        {
            array_data ad = make_array_data_for_null_layout(5);
            null_layout nl(ad);
            const null_layout cnl(ad);

            CHECK_EQ(nl[2], nullval);
            CHECK_EQ(cnl[2], nullval);
        }

        TEST_CASE("iterator")
        {
            array_data ad = make_array_data_for_null_layout(3);
            null_layout nl(ad);

            auto iter = nl.begin();
            auto citer = nl.cbegin();
            CHECK_EQ(*iter, nullval);
            CHECK_EQ(*citer, nullval);

            ++iter;
            ++citer;
            CHECK_EQ(*iter, nullval);
            CHECK_EQ(*citer, nullval);

            iter += 2;
            citer += 2;
            CHECK_EQ(iter, nl.end());
            CHECK_EQ(citer, nl.cend());
        }

        TEST_CASE("const_value_iterator")
        {
            array_data ad = make_array_data_for_null_layout(3);
            null_layout nl(ad);

            auto value_range = nl.values();
            auto iter = value_range.begin();
            CHECK_EQ(*iter, 0);
            iter += 3;
            CHECK_EQ(iter, value_range.end());
        }

        TEST_CASE("const_bitmap_iterator")
        {
            array_data ad = make_array_data_for_null_layout(3);
            null_layout nl(ad);

            auto bitmap_range = nl.bitmap();
            auto iter = bitmap_range.begin();
            CHECK_EQ(*iter, false);
            iter += 3;
            CHECK_EQ(iter, bitmap_range.end());
        }
    }
}
