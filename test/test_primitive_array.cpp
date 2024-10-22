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

#include "../test/external_array_data_creation.hpp"
#include "doctest/doctest.h"
#include "sparrow/layout/primitive_array.hpp"


namespace sparrow
{
    using scalar_value_type = std::int32_t;
    using array_test_type = primitive_array<scalar_value_type>;
    using test::make_arrow_proxy;

    TEST_SUITE("primitive_array")
    {
        constexpr std::size_t size = 10u;
        constexpr std::size_t offset = 1u;

        TEST_CASE("constructor")
        {
            auto pr = make_arrow_proxy<scalar_value_type>(size, offset);
            array_test_type ar(std::move(pr));
            CHECK_EQ(ar.size(), size - offset);
        }

        TEST_CASE("copy")
        {
            array_test_type ar(make_arrow_proxy<scalar_value_type>(size, offset));
            array_test_type ar2(ar);

            CHECK_EQ(ar, ar2);

            array_test_type ar3(make_arrow_proxy<scalar_value_type>(size + 3u, offset));
            CHECK_NE(ar, ar3);
            ar3 = ar;
            CHECK_EQ(ar, ar3);
        }

        TEST_CASE("move")
        {
            array_test_type ar(make_arrow_proxy<scalar_value_type>(size, offset));
            array_test_type ar2(ar);

            array_test_type ar3(std::move(ar));
            CHECK_EQ(ar2, ar3);

            array_test_type ar4(make_arrow_proxy<scalar_value_type>(size + 3u, offset));
            CHECK_NE(ar2, ar4);
            ar4 = std::move(ar2);
            CHECK_EQ(ar3, ar4);
        }

        TEST_CASE("const operator[]")
        {
            auto pr = make_arrow_proxy<scalar_value_type>(size, offset);
            std::vector<scalar_value_type> ref(size - offset);
            std::copy(
                pr.buffers()[1].data<scalar_value_type>() + offset,
                pr.buffers()[1].data<scalar_value_type>() + size,
                ref.begin()
            );
            array_test_type ar(std::move(pr));
            const array_test_type& car = ar;
            for (std::size_t i = 0; i < ref.size(); ++i)
            {
                CHECK_EQ(ar[i], ref[i]);
                CHECK_EQ(car[i], ref[i]);
            }
        }

        TEST_CASE("value_iterator_ordering")
        {
            array_test_type ar(make_arrow_proxy<scalar_value_type>(size, offset));
            auto ar_values = ar.values();
            array_test_type::const_value_iterator citer = ar_values.begin();
            CHECK(citer < ar_values.end());
        }

        TEST_CASE("value_iterator_equality")
        {
            array_test_type ar(make_arrow_proxy<scalar_value_type>(size, offset));
            auto ar_values = ar.values();
            array_test_type::const_value_iterator citer = ar_values.begin();
            for (std::size_t i = 0; i < ar.size(); ++i)
            {
                CHECK_EQ(*citer++, ar[i]);
            }
            CHECK_EQ(citer, ar_values.end());
        }

        TEST_CASE("const_value_iterator_ordering")
        {
            array_test_type ar(make_arrow_proxy<scalar_value_type>(size, offset));
            auto ar_values = ar.values();
            array_test_type::const_value_iterator citer = ar_values.begin();
            CHECK(citer < ar_values.end());
        }

        TEST_CASE("const_value_iterator_equality")
        {
            array_test_type ar(make_arrow_proxy<scalar_value_type>(size, offset));
            auto ar_values = ar.values();
            for (std::size_t i = 0; i < ar.size(); ++i)
            {
                ar[i] = static_cast<scalar_value_type>(i);
            }

            array_test_type::const_value_iterator citer = ar_values.begin();
            for (std::size_t i = 0; i < ar.size(); ++i, ++citer)
            {
                CHECK_EQ(*citer, i);
            }
        }

        TEST_CASE("const_bitmap_iterator_ordering")
        {
            array_test_type ar(make_arrow_proxy<scalar_value_type>(size, offset));
            auto ar_bitmap = ar.bitmap();
            array_test_type::const_bitmap_iterator citer = ar_bitmap.begin();
            CHECK(citer < ar_bitmap.end());
        }

        TEST_CASE("const_bitmap_iterator_equality")
        {
            array_test_type ar(make_arrow_proxy<scalar_value_type>(size, offset));
            auto ar_bitmap = ar.bitmap();
            for (std::size_t i = 0; i < ar.size(); ++i)
            {
                if (i % 2 != 0)
                {
                    ar[i] = nullval;
                }
            }

            array_test_type::const_bitmap_iterator citer = ar_bitmap.begin();
            for (std::size_t i = 0; i < ar.size(); ++i, ++citer)
            {
                CHECK_EQ(*citer, i % 2 == 0);
            }
        }

        TEST_CASE("iterator")
        {
            array_test_type ar(make_arrow_proxy<scalar_value_type>(size, offset));
            auto it = ar.begin();
            auto end = ar.end();

            for (std::size_t i = 0; i != ar.size(); ++it, ++i)
            {
                CHECK_EQ(*it, make_nullable(ar[i].value()));
                CHECK(it->has_value());
            }

            CHECK_EQ(it, end);

            for (auto v : ar)
            {
                CHECK(v.has_value());
        }

            array_test_type ar_empty(make_arrow_proxy<scalar_value_type>(0, 0));
            CHECK_EQ(ar_empty.begin(), ar_empty.end());
        }
        TEST_CASE_TEMPLATE("convenience_constructors", T, std::uint8_t) 
        {
            using inner_value_type = T;

            std::vector<inner_value_type> data = {
                static_cast<inner_value_type>(0), 
                static_cast<inner_value_type>(1), 
                static_cast<inner_value_type>(2),
                static_cast<inner_value_type>(3)
            };
            SUBCASE("range-of-inner-values") {
                primitive_array<T> arr(data);
                CHECK_EQ(arr.size(), data.size());
                for (std::size_t i = 0; i < data.size(); ++i) {
                    REQUIRE(arr[i].has_value());
                    CHECK_EQ(arr[i].value(), data[i]);
                }
            }
            SUBCASE("range-of-nullables")
            {
                using nullable_type = nullable<inner_value_type>;
                std::vector<nullable_type> nullable_vector{
                    nullable_type(data[0]),
                    nullable_type(data[1]),
                    nullval,
                    nullable_type(data[3])
                };
                primitive_array<T> arr(nullable_vector);
                REQUIRE(arr.size() == nullable_vector.size());
                REQUIRE(arr[0].has_value());
                REQUIRE(arr[1].has_value());
                REQUIRE(!arr[2].has_value());
                REQUIRE(arr[3].has_value());
                CHECK_EQ(arr[0].value(), data[0]);
                CHECK_EQ(arr[1].value(), data[1]);
                CHECK_EQ(arr[3].value(), data[3]);
            }
            SUBCASE("from iota")
            {   
                primitive_array<T> arr(std::ranges::iota_view{std::size_t(0), std::size_t(4)});
                REQUIRE(arr.size() == 4);
                for (std::size_t i = 0; i < 4; ++i) {
                    REQUIRE(arr[i].has_value());
                    CHECK_EQ(arr[i].value(), static_cast<inner_value_type>(i));
                }
            }
        }
    }
}
