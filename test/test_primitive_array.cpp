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
#include <ranges>

#include "sparrow/arrow_array_schema_proxy_factory.hpp"
#include "sparrow/layout/primitive_array.hpp"

#include "doctest/doctest.h"

namespace sparrow
{

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

    TEST_SUITE("primitive_array")
    {
        TEST_CASE_TEMPLATE_DEFINE("", T, primitive_array_id)
        {
            const std::array<T, 5> values{1, 2, 3, 4, 5};
            constexpr std::array<uint8_t, 1> nulls{2};
            constexpr int64_t offset = 1;

            auto make_array = [&nulls]<std::ranges::input_range R>(R values_range)
            {
                return make_primitive_arrow_proxy(values_range, nulls, offset, "test", std::nullopt);
            };

            // Elements: 2, null, 4, 5

            using array_test_type = primitive_array<T>;
            array_test_type ar{make_array(values)};

            SUBCASE("constructor")
            {
                CHECK_EQ(ar.size(), 4);
            }

            SUBCASE("operator[]")
            {
                SUBCASE("const")
                {
                    const array_test_type const_ar{make_array(values)};
                    REQUIRE_EQ(const_ar.size(), 4);
                    CHECK(const_ar[0].has_value());
                    CHECK_EQ(const_ar[0].get(), values[1]);
                    CHECK_FALSE(const_ar[1].has_value());
                    CHECK_EQ(const_ar[1].get(), values[2]);
                    CHECK(const_ar[2].has_value());
                    CHECK_EQ(const_ar[2].get(), values[3]);
                    CHECK(const_ar[3].has_value());
                    CHECK_EQ(const_ar[3].get(), values[4]);
                }

                SUBCASE("mutable")
                {
                    REQUIRE_EQ(ar.size(), 4);
                    CHECK(ar[0].has_value());
                    CHECK_EQ(ar[0].get(), values[1]);
                    CHECK_FALSE(ar[1].has_value());
                    CHECK_EQ(ar[1].get(), values[2]);
                    CHECK(ar[2].has_value());
                    CHECK_EQ(ar[2].get(), values[3]);
                    CHECK(ar[3].has_value());
                    CHECK_EQ(ar[3].get(), values[4]);

                    ar[1] = make_nullable<T>(99);
                    CHECK(ar[1].has_value());
                    CHECK_EQ(ar[1].get(), static_cast<T>(99));
                }
            }

            SUBCASE("copy")
            {
                array_test_type ar2(ar);

                CHECK_EQ(ar, ar2);

                array_test_type ar3(make_array(std::vector<T>{1, 2, 3, 4, 5, 6, 7}));
                CHECK_NE(ar, ar3);
                ar3 = ar;
                CHECK_EQ(ar, ar3);
            }

            SUBCASE("move")
            {
                array_test_type ar2(ar);

                array_test_type ar3(std::move(ar));
                CHECK_EQ(ar2, ar3);

                array_test_type ar4(make_array(std::vector<T>{1, 2, 3, 4, 5, 6, 7}));
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
                CHECK_EQ(*citer, values[1]);
                ++citer;
                CHECK_EQ(*citer, values[2]);
                ++citer;
                CHECK_EQ(*citer, values[3]);
                ++citer;
                CHECK_EQ(*citer, values[4]);
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
                CHECK_EQ(*citer, values[1]);
                ++citer;
                CHECK_EQ(*citer, values[2]);
                ++citer;
                CHECK_EQ(*citer, values[3]);
                ++citer;
                CHECK_EQ(*citer, values[4]);
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
                CHECK_EQ(*it, values[1]);
                ++it;
                CHECK_FALSE(it->has_value());
                CHECK_EQ(*it, make_nullable(values[2], false));
                ++it;
                CHECK(it->has_value());
                CHECK_EQ(*it, make_nullable(values[3]));
                ++it;
                CHECK(it->has_value());
                CHECK_EQ(*it, make_nullable(values[4]));
                ++it;

                CHECK_EQ(it, end);

                const array_test_type ar_empty(
                    make_primitive_arrow_proxy(std::array<T, 0>{}, std::array<uint32_t, 0>{}, 0, "test", std::nullopt)
                );
                CHECK_EQ(ar_empty.begin(), ar_empty.end());
            }

            SUBCASE("resize")
            {
                const T new_value{99};
                ar.resize(7, make_nullable<T>(99));
                REQUIRE_EQ(ar.size(), 7);
                CHECK(ar[0].has_value());
                CHECK_EQ(ar[0].get(), values[1]);
                CHECK_FALSE(ar[1].has_value());
                CHECK_EQ(ar[1].get(), values[2]);
                CHECK(ar[2].has_value());
                CHECK_EQ(ar[2].get(), values[3]);
                CHECK(ar[3].has_value());
                CHECK_EQ(ar[3].get(), values[4]);
                CHECK(ar[4].has_value());
                CHECK_EQ(ar[4].get(), new_value);
                CHECK(ar[5].has_value());
                CHECK_EQ(ar[5].get(), new_value);
                CHECK(ar[6].has_value());
                CHECK_EQ(ar[6].get(), new_value);
            }

            SUBCASE("insert")
            {
                SUBCASE("with pos and value")
                {
                    SUBCASE("at the beginning")
                    {
                        const auto pos = ar.cbegin();
                        const T new_value{99};
                        const auto iter = ar.insert(pos, make_nullable<T>(99));
                        CHECK_EQ(iter, ar.begin());
                        REQUIRE_EQ(ar.size(), 5);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), new_value);
                        CHECK(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), values[1]);
                        CHECK_FALSE(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), values[2]);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), values[3]);
                        CHECK(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), values[4]);
                    }

                    SUBCASE("in the middle")
                    {
                        const auto pos = sparrow::next(ar.cbegin(), 1);
                        const T new_value{99};
                        const auto iter = ar.insert(pos, make_nullable<T>(99));
                        CHECK_EQ(iter, sparrow::next(ar.begin(), 1));
                        REQUIRE_EQ(ar.size(), 5);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), values[1]);
                        CHECK(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), new_value);
                        CHECK_FALSE(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), values[2]);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), values[3]);
                        CHECK(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), values[4]);
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = ar.cend();
                        const T new_value{99};
                        const auto iter = ar.insert(pos, make_nullable<T>(99));
                        CHECK_EQ(iter, ar.begin() + 4);
                        REQUIRE_EQ(ar.size(), 5);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), values[1]);
                        CHECK_FALSE(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), values[2]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), values[3]);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), values[4]);
                        CHECK(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), new_value);
                    }
                }

                SUBCASE("with pos, count and value")
                {
                    SUBCASE("at the beginning")
                    {
                        const auto pos = ar.cbegin();
                        const T new_value{99};
                        const auto iter = ar.insert(pos, make_nullable(new_value), 3);
                        CHECK_EQ(iter, ar.begin());
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), new_value);
                        CHECK(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), new_value);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), new_value);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), values[1]);
                        CHECK_FALSE(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), values[2]);
                        CHECK(ar[5].has_value());
                        CHECK_EQ(ar[5].get(), values[3]);
                        CHECK(ar[6].has_value());
                        CHECK_EQ(ar[6].get(), values[4]);
                    }

                    SUBCASE("in the middle")
                    {
                        const auto pos = sparrow::next(ar.cbegin(), 1);
                        const T new_value{99};
                        const auto iter = ar.insert(pos, make_nullable(new_value), 3);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), 1));
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), values[1]);
                        CHECK(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), new_value);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), new_value);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), new_value);
                        CHECK_FALSE(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), values[2]);
                        CHECK(ar[5].has_value());
                        CHECK_EQ(ar[5].get(), values[3]);
                        CHECK(ar[6].has_value());
                        CHECK_EQ(ar[6].get(), values[4]);
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = ar.cend();
                        const T new_value{99};
                        const auto iter = ar.insert(pos, make_nullable(new_value), 3);
                        CHECK_EQ(iter, ar.begin() + 4);
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), values[1]);
                        CHECK_FALSE(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), values[2]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), values[3]);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), values[4]);
                        CHECK(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), new_value);
                        CHECK(ar[5].has_value());
                        CHECK_EQ(ar[5].get(), new_value);
                        CHECK(ar[6].has_value());
                        CHECK_EQ(ar[6].get(), new_value);
                    }
                }

                SUBCASE("with pos, first and last iterators")
                {
                    SUBCASE("at the beginning")
                    {
                        const auto pos = ar.cbegin();
                        const std::array<nullable<T>, 3> new_values{
                            make_nullable<T>(99),
                            make_nullable<T>(100),
                            make_nullable<T>(101)
                        };
                        const auto iter = ar.insert(pos, new_values);
                        CHECK_EQ(iter, ar.begin());
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), new_values[0]);
                        CHECK(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), new_values[1]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), new_values[2]);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), values[1]);
                        CHECK_FALSE(ar[4].has_value());
                        CHECK(ar[5].has_value());
                        CHECK_EQ(ar[5].get(), values[3]);
                        CHECK(ar[6].has_value());
                        CHECK_EQ(ar[6].get(), values[4]);
                    }

                    SUBCASE("in the middle")
                    {
                        const auto pos = sparrow::next(ar.cbegin(), 1);
                        const std::array<nullable<T>, 3> new_values{
                            make_nullable<T>(99),
                            make_nullable<T>(100),
                            make_nullable<T>(101)
                        };
                        const auto iter = ar.insert(pos, new_values);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), 1));
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), values[1]);
                        CHECK(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), new_values[0]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), new_values[1]);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), new_values[2]);
                        CHECK_FALSE(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), values[2]);
                        CHECK(ar[5].has_value());
                        CHECK_EQ(ar[5].get(), values[3]);
                        CHECK(ar[6].has_value());
                        CHECK_EQ(ar[6].get(), values[4]);
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = ar.cend();
                        const std::array<nullable<T>, 3> new_values{
                            make_nullable<T>(99),
                            make_nullable<T>(100),
                            make_nullable<T>(101)
                        };
                        const auto iter = ar.insert(pos, new_values);
                        CHECK_EQ(iter, ar.begin() + 4);
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), values[1]);
                        CHECK_FALSE(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), values[2]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), values[3]);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), values[4]);
                        CHECK(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), new_values[0]);
                        CHECK(ar[5].has_value());
                        CHECK_EQ(ar[5].get(), new_values[1]);
                        CHECK(ar[6].has_value());
                        CHECK_EQ(ar[6].get(), new_values[2]);
                    }
                }

                SUBCASE("with pos and initializer list")
                {
                    SUBCASE("at the beginning")
                    {
                        const auto pos = ar.cbegin();
                        auto new_val_99 = make_nullable<T>(99);
                        auto new_val_100 = make_nullable<T>(100);
                        auto new_val_101 = make_nullable<T>(101);
                        const auto iter = ar.insert(pos, {new_val_99, new_val_100, new_val_101});
                        CHECK_EQ(iter, ar.begin());
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK_EQ(ar[0], new_val_99);
                        CHECK_EQ(ar[1], new_val_100);
                        CHECK_EQ(ar[2], new_val_101);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), values[1]);
                        CHECK_FALSE(ar[4].has_value());
                        CHECK(ar[5].has_value());
                        CHECK_EQ(ar[5].get(), values[3]);
                        CHECK(ar[6].has_value());
                    }

                    SUBCASE("in the middle")
                    {
                        const auto pos = sparrow::next(ar.cbegin(), 1);
                        auto new_val_99 = make_nullable<T>(99);
                        auto new_val_100 = make_nullable<T>(100);
                        auto new_val_101 = make_nullable<T>(101);
                        const auto iter = ar.insert(pos, {new_val_99, new_val_100, new_val_101});
                        CHECK_EQ(iter, sparrow::next(ar.begin(), 1));
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), values[1]);
                        CHECK_EQ(ar[1], new_val_99);
                        CHECK_EQ(ar[2], new_val_100);
                        CHECK_EQ(ar[3], new_val_101);
                        CHECK_FALSE(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), values[2]);
                        CHECK(ar[5].has_value());
                        CHECK_EQ(ar[5].get(), values[3]);
                        CHECK(ar[6].has_value());
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = ar.cend();
                        auto new_val_99 = make_nullable<T>(99);
                        auto new_val_100 = make_nullable<T>(100);
                        auto new_val_101 = make_nullable<T>(101);
                        const auto iter = ar.insert(pos, {new_val_99, new_val_100, new_val_101});
                        CHECK_EQ(iter, ar.begin() + 4);
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), values[1]);
                        CHECK_FALSE(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), values[2]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), values[3]);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), values[4]);
                        CHECK_EQ(ar[4], new_val_99);
                        CHECK_EQ(ar[5], new_val_100);
                        CHECK_EQ(ar[6], new_val_101);
                    }
                }

                SUBCASE("with pos and range")
                {
                    SUBCASE("at the beginning")
                    {
                        const auto pos = ar.cbegin();
                        const std::array<nullable<T>, 3> new_values{
                            make_nullable<T>(99),
                            make_nullable<T>(100),
                            make_nullable<T>(101)
                        };
                        const auto iter = ar.insert(pos, new_values);
                        CHECK_EQ(iter, ar.begin());
                        REQUIRE_EQ(ar.size(), 7);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), new_values[0]);
                        CHECK(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), new_values[1]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), new_values[2]);
                        CHECK(ar[3].has_value());
                        CHECK_EQ(ar[3].get(), values[1]);
                        CHECK_FALSE(ar[4].has_value());
                        CHECK_EQ(ar[4].get(), values[2]);
                        CHECK(ar[5].has_value());
                        CHECK_EQ(ar[5].get(), values[3]);
                        CHECK(ar[6].has_value());
                        CHECK_EQ(ar[6].get(), values[4]);
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
                        CHECK_EQ(ar[0].get(), values[2]);
                        CHECK(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), values[3]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), values[4]);
                    }

                    SUBCASE("in the middle")
                    {
                        const auto pos = sparrow::next(ar.cbegin(), 1);
                        const auto iter = ar.erase(pos);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), 1));
                        REQUIRE_EQ(ar.size(), 3);
                        CHECK(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), values[1]);
                        CHECK(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), values[3]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), values[4]);
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = std::prev(ar.cend());
                        const auto iter = ar.erase(pos);
                        CHECK_EQ(iter, ar.begin() + 3);
                        REQUIRE_EQ(ar.size(), 3);
                        REQUIRE(ar[0].has_value());
                        CHECK_EQ(ar[0].get(), values[1]);
                        CHECK_FALSE(ar[1].has_value());
                        CHECK_EQ(ar[1].get(), values[2]);
                        CHECK(ar[2].has_value());
                        CHECK_EQ(ar[2].get(), values[3]);
                    }
                }

                SUBCASE("with iterators")
                {
                    const auto pos = ar.cbegin() + 1;
                    const auto iter = ar.erase(pos, pos + 2);
                    CHECK_EQ(iter, ar.begin() + 1);
                    REQUIRE_EQ(ar.size(), 2);
                    CHECK(ar[0].has_value());
                    CHECK_EQ(ar[0].get(), values[1]);
                    CHECK(ar[1].has_value());
                    CHECK_EQ(ar[1].get(), values[4]);
                }
            }

            SUBCASE("push_back")
            {
                const T new_value{99};
                ar.push_back(make_nullable<T>(99));
                REQUIRE_EQ(ar.size(), 5);
                CHECK(ar[0].has_value());
                CHECK_EQ(ar[0].get(), values[1]);
                CHECK_FALSE(ar[1].has_value());
                CHECK_EQ(ar[1].get(), values[2]);
                CHECK(ar[2].has_value());
                CHECK_EQ(ar[2].get(), values[3]);
                CHECK(ar[3].has_value());
                CHECK_EQ(ar[3].get(), values[4]);
                CHECK(ar[4].has_value());
                CHECK_EQ(ar[4].value(), new_value);
            }

            SUBCASE("pop_back")
            {
                ar.pop_back();
                REQUIRE_EQ(ar.size(), 3);
                CHECK(ar[0].has_value());
                CHECK_EQ(ar[0].get(), values[1]);
                CHECK_FALSE(ar[1].has_value());
                CHECK_EQ(ar[1].get(), values[2]);
                CHECK(ar[2].has_value());
                CHECK_EQ(ar[2].get(), values[3]);
            }
        }
        TEST_CASE_TEMPLATE_APPLY(primitive_array_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("convenience_constructors", T, convenience_constructors_id)
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
            SUBCASE("initializer list")
            {
                primitive_array<T> arr = { T(0), T(1), T(2) };
                CHECK_EQ(arr[0].value(), T(0));
                CHECK_EQ(arr[1].value(), T(1));
                CHECK_EQ(arr[2].value(), T(2));
            }
        }
        TEST_CASE_TEMPLATE_APPLY(convenience_constructors_id, testing_types);

        TEST_CASE("convenience_constructors_from_iota")
        {   
            primitive_array<std::size_t> arr(std::ranges::iota_view{std::size_t(0), std::size_t(4)});
            REQUIRE(arr.size() == 4);
            for (std::size_t i = 0; i < 4; ++i) {
                REQUIRE(arr[i].has_value());
                CHECK_EQ(arr[i].value(), static_cast<std::size_t>(i));
            }
        }
        TEST_CASE("convenience_constructors_index_of_missing")
        {   
            primitive_array<std::size_t> arr(
                std::ranges::iota_view{std::size_t(0), std::size_t(5)},
                std::vector<std::size_t>{1,3}
            );
            REQUIRE(arr.size() == 5);
            CHECK(arr[0].has_value());
            CHECK(!arr[1].has_value());
            CHECK(arr[2].has_value());
            CHECK(!arr[3].has_value());
            CHECK(arr[4].has_value());
            
            CHECK_EQ(arr[0].value(), std::size_t(0));
            CHECK_EQ(arr[2].value(), std::size_t(2));
            CHECK_EQ(arr[4].value(), std::size_t(4));
        }
    }
}
