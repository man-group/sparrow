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

#include <compare>
#include <vector>

#include "sparrow/algorithm.hpp"

#include "array_data_creation.hpp"
#include "doctest/doctest.h"

TEST_SUITE("array_data_creation")
{
    TEST_CASE("make_test_array_data")
    {
        SUBCASE("Default parameters")
        {
            constexpr size_t n = 10;
            const sparrow::array_data data = sparrow::test::make_test_array_data<int>(n);
            CHECK_EQ(data.m_length, n);
            CHECK_EQ(data.m_offset, 0);
            for (size_t i = 0; i < n; i++)
            {
                CHECK(data.m_bitmap[i]);
            }
        }

        SUBCASE("Custom parameters")
        {
            constexpr size_t n = 5;
            constexpr size_t offset = 2;
            const std::vector<size_t> false_bitmap = {1, 3};

            const sparrow::array_data data = sparrow::test::make_test_array_data<int>(n, offset, false_bitmap);
            CHECK_EQ(data.m_length, n);
            CHECK_EQ(data.m_offset, offset);
            CHECK(data.m_bitmap[0]);
            CHECK_FALSE(data.m_bitmap[1]);
            CHECK(data.m_bitmap[2]);
            CHECK_FALSE(data.m_bitmap[3]);
            CHECK(data.m_bitmap[4]);
        }
    }
}