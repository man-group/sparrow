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

#include "sparrow/array.hpp"
#include "sparrow/bool8_array.hpp"
#include "sparrow/layout/array_registry.hpp"

#include "doctest/doctest.h"
#include "metadata_sample.hpp"

namespace sparrow
{
    TEST_SUITE("bool8_array")
    {
        TEST_CASE("constructors")
        {
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
                    bool8_array ar{values_count, true, true, "test"sv, metadata_sample_opt};
                    CHECK_EQ(ar.size(), values_count);
                    for (size_t i = 0; i < ar.size(); ++i)
                    {
                        CHECK(ar[i].has_value());
                        CHECK_EQ(static_cast<bool>(ar[i].value()), true);
                    }
                    CHECK_EQ(ar.name(), "test");
                    std::vector<metadata_pair> expected_metadata = metadata_sample;
                    expected_metadata.emplace_back("ARROW:extension:name", bool8_array::EXTENSION_NAME);
                    expected_metadata.emplace_back("ARROW:extension:metadata", "");
                    test_metadata(expected_metadata, *(ar.metadata()));
                }

                SUBCASE("nullable == false")
                {
                    bool8_array ar{values_count, false, false, "test"sv, metadata_sample_opt};
                    CHECK_EQ(ar.size(), values_count);
                    for (size_t i = 0; i < ar.size(); ++i)
                    {
                        CHECK(ar[i].has_value());
                        CHECK_EQ(static_cast<bool>(ar[i].value()), false);
                    }
                    CHECK_EQ(ar.name(), "test");
                    std::vector<metadata_pair> expected_metadata = metadata_sample;
                    expected_metadata.emplace_back("ARROW:extension:name", bool8_array::EXTENSION_NAME);
                    expected_metadata.emplace_back("ARROW:extension:metadata", "");
                    test_metadata(expected_metadata, *(ar.metadata()));
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
                bool8_array ar{
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

            SUBCASE("from range of bool")
            {
                const std::vector<bool> values = {true, false, true, false};
                const bool8_array ar(values);
                REQUIRE_EQ(ar.size(), values.size());
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    REQUIRE(ar[i].has_value());
                    CHECK_EQ(ar[i].value(), values[i]);
                }
            }

            SUBCASE("from range of nullable<bool>")
            {
                const std::vector<nullable<bool>> values{true, false, nullval, true};
                const bool8_array ar(values);
                REQUIRE_EQ(ar.size(), values.size());
                REQUIRE(ar[0].has_value());
                REQUIRE(ar[1].has_value());
                REQUIRE(!ar[2].has_value());
                REQUIRE(ar[3].has_value());
                CHECK(ar[0].value());
                CHECK_FALSE(ar[1].value());
                CHECK(ar[3].value());
            }

            SUBCASE("initializer list")
            {
                const bool8_array ar{true, false, true};
                REQUIRE_EQ(ar.size(), 3);
                CHECK(ar[0].value());
                CHECK_FALSE(ar[1].value());
                CHECK(ar[2].value());
            }
        }

        TEST_CASE("operator[]")
        {
            const std::vector<bool> values{true, false, true, false, true};
            bool8_array ar(values);

            SUBCASE("const")
            {
                const bool8_array const_ar(values);
                REQUIRE_EQ(const_ar.size(), values.size());
                for (size_t i = 0; i < const_ar.size(); ++i)
                {
                    CHECK_EQ(const_ar[i].value(), values[i]);
                }
            }

            SUBCASE("mutable")
            {
                auto ref = ar[1];
                ref = true;
                REQUIRE(ar[1].has_value());
                CHECK_EQ(ar[1].value(), true);

                ar[2] = false;
                REQUIRE(ar[2].has_value());
                CHECK_EQ(ar[2].value(), false);
            }
        }

        TEST_CASE("front and back")
        {
            const std::vector<bool> values = {true, false, true};
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
            const std::vector<bool> values = {true, false, true, false};
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
            const std::vector<bool> values = {true, false, true, false};
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
            const std::vector<bool> values = {true, false, true, false, true};
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
            const std::vector<bool> values = {true, false, true};
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
            const std::vector<bool> values = {true, false, true, false};
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
                std::vector<nullable<bool>> values{true, nullval, false, nullval, true};
                bool8_array ar(values);

                CHECK_EQ(ar.offset(), 0);
                CHECK_EQ(ar.null_count(), 2);
                CHECK_EQ(ar.size(), 5);
            }
        }

        TEST_CASE("zero_null_values")
        {
            std::vector<nullable<bool>> values{true, nullval, false, nullval, true};
            bool8_array ar(values);

            ar.zero_null_values();
            for (size_t i = 0; i < ar.size(); ++i)
            {
                if (!values[i].has_value())
                {
                    // Null values should be zeroed
                    CHECK_EQ(static_cast<int8_t>(ar[i].get()), 0);
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
            std::vector<nullable<bool>> values{true, nullval, false, nullval, true};
            bool8_array ar(values);
            const std::string formatted = std::format("{}", ar);
            constexpr std::string_view expected = "Bool8 array [5]: [true, null, false, null, true]";
            CHECK_EQ(formatted, expected);
        }
#endif

        TEST_CASE("array_registry integration")
        {
            auto& registry = array_registry::instance();

            SUBCASE("bool8_array dispatch with size visitor")
            {
                std::vector<bool> values = {true, false, true, false, true};
                bool8_array bool8_arr(values);
                array arr(std::move(bool8_arr));
                
                // Test size dispatch
                auto size = arr.visit([](auto&& typed_array) {
                    return typed_array.size();
                });
                
                CHECK_EQ(size, 5);
            }

            SUBCASE("bool8_array dispatch to access elements")
            {
                std::vector<bool> values = {true, false, true};
                bool8_array bool8_arr(values);
                array arr(std::move(bool8_arr));
                
                // Access element via visit - just check it has a value
                auto has_value = arr.visit([](auto&& typed_array) {
                    return typed_array[0].has_value();
                });
                
                CHECK(has_value);
            }

            SUBCASE("bool8_array dispatch with iteration")
            {
                std::vector<bool> values = {true, false, true, true, false};
                bool8_array bool8_arr(values);
                array arr(std::move(bool8_arr));
                
                // Count all elements via dispatch
                size_t count = arr.visit([](auto&& typed_array) {
                    size_t c = 0;
                    for ([[maybe_unused]] const auto& elem : typed_array) {
                        c++;
                    }
                    return c;
                });
                
                CHECK_EQ(count, 5);
            }

            SUBCASE("bool8_array type detection")
            {
                std::vector<bool> values = {true, false};
                bool8_array bool8_arr(values);
                array arr(std::move(bool8_arr));
                
                // bool8_array is stored as INT8 (with bool8 extension metadata)
                CHECK_EQ(arr.data_type(), data_type::INT8);
                
                // The array class dispatches to the underlying storage type (primitive_array<int8_t>),
                // not the extension type (bool8_array), which is correct behavior
                auto result = arr.visit([](auto&& typed_array) {
                    using array_type = std::decay_t<decltype(typed_array)>;
                    return std::is_same_v<array_type, primitive_array<std::int8_t>>;
                });
                
                CHECK(result);
            }

            SUBCASE("bool8_array with null values")
            {
                std::vector<nullable<bool>> values = {
                    nullable<bool>(true),
                    nullable<bool>(),  // null
                    nullable<bool>(false),
                    nullable<bool>(),  // null
                    nullable<bool>(true)
                };
                bool8_array bool8_arr(values);
                array arr(std::move(bool8_arr));
                
                auto non_null_count = arr.visit([](auto&& typed_array) {
                    size_t count = 0;
                    for (size_t i = 0; i < typed_array.size(); ++i) {
                        if (typed_array[i].has_value()) {
                            count++;
                        }
                    }
                    return count;
                });
                
                CHECK_EQ(non_null_count, 3);
            }

            SUBCASE("registry dispatch via underlying wrapper")
            {
                std::vector<bool> values = {true, false};
                bool8_array bool8_arr(values);
                
                // Create wrapper manually for registry dispatch test
                auto wrapper_ptr = std::make_unique<array_wrapper_impl<bool8_array>>(std::move(bool8_arr));
                
                // Dispatch via registry  
                auto size = registry.dispatch([](auto&& typed_array) {
                    return typed_array.size();
                }, *wrapper_ptr);
                
                CHECK_EQ(size, 2);
            }

            SUBCASE("bool8_array counting true/false values")
            {
                std::vector<bool> values = {true, true, false, true, false, false};
                bool8_array bool8_arr(values);
                array arr(std::move(bool8_arr));
                
                // Just verify we can count all elements
                auto total_count = arr.visit([](auto&& typed_array) {
                    return typed_array.size();
                });
                
                CHECK_EQ(total_count, 6);
            }

            SUBCASE("bool8_array all elements have values")
            {
                std::vector<bool> values = {true, true, true, true};
                bool8_array bool8_arr(values);
                array arr(std::move(bool8_arr));
                
                // Check all elements have values
                auto all_have_values = arr.visit([](auto&& typed_array) {
                    for (size_t i = 0; i < typed_array.size(); ++i) {
                        if (!typed_array[i].has_value()) {
                            return false;
                        }
                    }
                    return true;
                });
                
                CHECK(all_have_values);
            }

            SUBCASE("bool8_array empty check")
            {
                std::vector<bool> values = {false, false, false};
                bool8_array bool8_arr(values);
                array arr(std::move(bool8_arr));
                
                // Verify array is not empty
                auto not_empty = arr.visit([](auto&& typed_array) {
                    return typed_array.size() > 0;
                });
                
                CHECK(not_empty);
            }
        }
    }
}
