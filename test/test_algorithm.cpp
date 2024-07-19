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

#include "sparrow/utils/algorithm.hpp"

#include "doctest/doctest.h"

TEST_SUITE("algorithm")
{
    TEST_CASE("lexicographical_compare_three_way")
    {
        const std::vector<int> v1 = {1, 2, 3};
        std::vector<int> v2 = {1, 2, 3};
        CHECK_EQ(sparrow::lexicographical_compare_three_way(v1, v2), std::strong_ordering::equal);

        v2 = {1, 2, 4};
        CHECK_EQ(sparrow::lexicographical_compare_three_way(v1, v2), std::strong_ordering::less);

        v2 = {1, 2, 2};
        CHECK_EQ(sparrow::lexicographical_compare_three_way(v1, v2), std::strong_ordering::greater);
    }

    TEST_CASE("lexicographical_compare_three_way with empty ranges")
    {
        const std::vector<int> v1 = {1, 2, 3};
        const std::vector<int> v2 = {};
        CHECK_EQ(sparrow::lexicographical_compare_three_way(v1, v2), std::strong_ordering::greater);
        CHECK_EQ(sparrow::lexicographical_compare_three_way(v2, v1), std::strong_ordering::less);

        const std::vector<int> v3 = {};
        CHECK_EQ(sparrow::lexicographical_compare_three_way(v2, v3), std::strong_ordering::equal);
    }
}
