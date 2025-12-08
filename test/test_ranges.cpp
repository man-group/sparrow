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
#include <vector>

#include "sparrow/utils/ranges.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    TEST_SUITE("ranges")
    {
        TEST_CASE("range_size")
        {
            SUBCASE("for sized_range")
            {
                std::vector<int> v{1, 2, 3, 4, 5};
                CHECK_EQ(range_size(v), 5);
            }

            SUBCASE("for non-sized_range")
            {
                std::vector<int> v{1, 2, 3, 4, 5};
                CHECK_EQ(
                    range_size(
                        v
                        | std::views::filter(
                            [](int i)
                            {
                                return i % 2 == 0;
                            }
                        )
                    ),
                    2
                );
            }

            SUBCASE("for empty range")
            {
                std::vector<int> v;
                CHECK_EQ(range_size(v), 0);
            }
        }

        TEST_CASE("all_same_size")
        {
            SUBCASE("for std::array")
            {
                std::vector<std::array<int, 3>> v{
                    {std::array<int, 3>{1, 2, 3}, std::array<int, 3>{4, 5, 6}, std::array<int, 3>{7, 8, 9}}
                };
                CHECK(all_same_size(v));
            }

            SUBCASE("for std::vector")
            {
                std::vector<std::vector<int>> v{
                    std::vector<int>{1, 2, 3},
                    std::vector<int>{4, 5, 6},
                    std::vector<int>{7, 8, 9}
                };
                CHECK(all_same_size(v));
            }

            SUBCASE("for std::vector with different sizes")
            {
                std::vector<std::vector<int>> v{
                    std::vector<int>{1, 2, 3},
                    std::vector<int>{4, 5, 6, 7},
                    std::vector<int>{8, 9}
                };
                CHECK_FALSE(all_same_size(v));
            }

            SUBCASE("for empty range")
            {
                std::vector<std::vector<int>> v;
                CHECK(all_same_size(v));
            }
        }

        TEST_CASE("accumulate")
        {
            SUBCASE("default operator")
            {
                std::vector<int> v{1, 2, 3, 4};
                auto exp = std::accumulate(v.cbegin(), v.cend(), 0);
                auto res = sparrow::ranges::accumulate(v, 0);
                CHECK_EQ(res, exp);
            }

            SUBCASE("custom operator")
            {
                std::vector<int> v{1, 2, 3, 4};
                auto exp = std::accumulate(v.cbegin(), v.cend(), 0, std::multiplies{});
                auto res = sparrow::ranges::accumulate(v, 0, std::multiplies{});
                CHECK_EQ(res, exp);
            }
        }
    }
}
