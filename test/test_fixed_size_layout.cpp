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
#include <iostream>
#include <numeric>

#include "sparrow/array_data_factory.hpp"
#include "sparrow/fixed_size_layout.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    // TODO: Test all the other base types once #15 is addressed.
    // static_assert(std::ranges::contiguous_range<fixed_size_layout<uint8_t>>);
    // static_assert(std::contiguous_iterator<fixed_size_layout_iterator<uint8_t, true>>);
    // static_assert(std::contiguous_iterator<fixed_size_layout_iterator<uint8_t, false>>);

    using data_type_t = int32_t;
    using layout_test_type = fixed_size_layout<data_type_t>;

    namespace
    {
        array_data make_test_array_data(size_t n = 10, std::int64_t offset = 0)
        {
            std::vector<data_type_t> v(n);
            std::iota(v.begin(), v.end(), -8);
            array_data::bitmap_type bitmap(n, true);
            return default_array_data_factory<layout_test_type>(v, bitmap, offset);
        }

    }

    TEST_SUITE("fixed_size_layout")
    {
        TEST_CASE("constructors")
        {
            array_data ad = make_test_array_data(10, 1);
            layout_test_type lt(ad);
            CHECK_EQ(lt.size(), ad.length - ad.offset);

            auto buffer_data = ad.buffers[0].data<data_type_t>();
            for (std::size_t i = 0; i < lt.size(); ++i)
            {
                CHECK_EQ(lt[i].value(), buffer_data[i + static_cast<size_t>(ad.offset)]);
            }
        }

        TEST_CASE("value_iterator_ordering")
        {
            array_data ad = make_test_array_data(10, 1);
            layout_test_type lt(ad);
            auto lt_values = lt.values();
            layout_test_type::const_value_iterator citer = lt_values.begin();
            CHECK(citer < lt_values.end());
        }

        TEST_CASE("value_iterator_equality")
        {
            array_data ad = make_test_array_data(10, 1);
            layout_test_type lt(ad);
            auto lt_values = lt.values();
            layout_test_type::const_value_iterator citer = lt_values.begin();
            for (std::size_t i = 0; i < lt.size(); ++i)
            {
                CHECK_EQ(*citer++, lt[i]);
            }
            CHECK_EQ(citer, lt_values.end());
        }

        TEST_CASE("const_value_iterator_ordering")
        {
            array_data ad = make_test_array_data(10, 1);
            layout_test_type lt(ad);
            auto lt_values = lt.values();
            layout_test_type::const_value_iterator citer = lt_values.begin();
            CHECK(citer < lt_values.end());
        }

        TEST_CASE("const_value_iterator_equality")
        {
            array_data ad = make_test_array_data(10, 1);
            layout_test_type lt(ad);
            auto lt_values = lt.values();
            for (std::size_t i = 0; i < lt.size(); ++i)
            {
                lt[i] = static_cast<int>(i);
            }

            layout_test_type::const_value_iterator citer = lt_values.begin();
            for (std::size_t i = 0; i < lt.size(); ++i, ++citer)
            {
                CHECK_EQ(*citer, i);
            }
        }

        TEST_CASE("const_bitmap_iterator_ordering")
        {
            array_data ad = make_test_array_data(10, 1);
            layout_test_type lt(ad);
            auto lt_bitmap = lt.bitmap();
            layout_test_type::const_bitmap_iterator citer = lt_bitmap.begin();
            CHECK(citer < lt_bitmap.end());
        }

        TEST_CASE("const_bitmap_iterator_equality")
        {
            array_data ad = make_test_array_data(10, 1);
            layout_test_type lt(ad);
            auto lt_bitmap = lt.bitmap();
            for (std::size_t i = 0; i < lt.size(); ++i)
            {
                if (i % 2 != 0)
                {
                    lt[i] = std::nullopt;
                }
            }

            layout_test_type::const_bitmap_iterator citer = lt_bitmap.begin();
            for (std::size_t i = 0; i < lt.size(); ++i, ++citer)
            {
                CHECK_EQ(*citer, i % 2 == 0);
            }
        }

        TEST_CASE("iterator")
        {
            array_data ad = make_test_array_data(10, 1);
            layout_test_type lt(ad);
            auto it = lt.begin();
            auto end = lt.end();

            for (std::size_t i = 0; i != lt.size(); ++it, ++i)
            {
                CHECK_EQ(*it, std::make_optional(lt[i].value()));
                CHECK(it->has_value());
            }

            CHECK_EQ(it, end);

            for (auto v : lt)
            {
                CHECK(v.has_value());
            }

            array_data ad_empty = make_test_array_data(0, 0);
            layout_test_type lt_empty(ad_empty);
            CHECK_EQ(lt_empty.begin(), lt_empty.end());
        }
    }

}
