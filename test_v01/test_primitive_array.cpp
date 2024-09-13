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
#include "sparrow_v01/layout/primitive_array.hpp"


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
    }
}
