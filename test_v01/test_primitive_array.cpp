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

#include <cstdint>
#include <numeric>
#include <ranges>

#include "sparrow/array/data_traits.hpp"

#include "doctest/doctest.h"
#include "sparrow_v01/arrow_interface/arrow_array_schema_factory.hpp"
#include "sparrow_v01/layout/primitive_array.hpp"

template <typename T,std::ranges::input_range Nulls>
sparrow::arrow_proxy make_arrow_proxy(size_t length, size_t offset, const Nulls& nulls)
{
    std::vector<T> values(length, 0);
    std::iota(values.begin(), values.end(), T());
    const sparrow::data_type values_data_type = sparrow::arrow_traits<T>::type_id;
    return sparrow::arrow_proxy{
        sparrow::make_primitive_arrow_array(values, nulls, static_cast<int64_t>(offset)),
        sparrow::make_arrow_schema(
            data_type_to_format(values_data_type),
            "primitive values",
            std::nullopt,
            std::nullopt,
            0,
            nullptr,
            nullptr
        )
    };
}

namespace sparrow
{
    TEST_SUITE("primitive_array")
    {
        constexpr std::size_t size = 10u;
        constexpr std::size_t offset = 1u;
        constexpr std::array<size_t, 1> nulls{4ULL};

        TEST_CASE_TEMPLATE_DEFINE("scalar", T, generic_scalar_test)
        {
            SUBCASE("constructor")
            {
                primitive_array<T> ar(make_arrow_proxy<T>(size, offset, nulls));
                CHECK_EQ(ar.size(), size - offset);
            }

            SUBCASE("const operator[]")
            {
                const primitive_array<T> ar(make_arrow_proxy<T>(size, offset, nulls));
                std::vector<T> ref(size - offset);
                REQUIRE_EQ(ar.size(), size - offset);
                REQUIRE(ar[0].has_value());
                CHECK_EQ(ar[0].value(), T(1));
                REQUIRE(ar[1].has_value());
                CHECK_EQ(ar[1].value(), T(2));
                REQUIRE(ar[2].has_value());
                CHECK_EQ(ar[2].value(), T(3));
                CHECK_FALSE(ar[3].has_value());
                REQUIRE(ar[4].has_value());
                CHECK_EQ(ar[4].value(), T(5));
                REQUIRE(ar[5].has_value());
                CHECK_EQ(ar[5].value(), T(6));
                REQUIRE(ar[6].has_value());
                CHECK_EQ(ar[6].value(), T(7));
                REQUIRE(ar[7].has_value());
                CHECK_EQ(ar[7].value(), T(8));
                REQUIRE(ar[8].has_value());
                CHECK_EQ(ar[8].value(), T(9));
            }

            SUBCASE("value_iterator_ordering")
            {
                const primitive_array<T> ar(make_arrow_proxy<T>(size, offset, nulls));
                auto ar_values = ar.values();
                auto citer = ar_values.begin();
                CHECK(citer < ar_values.end());
            }

            SUBCASE("value_iterator_equality")
            {
                const primitive_array<T> ar(make_arrow_proxy<T>(size, offset, nulls));
                auto ar_values = ar.values();
                auto citer = ar_values.begin();
                for (std::size_t i = 0; i < ar.size(); ++i)
                {
                    if (ar[i].has_value())
                    {
                        CHECK_EQ(*citer, ar[i].value());
                    }
                    citer++;
                }
                CHECK_EQ(citer, ar_values.end());
            }

            SUBCASE("const_value_iterator_ordering")
            {
                const primitive_array<T> ar(make_arrow_proxy<T>(size, offset, nulls));
                auto ar_values = ar.values();
                auto citer = ar_values.begin();
                CHECK(citer < ar_values.end());
            }

            SUBCASE("const_value_iterator_equality")
            {
                primitive_array<T> ar(make_arrow_proxy<T>(size, offset, nulls));
                auto ar_values = ar.values();
                auto citer = ar_values.begin();
                for (std::size_t i = 0; i < ar.size(); ++i, ++citer)
                {
                    CHECK_EQ(*citer, static_cast<T>(i + 1));
                }
            }

            SUBCASE("const_bitmap_iterator_ordering")
            {
                const primitive_array<T> ar(make_arrow_proxy<T>(size, offset, nulls));
                auto ar_bitmap = ar.bitmap();
                auto citer = ar_bitmap.begin();
                CHECK(citer < ar_bitmap.end());
            }

            SUBCASE("const_bitmap_iterator_equality")
            {
                const primitive_array<T> ar(make_arrow_proxy<T>(size, offset, nulls));
                auto citer = ar.bitmap().begin();
                for (std::size_t i = 0; i < ar.size(); ++i, ++citer)
                {
                    CHECK_EQ(*citer, std::ranges::find(nulls, i + offset) == nulls.end());
                }
            }

            SUBCASE("iterator")
            {
                const primitive_array<T> ar(make_arrow_proxy<T>(size, offset, nulls));
                auto it = ar.begin();
                auto end = ar.end();

                for (std::size_t i = 0; i != ar.size(); ++it, ++i)
                {
                    if (std::ranges::find(nulls, i + offset) != nulls.end())
                    {
                        CHECK_FALSE(it->has_value());
                    }
                    else
                    {
                        CHECK(it->has_value());
                        CHECK_EQ(*it, make_nullable(static_cast<T>(i + 1)));
                    }
                }

                CHECK_EQ(it, end);

                size_t i = 0;
                for (auto v : ar)
                {
                    CHECK_EQ(v.has_value(), std::ranges::find(nulls, i + offset) == nulls.end());
                    i++;
                }

                const primitive_array<T> ar_empty(make_arrow_proxy<T>(0, 0, std::array<size_t, 0>{}));
                CHECK_EQ(ar_empty.begin(), ar_empty.end());
            }
        }
        TEST_CASE_TEMPLATE_INVOKE(
            generic_scalar_test,
            // bool,
            std::uint8_t,
            std::int8_t,
            std::uint16_t,
            std::int16_t,
            std::uint32_t,
            std::int32_t,
            std::uint64_t,
            std::int64_t,
            // float16_t, // TODO: To fix
            float32_t,
            float64_t
        );
    }
}
