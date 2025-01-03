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

#include "sparrow/array.hpp"
#include "sparrow/layout/fixed_width_binary_array.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    template <typename T>
    class my_class
    {
        std::array<T, 3> data;
        T my_value;
    };

    template <typename T>
    class my_incompatible_class
    {
        std::vector<T> data;
    };

    static_assert(fixed_width_binary_array_accepted_types<std::int8_t> == true);
    static_assert(fixed_width_binary_array_accepted_types<std::uint8_t> == true);
    static_assert(fixed_width_binary_array_accepted_types<std::int16_t> == true);
    static_assert(fixed_width_binary_array_accepted_types<std::uint16_t> == true);
    static_assert(fixed_width_binary_array_accepted_types<std::int32_t> == true);
    static_assert(fixed_width_binary_array_accepted_types<std::uint32_t> == true);
    static_assert(fixed_width_binary_array_accepted_types<std::int64_t> == true);
    static_assert(fixed_width_binary_array_accepted_types<std::uint64_t> == true);
    static_assert(fixed_width_binary_array_accepted_types<float16_t> == true);
    static_assert(fixed_width_binary_array_accepted_types<float32_t> == true);
    static_assert(fixed_width_binary_array_accepted_types<float64_t> == true);
    static_assert(fixed_width_binary_array_accepted_types<std::string> == false);
    static_assert(fixed_width_binary_array_accepted_types<std::vector<std::byte>> == false);
    static_assert(fixed_width_binary_array_accepted_types<std::array<std::byte, 3>> == true);
    static_assert(fixed_width_binary_array_accepted_types<my_class<std::int32_t>> == true);
    static_assert(fixed_width_binary_array_accepted_types<my_incompatible_class<std::int32_t>> == false);

    using testing_types = std::tuple<
        std::int8_t,
        std::uint8_t,
        std::int16_t,
        std::uint16_t,
        std::int32_t,
        std::uint32_t,
        std::int64_t,
        std::uint64_t,
        float16_t,
        float32_t,
        float64_t>;

    TEST_SUITE("fixed_width_binary_array")
    {
        TEST_CASE_TEMPLATE_DEFINE("", T, fixed_width_binary_array_id)
        {
            using array_test_type = fixed_width_binary_array<std::array<T, 3>>;

            std::vector<std::array<T, 3>> input_values;

            auto make_array = [&input_values](size_t count, size_t offset = 0)
            {
                input_values.clear();
                input_values.reserve(count);
                for (T i = 0; i < static_cast<T>(static_cast<int>(count)); ++i)
                {
                    input_values.push_back(std::array<T, 3>{
                        static_cast<T>(i),
                        static_cast<T>(static_cast<T>(i) + static_cast<T>(1)),
                        static_cast<T>(static_cast<T>(i) + static_cast<T>(2))
                    });
                }

                const array_test_type arr(
                    input_values,
                    count > 2 ? std::vector<std::size_t>{2} : std::vector<std::size_t>{}
                );
                if (offset != 0)
                {
                    return arr.slice(offset, arr.size());
                }
                else
                {
                    return arr;
                }
            };

            // Elements: {2, 3, 4}, null, {5, 6, 7}, {8, 9, 10}
            array_test_type ar = make_array(5, 1);

            auto new_value_1 = std::array<T, 3>{99, 100, 101};
            const auto new_nullable_value_1 = nullable<std::array<T, 3>>(new_value_1, true);
            auto new_value_2 = std::array<T, 3>{102, 103, 104};
            const auto new_nullable_value_2 = nullable<std::array<T, 3>>(new_value_2, true);
            auto new_value_3 = std::array<T, 3>{105, 106, 107};
            const auto new_nullable_value_3 = nullable<std::array<T, 3>>(new_value_3, true);
            const auto new_nullable_values = std::array<nullable<std::array<T, 3>>, 3>{
                new_nullable_value_1,
                new_nullable_value_2,
                new_nullable_value_3
            };

            SUBCASE("constructor")
            {
                CHECK_EQ(ar.size(), 4);
            }

            SUBCASE("operator[]")
            {
                SUBCASE("const")
                {
                    const array_test_type const_ar = make_array(5, 1);
                    REQUIRE_EQ(const_ar.size(), 4);
                    CHECK(const_ar[0].has_value());
                    CHECK_EQ(const_ar[0].get(), input_values[1]);
                    CHECK_FALSE(const_ar[1].has_value());
                    CHECK_EQ(const_ar[1].get(), input_values[2]);
                    CHECK(const_ar[2].has_value());
                    CHECK_EQ(const_ar[2].get(), input_values[3]);
                    CHECK(const_ar[3].has_value());
                    CHECK_EQ(const_ar[3].get(), input_values[4]);
                }

                SUBCASE("mutable")
                {
                    REQUIRE_EQ(ar.size(), 4);
                    CHECK(ar[0].has_value());
                    CHECK_EQ(ar[0].get(), input_values[1]);
                    CHECK_FALSE(ar[1].has_value());
                    CHECK_EQ(ar[1].get(), input_values[2]);
                    CHECK(ar[2].has_value());
                    CHECK_EQ(ar[2].get(), input_values[3]);
                    CHECK(ar[3].has_value());
                    CHECK_EQ(ar[3].get(), input_values[4]);

                    ar[1] = new_nullable_value_1;
                    CHECK(ar[1].has_value());
                    CHECK_EQ(ar[1].get(), new_value_1);
                }
            }

            SUBCASE("front")
            {
                SUBCASE("const")
                {
                    const array_test_type const_ar{ar};
                    REQUIRE_EQ(const_ar.size(), 4);
                    CHECK(ar.front().has_value());
                    CHECK_EQ(ar.front().value(), input_values[1]);
                }
            }

            SUBCASE("back")
            {
                SUBCASE("const")
                {
                    const array_test_type const_ar{ar};
                    REQUIRE_EQ(const_ar.size(), 4);
                    CHECK(ar.back().has_value());
                    CHECK_EQ(ar.back().value(), input_values[4]);
                }
            }

            SUBCASE("copy")
            {
                array_test_type ar2(ar);

                CHECK_EQ(ar, ar2);

                array_test_type ar3(make_array(7, 1));
                CHECK_NE(ar, ar3);
                ar3 = ar;
                CHECK_EQ(ar, ar3);
            }

            SUBCASE("move")
            {
                array_test_type ar2(ar);

                array_test_type ar3(std::move(ar));
                CHECK_EQ(ar2, ar3);

                array_test_type ar4(make_array(7, 1));
                CHECK_NE(ar2, ar4);
                ar4 = std::move(ar2);
                CHECK_EQ(ar3, ar4);
            }

            SUBCASE("value_iterator_ordering")
            {
                auto ar_values = ar.values();
                auto citer = ar_values.begin();
                CHECK(citer < ar_values.end());
            }

            SUBCASE("value_iterator_equality")
            {
                const auto ar_values = ar.values();
                auto citer = ar_values.begin();
                CHECK_EQ(*citer, input_values[1]);
                ++citer;
                CHECK_EQ(*citer, input_values[2]);
                ++citer;
                CHECK_EQ(*citer, input_values[3]);
                ++citer;
                CHECK_EQ(*citer, input_values[4]);
                ++citer;
                CHECK_EQ(citer, ar_values.end());
            }

            SUBCASE("const_value_iterator_ordering")
            {
                auto ar_values = ar.values();
                auto citer = ar_values.begin();
                CHECK(citer < ar_values.end());
            }

            SUBCASE("const_value_iterator_equality")
            {
                auto ar_values = ar.values();
                auto citer = ar_values.begin();
                CHECK_EQ(*citer, input_values[1]);
                ++citer;
                CHECK_EQ(*citer, input_values[2]);
                ++citer;
                CHECK_EQ(*citer, input_values[3]);
                ++citer;
                CHECK_EQ(*citer, input_values[4]);
                ++citer;
                CHECK_EQ(citer, ar_values.end());
            }

            SUBCASE("const_bitmap_iterator_ordering")
            {
                const auto ar_bitmap = ar.bitmap();
                const auto citer = ar_bitmap.begin();
                CHECK(citer < ar_bitmap.end());
            }

            SUBCASE("const_bitmap_iterator_equality")
            {
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

            SUBCASE("iterator")
            {
                auto it = ar.begin();
                const auto end = ar.end();
                CHECK(it->has_value());
                CHECK_EQ(*it, make_nullable(input_values[1]));
                ++it;
                CHECK_FALSE(it->has_value());
                CHECK_EQ(*it, make_nullable(input_values[2], false));
                ++it;
                CHECK(it->has_value());
                CHECK_EQ(*it, make_nullable(input_values[3]));
                ++it;
                CHECK(it->has_value());
                CHECK_EQ(*it, make_nullable(input_values[4]));
                ++it;

                CHECK_EQ(it, end);

                const array_test_type ar_empty = make_array(0);
                CHECK_EQ(ar_empty.begin(), ar_empty.end());
            }

            SUBCASE("revert_iterator")
            {
                auto rit = ar.rbegin();
                const auto rend = ar.rend();
                CHECK(rit->has_value());
                CHECK_EQ(*rit, make_nullable(input_values[4]));
                ++rit;
                CHECK(rit->has_value());
                CHECK_EQ(*rit, make_nullable(input_values[3]));
                ++rit;
                CHECK_FALSE(rit->has_value());
                CHECK_EQ(*rit, make_nullable(input_values[2], false));
                ++rit;
                CHECK(rit->has_value());
                CHECK_EQ(*rit, make_nullable(input_values[1]));
                ++rit;

                CHECK_EQ(rit, rend);

                const array_test_type ar_empty = make_array(0);
                CHECK_EQ(ar_empty.rbegin(), ar_empty.rend());
            }

            SUBCASE("resize")
            {
                ar.resize(7, new_nullable_value_1);
                REQUIRE_EQ(ar.size(), 7);
                CHECK(ar[0].has_value());
                CHECK_EQ(ar[0].get(), input_values[1]);
                CHECK_FALSE(ar[1].has_value());
                CHECK_EQ(ar[1].get(), input_values[2]);
                CHECK(ar[2].has_value());
                CHECK_EQ(ar[2].get(), input_values[3]);
                CHECK(ar[3].has_value());
                CHECK_EQ(ar[3].get(), input_values[4]);
                CHECK_EQ(ar[4], new_nullable_value_1);
                CHECK_EQ(ar[5], new_nullable_value_1);
                CHECK_EQ(ar[6], new_nullable_value_1);
            }

            SUBCASE("insert")
            {
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
                        CHECK_EQ(ar[1].get(), input_values[1]);
                        CHECK_FALSE(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), input_values[2]);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), input_values[3]);
                        CHECK(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), input_values[4]);
                    }

                    SUBCASE("in the middle")
                    {
                        const auto pos = sparrow::next(ar.cbegin(), 1);
                        const auto iter = ar.insert(pos, new_value_1);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), 1));
                        REQUIRE_EQ(ar.size(), 5);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), input_values[1]);
                        CHECK_EQ(ar[1], new_nullable_value_1);
                        CHECK_FALSE(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), input_values[2]);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), input_values[3]);
                        CHECK(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), input_values[4]);
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = ar.cend();
                        const auto iter = ar.insert(pos, new_nullable_value_1);
                        CHECK_EQ(iter, ar.begin() + 4);
                        REQUIRE_EQ(ar.size(), 5);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), input_values[1]);
                        CHECK_FALSE(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), input_values[2]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), input_values[3]);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), input_values[4]);
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
                        CHECK_EQ(ar[3].get(), input_values[1]);
                        CHECK_FALSE(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), input_values[2]);
                        CHECK(ar[5].has_value());
                        CHECK_EQ(ar[5].get(), input_values[3]);
                        CHECK(ar[6].has_value());
                        CHECK_EQ(ar[6].get(), input_values[4]);
                    }

                    SUBCASE("in the middle")
                    {
                        const auto pos = sparrow::next(ar.cbegin(), 1);
                        const auto iter = ar.insert(pos, new_nullable_value_1, 3);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), 1));
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), input_values[1]);
                        CHECK_EQ(ar[1], new_nullable_value_1);
                        CHECK_EQ(ar[2], new_nullable_value_1);
                        CHECK_EQ(ar[3], new_nullable_value_1);
                        CHECK_FALSE(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), input_values[2]);
                        CHECK(ar[5].has_value());
                        CHECK_EQ(ar[5].get(), input_values[3]);
                        CHECK(ar[6].has_value());
                        CHECK_EQ(ar[6].get(), input_values[4]);
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = ar.cend();
                        const auto iter = ar.insert(pos, new_nullable_value_1, 3);
                        CHECK_EQ(iter, ar.begin() + 4);
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), input_values[1]);
                        CHECK_FALSE(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), input_values[2]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), input_values[3]);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), input_values[4]);
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
                        const auto iter = ar.insert(pos, {new_value_1, new_value_2, new_value_3});
                        CHECK_EQ(iter, ar.begin());
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), new_value_1);
                        CHECK(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), new_value_2);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), new_value_3);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), input_values[1]);
                        CHECK_FALSE(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), input_values[2]);
                        CHECK(ar[5].has_value());
                        CHECK_EQ(ar[5].get(), input_values[3]);
                        CHECK(ar[6].has_value());
                        CHECK_EQ(ar[6].get(), input_values[4]);
                    }

                    SUBCASE("in the middle")
                    {
                        const auto pos = sparrow::next(ar.cbegin(), 1);
                        const auto iter = ar.insert(pos, {new_value_1, new_value_2, new_value_3});
                        CHECK_EQ(iter, sparrow::next(ar.begin(), 1));
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), input_values[1]);
                        CHECK(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), new_value_1);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), new_value_2);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), new_value_3);
                        CHECK_FALSE(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), input_values[2]);
                        CHECK(ar[5].has_value());
                        CHECK_EQ(ar[5].get(), input_values[3]);
                        CHECK(ar[6].has_value());
                        CHECK_EQ(ar[6].get(), input_values[4]);
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = ar.cend();
                        const auto iter = ar.insert(pos, {new_value_1, new_value_2, new_value_3});
                        CHECK_EQ(iter, ar.begin() + 4);
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), input_values[1]);
                        CHECK_FALSE(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), input_values[2]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), input_values[3]);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), input_values[4]);
                        CHECK(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), new_value_1);
                        CHECK(ar[5].has_value());
                        CHECK_EQ(ar[5].get(), new_value_2);
                        CHECK(ar[6].has_value());
                        CHECK_EQ(ar[6].get(), new_value_3);
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
                        CHECK_EQ(ar[3].get(), input_values[1]);
                        CHECK_FALSE(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), input_values[2]);
                        CHECK(ar[5].has_value());
                        CHECK_EQ(ar[5].get(), input_values[3]);
                        CHECK(ar[6].has_value());
                        CHECK_EQ(ar[6].get(), input_values[4]);
                    }

                    SUBCASE("in the middle")
                    {
                        const auto pos = sparrow::next(ar.cbegin(), 1);
                        const auto iter = ar.insert(pos, new_nullable_values);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), 1));
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), input_values[1]);
                        CHECK(ar[1].has_value());
                        CHECK_EQ(ar[1], new_nullable_values[0]);
                        CHECK_EQ(ar[2], new_nullable_values[1]);
                        CHECK_EQ(ar[3], new_nullable_values[2]);
                        CHECK_FALSE(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), input_values[2]);
                        CHECK(ar[5].has_value());
                        CHECK_EQ(ar[5].get(), input_values[3]);
                        CHECK(ar[6].has_value());
                        CHECK_EQ(ar[6].get(), input_values[4]);
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = ar.cend();
                        const auto iter = ar.insert(pos, new_nullable_values);
                        CHECK_EQ(iter, ar.begin() + 4);
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), input_values[1]);
                        CHECK_FALSE(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), input_values[2]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), input_values[3]);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), input_values[4]);
                        CHECK_EQ(ar[4].get(), new_nullable_values[0]);
                        CHECK_EQ(ar[5].get(), new_nullable_values[1]);
                        CHECK_EQ(ar[6].get(), new_nullable_values[2]);
                    }
                }
            }

            SUBCASE("erase")
            {
                SUBCASE("with pos")
                {
                    SUBCASE("at the beginning")
                    {
                        const auto pos = ar.cbegin();
                        const auto iter = ar.erase(pos);
                        CHECK_EQ(iter, ar.begin());
                        REQUIRE_EQ(ar.size(), 3);
                        CHECK_FALSE(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), input_values[2]);
                        CHECK(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), input_values[3]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), input_values[4]);
                    }

                    SUBCASE("in the middle")
                    {
                        const auto pos = sparrow::next(ar.cbegin(), 1);
                        const auto iter = ar.erase(pos);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), 1));
                        REQUIRE_EQ(ar.size(), 3);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), input_values[1]);
                        CHECK(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), input_values[3]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), input_values[4]);
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = std::prev(ar.cend());
                        const auto iter = ar.erase(pos);
                        CHECK_EQ(iter, ar.begin() + 3);
                        REQUIRE_EQ(ar.size(), 3);
                        REQUIRE(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), input_values[1]);
                        CHECK_FALSE(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), input_values[2]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), input_values[3]);
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
                        CHECK_EQ(ar[0].get(), input_values[3]);
                        CHECK(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), input_values[4]);
                    }

                    SUBCASE("in the middle")
                    {
                        const auto pos = sparrow::next(ar.cbegin(), 1);
                        const auto iter = ar.erase(pos, sparrow::next(pos, 2));
                        CHECK_EQ(iter, sparrow::next(ar.begin(), 1));
                        REQUIRE_EQ(ar.size(), 2);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), input_values[1]);
                        CHECK(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), input_values[4]);
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = std::prev(ar.cend(), 2);
                        const auto iter = ar.erase(pos, ar.cend());
                        CHECK_EQ(iter, ar.begin() + 2);
                        REQUIRE_EQ(ar.size(), 2);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), input_values[1]);
                        CHECK_FALSE(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), input_values[2]);
                    }
                }

                SUBCASE("push_back")
                {
                    ar.push_back(new_nullable_value_1);
                    REQUIRE_EQ(ar.size(), 5);
                    CHECK(ar[0].has_value());
                    CHECK_EQ(ar[0].get(), input_values[1]);
                    CHECK_FALSE(ar[1].has_value());
                    CHECK_EQ(ar[1].get(), input_values[2]);
                    CHECK(ar[2].has_value());
                    CHECK_EQ(ar[2].get(), input_values[3]);
                    CHECK(ar[3].has_value());
                    CHECK_EQ(ar[3].get(), input_values[4]);
                    CHECK_EQ(ar[4], new_nullable_value_1);
                }

                SUBCASE("pop_back")
                {
                    ar.pop_back();
                    REQUIRE_EQ(ar.size(), 3);
                    CHECK(ar[0].has_value());
                    CHECK_EQ(ar[0].get(), input_values[1]);
                    CHECK_FALSE(ar[1].has_value());
                    CHECK_EQ(ar[1].get(), input_values[2]);
                    CHECK(ar[2].has_value());
                    CHECK_EQ(ar[2].get(), input_values[3]);
                }
            }
        }
        TEST_CASE_TEMPLATE_APPLY(fixed_width_binary_array_id, testing_types);

#if defined(__cpp_lib_format)
        TEST_CASE("formatting")
        {
            fixed_width_binary_array<std::array<uint32_t, 3>> arr{
                std::vector<std::array<uint32_t, 3>>{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}},
                std::vector<std::size_t>{1}
            };
            const std::string formatted = std::format("{}", arr);
            constexpr std::string_view
                expected = "Fixed width binary [name=nullptr | size=3] <[1, 2, 3], null, [7, 8, 9]>";
            CHECK_EQ(formatted, expected);
        }
#endif
    }
}
