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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or mplied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string>

#include "sparrow/utils/repeat_container.hpp"

#include "doctest/doctest.h"

TEST_SUITE("repeat_container")
{
    const std::string value = "test";

    TEST_CASE("constructor")
    {
        const sparrow::repeat_view view(value, 5);
    }

    TEST_CASE("begin")
    {
        const sparrow::repeat_view view(value, 5);
        const auto iter = view.begin();
        CHECK_EQ(*iter, value);
    }

    TEST_CASE("end")
    {
        const sparrow::repeat_view view(value, 5);
        const auto iter = view.end();
        CHECK_EQ(*iter, value);
    }

    TEST_CASE("size")
    {
        const sparrow::repeat_view view(value, 5);
        CHECK_EQ(view.size(), 5);
    }

    TEST_CASE("iterators")
    {
        const sparrow::repeat_view view(value, 5);
        auto iter = view.begin();
        CHECK_EQ(*iter, value);
        CHECK_EQ(*(++iter), value);
        CHECK_EQ(*(++iter), value);
        CHECK_EQ(*(++iter), value);
        CHECK_EQ(*(++iter), value);
        CHECK_EQ(++iter, view.end());
    }
}
