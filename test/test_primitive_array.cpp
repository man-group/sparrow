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

#include "sparrow/types/data_type.hpp"

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#endif
#include <ranges>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif
#include <vector>

#include "sparrow/array.hpp"
#include "sparrow/primitive_array.hpp"

#include "doctest/doctest.h"
#include "metadata_sample.hpp"

namespace sparrow
{
    using testing_types = std::tuple<
        bool,
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
            using array_test_type = primitive_array<T>;

            const auto make_nullable_values = [](size_t count)
            {
                std::vector<nullable<T>> values;
                values.reserve(count);
                for (size_t i = 0; i < count; ++i)
                {
                    if constexpr (std::is_same_v<T, float16_t>)
                    {
                        values.push_back(make_nullable<T>(static_cast<T>(static_cast<int>(i)), i % 2));
                    }
                    else if constexpr (std::is_same_v<T, bool>)
                    {
                        values.push_back(
                            make_nullable<T>(static_cast<bool>(i % 4 != 0), static_cast<bool>(i % 2))
                        );
                    }
                    else
                    {
                        values.push_back(make_nullable<T>(static_cast<T>(i), i % 2));
                    }
                }
                return values;
            };

            const auto make_test_nullable = [](auto value, bool flag = true)
            {
                if constexpr (std::same_as<T, bool>)
                {
                    return make_nullable<bool>(value != 0, std::move(flag));
                }
                else
                {
                    return make_nullable<T>(static_cast<T>(value), std::move(flag));
                }
            };

            const size_t values_count = 100;
            const auto nullable_values = make_nullable_values(values_count);

            const auto make_array = [](const std::vector<nullable<T>>& values, size_t offset = 0)
            {
                auto arr = array_test_type(values);
                if (offset != 0)
                {
                    return arr.slice(offset, arr.size());
                }
                else
                {
                    return arr;
                }
            };

            const size_t offset = 9;

            SUBCASE("constructors")
            {
                SUBCASE("value count, value, nullable, name, metadata")
                {
                    using namespace std::literals;
                    SUBCASE("nullable == true")
                    {
                        array_test_type ar{values_count, T(99), true, "test"sv, metadata_sample_opt};
                        CHECK_EQ(ar.size(), values_count);
                        for (size_t i = 0; i < ar.size(); ++i)
                        {
                            CHECK(ar[i].has_value());
                        }
                        CHECK_EQ(ar.name(), "test");
                        test_metadata(metadata_sample, *(ar.metadata()));
                    }

                    SUBCASE("nullable == false")
                    {
                        array_test_type ar{values_count, T(99), false, "test"sv, metadata_sample_opt};
                        CHECK_EQ(ar.size(), values_count);
                        for (size_t i = 0; i < ar.size(); ++i)
                        {
                            CHECK(ar[i].has_value());
                        }
                        CHECK_EQ(ar.name(), "test");
                        test_metadata(metadata_sample, *(ar.metadata()));
                    }
                }

                SUBCASE("u8_buffer, size, bitmap, name, metadata")
                {
                    u8_buffer<T> buffer{
                        nullable_values
                        | std::views::transform(
                            [](const auto& v)
                            {
                                return v.get();
                            }
                        )
                    };
                    array_test_type ar{
                        std::move(buffer),
                        values_count,
                        nullable_values
                            | std::views::transform(
                                [](const auto& v)
                                {
                                    return v.has_value();
                                }
                            )
                    };
                    CHECK_EQ(ar.size(), values_count);
                    for (size_t i = 0; i < ar.size(); ++i)
                    {
                        CHECK_EQ(nullable_values[i].has_value(), ar[i].has_value());
                    }
                }
            }

            array_test_type ar = make_array(nullable_values, offset);
            CHECK_EQ(ar.size(), nullable_values.size() - offset);

            SUBCASE("operator[]")
            {
                SUBCASE("const")
                {
                    const array_test_type const_ar = make_array(nullable_values, offset);
                    REQUIRE_EQ(const_ar.size(), nullable_values.size() - offset);
                    for (size_t i = 0; i < const_ar.size(); ++i)
                    {
                        CHECK_EQ(const_ar[i], nullable_values[i + offset]);
                    }
                }

                SUBCASE("mutable")
                {
                    REQUIRE_EQ(ar.size(), nullable_values.size() - offset);
                    for (size_t i = 0; i < ar.size(); ++i)
                    {
                        CHECK_EQ(ar[i], nullable_values[i + offset]);
                    }

                    auto new_value = make_test_nullable(99);
                    ar[1] = new_value;
                    CHECK(ar[1].has_value());
                    CHECK_EQ(ar[1].get(), new_value.get());
                }
            }

            SUBCASE("front")
            {
                SUBCASE("const")
                {
                    const array_test_type const_ar{ar};
                    CHECK_EQ(ar.front(), nullable_values[offset]);
                }
            }

            SUBCASE("back")
            {
                SUBCASE("const")
                {
                    const array_test_type const_ar{ar};
                    CHECK_EQ(ar.back(), nullable_values.back());
                }
            }

            SUBCASE("copy")
            {
                array_test_type ar2(ar);

                CHECK_EQ(ar, ar2);

                array_test_type ar3(make_array(make_nullable_values(7)));
                CHECK_NE(ar, ar3);
                ar3 = ar;
                CHECK_EQ(ar, ar3);
            }

            SUBCASE("move")
            {
                array_test_type ar2(ar);

                array_test_type ar3(std::move(ar));
                CHECK_EQ(ar2, ar3);

                array_test_type ar4(make_array(make_nullable_values(7)));
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
                for (size_t i = 0; i < ar_values.size(); ++i)
                {
                    CHECK_EQ(*citer, nullable_values[i + offset].get());
                    ++citer;
                }
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
                for (size_t i = 0; i < ar_values.size(); ++i)
                {
                    CHECK_EQ(*citer, nullable_values[i + offset].get());
                    ++citer;
                }
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
                for (size_t i = 0; i < ar_bitmap.size(); ++i)
                {
                    CHECK_EQ(*citer, nullable_values[i + offset].has_value());
                    ++citer;
                }
            }

            SUBCASE("iterator")
            {
                auto it = ar.begin();
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    CHECK_EQ(*it, nullable_values[i + offset]);
                    ++it;
                }
                CHECK_EQ(it, ar.end());
            }

            SUBCASE("reverse_iterator")
            {
                auto it = ar.rbegin();
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    const auto idx = ar.size() - 1 - i + offset;
                    CHECK_EQ(*it, nullable_values[idx]);
                    ++it;
                }
                CHECK_EQ(it, ar.rend());
            }

            SUBCASE("resize")
            {
                const size_t new_size = nullable_values.size() - offset + 3;
                const nullable<T> new_value_nullable = make_test_nullable(99);
                ar.resize(new_size, new_value_nullable);
                REQUIRE_EQ(ar.size(), new_size);
                for (size_t i = 0; i < ar.size() - 3; ++i)
                {
                    CHECK_EQ(ar[i], nullable_values[i + offset]);
                }
                CHECK_EQ(ar[ar.size() - 3], new_value_nullable);
                CHECK_EQ(ar[ar.size() - 2], new_value_nullable);
                CHECK_EQ(ar[ar.size() - 1], new_value_nullable);
            }

            SUBCASE("insert")
            {
                const nullable<T> new_value_nullable = make_test_nullable(99, false);

                SUBCASE("with pos and value")
                {
                    SUBCASE("at the beginning")
                    {
                        const auto pos = ar.cbegin();
                        const auto iter = ar.insert(pos, new_value_nullable);
                        CHECK_EQ(iter, ar.begin());
                        CHECK_EQ(ar[0], new_value_nullable);
                        for (size_t i = 1; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset - 1]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        const size_t idx = ar.size() / 2;
                        const auto pos = sparrow::next(ar.cbegin(), idx);
                        const auto iter = ar.insert(pos, new_value_nullable);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), idx));
                        for (size_t i = 0; i < idx; ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset]);
                        }
                        CHECK_EQ(ar[idx], new_value_nullable);
                        for (size_t i = idx + 1; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset - 1]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = ar.cend();
                        const auto distance = std::distance(ar.cbegin(), ar.cend());
                        const auto iter = ar.insert(pos, new_value_nullable);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), distance));
                        for (size_t i = 0; i < ar.size() - 1; ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset]);
                        }
                        CHECK_EQ(ar[ar.size() - 1], new_value_nullable);
                    }
                }

                SUBCASE("with pos, count and value")
                {
                    const size_t count = 3;

                    SUBCASE("at the beginning")
                    {
                        const auto pos = ar.cbegin();
                        const auto iter = ar.insert(pos, new_value_nullable, count);
                        CHECK_EQ(iter, ar.begin());
                        for (size_t i = 0; i < count; ++i)
                        {
                            CHECK_EQ(ar[i], new_value_nullable);
                        }
                        for (size_t i = count; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset - count]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        const auto idx = ar.size() / 2;
                        const auto pos = sparrow::next(ar.cbegin(), idx);
                        const auto iter = ar.insert(pos, new_value_nullable, count);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), idx));
                        for (size_t i = 0; i < idx; ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset]);
                        }
                        for (size_t i = idx; i < idx + count; ++i)
                        {
                            CHECK_EQ(ar[i], new_value_nullable);
                        }
                        for (size_t i = idx + count; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset - count]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = ar.cend();
                        const auto distance = std::distance(ar.cbegin(), ar.cend());
                        const auto iter = ar.insert(pos, new_value_nullable, 3);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), distance));
                        for (size_t i = 0; i < ar.size() - 3; ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset]);
                        }
                        for (size_t i = ar.size() - 3; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], new_value_nullable);
                        }
                    }
                }

                const auto new_val_99 = make_test_nullable(99, true);
                const auto new_val_100 = make_test_nullable(100, false);
                const auto new_val_101 = make_test_nullable(101, true);
                const std::array<nullable<T>, 3> new_values{new_val_99, new_val_100, new_val_101};

                SUBCASE("with pos, first and last iterators")
                {
                    SUBCASE("at the beginning")
                    {
                        const auto pos = ar.cbegin();
                        const auto iter = ar.insert(pos, new_values);
                        CHECK_EQ(iter, ar.begin());
                        for (size_t i = 0; i < 3; ++i)
                        {
                            CHECK_EQ(ar[i], new_values[i]);
                        }
                        for (size_t i = 3; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset - 3]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        const size_t idx = ar.size() / 2;
                        const auto pos = sparrow::next(ar.cbegin(), idx);
                        const auto iter = ar.insert(pos, new_values);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), idx));
                        for (size_t i = 0; i < idx; ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset]);
                        }
                        for (size_t i = idx; i < idx + 3; ++i)
                        {
                            CHECK_EQ(ar[i], new_values[i - idx]);
                        }
                        for (size_t i = idx + 3; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset - 3]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = ar.cend();
                        const auto distance = std::distance(ar.cbegin(), ar.cend());
                        const auto iter = ar.insert(pos, new_values);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), distance));
                        for (size_t i = 0; i < ar.size() - 3; ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset]);
                        }
                        for (size_t i = ar.size() - 3; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], new_values[i - ar.size() + 3]);
                        }
                    }
                }

                SUBCASE("with pos and initializer list")
                {
                    SUBCASE("at the beginning")
                    {
                        const auto pos = ar.cbegin();
                        const auto iter = ar.insert(pos, {new_val_99, new_val_100, new_val_101});
                        CHECK_EQ(iter, ar.begin());
                        CHECK_EQ(ar[0], new_val_99);
                        CHECK_EQ(ar[1], new_val_100);
                        CHECK_EQ(ar[2], new_val_101);
                        for (size_t i = 3; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset - 3]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        const size_t idx = ar.size() / 2;
                        const auto pos = sparrow::next(ar.cbegin(), idx);
                        const auto iter = ar.insert(pos, {new_val_99, new_val_100, new_val_101});
                        CHECK_EQ(iter, sparrow::next(ar.begin(), idx));
                        for (size_t i = 0; i < idx; ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset]);
                        }
                        CHECK_EQ(ar[idx], new_val_99);
                        CHECK_EQ(ar[idx + 1], new_val_100);
                        CHECK_EQ(ar[idx + 2], new_val_101);
                        for (size_t i = idx + 3; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset - 3]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = ar.cend();
                        const auto distance = std::distance(ar.cbegin(), ar.cend());
                        const auto iter = ar.insert(pos, {new_val_99, new_val_100, new_val_101});
                        CHECK_EQ(iter, sparrow::next(ar.begin(), distance));
                        for (size_t i = 0; i < ar.size() - 3; ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset]);
                        }
                        CHECK_EQ(ar[ar.size() - 3], new_val_99);
                        CHECK_EQ(ar[ar.size() - 2], new_val_100);
                        CHECK_EQ(ar[ar.size() - 1], new_val_101);
                    }
                }

                SUBCASE("with pos and range")
                {
                    SUBCASE("at the beginning")
                    {
                        const auto pos = ar.cbegin();
                        const auto iter = ar.insert(pos, new_values);
                        CHECK_EQ(iter, ar.begin());
                        for (size_t i = 0; i < 3; ++i)
                        {
                            CHECK_EQ(ar[i], new_values[i]);
                        }
                        for (size_t i = 3; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset - 3]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        const size_t idx = ar.size() / 2;
                        const auto pos = sparrow::next(ar.cbegin(), idx);
                        const auto iter = ar.insert(pos, new_values);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), idx));
                        for (size_t i = 0; i < idx; ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset]);
                        }
                        for (size_t i = idx; i < idx + 3; ++i)
                        {
                            CHECK_EQ(ar[i], new_values[i - idx]);
                        }
                        for (size_t i = idx + 3; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset - 3]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = ar.cend();
                        const auto distance = std::distance(ar.cbegin(), ar.cend());
                        const auto iter = ar.insert(pos, new_values);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), distance));
                        for (size_t i = 0; i < ar.size() - 3; ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset]);
                        }
                        for (size_t i = ar.size() - 3; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], new_values[i - ar.size() + 3]);
                        }
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
                        REQUIRE_EQ(ar.size(), nullable_values.size() - offset - 1);
                        for (size_t i = 0; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset + 1]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        const size_t idx = ar.size() / 2;
                        const auto pos = sparrow::next(ar.cbegin(), idx);
                        const auto iter = ar.erase(pos);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), idx));
                        REQUIRE_EQ(ar.size(), nullable_values.size() - offset - 1);
                        for (size_t i = 0; i < idx; ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset]);
                        }
                        for (size_t i = idx; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset + 1]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        const auto pos = std::prev(ar.cend());
                        const auto iter = ar.erase(pos);
                        CHECK_EQ(iter, sparrow::next(ar.begin(), ar.size()));
                        REQUIRE_EQ(ar.size(), nullable_values.size() - offset - 1);
                        for (size_t i = 0; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], nullable_values[i + offset]);
                        }
                    }
                }

                SUBCASE("with iterators")
                {
                    const auto pos = ar.cbegin() + 1;
                    const size_t count = 2;
                    const auto iter = ar.erase(pos, pos + count);
                    CHECK_EQ(iter, ar.begin() + 1);
                    REQUIRE_EQ(ar.size(), nullable_values.size() - offset - count);
                    for (size_t i = 0; i < 1; ++i)
                    {
                        CHECK_EQ(ar[i], nullable_values[i + offset]);
                    }
                    for (size_t i = 1; i < ar.size(); ++i)
                    {
                        CHECK_EQ(ar[i], nullable_values[i + offset + count]);
                    }
                }
            }

            SUBCASE("push_back")
            {
                const nullable<T> new_value = make_test_nullable(99, true);
                ar.push_back(new_value);
                REQUIRE_EQ(ar.size(), nullable_values.size() - offset + 1);
                for (size_t i = 0; i < ar.size() - 1; ++i)
                {
                    CHECK_EQ(ar[i], nullable_values[i + offset]);
                }
            }

            SUBCASE("pop_back")
            {
                ar.pop_back();
                REQUIRE_EQ(ar.size(), nullable_values.size() - offset - 1);
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    CHECK_EQ(ar[i], nullable_values[i + offset]);
                }
            }

            SUBCASE("zero_null_values")
            {
                ar.zero_null_values();
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    if (!ar[i].has_value())
                    {
                        CHECK_EQ(ar[i].get(), T(0));
                    }
                }
            }
        }
        TEST_CASE_TEMPLATE_APPLY(primitive_array_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("convenience_constructors", T, convenience_constructors_id)
        {
            using inner_value_type = T;

            const std::vector<inner_value_type> data = {
                static_cast<inner_value_type>(0),
                static_cast<inner_value_type>(1),
                static_cast<inner_value_type>(2),
                static_cast<inner_value_type>(3)
            };
            SUBCASE("range-of-inner-values")
            {
                primitive_array<T> arr(data);
                CHECK_EQ(arr.size(), data.size());
                for (std::size_t i = 0; i < data.size(); ++i)
                {
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
                primitive_array<T> arr = {T(0), T(1), T(2)};
                CHECK_EQ(arr[0].value(), T(0));
                CHECK_EQ(arr[1].value(), T(1));
                CHECK_EQ(arr[2].value(), T(2));
            }
        }
        TEST_CASE_TEMPLATE_APPLY(convenience_constructors_id, testing_types);

        static constexpr std::string_view name = "name";

        TEST_CASE("convenience_constructors_from_iota")
        {
            constexpr size_t count = 4;
            auto iota = std::ranges::iota_view{std::size_t(0), std::size_t(count)};
            const primitive_array<std::size_t> arr(iota, false, name, metadata_sample_opt);
            CHECK_EQ(arr.name(), name);
            test_metadata(metadata_sample, *(arr.metadata()));
            REQUIRE(arr.size() == count);
            for (std::size_t i = 0; i < count; ++i)
            {
                REQUIRE(arr[i].has_value());
                CHECK_EQ(arr[i].value(), i);
            }
        }

        TEST_CASE("convenience_constructors_index_of_missing")
        {
            constexpr size_t count = 5;
            auto iota = std::ranges::iota_view{std::size_t(0), std::size_t(count)};
            const primitive_array<std::size_t> arr(iota, std::vector<std::size_t>{1, 3}, name, metadata_sample_opt);
            CHECK_EQ(arr.name(), name);
            test_metadata(metadata_sample, *(arr.metadata()));
            REQUIRE(arr.size() == count);
            CHECK(arr[0].has_value());
            CHECK(!arr[1].has_value());
            CHECK(arr[2].has_value());
            CHECK(!arr[3].has_value());
            CHECK(arr[4].has_value());

            CHECK_EQ(arr[0].value(), std::size_t(0));
            CHECK_EQ(arr[2].value(), std::size_t(2));
            CHECK_EQ(arr[4].value(), std::size_t(4));
        }

        TEST_CASE("convenience_constructor_from_u8_buffer")
        {
            size_t size = 10;
            auto* data = new int32_t[size];
            for (auto i = 0u; i < size; ++i)
            {
                data[i] = static_cast<int32_t>(i);
            }
            sparrow::u8_buffer<int32_t> buffer(data, size, sparrow::u8_buffer<int32_t>::default_allocator());
            sparrow::primitive_array<int32_t> primitive_array(std::move(buffer), size);
            CHECK_EQ(primitive_array.size(), size);
        }

#if defined(__cpp_lib_format)
        TEST_CASE("formatting")
        {
            auto iota = std::ranges::iota_view{std::uint32_t(0), std::uint32_t(5)};
            primitive_array<uint32_t> arr(iota, std::vector<std::size_t>{1, 3});
            const std::string formatted = std::format("{}", arr);
            constexpr std::string_view expected = "uint32 [name=nullptr | size=5] <0, null, 2, null, 4>";
            CHECK_EQ(formatted, expected);
        }
#endif

        TEST_CASE("Check no copy")
        {
#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-align"
#endif
            size_t num_rows = 100000;
            using allocator_type = sparrow::u8_buffer<uint64_t>::default_allocator;
            uint8_t* data_ptr = allocator_type().allocate(sizeof(uint64_t) * num_rows);
            auto cast_ptr = reinterpret_cast<uint64_t*>(data_ptr);
            for (size_t idx = 0; idx < num_rows; ++idx)
            {
                cast_ptr[idx] = idx;
            }
            sparrow::u8_buffer<uint64_t> u8_buffer(
                reinterpret_cast<uint64_t*>(data_ptr),
                num_rows,
                sparrow::u8_buffer<uint64_t>::default_allocator()
            );
            for (size_t idx = 0; idx < num_rows; ++idx)
            {
                CHECK_EQ(cast_ptr[idx], idx);
                CHECK_EQ(u8_buffer[idx], idx);
            }

            sparrow::primitive_array<uint64_t> primitive_array{std::move(u8_buffer), num_rows};
            for (size_t idx = 0; idx < num_rows; ++idx)
            {
                CHECK_EQ(cast_ptr[idx], idx);
            }
#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif
        }

        TEST_CASE("offset_and_null_count")
        {
            SUBCASE("initial offset is 0")
            {
                primitive_array<int32_t> arr{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
                CHECK_EQ(arr.offset(), 0);
                CHECK_EQ(arr.null_count(), 0);
                CHECK_EQ(arr.size(), 10);
            }

            SUBCASE("offset after slicing")
            {
                constexpr size_t slice_start = 3;
                constexpr size_t slice_end = 8;
                primitive_array<int32_t> arr{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

                auto sliced = arr.slice(slice_start, slice_end);
                CHECK_EQ(sliced.offset(), slice_start);
                CHECK_EQ(sliced.size(), slice_end - slice_start);
            }

            SUBCASE("null_count with nulls")
            {
                std::vector<std::size_t> null_indices = {1, 3, 5};
                primitive_array<int32_t> arr(std::vector<int32_t>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, null_indices);

                CHECK_EQ(arr.offset(), 0);
                CHECK_EQ(arr.null_count(), static_cast<std::int64_t>(null_indices.size()));
                CHECK_EQ(arr.size(), 10);
            }

            SUBCASE("null_count after slicing array with nulls")
            {
                constexpr size_t slice_start = 2;
                constexpr size_t slice_end = 7;
                std::vector<std::size_t> null_indices = {1, 3, 5};
                primitive_array<int32_t> arr{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

                auto sliced = arr.slice(slice_start, slice_end);
                CHECK_EQ(sliced.offset(), slice_start);
                CHECK_EQ(sliced.size(), slice_end - slice_start);
                // Note: null_count is typically -1 (unknown) after slicing
                // unless explicitly recomputed
            }
        }
    }
}
