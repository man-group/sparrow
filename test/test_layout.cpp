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

#include "doctest/doctest.h"

#include "sparrow/layout.hpp"

namespace sparrow
{
    // TODO: Test all the other base types once #15 is addressed.
    static_assert(std::ranges::range<primitive_layout<uint8_t>>);
    static_assert(std::contiguous_iterator<primitive_layout_iterator<uint8_t, true>>);
    static_assert(std::contiguous_iterator<primitive_layout_iterator<uint8_t, false>>);

    using layout_test_type = primitive_layout<uint8_t>;

    namespace
    {
        array_data make_test_array_data()
        {
            size_t n = 10;
            array_data ad;

            ad.type = data_descriptor(data_type::UINT8);
            ad.bitmap = dynamic_bitset<uint8_t>(n);
            buffer<uint8_t> b(n);
            std::iota(b.begin(), b.end(), 0);

            ad.buffers.push_back(b);
            ad.length = n;
            ad.offset = 0;
            ad.child_data.push_back(array_data());
            return ad;
        }

    }

    TEST_SUITE("layout")
    {
        TEST_CASE("constructors")
        {
            array_data ad = make_test_array_data();
            layout_test_type lt(ad);
            REQUIRE(lt.size() == ad.length);
        }

        TEST_CASE("layout_iterator")
        {
            layout_test_type lt(make_test_array_data());
            layout_test_type::iterator it = lt.begin();
            layout_test_type::const_iterator cit = lt.cbegin();
            REQUIRE(it < lt.end());
            REQUIRE(cit < lt.cend());
        }

        TEST_CASE("iterator")
        {
            layout_test_type lt(make_test_array_data());
            auto iter = lt.begin();
            auto citer = lt.cbegin();
            for (std::size_t i = 0; i < lt.size(); ++i)
            {
                CHECK_EQ(*iter++, lt.element(i));
                CHECK_EQ(*citer++, lt.element(i));
            }
            CHECK_EQ(iter, lt.end());
            CHECK_EQ(citer, lt.cend());
        }

    }

}
