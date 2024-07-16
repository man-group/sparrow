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

#include <chrono>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <iostream>  // For Doctest
#include <sstream>
#include <string>
#include <type_traits>

#include "sparrow/data_type.hpp"
#include "sparrow/typed_array.hpp"

#include "array_data_creation.hpp"
#include "doctest/doctest.h"

namespace
{
    constexpr std::size_t test_n = 10;
    constexpr std::size_t test_offset = 1;
    constexpr date::sys_days unix_time = date::sys_days(date::year(1970) / date::January / date::day(1));
    const std::vector<size_t> false_bitmap = {9};
    using sys_time = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;
}

TEST_SUITE("typed_array_timestamp")
{
    TEST_CASE("constructor with parameter")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(test_n, test_offset);
        const sparrow::typed_array<sparrow::timestamp> ta{array_data};
        CHECK_EQ(ta.size(), test_n - test_offset);
    }

    // Element access

    TEST_CASE("at")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n,
            test_offset,
            false_bitmap
        );
        sparrow::typed_array<sparrow::timestamp> ta{array_data};
        for (typename sparrow::typed_array<sparrow::timestamp>::size_type i = 0; i < ta.size() - 1; ++i)
        {
            CHECK_EQ(ta.at(i).value(), sparrow::timestamp(unix_time + date::days(i + 1)));
        }
        CHECK_FALSE(ta.at(false_bitmap[0] - test_offset).has_value());

        CHECK_THROWS_AS(ta.at(ta.size()), std::out_of_range);
    }

    TEST_CASE("const at")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n,
            test_offset,
            false_bitmap
        );
        const sparrow::typed_array<sparrow::timestamp> ta{array_data};
        for (typename sparrow::typed_array<sparrow::timestamp>::size_type i = 0; i < ta.size() - 1; ++i)
        {
            CHECK_EQ(ta.at(i).value(), sparrow::timestamp(unix_time + date::days(i + 1)));
        }
        CHECK_FALSE(ta.at(false_bitmap[0] - test_offset).has_value());

        CHECK_THROWS_AS(ta.at(ta.size()), std::out_of_range);
    }

    TEST_CASE("operator[]")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n,
            test_offset,
            false_bitmap
        );
        sparrow::typed_array<sparrow::timestamp> ta{array_data};
        for (typename sparrow::typed_array<sparrow::timestamp>::size_type i = 0; i < ta.size() - 1; ++i)
        {
            CHECK_EQ(ta[i].value(), sparrow::timestamp(unix_time + date::days(i + 1)));
        }
        CHECK_FALSE(ta[ta.size() - 1].has_value());
    }

    TEST_CASE("const operator[]")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n,
            test_offset,
            false_bitmap
        );
        const sparrow::typed_array<sparrow::timestamp> ta{array_data};
        for (typename sparrow::typed_array<sparrow::timestamp>::size_type i = 0; i < ta.size() - 1; ++i)
        {
            CHECK_EQ(ta[i].value(), sparrow::timestamp(unix_time + date::days(i + 1)));
        }
        CHECK_FALSE(ta[false_bitmap[0] - test_offset].has_value());
    }

    TEST_CASE("front")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n,
            test_offset,
            false_bitmap
        );
        sparrow::typed_array<sparrow::timestamp> ta{array_data};
        CHECK_EQ(ta.front().value(), sparrow::timestamp(unix_time + date::days(1)));
    }

    TEST_CASE("const front")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n,
            test_offset,
            false_bitmap
        );
        const sparrow::typed_array<sparrow::timestamp> ta{array_data};
        CHECK_EQ(ta.front().value(), sparrow::timestamp(unix_time + date::days(1)));
    }

    TEST_CASE("back")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n,
            test_offset,
            false_bitmap
        );
        sparrow::typed_array<sparrow::timestamp> ta{array_data};
        CHECK_FALSE(ta.back().has_value());
    }

    TEST_CASE("const back")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n,
            test_offset,
            false_bitmap
        );
        const sparrow::typed_array<sparrow::timestamp> ta{array_data};
        CHECK_FALSE(ta.back().has_value());
    }

    // Iterators

    TEST_CASE("const iterators")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n,
            test_offset,
            false_bitmap
        );
        const sparrow::typed_array<sparrow::timestamp> ta{array_data};

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

        for (typename sparrow::typed_array<sparrow::timestamp>::size_type i = 0; i < ta.size() - 1; ++iter, ++i)
        {
            REQUIRE(iter->has_value());
            CHECK_EQ(*iter, sparrow::make_nullable(ta[i].value()));
        }

        CHECK_EQ(++iter, end);

        const auto array_data_empty = sparrow::test::make_test_array_data<sparrow::timestamp>(0, 0);
        const sparrow::typed_array<sparrow::timestamp> typed_array_empty(array_data_empty);
        CHECK_EQ(typed_array_empty.cbegin(), typed_array_empty.cend());
    }

    TEST_CASE("bitmap")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n,
            test_offset,
            false_bitmap
        );
        const sparrow::typed_array<sparrow::timestamp> ta{array_data};
        const auto bitmap = ta.bitmap();
        REQUIRE_EQ(bitmap.size(), test_n - test_offset);
        for (std::size_t i = 0; i < bitmap.size() - 1; ++i)
        {
            CHECK(bitmap[static_cast<std::ptrdiff_t>(i)]);
        }
        CHECK_FALSE(bitmap[8]);
    }

    TEST_CASE("values")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n,
            test_offset,
            false_bitmap
        );
        const sparrow::typed_array<sparrow::timestamp> ta{array_data};
        const auto values = ta.values();
        CHECK_EQ(values.size(), test_n - test_offset);
        for (typename sparrow::typed_array<sparrow::timestamp>::size_type i = 0; i < values.size(); ++i)
        {
            CHECK_EQ(values[static_cast<std::ptrdiff_t>(i)], sparrow::timestamp(unix_time + date::days(i + 1)));
        }
    }

    // Capacity

    TEST_CASE("empty")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n,
            test_offset,
            false_bitmap
        );
        const sparrow::typed_array<sparrow::timestamp> ta{array_data};
        CHECK_FALSE(ta.empty());

        const auto array_data_empty = sparrow::test::make_test_array_data<sparrow::timestamp>(0, 0);
        const sparrow::typed_array<sparrow::timestamp> typed_array_empty(array_data_empty);
        CHECK(typed_array_empty.empty());
    }

    TEST_CASE("size")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n,
            test_offset,
            false_bitmap
        );
        const sparrow::typed_array<sparrow::timestamp> ta{array_data};
        CHECK_EQ(ta.size(), test_n - test_offset);
    }

    // Operators

    TEST_CASE("==")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n,
            test_offset,
            false_bitmap
        );
        const sparrow::typed_array<sparrow::timestamp> ta{array_data};
        const sparrow::typed_array<sparrow::timestamp> ta_same{array_data};
        CHECK(ta == ta);
        CHECK(ta == ta_same);

        const auto array_data_less = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n - 1,
            test_offset - 1,
            {8}
        );
        const sparrow::typed_array<sparrow::timestamp> ta_less{array_data_less};
        CHECK_FALSE(ta == ta_less);
        CHECK_FALSE(ta_less == ta);
    }

    TEST_CASE("!=")
    {
        const auto array_data = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n,
            test_offset,
            false_bitmap
        );
        const sparrow::typed_array<sparrow::timestamp> ta{array_data};
        const sparrow::typed_array<sparrow::timestamp> ta_same{array_data};
        CHECK_FALSE(ta != ta);
        CHECK_FALSE(ta != ta_same);

        const auto array_data_less = sparrow::test::make_test_array_data<sparrow::timestamp>(
            test_n - 1,
            test_offset - 1,
            {8}
        );
        const sparrow::typed_array<sparrow::timestamp> ta_less{array_data_less};
        CHECK(ta != ta_less);
        CHECK(ta_less != ta);
    }
}
