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

#include <cstddef>

#include "sparrow/arrow_array_schema_utils.hpp"
#include "sparrow/buffer.hpp"

#include "doctest/doctest.h"

TEST_SUITE("C Data Interface")
{
    TEST_CASE("Arrow Array and schema utils")
    {
        SUBCASE("get_size")
        {
            SUBCASE("std::nullptr_t")
            {
                std::nullptr_t ptr = nullptr;
                const auto size = sparrow::get_size(ptr);
                CHECK_EQ(size, 0);
            }

            SUBCASE("std::vector")
            {
                std::vector<int> vec = {0, 1, 2, 3, 4, 5};
                const auto size = sparrow::get_size(vec);
                CHECK_EQ(size, vec.size());
            }

            SUBCASE("std::tuple")
            {
                std::tuple<int, int, int> tuple{0, 1, 2};
                constexpr auto size = sparrow::get_size(tuple);
                CHECK_EQ(size, 3);
            }
        }
    }
}