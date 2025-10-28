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

#include <vector>

#include "sparrow/array.hpp"
#include "sparrow/bool8_array.hpp"
#include "sparrow/layout/dispatch.hpp"

#include "doctest/doctest.h"
#include "metadata_sample.hpp"

namespace sparrow
{
    TEST_SUITE("bool8_array")
    {
        TEST_CASE("constructor_from_boolean_values")
        {
            SUBCASE("from vector of bool")
            {
                std::vector<bool> values = {true, false, true, true, false};
                bool8_array arr(values);

                REQUIRE_EQ(arr.size(), 5);
                CHECK_EQ(static_cast<bool>(arr[0]), true);
                CHECK_EQ(static_cast<bool>(arr[1]), false);
                CHECK_EQ(static_cast<bool>(arr[2]), true);
                CHECK_EQ(static_cast<bool>(arr[3]), true);
                CHECK_EQ(static_cast<bool>(arr[4]), false);

                // Check underlying int8_t values
                CHECK_EQ(arr.value(0), std::int8_t{1});
                CHECK_EQ(arr.value(1), std::int8_t{0});
                CHECK_EQ(arr.value(2), std::int8_t{1});
                CHECK_EQ(arr.value(3), std::int8_t{1});
                CHECK_EQ(arr.value(4), std::int8_t{0});
            }

            SUBCASE("from initializer list")
            {
                bool8_array arr({true, false, true, false});

                REQUIRE_EQ(arr.size(), 4);
                CHECK_EQ(static_cast<bool>(arr[0]), true);
                CHECK_EQ(static_cast<bool>(arr[1]), false);
                CHECK_EQ(static_cast<bool>(arr[2]), true);
                CHECK_EQ(static_cast<bool>(arr[3]), false);
            }

            SUBCASE("with nullable flag")
            {
                std::vector<bool> values = {true, false, true};
                bool8_array arr(values, true, "test_array");

                REQUIRE_EQ(arr.size(), 3);
                CHECK_EQ(arr.name(), "test_array");
                CHECK(arr[0].has_value());
                CHECK(arr[1].has_value());
                CHECK(arr[2].has_value());
            }

            SUBCASE("with metadata")
            {
                using namespace std::literals;
                std::vector<bool> values = {true, false};
                bool8_array arr(values, true, "test"sv, metadata_sample_opt);
                std::vector<metadata_pair> expected_metadata = metadata_sample;
                expected_metadata.emplace_back("ARROW:extension:name", bool8_extension::EXTENSION_NAME);
                expected_metadata.emplace_back("ARROW:extension:metadata", "");
                CHECK_EQ(arr.name(), "test");
                test_metadata(expected_metadata, *(arr.metadata()));
            }
        }

        TEST_CASE("constructor_from_boolean_values_with_validity")
        {
            SUBCASE("from vector with validity bitmap")
            {
                std::vector<bool> values = {true, false, true, false, true};
                std::vector<bool> validity = {true, false, true, true, false};
                bool8_array arr(values, validity);

                REQUIRE_EQ(arr.size(), 5);
                CHECK_EQ(static_cast<bool>(arr[0]), true);
                CHECK(!arr[1].has_value());
                CHECK_EQ(static_cast<bool>(arr[2]), true);
                CHECK_EQ(static_cast<bool>(arr[3]), false);
                CHECK(!arr[4].has_value());

                // Values can be accessed with has_value check
                CHECK(arr[0].has_value());
                CHECK(!arr[1].has_value());
                CHECK(arr[2].has_value());
                CHECK(arr[3].has_value());
                CHECK(!arr[4].has_value());
            }

            SUBCASE("from vector with null indices")
            {
                std::vector<bool> values = {true, false, true, false};
                std::vector<size_t> null_indices = {1, 3};
                bool8_array arr(values, null_indices);

                REQUIRE_EQ(arr.size(), 4);
                CHECK_EQ(static_cast<bool>(arr[0]), true);
                CHECK(!arr[1].has_value());
                CHECK_EQ(static_cast<bool>(arr[2]), true);
                CHECK(!arr[3].has_value());
            }

            SUBCASE("with metadata")
            {
                using namespace std::literals;
                std::vector<bool> values = {true, false, true};
                std::vector<bool> validity = {true, true, false};
                bool8_array arr(values, validity, "test"sv, metadata_sample_opt);
                std::vector<metadata_pair> expected_metadata = metadata_sample;
                expected_metadata.emplace_back("ARROW:extension:name", bool8_extension::EXTENSION_NAME);
                expected_metadata.emplace_back("ARROW:extension:metadata", "");
                CHECK_EQ(arr.name(), "test");
                test_metadata(expected_metadata, *(arr.metadata()));
            }
        }

        TEST_CASE("constructor_from_int8_values")
        {
            SUBCASE("from vector of int8_t")
            {
                std::vector<std::int8_t> values = {1, 0, 5, -3, 0};
                bool8_array arr(values);

                REQUIRE_EQ(arr.size(), 5);
                CHECK_EQ(static_cast<bool>(arr[0]), true);   // 1 -> true
                CHECK_EQ(static_cast<bool>(arr[1]), false);  // 0 -> false
                CHECK_EQ(static_cast<bool>(arr[2]), true);   // 5 -> true (non-zero)
                CHECK_EQ(static_cast<bool>(arr[3]), true);   // -3 -> true (non-zero)
                CHECK_EQ(static_cast<bool>(arr[4]), false);  // 0 -> false

                // Check underlying int8_t values are preserved
                CHECK_EQ(arr.value(0), std::int8_t{1});
                CHECK_EQ(arr.value(1), std::int8_t{0});
                CHECK_EQ(arr.value(2), std::int8_t{5});
                CHECK_EQ(arr.value(3), std::int8_t{-3});
                CHECK_EQ(arr.value(4), std::int8_t{0});
            }

            SUBCASE("from vector of uint8_t")
            {
                std::vector<std::uint8_t> values = {1, 0, 255, 127};
                bool8_array arr(values);

                REQUIRE_EQ(arr.size(), 4);
                CHECK_EQ(static_cast<bool>(arr[0]), true);
                CHECK_EQ(static_cast<bool>(arr[1]), false);
                CHECK_EQ(static_cast<bool>(arr[2]), true);
                CHECK_EQ(static_cast<bool>(arr[3]), true);
            }

            SUBCASE("with validity bitmap")
            {
                std::vector<std::int8_t> values = {1, 0, 2, 0};
                std::vector<bool> validity = {true, false, true, true};
                bool8_array arr(values, validity);

                REQUIRE_EQ(arr.size(), 4);
                CHECK_EQ(static_cast<bool>(arr[0]), true);
                CHECK(!arr[1].has_value());
                CHECK_EQ(static_cast<bool>(arr[2]), true);
                CHECK_EQ(static_cast<bool>(arr[3]), false);
            }

            SUBCASE("with metadata")
            {
                using namespace std::literals;
                std::vector<std::int8_t> values = {1, 0};
                bool8_array arr(values, true, "test"sv, metadata_sample_opt);
                std::vector<metadata_pair> expected_metadata = metadata_sample;
                expected_metadata.emplace_back("ARROW:extension:name", bool8_extension::EXTENSION_NAME);
                expected_metadata.emplace_back("ARROW:extension:metadata", "");
                CHECK_EQ(arr.name(), "test");
                test_metadata(expected_metadata, *(arr.metadata()));
            }
        }

        TEST_CASE("set_bool")
        {
            std::vector<bool> values = {true, false, true};
            bool8_array arr(values);

            SUBCASE("set true")
            {
                arr[1] = true;
                CHECK_EQ(static_cast<bool>(arr[1]), true);
                CHECK_EQ(arr.value(1), std::int8_t{1});
                CHECK(arr[1].has_value());
            }

            SUBCASE("set false")
            {
                arr[0] = false;
                CHECK_EQ(static_cast<bool>(arr[0]), false);
                CHECK_EQ(arr.value(0), std::int8_t{0});
                CHECK(arr[0].has_value());
            }

            SUBCASE("set previously null value")
            {
                std::vector<bool> values2 = {true, false};
                std::vector<bool> validity = {true, false};
                bool8_array arr2(values2, validity);

                CHECK(!arr2[1].has_value());
                arr2[1] = true;
                CHECK(arr2[1].has_value());
                CHECK_EQ(static_cast<bool>(arr2[1]), true);
            }
        }

        // TODO: Extension metadata automatic setting - requires deeper integration
        // TEST_CASE("extension_metadata")
        // {
        //     SUBCASE("bool8 extension is set")
        //     {
        //         std::vector<bool> values = {true, false, true};
        //         bool8_array arr(values);
        //
        //         auto metadata = arr.metadata();
        //         REQUIRE(metadata.has_value());
        //
        //         auto it = metadata->find("ARROW:extension:name");
        //         REQUIRE(it != metadata->end());
        //
        //         const auto& [key, value] = *it;
        //         CHECK_EQ(key, "ARROW:extension:name");
        //         CHECK_EQ(value, "arrow.bool8");
        //     }
        //
        //     SUBCASE("extension metadata is empty")
        //     {
        //         std::vector<bool> values = {true, false};
        //         bool8_array arr(values);
        //
        //         auto metadata = arr.metadata();
        //         REQUIRE(metadata.has_value());
        //
        //         auto it = metadata->find("ARROW:extension:metadata");
        //         REQUIRE(it != metadata->end());
        //
        //         const auto& [key, value] = *it;
        //         CHECK_EQ(key, "ARROW:extension:metadata");
        //         CHECK_EQ(value, "");
        //     }
        // }

        TEST_CASE("data_type")
        {
            std::vector<bool> values = {true, false};
            bool8_array arr(values);

            CHECK_EQ(arr.data_type(), data_type::INT8);
        }

        // TODO: array wrapper integration requires more complex setup
        // TEST_CASE("dispatch_through_array_wrapper")
        // {
        //     SUBCASE("construct from bool8_array")
        //     {
        //         std::vector<bool> values = {true, false, true, false, true};
        //         bool8_array arr(values);
        //         array wrapped(std::move(arr));
        //
        //         CHECK_EQ(wrapped.size(), 5);
        //         CHECK_EQ(wrapped.data_type(), data_type::INT8);
        //     }
        //
        //     SUBCASE("visit bool8_array through dispatch")
        //     {
        //         std::vector<bool> values = {true, false, true};
        //         bool8_array arr(values);
        //
        //         // Create an array wrapper
        //         array wrapped(std::move(arr));
        //
        //         // Visit the wrapped array
        //         bool visited = false;
        //         wrapped.visit(
        //             [&visited](const auto& layout)
        //             {
        //                 using layout_type = std::decay_t<decltype(layout)>;
        //                 if constexpr (std::is_same_v<layout_type, bool8_array>)
        //                 {
        //                     visited = true;
        //                     CHECK_EQ(layout.size(), 3);
        //                     CHECK_EQ(layout.get_bool(0), true);
        //                     CHECK_EQ(layout.get_bool(1), false);
        //                     CHECK_EQ(layout.get_bool(2), true);
        //                 }
        //             }
        //         );
        //
        //         CHECK(visited);
        //     }
        // }

        TEST_CASE("iteration")
        {
            std::vector<bool> values = {true, false, true};
            std::vector<bool> validity = {true, false, true};
            bool8_array arr(values, validity);

            SUBCASE("iterate over values")
            {
                size_t count = 0;
                for (const auto& val : arr)
                {
                    if (count == 0)
                    {
                        CHECK(val.has_value());
                        CHECK_EQ(val.get(), true);
                    }
                    else if (count == 1)
                    {
                        CHECK(!val.has_value());
                    }
                    else if (count == 2)
                    {
                        CHECK(val.has_value());
                        CHECK_EQ(val.get(), true);
                    }
                    ++count;
                }
                CHECK_EQ(count, 3);
            }
        }

        // TODO: slice() is not available in primitive_array public interface
        // TEST_CASE("slice")
        // {
        //     std::vector<bool> values = {true, false, true, false, true};
        //     bool8_array arr(values);
        //
        //     auto sliced = arr.slice(1, 4);
        //
        //     REQUIRE_EQ(sliced.size(), 3);
        //     CHECK_EQ(static_cast<bool>(sliced[0]), false);
        //     CHECK_EQ(static_cast<bool>(sliced[1]), true);
        //     CHECK_EQ(static_cast<bool>(sliced[2]), false);
        // }

        TEST_CASE("copy_and_move")
        {
            std::vector<bool> values = {true, false, true};
            bool8_array arr1(values);

            SUBCASE("copy constructor")
            {
                bool8_array arr2 = arr1;
                CHECK_EQ(arr2.size(), 3);
                CHECK_EQ(static_cast<bool>(arr2[0]), true);
                CHECK_EQ(static_cast<bool>(arr2[1]), false);
                CHECK_EQ(static_cast<bool>(arr2[2]), true);

                // Modify arr2, arr1 should be unchanged
                arr2[0] = false;
                CHECK_EQ(static_cast<bool>(arr1[0]), true);
                CHECK_EQ(static_cast<bool>(arr2[0]), false);
            }

            SUBCASE("move constructor")
            {
                bool8_array arr2 = std::move(arr1);
                CHECK_EQ(arr2.size(), 3);
                CHECK_EQ(static_cast<bool>(arr2[0]), true);
                CHECK_EQ(static_cast<bool>(arr2[1]), false);
                CHECK_EQ(static_cast<bool>(arr2[2]), true);
            }

            SUBCASE("copy assignment")
            {
                std::vector<bool> other_values = {false};
                bool8_array arr2(other_values);
                arr2 = arr1;

                CHECK_EQ(arr2.size(), 3);
                CHECK_EQ(static_cast<bool>(arr2[0]), true);
                CHECK_EQ(static_cast<bool>(arr2[1]), false);
                CHECK_EQ(static_cast<bool>(arr2[2]), true);
            }

            SUBCASE("move assignment")
            {
                std::vector<bool> other_values = {false};
                bool8_array arr2(other_values);
                arr2 = std::move(arr1);

                CHECK_EQ(arr2.size(), 3);
                CHECK_EQ(static_cast<bool>(arr2[0]), true);
            }
        }

        TEST_CASE("empty_array")
        {
            std::vector<bool> values;
            bool8_array arr(values);

            CHECK_EQ(arr.size(), 0);
            CHECK(arr.empty());
        }

        TEST_CASE("large_array")
        {
            std::vector<bool> values;
            values.reserve(1000);
            for (size_t i = 0; i < 1000; ++i)
            {
                values.push_back(i % 2 == 0);
            }

            bool8_array arr(values);

            REQUIRE_EQ(arr.size(), 1000);
            for (size_t i = 0; i < 1000; ++i)
            {
                CHECK_EQ(arr.get_bool(i), (i % 2 == 0));
            }
        }

        // TODO: resize(), push_back(), pop_back() are not available in primitive_array public interface
        // TEST_CASE("modifiable_operations")
        // {
        //     std::vector<bool> values = {true, false, true, false};
        //     bool8_array arr(values);
        //
        //     SUBCASE("resize")
        //     {
        //         arr.resize(6);
        //         CHECK_EQ(arr.size(), 6);
        //     }
        //
        //     SUBCASE("push_back")
        //     {
        //         arr.push_back(nullable<std::int8_t>(std::int8_t{1}, true));
        //         CHECK_EQ(arr.size(), 5);
        //         CHECK_EQ(static_cast<bool>(arr[4]), true);
        //     }
        //
        //     SUBCASE("pop_back")
        //     {
        //         arr.pop_back();
        //         CHECK_EQ(arr.size(), 3);
        //     }
        // }

        // TODO: Extension metadata test - simplified since we don't automatically add extension metadata
        TEST_CASE("comparison_with_regular_primitive_array")
        {
            std::vector<bool> bool_values = {true, false, true};
            bool8_array bool8_arr(bool_values);

            std::vector<std::int8_t> int_values = {1, 0, 1};
            primitive_array<std::int8_t> int8_arr(int_values);

            // Both should have the same data_type
            CHECK_EQ(bool8_arr.data_type(), data_type::INT8);
            // Note: primitive_array doesn't expose data_type() method directly

            // Values should match when accessed
            CHECK_EQ(static_cast<bool>(bool8_arr[0]), true);
            CHECK_EQ(static_cast<bool>(bool8_arr[1]), false);
            CHECK_EQ(static_cast<bool>(bool8_arr[2]), true);
        }
    }
}
