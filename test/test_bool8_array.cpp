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

#include <ranges>
#include <vector>

#include "sparrow/bool8_array.hpp"

#include "doctest/doctest.h"
#include "metadata_sample.hpp"

namespace sparrow
{
    TEST_SUITE("bool8_array")
    {
        TEST_CASE("constructors")
        {
            using array_test_type = bool8_array;

            const auto make_nullable_values = [](size_t count)
            {
                std::vector<nullable<bool>> values;
                values.reserve(count);
                for (size_t i = 0; i < count; ++i)
                {
                    if (i % 3 == 0)
                    {
                        values.emplace_back(nullval);
                    }
                    else
                    {
                        values.emplace_back(i % 2 == 0);
                    }
                }
                return values;
            };

            const size_t values_count = 100;
            const auto nullable_values = make_nullable_values(values_count);

            SUBCASE("value count, value, nullable, name, metadata")
            {
                using namespace std::literals;
                SUBCASE("nullable == true")
                {
                    array_test_type ar{values_count, true, true, "test"sv, metadata_sample_opt};
                    CHECK_EQ(ar.size(), values_count);
                    for (size_t i = 0; i < ar.size(); ++i)
                    {
                        CHECK(ar[i].has_value());
                        CHECK_EQ(static_cast<bool>(ar[i].value()), true);
                    }
                    CHECK_EQ(ar.name(), "test");
                    test_metadata(metadata_sample, *(ar.metadata()));
                }

                SUBCASE("nullable == false")
                {
                    array_test_type ar{values_count, false, false, "test"sv, metadata_sample_opt};
                    CHECK_EQ(ar.size(), values_count);
                    for (size_t i = 0; i < ar.size(); ++i)
                    {
                        CHECK(ar[i].has_value());
                        CHECK_EQ(static_cast<bool>(ar[i].value()), false);
                    }
                    CHECK_EQ(ar.name(), "test");
                    test_metadata(metadata_sample, *(ar.metadata()));
                }
            }

            SUBCASE("u8_buffer, size, bitmap, name, metadata")
            {
                u8_buffer<bool> buffer{
                    nullable_values
                    | std::views::transform(
                        [](const auto& v)
                        {
                            return v.has_value() ? v.value() : false;
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
                    CHECK_EQ(ar[i], nullable_values[i]);
                }
            }

            SUBCASE("from range of bool8_t")
            {
                std::vector<bool> values = {true, false, true, false};
                array_test_type ar(values);
                CHECK_EQ(ar.size(), values.size());
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    CHECK(ar[i].has_value());
                    CHECK_EQ(ar[i].value(), values[i]);
                }
            }

            SUBCASE("from range of nullable<bool8_t>")
            {
                std::vector<nullable<bool>> values{true, false, nullval, true};
                array_test_type ar(values);
                CHECK_EQ(ar.size(), values.size());
                CHECK(ar[0].has_value());
                CHECK(ar[1].has_value());
                CHECK(!ar[2].has_value());
                CHECK(ar[3].has_value());
                CHECK_EQ(static_cast<bool>(ar[0].value()), true);
                CHECK_EQ(static_cast<bool>(ar[1].value()), false);
                CHECK_EQ(static_cast<bool>(ar[3].value()), true);
            }

            SUBCASE("initializer list")
            {
                std::vector<bool> init_values = {true, false, true};
                array_test_type ar(init_values);
                CHECK_EQ(ar.size(), 3);
                CHECK_EQ(static_cast<bool>(ar[0].value()), true);
                CHECK_EQ(static_cast<bool>(ar[1].value()), false);
                CHECK_EQ(static_cast<bool>(ar[2].value()), true);
            }
        }

        TEST_CASE("operator[]")
        {
            using array_test_type = bool8_array;
            std::vector<bool> values{true, false, true, false, true};
            array_test_type ar(values);

            SUBCASE("const")
            {
                const array_test_type const_ar(values);
                REQUIRE_EQ(const_ar.size(), values.size());
                for (size_t i = 0; i < const_ar.size(); ++i)
                {
                    CHECK_EQ(const_ar[i].value(), values[i]);
                }
            }

            SUBCASE("mutable")
            {
                REQUIRE_EQ(ar.size(), values.size());
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    CHECK_EQ(ar[i].value(), values[i]);
                }

                // Test modifying an element
                auto ref = ar[1];
                ref = true;
                CHECK(ar[1].has_value());
                CHECK_EQ(static_cast<bool>(ar[1].value()), true);
            }
        }

        TEST_CASE("front and back")
        {
            std::vector<bool> values = {true, false, true};
            const bool8_array ar(values);

            SUBCASE("front")
            {
                CHECK_EQ(static_cast<bool>(ar.front().value()), true);
            }

            SUBCASE("back")
            {
                CHECK_EQ(static_cast<bool>(ar.back().value()), true);
            }
        }

        TEST_CASE("copy")
        {
            std::vector<bool> values = {true, false, true, false};
            bool8_array ar(values);

            bool8_array ar2(ar);
            CHECK_EQ(ar, ar2);

            std::vector<bool> other_values = {false, false};
            bool8_array ar3(other_values);
            CHECK_NE(ar, ar3);
            ar3 = ar;
            CHECK_EQ(ar, ar3);
        }

        TEST_CASE("move")
        {
            std::vector<bool> values = {true, false, true, false};
            bool8_array ar(values);
            bool8_array ar2(ar);

            bool8_array ar3(std::move(ar));
            CHECK_EQ(ar2, ar3);

            std::vector<bool> other_values = {false, false};
            bool8_array ar4(other_values);
            CHECK_NE(ar2, ar4);
            ar4 = std::move(ar2);
            CHECK_EQ(ar3, ar4);
        }

        TEST_CASE("iterators")
        {
            std::vector<bool> values = {true, false, true, false, true};
            bool8_array ar(values);

            SUBCASE("iterator")
            {
                auto it = ar.begin();
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    CHECK_EQ((*it).value(), values[i]);
                    ++it;
                }
                CHECK_EQ(it, ar.end());
            }

            SUBCASE("reverse_iterator")
            {
                auto it = ar.rbegin();
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    CHECK_EQ((*it).value(), values[ar.size() - 1 - i]);
                    ++it;
                }
                CHECK_EQ(it, ar.rend());
            }

            SUBCASE("value_iterator")
            {
                auto ar_values = ar.values();
                auto it = ar_values.begin();
                for (size_t i = 0; i < ar_values.size(); ++i)
                {
                    CHECK_EQ(*it, values[i]);
                    ++it;
                }
                CHECK_EQ(it, ar_values.end());
            }

            SUBCASE("bitmap_iterator")
            {
                const auto ar_bitmap = ar.bitmap();
                auto it = ar_bitmap.begin();
                for (size_t i = 0; i < ar_bitmap.size(); ++i)
                {
                    CHECK(*it);
                    ++it;
                }
            }
        }

        TEST_CASE("resize")
        {
            std::vector<bool> values = {true, false, true};
            bool8_array ar(values);

            const size_t new_size = values.size() + 3;
            const nullable<bool> fill_value = false;
            ar.resize(new_size, fill_value);

            REQUIRE_EQ(ar.size(), new_size);
            for (size_t i = 0; i < values.size(); ++i)
            {
                CHECK_EQ(ar[i].value(), values[i]);
            }
        }

        TEST_CASE("insert")
        {
            std::vector<bool> values = {true, false, true, false};
            bool8_array ar(values);

            SUBCASE("with pos and value")
            {
                SUBCASE("at the beginning")
                {
                    const nullable<bool> new_value_nullable = true;
                    const auto pos = ar.cbegin();
                    const auto iter = ar.insert(pos, new_value_nullable);
                    CHECK_EQ(iter, ar.begin());
                    CHECK_EQ(static_cast<bool>(ar[0].value()), true);
                    for (size_t i = 1; i < ar.size(); ++i)
                    {
                        CHECK_EQ(ar[i].value(), values[i - 1]);
                    }
                }

                SUBCASE("in the middle")
                {
                    const nullable<bool> new_value_nullable = true;
                    const size_t idx = ar.size() / 2;
                    const auto pos = sparrow::next(ar.cbegin(), idx);
                    const auto iter = ar.insert(pos, new_value_nullable);
                    CHECK_EQ(iter, sparrow::next(ar.begin(), idx));
                    for (size_t i = 0; i < idx; ++i)
                    {
                        CHECK_EQ(ar[i].value(), values[i]);
                    }
                    CHECK_EQ(static_cast<bool>(ar[idx].value()), true);
                    for (size_t i = idx + 1; i < ar.size(); ++i)
                    {
                        CHECK_EQ(ar[i].value(), values[i - 1]);
                    }
                }

                SUBCASE("at the end")
                {
                    const nullable<bool> new_value_nullable = true;
                    const auto pos = ar.cend();
                    const auto distance = std::distance(ar.cbegin(), ar.cend());
                    const auto iter = ar.insert(pos, new_value_nullable);
                    CHECK_EQ(iter, sparrow::next(ar.begin(), distance));
                    for (size_t i = 0; i < ar.size() - 1; ++i)
                    {
                        CHECK_EQ(ar[i].value(), values[i]);
                    }
                    CHECK_EQ(static_cast<bool>(ar[ar.size() - 1].value()), true);
                }
            }

            SUBCASE("with pos, count and value")
            {
                const size_t count = 3;
                const nullable<bool> new_value_nullable = true;

                SUBCASE("at the beginning")
                {
                    const auto pos = ar.cbegin();
                    const auto iter = ar.insert(pos, new_value_nullable, count);
                    CHECK_EQ(iter, ar.begin());
                    for (size_t i = 0; i < count; ++i)
                    {
                        CHECK_EQ(static_cast<bool>(ar[i].value()), true);
                    }
                    for (size_t i = count; i < ar.size(); ++i)
                    {
                        CHECK_EQ(ar[i].value(), values[i - count]);
                    }
                }
            }

            SUBCASE("with pos and range")
            {
                const std::vector<nullable<bool>> new_values{true, false, true};

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
                        CHECK_EQ(ar[i].value(), values[i - 3]);
                    }
                }
            }
        }

        TEST_CASE("erase")
        {
            std::vector<bool> values = {true, false, true, false, true};
            bool8_array ar(values);

            SUBCASE("with pos")
            {
                SUBCASE("at the beginning")
                {
                    const auto pos = ar.cbegin();
                    const auto iter = ar.erase(pos);
                    CHECK_EQ(iter, ar.begin());
                    REQUIRE_EQ(ar.size(), values.size() - 1);
                    for (size_t i = 0; i < ar.size(); ++i)
                    {
                        CHECK_EQ(ar[i].value(), values[i + 1]);
                    }
                }

                SUBCASE("in the middle")
                {
                    const size_t idx = ar.size() / 2;
                    const auto pos = sparrow::next(ar.cbegin(), idx);
                    const auto iter = ar.erase(pos);
                    CHECK_EQ(iter, sparrow::next(ar.begin(), idx));
                    REQUIRE_EQ(ar.size(), values.size() - 1);
                    for (size_t i = 0; i < idx; ++i)
                    {
                        CHECK_EQ(ar[i].value(), values[i]);
                    }
                    for (size_t i = idx; i < ar.size(); ++i)
                    {
                        CHECK_EQ(ar[i].value(), values[i + 1]);
                    }
                }

                SUBCASE("at the end")
                {
                    const auto pos = std::prev(ar.cend());
                    const auto iter = ar.erase(pos);
                    CHECK_EQ(iter, sparrow::next(ar.begin(), ar.size()));
                    REQUIRE_EQ(ar.size(), values.size() - 1);
                    for (size_t i = 0; i < ar.size(); ++i)
                    {
                        CHECK_EQ(ar[i].value(), values[i]);
                    }
                }
            }

            SUBCASE("with iterators")
            {
                const auto pos = ar.cbegin() + 1;
                const size_t count = 2;
                const auto iter = ar.erase(pos, pos + count);
                CHECK_EQ(iter, ar.begin() + 1);
                REQUIRE_EQ(ar.size(), values.size() - count);
                CHECK_EQ(ar[0].value(), values[0]);
                for (size_t i = 1; i < ar.size(); ++i)
                {
                    CHECK_EQ(ar[i].value(), values[i + count]);
                }
            }
        }

        TEST_CASE("push_back and pop_back")
        {
            std::vector<bool> values = {true, false, true};
            bool8_array ar(values);

            SUBCASE("push_back")
            {
                const nullable<bool> new_value = false;
                ar.push_back(new_value);
                REQUIRE_EQ(ar.size(), values.size() + 1);
                for (size_t i = 0; i < values.size(); ++i)
                {
                    CHECK_EQ(ar[i].value(), values[i]);
                }
                CHECK_EQ(static_cast<bool>(ar[ar.size() - 1].value()), false);
            }

            SUBCASE("pop_back")
            {
                ar.pop_back();
                REQUIRE_EQ(ar.size(), values.size() - 1);
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    CHECK_EQ(ar[i].value(), values[i]);
                }
            }
        }

        TEST_CASE("slice")
        {
            std::vector<bool> values = {true, false, true, false, true};
            bool8_array ar(values);

            SUBCASE("slice in the middle")
            {
                constexpr size_t slice_start = 1;
                constexpr size_t slice_end = 4;
                auto sliced = ar.slice(slice_start, slice_end);

                CHECK_EQ(sliced.offset(), slice_start);
                CHECK_EQ(sliced.size(), slice_end - slice_start);
                for (size_t i = 0; i < sliced.size(); ++i)
                {
                    CHECK_EQ(sliced[i].value(), values[slice_start + i]);
                }
            }
        }

        TEST_CASE("offset_and_null_count")
        {
            SUBCASE("initial offset is 0")
            {
                std::vector<bool> values = {true, false, true};
                bool8_array ar(values);
                CHECK_EQ(ar.offset(), 0);
                CHECK_EQ(ar.null_count(), 0);
                CHECK_EQ(ar.size(), 3);
            }

            SUBCASE("offset after slicing")
            {
                constexpr size_t slice_start = 2;
                constexpr size_t slice_end = 5;
                std::vector<bool> values = {true, false, true, false, true, false};
                bool8_array ar(values);

                auto sliced = ar.slice(slice_start, slice_end);
                CHECK_EQ(sliced.offset(), slice_start);
                CHECK_EQ(sliced.size(), slice_end - slice_start);
            }

            SUBCASE("null_count with nulls")
            {
                std::vector<nullable<bool8_t>>
                    values{bool8_t(true), nullval, bool8_t(false), nullval, bool8_t(true)};
                bool8_array ar(values);

                CHECK_EQ(ar.offset(), 0);
                CHECK_EQ(ar.null_count(), 2);
                CHECK_EQ(ar.size(), 5);
            }
        }

        TEST_CASE("zero_null_values")
        {
            std::vector<nullable<bool8_t>> values{bool8_t(true), nullval, bool8_t(false), nullval, bool8_t(true)};
            bool8_array ar(values);

            ar.zero_null_values();
            for (size_t i = 0; i < ar.size(); ++i)
            {
                if (!values[i].has_value())
                {
                    // Null values should be zeroed
                    CHECK_EQ(static_cast<int8_t>(ar[i].value()), 0);
                }
                else
                {
                    CHECK_EQ(ar[i].value(), values[i].value());
                }
            }
        }

#if defined(__cpp_lib_format)
        TEST_CASE("formatting")
        {
            std::vector<nullable<bool8_t>> values{bool8_t(true), nullval, bool8_t(false), nullval, bool8_t(true)};
            bool8_array ar(values);
            const std::string formatted = std::format("{}", ar);
            constexpr std::string_view expected = "Bool8 array [5]: [true, null, false, null, true]";
            CHECK_EQ(formatted, expected);
        }
#endif
    }
}
