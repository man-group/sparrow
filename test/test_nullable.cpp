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

#include "sparrow/nullable.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    using nullable_double = nullable<double, bool>;
    using nullable_int = nullable<int, bool>;

    TEST_SUITE("nullable value")
    {
        TEST_CASE("constructors")
        {
            SUBCASE("default")
            {
                nullable_double d;
                CHECK_FALSE(d.has_value());
            }
            
            SUBCASE("from nullopt")
            {
                nullable_double d(std::nullopt);
                CHECK_FALSE(d.has_value());
            }

            SUBCASE("from value")
            {
                nullable_double d(1.2);
                REQUIRE(d.has_value());
                CHECK_EQ(d.value(), 1.2);
            }

            SUBCASE("from value with conversion")
            {
                int i = 3;
                nullable_double d(i);
                REQUIRE(d.has_value());
                CHECK_EQ(d.value(), 3.);
            }

            SUBCASE("from value and flag")
            {
                double val = 1.2;
                bool b1 = true;

                nullable_double td1(val, b1);
                nullable_double td2(std::move(val), b1);
                nullable_double td3(val, std::move(b1));
                nullable_double td4(std::move(val), std::move(b1));

                REQUIRE(td1.has_value());
                CHECK_EQ(td1.value(), val);
                REQUIRE(td2.has_value());
                CHECK_EQ(td2.value(), val);
                REQUIRE(td3.has_value());
                CHECK_EQ(td3.value(), val);
                REQUIRE(td4.has_value());
                CHECK_EQ(td4.value(), val);

                bool b2 = false;
                nullable_double fd1(val, b2);
                nullable_double fd2(std::move(val), b2);
                nullable_double fd3(val, std::move(b2));
                nullable_double fd4(std::move(val), std::move(b2));

                CHECK_FALSE(fd1.has_value());
                CHECK_FALSE(fd2.has_value());
                CHECK_FALSE(fd3.has_value());
                CHECK_FALSE(fd4.has_value());
            }
        }

        TEST_CASE("copy constructors")
        {
            SUBCASE("default")
            {
                nullable_double d1(1.2);
                nullable_double d2(d1);
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
            }

            SUBCASE("with conversion")
            {
                nullable_int i(2);
                nullable_double d(i);
                REQUIRE(d.has_value());
                CHECK_EQ(i.value(), d.value());
            }

            SUBCASE("from empty nullable")
            {
                nullable_double d1(std::nullopt);
                nullable_double d2(d1);
                CHECK_FALSE(d2.has_value());
            }
        }

        TEST_CASE("move constructors")
        {
            SUBCASE("default")
            {
                nullable_double d0(1.2);
                nullable_double d1(d0);
                nullable_double d2(std::move(d0));
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
            }

            SUBCASE("with conversion")
            {
                nullable_int i(2);
                nullable_int ci(i);
                nullable_double d(std::move(i));
                REQUIRE(d.has_value());
                CHECK_EQ(ci.value(), d.value());
            }

            SUBCASE("from empty nullable")
            {
                nullable_double d1(std::nullopt);
                nullable_double d2(std::move(d1));
                CHECK_FALSE(d2.has_value());
            }
        }

        TEST_CASE("copy assign")
        {
            SUBCASE("default")
            {
                nullable_double d1(1.2);
                nullable_double d2(2.5);
                d2 = d1;
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
            }

            SUBCASE("with conversion")
            {
                nullable_int d1(1);
                nullable_double d2(2.5);
                d2 = d1;
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
            }

            SUBCASE("from empty nullable")
            {
                nullable_double d1(std::nullopt);
                nullable_double d2(2.5);
                d2 = d1;
                CHECK_FALSE(d2.has_value());
            }
        }

        TEST_CASE("move assign")
        {
            SUBCASE("default")
            {
                nullable_double d0(1.2);
                nullable_double d1(d0);
                nullable_double d2(2.5);
                d2 = std::move(d0);
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
            }

            SUBCASE("with conversion")
            {
                nullable_int d0(1);
                nullable_int d1(d0);
                nullable_double d2(2.5);
                d2 = std::move(d0);
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
            }

            SUBCASE("from empty nullable")
            {
                nullable_double d1(std::nullopt);
                nullable_double d2(2.3);
                d2 = std::move(d1);
                CHECK_FALSE(d2.has_value());
            }
        }

        TEST_CASE("conversion to bool")
        {
            nullable_double d1(1.2);
            CHECK(d1);

            nullable_double d2(std::nullopt);
            CHECK_FALSE(d2);
        }

        TEST_CASE("value / operator*")
        {
            constexpr double initial = 1.2;
            constexpr double expected = 2.5;
            
            SUBCASE("& overload")
            {
                nullable_double d(initial);
                nullable_double& d1(d);
                d1.value() = expected;
                CHECK_EQ(d.value(), expected);
                CHECK_EQ(*d, expected);
            }

            SUBCASE("const & overload")
            {
                nullable_double d(initial);
                const nullable_double& d2(d);
                CHECK_EQ(d2.value(), initial);
                CHECK_EQ(*d2, initial);
            }

            SUBCASE("&& overload")
            {
                nullable_double d(initial);
                nullable_double&& d3(std::move(d));
                d3.value() = expected;
                CHECK_EQ(d.value(), expected);
                CHECK_EQ(*d, expected);
            }

            SUBCASE("const && overload")
            {
                nullable_double d(initial);
                const nullable_double&& d4(std::move(d));
                CHECK_EQ(d4.value(), initial);
                CHECK_EQ(*d4, initial);
            }

            SUBCASE("empty")
            {
                nullable_double empty = std::nullopt;
                CHECK_THROWS_AS(empty.value(), std::bad_optional_access);
                CHECK_NOTHROW(*empty);
            }
        }

        TEST_CASE("value_or")
        {
            constexpr double initial = 1.2;
            constexpr double expected = 2.5;

            nullable_double d(initial);
            nullable_double empty(std::nullopt);

            SUBCASE("const & overload")
            {
                const nullable_double& ref(d);
                const nullable_double& ref_empty(empty);

                double res = ref.value_or(expected);
                double res_empty = ref_empty.value_or(expected);

                CHECK_EQ(res, initial);
                CHECK_EQ(res_empty, expected);
            }

            SUBCASE("&& overload")
            {
                nullable_double&& ref(std::move(d));
                nullable_double&& ref_empty(std::move(empty));

                double res = ref.value_or(expected);
                double res_empty = ref_empty.value_or(expected);

                CHECK_EQ(res, initial);
                CHECK_EQ(res_empty, expected);
            }

        }

        TEST_CASE("swap")
        {
            constexpr double initial = 1.2;
            constexpr double expected = 2.5;
            nullable_double d1(initial);
            nullable_double d2(expected);
            nullable_double empty(std::nullopt);

            swap(d1, d2);
            CHECK_EQ(d1.value(), expected);
            CHECK_EQ(d2.value(), initial);

            swap(d1, empty);
            CHECK_EQ(empty.value(), expected);
            CHECK_FALSE(d1.has_value());
        }

        TEST_CASE("reset")
        {
            constexpr double initial = 1.2;
            nullable_double d(initial);
            d.reset();
            CHECK_FALSE(d.has_value());
        }

        TEST_CASE("equality comparison")
        {
            constexpr double initial = 1.2;
            constexpr double other = 2.5;

            nullable_double d1(initial);
            nullable_double d2(other);
            nullable_double empty;

            CHECK(d1 == d1);
            CHECK(d1 == d1.value());
            CHECK(d1 != d2);
            CHECK(d1 != d2.value());
            CHECK(d1 != empty);
            CHECK(empty == empty);
        }

        TEST_CASE("inequality comparison")
        {
            constexpr double initial = 1.2;
            constexpr double other = 2.5;

            nullable_double d1(initial);
            nullable_double d2(other);
            nullable_double empty;

            // opearator <=
            CHECK(d1 <= d1);
            CHECK(d1 <= d1.value());
            CHECK(d1 <= d2);
            CHECK(d1 <= d2.value());
            CHECK_FALSE(d2 <= d1);
            CHECK_FALSE(d2 <= d1.value());
            CHECK(empty <= d1);
            CHECK_FALSE(d1 <= empty);

            // operator >=
            CHECK(d1 >= d1);
            CHECK(d1 >= d1.value());
            CHECK(d2 >= d1);
            CHECK(d2 >= d1.value());
            CHECK_FALSE(d1 >= d2);
            CHECK_FALSE(d1 >= d2.value());
            CHECK(d1 >= empty);
            CHECK_FALSE(empty >= d1);

            // operator<
            CHECK_FALSE(d1 < d1);
            CHECK_FALSE(d1 < d1.value());
            CHECK(d1 < d2);
            CHECK(d1 < d2.value());
            CHECK(empty < d1);
            CHECK_FALSE(d1 < empty);

            // oprator>
            CHECK_FALSE(d1 > d1);
            CHECK_FALSE(d1 > d1.value());
            CHECK(d2 > d1);
            CHECK(d2 > d1.value());
            CHECK(d1 > empty);
            CHECK_FALSE(empty > d1);
        }

        TEST_CASE("make_nullable")
        {
            double value = 2.5;
            auto opt = make_nullable(std::move(value), true);
            static_assert(std::same_as<std::decay_t<decltype(opt)>, nullable_double>);
            REQUIRE(opt.has_value());
            CHECK_EQ(opt.value(), value);
        }
    }

    using nullable_proxy = nullable<double&, bool>;

    TEST_SUITE("nullable proxy")
    {
        TEST_CASE("constructors")
        {
            double val = 1.2;
            bool b1 = true;

            nullable_proxy td(val);
            REQUIRE(td.has_value());
            CHECK_EQ(td.value(), val);

            nullable_proxy td1(val, b1);
            nullable_proxy td2(val, std::move(b1));

            REQUIRE(td1.has_value());
            CHECK_EQ(td1.value(), val);
            REQUIRE(td2.has_value());
            CHECK_EQ(td2.value(), val);

            bool b2 = false;
            nullable_proxy fd1(val, b2);
            nullable_proxy fd2(val, std::move(b2));

            CHECK_FALSE(fd1.has_value());
            CHECK_FALSE(fd2.has_value());
        }

        TEST_CASE("copy constructors")
        {
            double val = 1.2;
            nullable_proxy d1(val);
            nullable_proxy d2(d1);
            REQUIRE(d2.has_value());
            CHECK_EQ(d1.value(), d2.value());
        }

        TEST_CASE("move constructor")
        {
            double val = 1.2;
            nullable_proxy d1(val);
            nullable_proxy d2(std::move(d1));
            REQUIRE(d2.has_value());
            CHECK_EQ(d2.value(), val);
        }

        TEST_CASE("copy assign")
        {
            SUBCASE("default")
            {
                double initial = 1.2;
                double expected = 2.5;
                nullable_proxy d1(initial);
                nullable_proxy d2(expected);
                d2 = d1;
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
                CHECK_EQ(initial, expected);
            }

            SUBCASE("with conversion")
            {
                double initial = 1.2;
                double expected = 2.5;
                nullable_double d1(initial);
                nullable_proxy d2(expected);
                d2 = d1;
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
                CHECK_EQ(initial, expected);
            }

            SUBCASE("from empty nullable")
            {
                double initial = 1.2;
                nullable_proxy d2(initial);
                d2 = std::nullopt;
                CHECK_FALSE(d2.has_value());
            }
        }

        TEST_CASE("move assign")
        {
            SUBCASE("default")
            {
                double initial = 1.2;
                double expected = 2.5;
                nullable_proxy d1(initial);
                nullable_proxy d2(expected);
                d2 = std::move(d1);
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
                CHECK_EQ(initial, expected);

            }

            SUBCASE("with conversion")
            {
                double initial = 1.2;
                double expected = 2.5;
                nullable_double d1(initial);
                nullable_proxy d2(expected);
                d2 = std::move(d1);
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
                CHECK_EQ(initial, expected);
            }
        }

        TEST_CASE("conversion to bool")
        {
            double val = 1.2;
            nullable_proxy d1(val);
            CHECK(d1);

            d1 = std::nullopt;
            CHECK_FALSE(d1);
        }

        TEST_CASE("value / operator*")
        {
            double initial = 1.2;
            double expected = 2.5;
            
            SUBCASE("& overload")
            {
                nullable_proxy d(initial);
                nullable_proxy& d1(d);
                d1.value() = expected;
                CHECK_EQ(d.value(), expected);
                CHECK_EQ(*d, expected);
            }

            SUBCASE("const & overload")
            {
                nullable_proxy d(initial);
                const nullable_proxy& d2(d);
                CHECK_EQ(d2.value(), initial);
                CHECK_EQ(*d2, initial);
            }

            SUBCASE("&& overload")
            {
                nullable_proxy d(initial);
                nullable_proxy&& d3(std::move(d));
                d3.value() = expected;
                CHECK_EQ(d.value(), expected);
                CHECK_EQ(*d, expected);
            }

            SUBCASE("const && overload")
            {
                nullable_proxy d(initial);
                const nullable_proxy&& d4(std::move(d));
                CHECK_EQ(d4.value(), initial);
                CHECK_EQ(*d4, initial);
            }

            SUBCASE("empty")
            {
                nullable_proxy empty(initial);
                empty = std::nullopt;
                CHECK_THROWS_AS(empty.value(), std::bad_optional_access);
                CHECK_NOTHROW(*empty);
            }
        }

        TEST_CASE("value_or")
        {
            double initial = 1.2;
            double expected = 2.5;

            nullable_proxy d(initial);
            nullable_proxy empty(initial);
            empty = std::nullopt;

            SUBCASE("const & overload")
            {
                const nullable_proxy& ref(d);
                const nullable_proxy& ref_empty(empty);

                double res = ref.value_or(expected);
                double res_empty = ref_empty.value_or(expected);

                CHECK_EQ(res, initial);
                CHECK_EQ(res_empty, expected);
            }

            SUBCASE("&& overload")
            {
                nullable_proxy&& ref(std::move(d));
                nullable_proxy&& ref_empty(std::move(empty));

                double res = ref.value_or(expected);
                double res_empty = ref_empty.value_or(expected);

                CHECK_EQ(res, initial);
                CHECK_EQ(res_empty, expected);
            }
        }

        TEST_CASE("swap")
        {
            double initial = 1.2;
            double expected = 2.5;
            double initial_bu = initial;
            double expected_bu = expected;
            double empty_val = 3.7;
            nullable_proxy d1(initial);
            nullable_proxy d2(expected);
            nullable_proxy empty(empty_val);
            empty = std::nullopt;

            swap(d1, d2);
            CHECK_EQ(d1.value(), expected_bu);
            CHECK_EQ(d2.value(), initial_bu);

            swap(d1, empty);
            CHECK_EQ(empty.value(), expected_bu);
            CHECK_FALSE(d1.has_value());
        }

        TEST_CASE("reset")
        {
            double initial = 1.2;
            nullable_proxy d(initial);
            d.reset();
            CHECK_FALSE(d.has_value());
        }

        TEST_CASE("equality comparison")
        {
            double initial = 1.2;
            double other = 2.5;
            double empty_val = 3.7;

            nullable_proxy d1(initial);
            nullable_proxy d2(other);
            nullable_proxy empty(empty_val);
            empty = std::nullopt;

            CHECK(d1 == d1);
            CHECK(d1 == d1.value());
            CHECK(d1 != d2);
            CHECK(d1 != d2.value());
            CHECK(d1 != empty);
            CHECK(empty == empty);
        }

        TEST_CASE("inequality comparison")
        {
            double initial = 1.2;
            double other = 2.5;
            double empty_val = 3.7;

            nullable_proxy d1(initial);
            nullable_proxy d2(other);
            nullable_proxy empty(empty_val);
            empty = std::nullopt;

            // opearator <=
            CHECK(d1 <= d1);
            CHECK(d1 <= d1.value());
            CHECK(d1 <= d2);
            CHECK(d1 <= d2.value());
            CHECK_FALSE(d2 <= d1);
            CHECK_FALSE(d2 <= d1.value());
            CHECK(empty <= d1);
            CHECK_FALSE(d1 <= empty);

            // operator >=
            CHECK(d1 >= d1);
            CHECK(d1 >= d1.value());
            CHECK(d2 >= d1);
            CHECK(d2 >= d1.value());
            CHECK_FALSE(d1 >= d2);
            CHECK_FALSE(d1 >= d2.value());
            CHECK(d1 >= empty);
            CHECK_FALSE(empty >= d1);

            // operator<
            CHECK_FALSE(d1 < d1);
            CHECK_FALSE(d1 < d1.value());
            CHECK(d1 < d2);
            CHECK(d1 < d2.value());
            CHECK(empty < d1);
            CHECK_FALSE(d1 < empty);

            // oprator>
            CHECK_FALSE(d1 > d1);
            CHECK_FALSE(d1 > d1.value());
            CHECK(d2 > d1);
            CHECK(d2 > d1.value());
            CHECK(d1 > empty);
            CHECK_FALSE(empty > d1);
        }

        TEST_CASE("make_nullable")
        {
            double value = 2.7;
            auto opt = make_nullable(value, true);
            static_assert(std::same_as<std::decay_t<decltype(opt)>, nullable_proxy>);
            REQUIRE(opt.has_value());
            CHECK_EQ(opt.value(), value);
        }
    }
}

