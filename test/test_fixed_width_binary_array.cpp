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

#include <algorithm>
#include <cstddef>

#include "sparrow/array.hpp"
#include "sparrow/layout/fixed_width_binary_array.hpp"
#include "sparrow/types/data_type.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    TEST_SUITE("fixed_width_binary_array")
    {
        static const auto make_array =
            [](size_t count,
               size_t offset = 0) -> std::pair<fixed_width_binary_array, std::vector<std::array<byte_t, 3>>>
        {
            std::vector<std::array<byte_t, 3>> input_values;
            input_values.reserve(count);
            for (size_t i = 0; i < count; ++i)
            {
                const uint8_t val1 = static_cast<uint8_t>(i);
                const uint8_t val2 = static_cast<uint8_t>(i + 1);
                const uint8_t val3 = static_cast<uint8_t>(i + 2);
                input_values.push_back(std::array<byte_t, 3>{byte_t{val1}, byte_t{val2}, byte_t{val3}});
            }

            fixed_width_binary_array arr(
                input_values,
                count > 2 ? std::vector<std::size_t>{2} : std::vector<std::size_t>{}
            );
            if (offset != 0)
            {
                return {std::move(arr).slice(offset, arr.size()), input_values};
            }
            else
            {
                return {std::move(arr), input_values};
            }
        };

        static const auto new_value_1 = std::array<byte_t, 3>{byte_t{99}, byte_t{100}, byte_t{101}};
        static const auto new_nullable_value_1 = nullable<const std::array<byte_t, 3>>(new_value_1, true);
        static const auto new_value_2 = std::array<byte_t, 3>{byte_t{102}, byte_t{103}, byte_t{104}};
        static const auto new_nullable_value_2 = nullable<const std::array<byte_t, 3>>(new_value_2, true);
        static const auto new_value_3 = std::array<byte_t, 3>{byte_t{105}, byte_t{106}, byte_t{107}};
        static const auto new_nullable_value_3 = nullable<const std::array<byte_t, 3>>(new_value_3, true);
        static const auto new_values = std::array<std::array<byte_t, 3>, 3>{new_value_1, new_value_2, new_value_3};
        static const auto new_nullable_values = std::array<nullable<std::array<byte_t, 3>>, 3>{
            new_nullable_value_1,
            new_nullable_value_2,
            new_nullable_value_3
        };

        TEST_CASE("constructor")
        {
            SUBCASE("basic")
            {
                const auto [ar, input_values] = make_array(5, 1);
                CHECK_EQ(ar.size(), 4);
            }

            SUBCASE("empty range")
            {
                const fixed_width_binary_array ar(std::vector<std::array<byte_t, 3>>{});
            }

            SUBCASE("range with empty values")
            {
                constexpr std::array<byte_t, 0> empty_value;
                const fixed_width_binary_array ar(
                    std::vector<std::array<byte_t, 0>>{empty_value, empty_value, empty_value}
                );
                CHECK_EQ(ar.size(), 3);
                CHECK(std::ranges::equal(ar[0].get(), empty_value));
            }

            SUBCASE("values range and nullable")
            {
                SUBCASE("nullable == true")
                {
                    const fixed_width_binary_array ar(new_values, true);
                    CHECK_EQ(ar.size(), 3);
                    for (size_t i = 0; i < ar.size(); ++i)
                    {
                        CHECK(std::ranges::equal(ar[i].get(), new_values[i]));
                    }
                }

                SUBCASE("nullable == false")
                {
                    const fixed_width_binary_array ar(new_values, false);
                    CHECK_EQ(ar.size(), 3);
                    for (size_t i = 0; i < ar.size(); ++i)
                    {
                        CHECK(std::ranges::equal(ar[i].get(), new_values[i]));
                    }
                }
            }
        }

        TEST_CASE("operator[]")
        {
            SUBCASE("const")
            {
                const auto [ar, input_values] = make_array(5, 1);
                REQUIRE_EQ(ar.size(), 4);
                CHECK(ar[0].has_value());
                CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
                CHECK_FALSE(ar[1].has_value());
                CHECK(std::ranges::equal(ar[1].get(), input_values[2]));
                CHECK(ar[2].has_value());
                CHECK(std::ranges::equal(ar[2].get(), input_values[3]));
                CHECK(ar[3].has_value());
                CHECK(std::ranges::equal(ar[3].get(), input_values[4]));
            }

            SUBCASE("mutable")
            {
                auto [ar, input_values] = make_array(5, 1);
                REQUIRE_EQ(ar.size(), 4);
                CHECK(ar[0].has_value());
                CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
                CHECK_FALSE(ar[1].has_value());
                CHECK(std::ranges::equal(ar[1].get(), input_values[2]));
                CHECK(ar[2].has_value());
                CHECK(std::ranges::equal(ar[2].get(), input_values[3]));
                CHECK(ar[3].has_value());
                CHECK(std::ranges::equal(ar[3].get(), input_values[4]));

                ar[1] = new_nullable_value_1;
                CHECK(ar[1].has_value());
                CHECK(std::ranges::equal(ar[1].get(), new_value_1));
            }
        }

        TEST_CASE("front")
        {
            SUBCASE("const")
            {
                const auto [ar, input_values] = make_array(5, 1);
                REQUIRE_EQ(ar.size(), 4);
                CHECK(ar.front().has_value());
                CHECK(std::ranges::equal(ar.front().value(), input_values[1]));
            }
        }

        TEST_CASE("back")
        {
            SUBCASE("const")
            {
                const auto [ar, input_values] = make_array(5, 1);
                REQUIRE_EQ(ar.size(), 4);
                CHECK(ar.back().has_value());
                CHECK(std::ranges::equal(ar.back().value(), input_values[4]));
            }
        }

        TEST_CASE("copy")
        {
            const auto [ar, input_values] = make_array(5, 1);
            fixed_width_binary_array ar2(ar);

            CHECK_EQ(ar, ar2);

            fixed_width_binary_array ar3(make_array(7, 1).first);
            CHECK_NE(ar, ar3);
            ar3 = ar;
            CHECK_EQ(ar, ar3);
        }

        TEST_CASE("move")
        {
            auto [ar, input_values] = make_array(5, 1);
            fixed_width_binary_array ar2(ar);

            fixed_width_binary_array ar3(std::move(ar));
            CHECK_EQ(ar2, ar3);

            fixed_width_binary_array ar4(make_array(7, 1).first);
            CHECK_NE(ar2, ar4);
            ar4 = std::move(ar2);
            CHECK_EQ(ar3, ar4);
        }

        TEST_CASE("value_iterator_ordering")
        {
            auto [ar, input_values] = make_array(5, 1);
            auto ar_values = ar.values();
            auto citer = ar_values.begin();
            CHECK(citer < ar_values.end());
        }

        TEST_CASE("value_iterator_equality")
        {
            auto [ar, input_values] = make_array(5, 1);
            const auto ar_values = ar.values();
            auto citer = ar_values.begin();
            CHECK(std::ranges::equal(*citer, input_values[1]));
            ++citer;
            CHECK(std::ranges::equal(*citer, input_values[2]));
            ++citer;
            CHECK(std::ranges::equal(*citer, input_values[3]));
            ++citer;
            CHECK(std::ranges::equal(*citer, input_values[4]));
            ++citer;
            CHECK_EQ(citer, ar_values.end());
        }

        TEST_CASE("const_value_iterator_ordering")
        {
            const auto [ar, input_values] = make_array(5, 1);
            auto ar_values = ar.values();
            auto citer = ar_values.begin();
            CHECK(citer < ar_values.end());
        }

        TEST_CASE("const_value_iterator_equality")
        {
            const auto [ar, input_values] = make_array(5, 1);
            auto ar_values = ar.values();
            auto citer = ar_values.begin();
            CHECK(std::ranges::equal(*citer, input_values[1]));
            ++citer;
            CHECK(std::ranges::equal(*citer, input_values[2]));
            ++citer;
            CHECK(std::ranges::equal(*citer, input_values[3]));
            ++citer;
            CHECK(std::ranges::equal(*citer, input_values[4]));
            ++citer;
            CHECK_EQ(citer, ar_values.end());
        }

        TEST_CASE("const_bitmap_iterator_ordering")
        {
            const auto [ar, input_values] = make_array(5, 1);
            const auto ar_bitmap = ar.bitmap();
            const auto citer = ar_bitmap.begin();
            CHECK(citer < ar_bitmap.end());
        }

        TEST_CASE("const_bitmap_iterator_equality")
        {
            const auto [ar, input_values] = make_array(5, 1);
            const auto ar_bitmap = ar.bitmap();
            auto citer = ar_bitmap.begin();
            CHECK(*citer);
            ++citer;
            CHECK_FALSE(*citer);
            ++citer;
            CHECK(*citer);
            ++citer;
            CHECK(*citer);
            ++citer;
        }

        TEST_CASE("iterator")
        {
            const auto [ar, input_values] = make_array(5, 1);
            auto it = ar.begin();
            const auto end = ar.end();
            CHECK(it->has_value());
            CHECK(std::ranges::equal(it->value(), input_values[1]));
            ++it;
            CHECK_FALSE(it->has_value());
            CHECK(std::ranges::equal(it->get(), input_values[2]));
            ++it;
            CHECK(it->has_value());
            CHECK(std::ranges::equal(it->value(), input_values[3]));
            ++it;
            CHECK(it->has_value());
            CHECK(std::ranges::equal(it->value(), input_values[4]));
            ++it;

            CHECK_EQ(it, end);
        }

        TEST_CASE("revert_iterator")
        {
            auto [ar, input_values] = make_array(5, 1);
            auto rit = ar.rbegin();
            const auto rend = ar.rend();
            CHECK(rit->has_value());
            CHECK(std::ranges::equal(rit->get(), input_values[4]));
            ++rit;
            CHECK(rit->has_value());
            CHECK(std::ranges::equal(rit->get(), input_values[3]));
            ++rit;
            CHECK_FALSE(rit->has_value());
            CHECK(std::ranges::equal(rit->get(), input_values[2]));
            ++rit;
            CHECK(rit->has_value());
            CHECK(std::ranges::equal(rit->get(), input_values[1]));
            ++rit;

            CHECK_EQ(rit, rend);
        }

        TEST_CASE("resize")
        {
            auto [ar, input_values] = make_array(5, 1);
            ar.resize(7, new_nullable_value_1);
            REQUIRE_EQ(ar.size(), 7);
            CHECK(ar[0].has_value());
            CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
            CHECK_FALSE(ar[1].has_value());
            CHECK(std::ranges::equal(ar[1].get(), input_values[2]));
            CHECK(ar[2].has_value());
            CHECK(std::ranges::equal(ar[2].get(), input_values[3]));
            CHECK(ar[3].has_value());
            CHECK(std::ranges::equal(ar[3].get(), input_values[4]));
            CHECK_EQ(ar[4], new_nullable_value_1);
            CHECK_EQ(ar[5], new_nullable_value_1);
            CHECK_EQ(ar[6], new_nullable_value_1);
        }

        TEST_CASE("insert")
        {
            auto [ar, input_values] = make_array(5, 1);
            SUBCASE("with pos and value")
            {
                SUBCASE("at the beginning")
                {
                    const auto pos = ar.cbegin();
                    const auto iter = ar.insert(pos, new_nullable_value_1);
                    CHECK_EQ(iter, ar.begin());
                    REQUIRE_EQ(ar.size(), 5);
                    CHECK_EQ(ar[0], new_nullable_value_1);
                    CHECK(ar[1].has_value());
                    CHECK(std::ranges::equal(ar[1].get(), input_values[1]));
                    CHECK_FALSE(ar[2].has_value());
                    CHECK(std::ranges::equal(ar[2].get(), input_values[2]));
                    CHECK(ar[3].has_value());
                    CHECK(std::ranges::equal(ar[3].get(), input_values[3]));
                    CHECK(ar[4].has_value());
                    CHECK(std::ranges::equal(ar[4].get(), input_values[4]));
                }

                SUBCASE("in the middle")
                {
                    const auto pos = sparrow::next(ar.cbegin(), 1);
                    const auto iter = ar.insert(pos, new_nullable_value_1);
                    CHECK_EQ(iter, ar.begin() + 1);
                    REQUIRE_EQ(ar.size(), 5);
                    CHECK(ar[0].has_value());
                    CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
                    CHECK_EQ(ar[1], new_nullable_value_1);
                    CHECK_FALSE(ar[2].has_value());
                    CHECK(std::ranges::equal(ar[2].get(), input_values[2]));
                    CHECK(ar[3].has_value());
                    CHECK(std::ranges::equal(ar[3].get(), input_values[3]));
                    CHECK(ar[4].has_value());
                    CHECK(std::ranges::equal(ar[4].get(), input_values[4]));
                }

                SUBCASE("at the end")
                {
                    const auto pos = ar.cend();
                    const auto iter = ar.insert(pos, new_nullable_value_1);
                    CHECK_EQ(iter, ar.begin() + 4);
                    REQUIRE_EQ(ar.size(), 5);
                    CHECK(ar[0].has_value());
                    CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
                    CHECK_FALSE(ar[1].has_value());
                    CHECK(std::ranges::equal(ar[1].get(), input_values[2]));
                    CHECK(ar[2].has_value());
                    CHECK(std::ranges::equal(ar[2].get(), input_values[3]));
                    CHECK(ar[3].has_value());
                    CHECK(std::ranges::equal(ar[3].get(), input_values[4]));
                    CHECK_EQ(ar[4], new_nullable_value_1);
                }
            }

            SUBCASE("with pos, count and value")
            {
                SUBCASE("at the beginning")
                {
                    const auto pos = ar.cbegin();
                    const auto iter = ar.insert(pos, new_nullable_value_1, 3);
                    CHECK_EQ(iter, ar.begin());
                    REQUIRE_EQ(ar.size(), 7);
                    CHECK_EQ(ar[0], new_nullable_value_1);
                    CHECK_EQ(ar[1], new_nullable_value_1);
                    CHECK_EQ(ar[2], new_nullable_value_1);
                    CHECK(ar[3].has_value());
                    CHECK(std::ranges::equal(ar[3].get(), input_values[1]));
                    CHECK_FALSE(ar[4].has_value());
                    CHECK(std::ranges::equal(ar[4].get(), input_values[2]));
                    CHECK(ar[5].has_value());
                    CHECK(std::ranges::equal(ar[5].get(), input_values[3]));
                    CHECK(ar[6].has_value());
                    CHECK(std::ranges::equal(ar[6].get(), input_values[4]));
                }

                SUBCASE("in the middle")
                {
                    const auto pos = sparrow::next(ar.cbegin(), 1);
                    const auto iter = ar.insert(pos, new_nullable_value_1, 3);
                    CHECK_EQ(iter, ar.begin() + 1);
                    REQUIRE_EQ(ar.size(), 7);
                    CHECK(ar[0].has_value());
                    CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
                    CHECK_EQ(ar[1], new_nullable_value_1);
                    CHECK_EQ(ar[2], new_nullable_value_1);
                    CHECK_EQ(ar[3], new_nullable_value_1);
                    CHECK_FALSE(ar[4].has_value());
                    CHECK(std::ranges::equal(ar[4].get(), input_values[2]));
                    CHECK(ar[5].has_value());
                    CHECK(std::ranges::equal(ar[5].get(), input_values[3]));
                    CHECK(ar[6].has_value());
                    CHECK(std::ranges::equal(ar[6].get(), input_values[4]));
                }

                SUBCASE("at the end")
                {
                    const auto pos = ar.cend();
                    const auto iter = ar.insert(pos, new_nullable_value_1, 3);
                    CHECK_EQ(iter, ar.begin() + 4);
                    REQUIRE_EQ(ar.size(), 7);
                    CHECK(ar[0].has_value());
                    CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
                    CHECK_FALSE(ar[1].has_value());
                    CHECK(std::ranges::equal(ar[1].get(), input_values[2]));
                    CHECK(ar[2].has_value());
                    CHECK(std::ranges::equal(ar[2].get(), input_values[3]));
                    CHECK(ar[3].has_value());
                    CHECK(std::ranges::equal(ar[3].get(), input_values[4]));
                    CHECK_EQ(ar[4], new_nullable_value_1);
                    CHECK_EQ(ar[5], new_nullable_value_1);
                    CHECK_EQ(ar[6], new_nullable_value_1);
                }
            }

            SUBCASE("with pos, initializer list")
            {
                SUBCASE("at the beginning")
                {
                    const auto pos = ar.cbegin();
                    const auto iter = ar.insert(
                        pos,
                        {new_nullable_value_1, new_nullable_value_2, new_nullable_value_3}
                    );
                    CHECK_EQ(iter, ar.begin());
                    REQUIRE_EQ(ar.size(), 7);
                    CHECK(ar[0].has_value());
                    CHECK(std::ranges::equal(ar[0].get(), new_value_1));
                    CHECK(ar[1].has_value());
                    CHECK(std::ranges::equal(ar[1].get(), new_value_2));
                    CHECK(ar[2].has_value());
                    CHECK(std::ranges::equal(ar[2].get(), new_value_3));
                    CHECK(ar[3].has_value());
                    CHECK(std::ranges::equal(ar[3].get(), input_values[1]));
                    CHECK_FALSE(ar[4].has_value());
                    CHECK(std::ranges::equal(ar[4].get(), input_values[2]));
                    CHECK(ar[5].has_value());
                    CHECK(std::ranges::equal(ar[5].get(), input_values[3]));
                    CHECK(ar[6].has_value());
                    CHECK(std::ranges::equal(ar[6].get(), input_values[4]));
                }

                SUBCASE("in the middle")
                {
                    const auto pos = sparrow::next(ar.cbegin(), 1);
                    const auto iter = ar.insert(
                        pos,
                        {new_nullable_value_1, new_nullable_value_2, new_nullable_value_3}
                    );
                    CHECK_EQ(iter, ar.begin() + 1);
                    REQUIRE_EQ(ar.size(), 7);
                    CHECK(ar[0].has_value());
                    CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
                    CHECK(ar[1].has_value());
                    CHECK(std::ranges::equal(ar[1].get(), new_value_1));
                    CHECK(ar[2].has_value());
                    CHECK(std::ranges::equal(ar[2].get(), new_value_2));
                    CHECK(ar[3].has_value());
                    CHECK(std::ranges::equal(ar[3].get(), new_value_3));
                    CHECK_FALSE(ar[4].has_value());
                    CHECK(std::ranges::equal(ar[4].get(), input_values[2]));
                    CHECK(ar[5].has_value());
                    CHECK(std::ranges::equal(ar[5].get(), input_values[3]));
                    CHECK(ar[6].has_value());
                    CHECK(std::ranges::equal(ar[6].get(), input_values[4]));
                }

                SUBCASE("at the end")
                {
                    const auto pos = ar.cend();
                    const auto iter = ar.insert(
                        pos,
                        {new_nullable_value_1, new_nullable_value_2, new_nullable_value_3}
                    );
                    CHECK_EQ(iter, ar.begin() + 4);
                    REQUIRE_EQ(ar.size(), 7);
                    CHECK(ar[0].has_value());
                    CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
                    CHECK_FALSE(ar[1].has_value());
                    CHECK(std::ranges::equal(ar[1].get(), input_values[2]));
                    CHECK(ar[2].has_value());
                    CHECK(std::ranges::equal(ar[2].get(), input_values[3]));
                    CHECK(ar[3].has_value());
                    CHECK(std::ranges::equal(ar[3].get(), input_values[4]));
                    CHECK(ar[4].has_value());
                    CHECK(std::ranges::equal(ar[4].get(), new_value_1));
                    CHECK(ar[5].has_value());
                    CHECK(std::ranges::equal(ar[5].get(), new_value_2));
                    CHECK(ar[6].has_value());
                    CHECK(std::ranges::equal(ar[6].get(), new_value_3));
                }
            }

            SUBCASE("with pos and range")
            {
                SUBCASE("at the beginning")
                {
                    const auto pos = ar.cbegin();
                    const auto iter = ar.insert(pos, new_nullable_values);
                    CHECK_EQ(iter, ar.begin());
                    REQUIRE_EQ(ar.size(), 7);
                    CHECK(ar[0].has_value());
                    CHECK_EQ(ar[0], new_nullable_values[0]);
                    CHECK_EQ(ar[1], new_nullable_values[1]);
                    CHECK_EQ(ar[2], new_nullable_values[2]);
                    CHECK(ar[3].has_value());
                    CHECK(std::ranges::equal(ar[3].get(), input_values[1]));
                    CHECK_FALSE(ar[4].has_value());
                    CHECK(std::ranges::equal(ar[4].get(), input_values[2]));
                    CHECK(ar[5].has_value());
                    CHECK(std::ranges::equal(ar[5].get(), input_values[3]));
                    CHECK(ar[6].has_value());
                    CHECK(std::ranges::equal(ar[6].get(), input_values[4]));
                }

                SUBCASE("in the middle")
                {
                    const auto pos = sparrow::next(ar.cbegin(), 1);
                    const auto iter = ar.insert(pos, new_nullable_values);
                    CHECK_EQ(iter, ar.begin() + 1);
                    REQUIRE_EQ(ar.size(), 7);
                    CHECK(ar[0].has_value());
                    CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
                    CHECK(ar[1].has_value());
                    CHECK_EQ(ar[1], new_nullable_values[0]);
                    CHECK_EQ(ar[2], new_nullable_values[1]);
                    CHECK_EQ(ar[3], new_nullable_values[2]);
                    CHECK_FALSE(ar[4].has_value());
                    CHECK(std::ranges::equal(ar[4].get(), input_values[2]));
                    CHECK(ar[5].has_value());
                    CHECK(std::ranges::equal(ar[5].get(), input_values[3]));
                    CHECK(ar[6].has_value());
                    CHECK(std::ranges::equal(ar[6].get(), input_values[4]));
                }

                SUBCASE("at the end")
                {
                    const auto pos = ar.cend();
                    const auto iter = ar.insert(pos, new_nullable_values);
                    CHECK_EQ(iter, ar.begin() + 4);
                    REQUIRE_EQ(ar.size(), 7);
                    CHECK(ar[0].has_value());
                    CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
                    CHECK_FALSE(ar[1].has_value());
                    CHECK(std::ranges::equal(ar[1].get(), input_values[2]));
                    CHECK(ar[2].has_value());
                    CHECK(std::ranges::equal(ar[2].get(), input_values[3]));
                    CHECK(ar[3].has_value());
                    CHECK(std::ranges::equal(ar[3].get(), input_values[4]));
                    CHECK_EQ(ar[4].get(), new_nullable_values[0]);
                    CHECK_EQ(ar[5].get(), new_nullable_values[1]);
                    CHECK_EQ(ar[6].get(), new_nullable_values[2]);
                }
            }
        }

        TEST_CASE("erase")
        {
            auto [ar, input_values] = make_array(5, 1);

            SUBCASE("with pos")
            {
                SUBCASE("at the beginning")
                {
                    const auto pos = ar.cbegin();
                    const auto iter = ar.erase(pos);
                    CHECK_EQ(iter, ar.begin());
                    REQUIRE_EQ(ar.size(), 3);
                    CHECK_FALSE(ar[0].has_value());
                    CHECK(std::ranges::equal(ar[0].get(), input_values[2]));
                    CHECK(ar[1].has_value());
                    CHECK(std::ranges::equal(ar[1].get(), input_values[3]));
                    CHECK(ar[2].has_value());
                    CHECK(std::ranges::equal(ar[2].get(), input_values[4]));
                }

                SUBCASE("in the middle")
                {
                    const auto pos = sparrow::next(ar.cbegin(), 1);
                    const auto iter = ar.erase(pos);
                    CHECK_EQ(iter, ar.begin() + 1);
                    REQUIRE_EQ(ar.size(), 3);
                    CHECK(ar[0].has_value());
                    CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
                    CHECK(ar[1].has_value());
                    CHECK(std::ranges::equal(ar[1].get(), input_values[3]));
                    CHECK(ar[2].has_value());
                    CHECK(std::ranges::equal(ar[2].get(), input_values[4]));
                }

                SUBCASE("at the end")
                {
                    const auto pos = std::prev(ar.cend());
                    const auto iter = ar.erase(pos);
                    CHECK_EQ(iter, ar.begin() + 3);
                    REQUIRE_EQ(ar.size(), 3);
                    REQUIRE(ar[0].has_value());
                    CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
                    CHECK_FALSE(ar[1].has_value());
                    CHECK(std::ranges::equal(ar[1].get(), input_values[2]));
                    CHECK(ar[2].has_value());
                    CHECK(std::ranges::equal(ar[2].get(), input_values[3]));
                }
            }

            SUBCASE("with iterators")
            {
                SUBCASE("at the beginning")
                {
                    const auto pos = ar.cbegin();
                    const auto iter = ar.erase(pos, sparrow::next(pos, 2));
                    CHECK_EQ(iter, ar.begin());
                    REQUIRE_EQ(ar.size(), 2);
                    CHECK(ar[0].has_value());
                    CHECK(std::ranges::equal(ar[0].get(), input_values[3]));
                    CHECK(ar[1].has_value());
                    CHECK(std::ranges::equal(ar[1].get(), input_values[4]));
                }

                SUBCASE("in the middle")
                {
                    const auto pos = sparrow::next(ar.cbegin(), 1);
                    const auto iter = ar.erase(pos, sparrow::next(pos, 2));
                    CHECK_EQ(iter, ar.begin() + 1);
                    REQUIRE_EQ(ar.size(), 2);
                    CHECK(ar[0].has_value());
                    CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
                    CHECK(ar[1].has_value());
                    CHECK(std::ranges::equal(ar[1].get(), input_values[4]));
                }

                SUBCASE("at the end")
                {
                    const auto pos = std::prev(ar.cend(), 2);
                    const auto iter = ar.erase(pos, ar.cend());
                    CHECK_EQ(iter, ar.begin() + 2);
                    REQUIRE_EQ(ar.size(), 2);
                    CHECK(ar[0].has_value());
                    CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
                    CHECK_FALSE(ar[1].has_value());
                    CHECK(std::ranges::equal(ar[1].get(), input_values[2]));
                }
            }

            SUBCASE("push_back")
            {
                ar.push_back(new_nullable_value_1);
                REQUIRE_EQ(ar.size(), 5);
                CHECK(ar[0].has_value());
                CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
                CHECK_FALSE(ar[1].has_value());
                CHECK(std::ranges::equal(ar[1].get(), input_values[2]));
                CHECK(ar[2].has_value());
                CHECK(std::ranges::equal(ar[2].get(), input_values[3]));
                CHECK(ar[3].has_value());
                CHECK(std::ranges::equal(ar[3].get(), input_values[4]));
                CHECK_EQ(ar[4], new_nullable_value_1);
            }

            SUBCASE("pop_back")
            {
                ar.pop_back();
                REQUIRE_EQ(ar.size(), 3);
                CHECK(ar[0].has_value());
                CHECK(std::ranges::equal(ar[0].get(), input_values[1]));
                CHECK_FALSE(ar[1].has_value());
                CHECK(std::ranges::equal(ar[1].get(), input_values[2]));
                CHECK(ar[2].has_value());
                CHECK(std::ranges::equal(ar[2].get(), input_values[3]));
            }
        }

        TEST_CASE("convenience constructors")
        {
            SUBCASE("from u8_buffer and validity bitmap")
            {
                u8_buffer buffer{
                    byte_t{1},
                    byte_t{2},
                    byte_t{3},
                    byte_t{4},
                    byte_t{5},
                    byte_t{6},
                    byte_t{7},
                    byte_t{8},
                    byte_t{9}
                };
                const fixed_width_binary_array arr{std::move(buffer), size_t(3), std::vector<std::size_t>{1}};
                REQUIRE_EQ(arr.size(), 3);
                CHECK(arr[0].has_value());
                CHECK(std::ranges::equal(arr[0].get(), std::array{byte_t{1}, byte_t{2}, byte_t{3}}));
                CHECK_FALSE(arr[1].has_value());
                CHECK(std::ranges::equal(arr[1].get(), std::array{byte_t{4}, byte_t{5}, byte_t{6}}));
                CHECK(arr[2].has_value());
                CHECK(std::ranges::equal(arr[2].get(), std::array{byte_t{7}, byte_t{8}, byte_t{9}}));
            }

            SUBCASE("from range and validity bitmap")
            {
                const std::array<std::array<byte_t, 3>, 3> buffer{
                    std::array{byte_t{1}, byte_t{2}, byte_t{3}},
                    std::array{byte_t{4}, byte_t{5}, byte_t{6}},
                    std::array{byte_t{7}, byte_t{8}, byte_t{9}}
                };
                const fixed_width_binary_array arr{buffer, std::array<size_t, 1>{1}};
                REQUIRE_EQ(arr.size(), 3);
                CHECK(arr[0].has_value());
                CHECK(std::ranges::equal(arr[0].get(), std::array{byte_t{1}, byte_t{2}, byte_t{3}}));
                CHECK_FALSE(arr[1].has_value());
                CHECK(std::ranges::equal(arr[1].get(), std::array{byte_t{4}, byte_t{5}, byte_t{6}}));
                CHECK(arr[2].has_value());
                CHECK(std::ranges::equal(arr[2].get(), std::array{byte_t{7}, byte_t{8}, byte_t{9}}));
            }

            SUBCASE("from nullable range")
            {
                std::array<nullable<std::array<byte_t, 3>>, 3> range{
                    nullable<std::array<byte_t, 3>>(std::array{byte_t{1}, byte_t{2}, byte_t{3}}),
                    nullable<std::array<byte_t, 3>>(std::array{byte_t{4}, byte_t{5}, byte_t{6}}, false),
                    nullable<std::array<byte_t, 3>>(std::array{byte_t{7}, byte_t{8}, byte_t{9}})
                };
                const fixed_width_binary_array arr{std::move(range)};
                REQUIRE_EQ(arr.size(), 3);
                CHECK(arr[0].has_value());
                CHECK(std::ranges::equal(arr[0].get(), std::array{byte_t{1}, byte_t{2}, byte_t{3}}));
                CHECK_FALSE(arr[1].has_value());
                CHECK(std::ranges::equal(arr[1].get(), std::array{byte_t{4}, byte_t{5}, byte_t{6}}));
                CHECK(arr[2].has_value());
                CHECK(std::ranges::equal(arr[2].get(), std::array{byte_t{7}, byte_t{8}, byte_t{9}}));
            }
        }


#if defined(__cpp_lib_format)
        TEST_CASE("formatting")
        {
            const fixed_width_binary_array arr{
                std::vector<std::array<byte_t, 3>>{
                    {byte_t{1}, byte_t{2}, byte_t{3}},
                    {byte_t{4}, byte_t{5}, byte_t{6}},
                    {byte_t{7}, byte_t{8}, byte_t{9}}
                },
                std::vector<std::size_t>{1}
            };
            const std::string formatted = std::format("{}", arr);
            constexpr std::string_view
                expected = "Fixed width binary [name=nullptr | size=3] <<1, 2, 3>, null, <7, 8, 9>>";
            CHECK_EQ(formatted, expected);
        }
#endif
    }
}
