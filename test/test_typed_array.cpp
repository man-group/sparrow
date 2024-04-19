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

#include "array_data_creation.hpp"
#include "doctest/doctest.h"

using namespace sparrow;

namespace
{
    template <typename O, std::integral I>
    constexpr O to_value_type(I i)
    {
        if constexpr (std::is_same_v<O, numeric::float16_t>)
        {
            return static_cast<float>(i);
        }
        else if constexpr (std::is_arithmetic_v<O>)
        {
            return static_cast<O>(i);
        }
        else if constexpr (std::is_same_v<O, std::string>)
        {
            return std::to_string(i);
        }
    }

    constexpr size_t n = 10;
    constexpr size_t offset = 1;
    const std::vector<size_t> false_bitmap = {9};
}

TEST_SUITE("typed_array")
{
    TEST_CASE_TEMPLATE_DEFINE("all", T, all)
    {
        SUBCASE("constructor with parameter")
        {
            constexpr size_t n = 10;
            constexpr size_t offset = 1;
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset);
            const typed_array<T> ta{array_data};
            CHECK_EQ(ta.size(), n - offset);
        }

        // Element access

        SUBCASE("at")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            typed_array<T> ta{array_data};
            for (typename typed_array<T>::size_type i = 0; i < ta.size() - 1; ++i)
            {
                CHECK_EQ(ta.at(i).value(), to_value_type<T>(i + offset));
            }
            CHECK_FALSE(ta.at(false_bitmap[0] - offset).has_value());

            CHECK_THROWS_AS(ta.at(ta.size()), std::out_of_range);
        }

        SUBCASE("const at")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            const typed_array<T> ta{array_data};
            for (typename typed_array<T>::size_type i = 0; i < ta.size() - 1; ++i)
            {
                CHECK_EQ(ta.at(i).value(), to_value_type<T>(i + offset));
            }
            CHECK_FALSE(ta.at(false_bitmap[0] - offset).has_value());

            CHECK_THROWS_AS(ta.at(ta.size()), std::out_of_range);
        }

        SUBCASE("operator[]")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            typed_array<T> ta{array_data};
            for (typename typed_array<T>::size_type i = 0; i < ta.size() - 1; ++i)
            {
                CHECK_EQ(ta[i].value(), to_value_type<T>(i + 1));
            }
            CHECK_FALSE(ta[ta.size() - 1].has_value());
        }

        SUBCASE("const operator[]")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            const typed_array<T> ta{array_data};
            for (typename typed_array<T>::size_type i = 0; i < ta.size() - 1; ++i)
            {
                CHECK_EQ(ta[i].value(), to_value_type<T>(i + offset));
            }
            CHECK_FALSE(ta[false_bitmap[0] - offset].has_value());
        }

        SUBCASE("front")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            typed_array<T> ta{array_data};
            CHECK_EQ(ta.front().value(), to_value_type<T>(1));
        }

        SUBCASE("const front")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            const typed_array<T> ta{array_data};
            CHECK_EQ(ta.front().value(), to_value_type<T>(1));
        }

        SUBCASE("back")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            typed_array<T> ta{array_data};
            CHECK_FALSE(ta.back().has_value());
        }

        SUBCASE("const back")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            const typed_array<T> ta{array_data};
            CHECK_FALSE(ta.back().has_value());
        }

        // Iterators

        SUBCASE("const iterators")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            const typed_array<T> ta{array_data};

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

            for (typename typed_array<T>::size_type i = 0; i < ta.size() - 1; ++iter, ++i)
            {
                REQUIRE(iter->has_value());
                CHECK_EQ(*iter, std::make_optional(ta[i].value()));
            }

            CHECK_EQ(++iter, end);

            const auto array_data_empty = sparrow::test::make_test_array_data<T>(0, 0);
            const typed_array<T> typed_array_empty(array_data_empty);
            CHECK_EQ(typed_array_empty.cbegin(), typed_array_empty.cend());
        }

        SUBCASE("bitmap")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            const typed_array<T> ta{array_data};
            const auto bitmap = ta.bitmap();
            REQUIRE_EQ(bitmap.size(), n - offset);
            for (size_t i = 0; i < bitmap.size() - 1; ++i)
            {
                CHECK(bitmap[i]);
            }
            CHECK_FALSE(bitmap[8]);
        }

        SUBCASE("values")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            const typed_array<T> ta{array_data};
            const auto values = ta.values();
            CHECK_EQ(values.size(), n - offset);
            for (size_t i = 0; i < values.size(); ++i)
            {
                CHECK_EQ(values[i], to_value_type<T>(i + 1));
            }
        }

        // Capacity

        SUBCASE("empty")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            const typed_array<T> ta{array_data};
            CHECK_FALSE(ta.empty());

            const auto array_data_empty = sparrow::test::make_test_array_data<T>(0, 0);
            const typed_array<T> typed_array_empty(array_data_empty);
            CHECK(typed_array_empty.empty());
        }

        SUBCASE("size")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            const typed_array<T> ta{array_data};
            CHECK_EQ(ta.size(), n - offset);
        }

        // Operators

        SUBCASE("<=>")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            const typed_array<T> ta{array_data};
            CHECK_EQ(ta <=> ta, std::strong_ordering::equal);

            const auto array_data_less = sparrow::test::make_test_array_data<T>(n - 1, offset - 1, {8});
            const typed_array<T> typed_array_less(array_data_less);
            CHECK_EQ(ta <=> typed_array_less, std::strong_ordering::greater);
            CHECK_EQ(typed_array_less <=> ta, std::strong_ordering::less);
        }

        SUBCASE("==")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            const typed_array<T> ta{array_data};
            const typed_array<T> ta_same{array_data};
            CHECK(ta == ta);
            CHECK(ta == ta_same);

            const auto array_data_less = sparrow::test::make_test_array_data<T>(n - 1, offset - 1, {8});
            const ::typed_array<T> ta_less{array_data_less};
            CHECK_FALSE(ta == ta_less);
            CHECK_FALSE(ta_less == ta);
        }

        SUBCASE("!=")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            const typed_array<T> ta{array_data};
            const typed_array<T> ta_same{array_data};
            CHECK_FALSE(ta != ta);
            CHECK_FALSE(ta != ta_same);

            const auto array_data_less = sparrow::test::make_test_array_data<T>(n - 1, offset - 1, {8});
            const typed_array<T> ta_less{array_data_less};
            CHECK(ta != ta_less);
            CHECK(ta_less != ta);
        }

        SUBCASE("<")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            const typed_array<T> ta{array_data};
            const typed_array<T> ta_same{array_data};
            CHECK_FALSE(ta < ta);
            CHECK_FALSE(ta < ta_same);

            const auto array_data_less = sparrow::test::make_test_array_data<T>(n - 1, offset - 1, {8});
            const typed_array<T> ta_less{array_data_less};
            CHECK_FALSE(ta < ta_less);
            CHECK(ta_less < ta);
        }

        SUBCASE(">")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(n, offset, false_bitmap);
            const typed_array<T> ta{array_data};
            const typed_array<T> ta_same{array_data};
            CHECK_FALSE(ta > ta);
            CHECK_FALSE(ta > ta_same);

            const auto array_data_less = sparrow::test::make_test_array_data<T>(n - 1, offset - 1, {8});
            const ::typed_array<T> ta_less{array_data_less};
            CHECK(ta > ta_less);
            CHECK_FALSE(ta_less > ta);
        }
    }

    TEST_CASE_TEMPLATE_INVOKE(
        all,
        bool,
        std::uint8_t,
        std::int8_t,
        std::uint16_t,
        std::int16_t,
        std::uint32_t,
        std::int32_t,
        std::uint64_t,
        std::int64_t,
        std::string,
        float16_t,
        float32_t,
        float64_t
    );
}
