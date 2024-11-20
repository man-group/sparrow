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

#include "sparrow/builder/builder_utils.hpp"
#include "sparrow/builder/nested_less.hpp"

#include "test_utils.hpp"

#include <vector>
#include <tuple>
#include <variant>
#include <array>


namespace sparrow
{   

    TEST_SUITE("nested-less")
    {   
        TEST_CASE("nullable-less")
        {
            using type = nullable<int>;
            using less_type = detail::nested_less<type>;

            CHECK(less_type{}(type{}, type{1}));
            CHECK_FALSE(less_type{}(type{1}, type{}));
            CHECK_FALSE(less_type{}(type{}, type{}));

            CHECK(less_type{}(type{1}, type{2}));
            CHECK_FALSE(less_type{}(type{2}, type{1}));
            CHECK_FALSE(less_type{}(type{1}, type{1}));
        }
        TEST_CASE("tuple")
        {   

            {
                using tuple_type = std::tuple<int>;
                using less_type = detail::nested_less<tuple_type>;
                CHECK(less_type{}(tuple_type{0}, tuple_type{1}));
            }
            {
                using tuple_type = std::tuple<int,int>;
                using less_type = detail::nested_less<tuple_type>;
                CHECK(less_type{}(tuple_type{0, 2}, tuple_type{1, 0}));
            }
            

        }
        TEST_CASE("very-nested-less")
        {
            using tuple_type = std::tuple<nullable<int>, float>;
            using nullable_tuple_type = nullable<tuple_type>;
            using less_type = detail::nested_less<nullable_tuple_type>;

            nullable_tuple_type a{};
            nullable_tuple_type b{tuple_type{nullable<int>{}, 0.0f}};
            nullable_tuple_type c{tuple_type{nullable<int>{1}, 1.0f}};

            CHECK(less_type{}(a, b));
            CHECK_FALSE(less_type{}(b, a));
            CHECK_FALSE(less_type{}(a, a));

            CHECK(less_type{}(b, c));
            CHECK_FALSE(less_type{}(c, b));
            CHECK_FALSE(less_type{}(b, b));

            CHECK(less_type{}(a, c));
            CHECK_FALSE(less_type{}(c, a));
            CHECK_FALSE(less_type{}(c, c));
        }
    }   
    TEST_SUITE("nested-eq")
    {   
        TEST_CASE("nullable-eq")
        {
            using type = nullable<int>;
            using eq_type = detail::nested_less<type>;

            CHECK(eq_type{}(type{}, type{}));
            CHECK_FALSE(eq_type{}(type{}, type{0}));
            CHECK_FALSE(eq_type{}(type{0}, type{}));

            CHECK(eq_type{}(type{1}, type{1}));
            CHECK_FALSE(eq_type{}(type{2}, type{1}));
            CHECK_FALSE(eq_type{}(type{1}, type{2}));
        }
        TEST_CASE("tuple")
        {   

            {
                using tuple_type = std::tuple<int>;
                using eq_type = detail::nested_less<tuple_type>;
                CHECK(eq_type{}(tuple_type{0}, tuple_type{0}));
                CHECK(eq_type{}(tuple_type{1}, tuple_type{1}));
                CHECK_FALSE(eq_type{}(tuple_type{1}, tuple_type{0}));
                CHECK_FALSE(eq_type{}(tuple_type{0}, tuple_type{1}));
            }
            {
                using tuple_type = std::tuple<int,int>;
                using eq_type = detail::nested_less<tuple_type>;
                CHECK(eq_type{}(tuple_type{0, 0}, tuple_type{0, 0}));
                CHECK_FALSE(eq_type{}(tuple_type{0, 1}, tuple_type{0, 1}));
                CHECK_FALSE(eq_type{}(tuple_type{1, 0}, tuple_type{1, 0}));
                CHECK(eq_type{}(tuple_type{1, 1}, tuple_type{1, 1}));
            }
            

        }
        TEST_CASE("very-nested-eq")
        {
            using tuple_type = std::tuple<nullable<int>, float>;
            using nullable_tuple_type = nullable<tuple_type>;
            using eq_type = detail::nested_less<nullable_tuple_type>;

            nullable_tuple_type a{};
            nullable_tuple_type b{tuple_type{nullable<int>{}, 0.0f}};
            nullable_tuple_type c{tuple_type{nullable<int>{1}, 1.0f}};

            CHECK_FALSE(eq_type{}(a, b));
            CHECK_FALSE(eq_type{}(b, a));
            CHECK(eq_type{}(a, a));

            CHECK_FALSE(eq_type{}(b, c));
            CHECK_FALSE(eq_type{}(c, b));
            CHECK(eq_type{}(b, b));

            CHECK_FALSE(eq_type{}(a, c));
            CHECK_FALSE(eq_type{}(c, a));
            CHECK(eq_type{}(c, c));
        }
    }
}

