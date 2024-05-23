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

#include <compare>
#include <cstddef>
#include <cstdint>
#include <iostream>  // For Doctest
#include <sstream>
#include <string>
#include <type_traits>

#include "sparrow/typed_array.hpp"
#include "sparrow/data_type.hpp"

#include "array_data_creation.hpp"
#include "doctest/doctest.h"

using namespace sparrow;

namespace
{
    constexpr size_t n = 10;
    constexpr size_t offset = 1;
    const std::vector<size_t> false_bitmap = {9};
    using sys_time = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;

    using namespace date::literals;
}

TEST_SUITE("typed_array_timestamp")
{
    TEST_CASE("constructor with parameter")
    {
        constexpr size_t n = 10;
        constexpr size_t offset = 1;
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(n, offset);
        const typed_array<sparrow::timestamp> ta{array_data};
        CHECK_EQ(ta.size(), n - offset);
    }

    // Element access

    TEST_CASE("at")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(n, offset, false_bitmap);
        typed_array<sparrow::timestamp> ta{array_data};
        for (typename typed_array<sparrow::timestamp>::size_type i = 0; i < ta.size() - 1; ++i)
        {
            CHECK_EQ(ta.at(i).value(), sparrow::timestamp(date::sys_days(1970_y/date::January/1_d) + date::days(i + 1)));
        }
        CHECK_FALSE(ta.at(false_bitmap[0] - offset).has_value());

        CHECK_THROWS_AS(ta.at(ta.size()), std::out_of_range);
    }

    TEST_CASE("const at")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(n, offset, false_bitmap);
        const typed_array<sparrow::timestamp> ta{array_data};
        for (typename typed_array<sparrow::timestamp>::size_type i = 0; i < ta.size() - 1; ++i)
        {
            CHECK_EQ(ta.at(i).value(), sparrow::timestamp(date::sys_days(1970_y/date::January/1_d) + date::days(i + 1)));
        }
        CHECK_FALSE(ta.at(false_bitmap[0] - offset).has_value());

        CHECK_THROWS_AS(ta.at(ta.size()), std::out_of_range);
    }

    TEST_CASE("operator[]")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(n, offset, false_bitmap);
        typed_array<sparrow::timestamp> ta{array_data};
        for (typename typed_array<sparrow::timestamp>::size_type i = 0; i < ta.size() - 1; ++i)
        {
            CHECK_EQ(ta[i].value(), sparrow::timestamp(date::sys_days(1970_y/date::January/1_d) + date::days(i + 1)));
        }
        CHECK_FALSE(ta[ta.size() - 1].has_value());
    }

    TEST_CASE("const operator[]")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(n, offset, false_bitmap);
        const typed_array<sparrow::timestamp> ta{array_data};
        for (typename typed_array<sparrow::timestamp>::size_type i = 0; i < ta.size() - 1; ++i)
        {
            CHECK_EQ(ta[i].value(), sparrow::timestamp(date::sys_days(1970_y/date::January/1_d) + date::days(i + 1)));
        }
        CHECK_FALSE(ta[false_bitmap[0] - offset].has_value());
    }

    TEST_CASE("front")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(n, offset, false_bitmap);
        typed_array<sparrow::timestamp> ta{array_data};
        CHECK_EQ(ta.front().value(), sparrow::timestamp(date::sys_days(1970_y/date::January/1_d) + date::days(1)));
    }

    TEST_CASE("const front")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(n, offset, false_bitmap);
        const typed_array<sparrow::timestamp> ta{array_data};
        CHECK_EQ(ta.front().value(), sparrow::timestamp(date::sys_days(1970_y/date::January/1_d) + date::days(1)));
    }

    TEST_CASE("back")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(n, offset, false_bitmap);
        typed_array<sparrow::timestamp> ta{array_data};
        CHECK_FALSE(ta.back().has_value());
    }

    TEST_CASE("const back")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(n, offset, false_bitmap);
        const typed_array<sparrow::timestamp> ta{array_data};
        CHECK_FALSE(ta.back().has_value());
    }

    // Iterators

    TEST_CASE("const iterators")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(n, offset, false_bitmap);
        const typed_array<sparrow::timestamp> ta{array_data};

        auto iter = ta.cbegin();
        CHECK(std::is_const_v<typename std::remove_reference_t<decltype(iter->value())>>);
        auto iter_bis = ta.begin();
        CHECK(std::is_const_v<typename std::remove_reference_t<decltype(iter_bis->value())>>);
        CHECK_EQ(iter, iter_bis);

        const auto end = ta.cend();
        CHECK(std::is_const_v<typename std::remove_reference_t<decltype(end->value())>>);
        const auto end_bis = ta.end();
        CHECK(std::is_const_v<typename std::remove_reference_t<decltype(end_bis->value())>>);
        CHECK_EQ(end, end_bis);

        for (typename typed_array<sparrow::timestamp>::size_type i = 0; i < ta.size() - 1; ++iter, ++i)
        {
            REQUIRE(iter->has_value());
            CHECK_EQ(*iter, std::make_optional(ta[i].value()));
        }

        CHECK_EQ(++iter, end);

        const auto array_data_empty = sparrow::test::make_test_array_data<sparrow::timestamp>(0, 0);
        const typed_array<sparrow::timestamp> typed_array_empty(array_data_empty);
        CHECK_EQ(typed_array_empty.cbegin(), typed_array_empty.cend());
    }

    TEST_CASE("bitmap")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(n, offset, false_bitmap);
        const typed_array<sparrow::timestamp> ta{array_data};
        const auto bitmap = ta.bitmap();
        REQUIRE_EQ(bitmap.size(), n - offset);
        for (size_t i = 0; i < bitmap.size() - 1; ++i)
        {
            CHECK(bitmap[i]);
        }
        CHECK_FALSE(bitmap[8]);
    }

    TEST_CASE("values")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(n, offset, false_bitmap);
        const typed_array<sparrow::timestamp> ta{array_data};
        const auto values = ta.values();
        CHECK_EQ(values.size(), n - offset);
        for (typename typed_array<sparrow::timestamp>::size_type i = 0; i < values.size(); ++i)
        {
            CHECK_EQ(values[i], sparrow::timestamp(date::sys_days(1970_y/date::January/1_d) + date::days(i + 1)));
        }
    }

    // Capacity

    TEST_CASE("empty")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(n, offset, false_bitmap);
        const typed_array<sparrow::timestamp> ta{array_data};
        CHECK_FALSE(ta.empty());

        const auto array_data_empty = sparrow::test::make_test_array_data<sparrow::timestamp>(0, 0);
        const typed_array<sparrow::timestamp> typed_array_empty(array_data_empty);
        CHECK(typed_array_empty.empty());
    }

    TEST_CASE("size")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(n, offset, false_bitmap);
        const typed_array<sparrow::timestamp> ta{array_data};
        CHECK_EQ(ta.size(), n - offset);
    }

    // Operators

    TEST_CASE("==")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(n, offset, false_bitmap);
        const typed_array<sparrow::timestamp> ta{array_data};
        const typed_array<sparrow::timestamp> ta_same{array_data};
        CHECK(ta == ta);
        CHECK(ta == ta_same);

        const auto array_data_less = sparrow::test::make_test_array_data<sparrow::timestamp>(n - 1, offset - 1, {8});
        const ::typed_array<sparrow::timestamp> ta_less{array_data_less};
        CHECK_FALSE(ta == ta_less);
        CHECK_FALSE(ta_less == ta);
    }

    TEST_CASE("!=")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(n, offset, false_bitmap);
        const typed_array<sparrow::timestamp> ta{array_data};
        const typed_array<sparrow::timestamp> ta_same{array_data};
        CHECK_FALSE(ta != ta);
        CHECK_FALSE(ta != ta_same);

        const auto array_data_less = sparrow::test::make_test_array_data<sparrow::timestamp>(n - 1, offset - 1, {8});
        const typed_array<sparrow::timestamp> ta_less{array_data_less};
        CHECK(ta != ta_less);
        CHECK(ta_less != ta);
    }
}
