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

#if defined(__cpp_lib_format)
#    include <format>
#endif
#include <string>
#include <vector>

#include "sparrow/utils/nullable.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    // Custom type that increments a counter
    // when an instane is move constructed or
    // move assigned (and decrements it upon
    // deletion). This allows to test that
    // nullable views do not move their underlying
    // data upon assignment.
    class Custom
    {
    public:

        static int counter;

        explicit Custom(int i = 0)
            : m_value(i)
        {
        }

        ~Custom()
        {
            if (m_moved)
            {
                --counter;
            }
        }

        Custom(const Custom& rhs)
            : m_value(rhs.m_value)
        {
        }

        Custom& operator=(const Custom& rhs)
        {
            m_value = rhs.m_value;
            return *this;
        }

        Custom(Custom&& rhs)
            : m_value(rhs.m_value)
        {
            m_moved = true;
            ++counter;
        }

        Custom& operator=(Custom&& rhs)
        {
            m_value = rhs.m_value;
            if (!m_moved)
            {
                m_moved = true;
                ++counter;
            }
            return *this;
        }

        Custom& operator=(const int& i)
        {
            m_value = i;
            return *this;
        }

        Custom& operator=(int&& i)
        {
            m_value = i;
            if (!m_moved)
            {
                m_moved = true;
                ++counter;
            }
            return *this;
        }

        const int& get_value() const
        {
            return m_value;
        }

    private:

        int m_value;
        bool m_moved = false;
    };

    int Custom::counter = 0;

    bool operator==(const Custom& lhs, const Custom& rhs)
    {
        return lhs.get_value() == rhs.get_value();
    }

    bool operator==(const Custom& lhs, const int& rhs)
    {
        return lhs.get_value() == rhs;
    }

    std::strong_ordering operator<=>(const Custom& lhs, const Custom& rhs)
    {
        return lhs.get_value() <=> rhs.get_value();
    }

    std::strong_ordering operator<=>(const Custom& lhs, const int& rhs)
    {
        return lhs.get_value() <=> rhs;
    }
}

#if defined(__cpp_lib_format)
template <>
struct std::formatter<sparrow::Custom>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const sparrow::Custom& custom, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "Custom({})", custom.get_value());
    }
};
#endif

namespace sparrow
{

    using testing_types = std::tuple<double, std::string, Custom>;

    namespace
    {
        template <class T>
        struct fixture;

        template <>
        struct fixture<double>
        {
            static double init()
            {
                return 1.2;
            }

            static double other()
            {
                return 2.5;
            }

            static int convert_init()
            {
                return 3;
            }

            using convert_type = int;

            static bool check_move_count(int)
            {
                return true;
            }
        };

        template <>
        struct fixture<std::string>
        {
            static std::string init()
            {
                return "And now young codebase ...";
            }

            static std::string other()
            {
                return "Darth Codius";
            }

            static const char* convert_init()
            {
                return "Noooooo that's impossible!";
            }

            using convert_type = const char*;

            static bool check_move_count(int)
            {
                return true;
            }
        };

        template <>
        struct fixture<Custom>
        {
            static Custom init()
            {
                return Custom(1);
            }

            static Custom other()
            {
                return Custom(2);
            }

            static int convert_init()
            {
                return 3;
            }

            using convert_type = int;

            static bool check_move_count(int ref)
            {
                return Custom::counter == ref;
            }
        };
    }

    TEST_SUITE("nullable value")
    {
        TEST_CASE_TEMPLATE_DEFINE("constructors", T, constructors_id)
        {
            SUBCASE("default")
            {
                nullable<T> d;
                CHECK_FALSE(d.has_value());
            }

            SUBCASE("from nullval")
            {
                nullable<T> d{nullval};
                CHECK_FALSE(d.has_value());
            }

            SUBCASE("from value")
            {
                T dval = fixture<T>::init();
                nullable<T> d{dval};
                REQUIRE(d.has_value());
                CHECK_EQ(d.value(), dval);
            }

            SUBCASE("from value with conversion")
            {
                auto val = fixture<T>::convert_init();
                nullable<T> d{val};
                REQUIRE(d.has_value());
                CHECK_EQ(d.value(), T(val));
            }

            SUBCASE("from value and flag")
            {
                T val = fixture<T>::init();
                bool b1 = true;

                nullable<T> td1(val, b1);
                T val2 = val;
                nullable<T> td2(std::move(val2), b1);
                nullable<T> td3(val, std::move(b1));
                val2 = val;
                nullable<T> td4(std::move(val2), std::move(b1));

                REQUIRE(td1.has_value());
                CHECK_EQ(td1.value(), val);
                REQUIRE(td2.has_value());
                CHECK_EQ(td2.value(), val);
                REQUIRE(td3.has_value());
                CHECK_EQ(td3.value(), val);
                REQUIRE(td4.has_value());
                CHECK_EQ(td4.value(), val);

                bool b2 = false;
                nullable<T> fd1(val, b2);
                val2 = val;
                nullable<T> fd2(std::move(val2), b2);
                nullable<T> fd3(val, std::move(b2));
                val2 = val;
                nullable<T> fd4(std::move(val2), std::move(b2));

                CHECK_FALSE(fd1.has_value());
                CHECK_FALSE(fd2.has_value());
                CHECK_FALSE(fd3.has_value());
                CHECK_FALSE(fd4.has_value());
            }
        }
        TEST_CASE_TEMPLATE_APPLY(constructors_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("copy constructors", T, copy_constructors_id)
        {
            SUBCASE("default")
            {
                auto val = fixture<T>::init();
                nullable<T> d1{val};
                nullable<T> d2(d1);
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
                CHECK(fixture<T>::check_move_count(0));
            }

            SUBCASE("with conversion")
            {
                nullable<typename fixture<T>::convert_type> i{fixture<T>::convert_init()};
                nullable<T> d(i);
                REQUIRE(d.has_value());
                CHECK_EQ(i.value(), d.value());
                CHECK(fixture<T>::check_move_count(0));
            }

            SUBCASE("from empty nullable")
            {
                nullable<T> d1(nullval);
                nullable<T> d2(d1);
                CHECK_FALSE(d2.has_value());
                CHECK(fixture<T>::check_move_count(0));
            }
        }
        TEST_CASE_TEMPLATE_APPLY(copy_constructors_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("move constructors", T, move_constructors_id)
        {
            SUBCASE("default")
            {
                auto val = fixture<T>::init();
                nullable<T> d0{val};
                nullable<T> d1(d0);
                nullable<T> d2(std::move(d0));
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
                CHECK(fixture<T>::check_move_count(1));
            }

            SUBCASE("with conversion")
            {
                using nullable_conv = nullable<typename fixture<T>::convert_type>;
                auto val = fixture<T>::convert_init();
                nullable_conv i{val};
                nullable_conv ci(i);
                nullable<T> d(std::move(i));
                REQUIRE(d.has_value());
                CHECK_EQ(ci.value(), d.value());
                // Custom(int) is called
                CHECK(fixture<T>::check_move_count(0));
            }

            SUBCASE("from empty nullable")
            {
                nullable<T> d1(nullval);
                nullable<T> d2(std::move(d1));
                CHECK_FALSE(d2.has_value());
                CHECK(fixture<T>::check_move_count(1));
            }
        }
        TEST_CASE_TEMPLATE_APPLY(move_constructors_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("copy assign", T, copy_assign_id)
        {
            SUBCASE("default")
            {
                auto val1 = fixture<T>::init();
                auto val2 = fixture<T>::other();
                nullable<T> d1{val1};
                nullable<T> d2{val2};
                d2 = d1;
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
                CHECK(fixture<T>::check_move_count(0));
            }

            SUBCASE("with conversion")
            {
                auto val1 = fixture<T>::convert_init();
                auto val2 = fixture<T>::init();
                nullable<typename fixture<T>::convert_type> d1{val1};
                nullable<T> d2{val2};
                d2 = d1;
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
                CHECK(fixture<T>::check_move_count(0));
            }

            SUBCASE("from empty nullable")
            {
                nullable<T> d1{nullval};
                auto val = fixture<T>::init();
                nullable<T> d2{val};
                d2 = d1;
                CHECK_FALSE(d2.has_value());
                CHECK(fixture<T>::check_move_count(0));
            }
        }
        TEST_CASE_TEMPLATE_APPLY(copy_assign_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("move assign", T, move_assign_id)
        {
            SUBCASE("default")
            {
                auto val0 = fixture<T>::init();
                nullable<T> d0{val0};
                nullable<T> d1(d0);
                auto val1 = fixture<T>::other();
                nullable<T> d2{val1};
                d2 = std::move(d0);
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
                CHECK(fixture<T>::check_move_count(1));
            }

            SUBCASE("with conversion")
            {
                using nullable_conv = nullable<typename fixture<T>::convert_type>;
                auto val0 = fixture<T>::convert_init();
                nullable_conv d0{val0};
                nullable_conv d1(d0);
                auto val1 = fixture<T>::init();
                nullable<T> d2{val1};
                d2 = std::move(d0);
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
                CHECK(fixture<T>::check_move_count(1));
            }

            SUBCASE("from empty nullable")
            {
                nullable<T> d1{nullval};
                auto val = fixture<T>::init();
                nullable<T> d2{val};
                d2 = std::move(d1);
                CHECK_FALSE(d2.has_value());
                CHECK(fixture<T>::check_move_count(1));
            }
        }
        TEST_CASE_TEMPLATE_APPLY(move_assign_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("conversion to bool", T, conversion_id)
        {
            nullable<T> d1{fixture<T>::init()};
            CHECK(d1);

            nullable<T> d2{nullval};
            CHECK_FALSE(d2);
        }
        TEST_CASE_TEMPLATE_APPLY(conversion_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("value / get", T, value_id)
        {
            const T initial = fixture<T>::init();
            const T expected = fixture<T>::other();

            SUBCASE("& overload")
            {
                nullable<T> d{initial};
                nullable<T>& d1{d};
                d1.value() = expected;
                CHECK_EQ(d.value(), expected);
                CHECK_EQ(d.get(), expected);
            }

            SUBCASE("const & overload")
            {
                nullable<T> d{initial};
                const nullable<T>& d2{d};
                CHECK_EQ(d2.value(), initial);
                CHECK_EQ(d2.get(), initial);
            }

            SUBCASE("&& overload")
            {
                nullable<T> d{initial};
                nullable<T>&& d3(std::move(d));
                d3.value() = expected;
                CHECK_EQ(d.value(), expected);
                CHECK_EQ(d.get(), expected);
            }

            SUBCASE("const && overload")
            {
                nullable<T> d{initial};
                const nullable<T>&& d4(std::move(d));
                CHECK_EQ(d4.value(), initial);
                CHECK_EQ(d4.get(), initial);
            }

            SUBCASE("empty")
            {
                nullable<T> empty = nullval;
                CHECK_THROWS_AS(empty.value(), bad_nullable_access);
                CHECK_NOTHROW(empty.get());
            }
        }
        TEST_CASE_TEMPLATE_APPLY(value_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("value_or", T, value_or_id)
        {
            const T initial = fixture<T>::init();
            const T expected = fixture<T>::other();

            nullable<T> d{initial};
            nullable<T> empty{nullval};

            SUBCASE("const & overload")
            {
                const nullable<T>& ref(d);
                const nullable<T>& ref_empty(empty);

                T res = ref.value_or(expected);
                T res_empty = ref_empty.value_or(expected);

                CHECK_EQ(res, initial);
                CHECK_EQ(res_empty, expected);
            }

            SUBCASE("&& overload")
            {
                nullable<T>&& ref(std::move(d));
                nullable<T>&& ref_empty(std::move(empty));

                T res = ref.value_or(expected);
                T res_empty = ref_empty.value_or(expected);

                CHECK_EQ(res, initial);
                CHECK_EQ(res_empty, expected);
            }
        }
        TEST_CASE_TEMPLATE_APPLY(value_or_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("swap", T, swap_id)
        {
            const T initial = fixture<T>::init();
            const T expected = fixture<T>::other();
            nullable<T> d1{initial};
            nullable<T> d2{expected};
            nullable<T> empty{nullval};

            swap(d1, d2);
            CHECK_EQ(d1.value(), expected);
            CHECK_EQ(d2.value(), initial);

            swap(d1, empty);
            CHECK_EQ(empty.value(), expected);
            CHECK_FALSE(d1.has_value());
        }
        TEST_CASE_TEMPLATE_APPLY(swap_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("reset", T, reset_id)
        {
            const T initial = fixture<T>::init();
            nullable<T> d{initial};
            d.reset();
            CHECK_FALSE(d.has_value());
        }
        TEST_CASE_TEMPLATE_APPLY(reset_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("equality comparison", T, equality_comparison_id)
        {
            const T initial = fixture<T>::init();
            const T other = fixture<T>::other();

            nullable<T> d1{initial};
            nullable<T> d2{other};
            nullable<T> empty;

            CHECK(d1 == d1);
            CHECK(d1 == d1.value());
            CHECK(d1 != d2);
            CHECK(d1 != d2.value());
            CHECK(d1 != empty);
            CHECK(empty == empty);
        }
        TEST_CASE_TEMPLATE_APPLY(equality_comparison_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("inequality comparison", T, inequality_comparison_id)
        {
            const T initial = fixture<T>::init();
            const T other = fixture<T>::other();

            nullable<T> d1{initial};
            nullable<T> d2{other};
            nullable<T> empty;

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
        TEST_CASE_TEMPLATE_APPLY(inequality_comparison_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("make_nullable", T, make_nullable_id)
        {
            T value = fixture<T>::init();
            T value_copy = value;
            auto opt = make_nullable(std::move(value), true);
            static_assert(std::same_as<std::decay_t<decltype(opt)>, nullable<T>>);
            REQUIRE(opt.has_value());
            CHECK_EQ(opt.value(), value_copy);
        }
        TEST_CASE_TEMPLATE_APPLY(make_nullable_id, testing_types);
    }

    TEST_SUITE("nullable proxy")
    {
        static_assert(std::is_convertible_v<
                      sparrow::nullable<const bool&>&&,
                      sparrow::nullable<const bool&, const bool&>>);

        static_assert(std::is_convertible_v<
                      const sparrow::nullable<const bool&>&,
                      sparrow::nullable<const bool&, const bool&>>);

        TEST_CASE_TEMPLATE_DEFINE("constructors", T, constructors_id)
        {
            T val = fixture<T>::init();
            bool b1 = true;

            nullable<T&> td(val);
            REQUIRE(td.has_value());
            CHECK_EQ(td.value(), val);

            nullable<T&> td1(val, b1);
            nullable<T&> td2(val, std::move(b1));

            REQUIRE(td1.has_value());
            CHECK_EQ(td1.value(), val);
            REQUIRE(td2.has_value());
            CHECK_EQ(td2.value(), val);

            bool b2 = false;
            nullable<T&> fd1(val, b2);
            nullable<T&> fd2(val, std::move(b2));

            CHECK_FALSE(fd1.has_value());
            CHECK_FALSE(fd2.has_value());
        }
        TEST_CASE_TEMPLATE_APPLY(constructors_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("copy constructors", T, copy_constructors_id)
        {
            T val = fixture<T>::init();
            nullable<T&> d1(val);
            nullable<T&> d2(d1);
            REQUIRE(d2.has_value());
            CHECK_EQ(d1.value(), d2.value());
            CHECK(fixture<T>::check_move_count(0));
        }
        TEST_CASE_TEMPLATE_APPLY(copy_constructors_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("move constructors", T, move_constructors_id)
        {
            T val = fixture<T>::init();
            nullable<T&> d1(val);
            nullable<T&> d2(std::move(d1));
            REQUIRE(d2.has_value());
            CHECK_EQ(d2.value(), val);
            CHECK(fixture<T>::check_move_count(0));
        }
        TEST_CASE_TEMPLATE_APPLY(move_constructors_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("copy assign", T, copy_assign_id)
        {
            SUBCASE("default")
            {
                T initial = fixture<T>::init();
                T expected = fixture<T>::other();
                nullable<T&> d1{initial};
                nullable<T&> d2{expected};
                d2 = d1;
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
                CHECK_EQ(initial, expected);
                CHECK(fixture<T>::check_move_count(0));
            }

            SUBCASE("with conversion")
            {
                T initial = fixture<T>::init();
                T expected = fixture<T>::other();
                nullable<T> d1{initial};
                nullable<T&> d2{expected};
                d2 = d1;
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
                CHECK_EQ(initial, expected);
                CHECK(fixture<T>::check_move_count(0));
            }

            SUBCASE("from empty nullable")
            {
                T initial = fixture<T>::init();
                nullable<T&> d2(initial);
                d2 = nullval;
                CHECK_FALSE(d2.has_value());
                CHECK(fixture<T>::check_move_count(0));
            }
        }
        TEST_CASE_TEMPLATE_APPLY(copy_assign_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("move assign", T, move_assign_id)
        {
            SUBCASE("default")
            {
                T initial = fixture<T>::init();
                T expected = fixture<T>::other();
                nullable<T&> d1{initial};
                nullable<T&> d2{expected};
                d2 = std::move(d1);
                REQUIRE(d2.has_value());
                CHECK_EQ(d1.value(), d2.value());
                CHECK_EQ(initial, expected);
                CHECK(fixture<T>::check_move_count(0));
            }

            SUBCASE("with conversion")
            {
                T initial = fixture<T>::init();
                T expected = fixture<T>::other();
                nullable<T> d1{initial};
                nullable<T&> d2{expected};
                d2 = std::move(d1);
                REQUIRE(d2.has_value());
                CHECK_EQ(initial, expected);
                // d1 is not a proxy, therefore it is moved
                CHECK(fixture<T>::check_move_count(1));
            }
        }
        TEST_CASE_TEMPLATE_APPLY(move_assign_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("conversion to bool", T, conversion_id)
        {
            T val = fixture<T>::init();
            nullable<T&> d1(val);
            CHECK(d1);

            d1 = nullval;
            CHECK_FALSE(d1);
        }
        TEST_CASE_TEMPLATE_APPLY(conversion_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("value / get", T, value_id)
        {
            T initial = fixture<T>::init();
            T expected = fixture<T>::other();

            SUBCASE("& overload")
            {
                nullable<T&> d{initial};
                nullable<T&>& d1(d);
                d1.value() = expected;
                CHECK_EQ(d.value(), expected);
                CHECK_EQ(d.get(), expected);
            }

            SUBCASE("const & overload")
            {
                nullable<T&> d{initial};
                const nullable<T&>& d2(d);
                CHECK_EQ(d2.value(), initial);
                CHECK_EQ(d2.get(), initial);
            }

            SUBCASE("&& overload")
            {
                nullable<T&> d{initial};
                nullable<T&>&& d3(std::move(d));
                d3.value() = expected;
                CHECK_EQ(d.value(), expected);
                CHECK_EQ(d.get(), expected);
            }

            SUBCASE("const && overload")
            {
                nullable<T&> d{initial};
                const nullable<T&>&& d4(std::move(d));
                CHECK_EQ(d4.value(), initial);
                CHECK_EQ(d4.get(), initial);
            }

            SUBCASE("empty")
            {
                nullable<T&> empty(initial);
                empty = nullval;
                CHECK_THROWS_AS(empty.value(), bad_nullable_access);
                CHECK_NOTHROW(empty.get());
            }
        }
        TEST_CASE_TEMPLATE_APPLY(value_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("value_or", T, value_or_id)
        {
            T initial = fixture<T>::init();
            T expected = fixture<T>::other();

            nullable<T&> d{initial};
            nullable<T&> empty(initial);
            empty = nullval;

            SUBCASE("const & overload")
            {
                const nullable<T&>& ref(d);
                const nullable<T&>& ref_empty(empty);

                T res = ref.value_or(expected);
                T res_empty = ref_empty.value_or(expected);

                CHECK_EQ(res, initial);
                CHECK_EQ(res_empty, expected);
            }

            SUBCASE("&& overload")
            {
                nullable<T&>&& ref(std::move(d));
                nullable<T&>&& ref_empty(std::move(empty));

                T res = ref.value_or(expected);
                T res_empty = ref_empty.value_or(expected);

                CHECK_EQ(res, initial);
                CHECK_EQ(res_empty, expected);
            }
        }
        TEST_CASE_TEMPLATE_APPLY(value_or_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("swap", T, swap_id)
        {
            T initial = fixture<T>::init();
            T expected = fixture<T>::other();
            T initial_bu = initial;
            T expected_bu = expected;
            T empty_val = T(fixture<T>::convert_init());
            nullable<T&> d1{initial};
            nullable<T&> d2{expected};
            nullable<T&> empty{empty_val};
            empty = nullval;

            swap(d1, d2);
            CHECK_EQ(d1.value(), expected_bu);
            CHECK_EQ(d2.value(), initial_bu);

            swap(d1, empty);
            CHECK_EQ(empty.value(), expected_bu);
            CHECK_FALSE(d1.has_value());
        }
        TEST_CASE_TEMPLATE_APPLY(swap_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("reset", T, reset_id)
        {
            T initial = fixture<T>::init();
            nullable<T&> d{initial};
            d.reset();
            CHECK_FALSE(d.has_value());
        }
        TEST_CASE_TEMPLATE_APPLY(reset_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("equality comparison", T, equality_comparison_id)
        {
            T initial = fixture<T>::init();
            T other = fixture<T>::other();
            T empty_val = T(fixture<T>::convert_init());

            nullable<T&> d1{initial};
            nullable<T&> d2{other};
            nullable<T&> empty{empty_val};
            empty = nullval;

            CHECK(d1 == d1);
            CHECK(d1 == d1.value());
            CHECK(d1 != d2);
            CHECK(d1 != d2.value());
            CHECK(d1 != empty);
            CHECK(empty == empty);
        }
        TEST_CASE_TEMPLATE_APPLY(equality_comparison_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("inequality comparison", T, inequality_comparison_id)
        {
            T initial = fixture<T>::init();
            T other = fixture<T>::other();
            T empty_val = T(fixture<T>::convert_init());

            nullable<T&> d1{initial};
            nullable<T&> d2{other};
            nullable<T&> empty{empty_val};
            empty = nullval;

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
        TEST_CASE_TEMPLATE_APPLY(inequality_comparison_id, testing_types);
#if defined(__cpp_lib_format)
        TEST_CASE_TEMPLATE_DEFINE("formatter", T, formatter_id)
        {
            T initial = fixture<T>::init();
            T other = fixture<T>::other();
            T empty_val = T(fixture<T>::convert_init());

            nullable<T&> d1{initial};
            nullable<T&> d2{other};
            nullable<T&> empty{empty_val};
            empty = nullval;

            if constexpr (std::is_same_v<T, Custom>)
            {
                CHECK_EQ(std::format("{}", d1), "Custom(1)");
                CHECK_EQ(std::format("{}", d2), "Custom(2)");
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                CHECK_EQ(std::format("{}", d1), "And now young codebase ...");
                CHECK_EQ(std::format("{}", d2), "Darth Codius");
            }
            else if (std::is_floating_point_v<T>)
            {
                CHECK_EQ(std::format("{}", d1), "1.2");
                CHECK_EQ(std::format("{}", d2), "2.5");
            }

            CHECK_EQ(std::format("{}", empty), "null");
        }
        TEST_CASE_TEMPLATE_APPLY(formatter_id, testing_types);
#endif
    }

    TEST_SUITE("nullable_variant")
    {
        using nullable_variant_type = nullable_variant<nullable<short>, nullable<int>, nullable<double>>;

        TEST_CASE("has_value")
        {
            nullable<double> d = 1.2;
            nullable_variant_type v = d;
            CHECK(v.has_value());

            nullable_variant_type v2 = nullable<int>();
            CHECK(!v2.has_value());
        }

        TEST_CASE("oprator bool")
        {
            nullable<double> d = 1.2;
            nullable_variant_type v = d;
            CHECK(bool(v));

            nullable_variant_type v2 = nullable<int>();
            CHECK(!bool(v2));
        }

        TEST_CASE("visit")
        {
            double vd = 1.2;
            nullable<double> d = vd;
            nullable_variant_type v = d;

            bool res = std::visit(
                [vd](const auto& val)
                {
                    return val.has_value() && val.value() == vd;
                },
                v
            );
            CHECK(res);
        }

        TEST_CASE("assignment")
        {
            nullable<double> d = 1.2;
            nullable_variant_type nv = d;
            nullable<double> d2 = 2.3;
            nullable_variant_type nv2 = d2;

            CHECK_NE(nv, nv2);
            nv2 = nv;
            CHECK_EQ(nv, nv2);
        }

        TEST_CASE("move assign")
        {
            nullable<double> d = 1.2;
            nullable_variant_type nv = d;
            nullable<double> d2 = 2.3;
            nullable_variant_type nv2 = d2;
            nullable_variant_type nv3(nv);

            CHECK_NE(nv, nv2);
            nv2 = std::move(nv3);
            CHECK_EQ(nv, nv2);
        }
    }
}
