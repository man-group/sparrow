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

#include <array>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

#include "sparrow/builder.hpp"

#include "test_utils.hpp"

namespace sparrow
{
    // to keep everything very short for very deep nested types
    template <class T>
    using nt = nullable<T>;

    TEST_SUITE("builder")
    {
        TEST_CASE("primitive-layout")
        {
            // arr[float]
            SUBCASE("float")
            {
                std::vector<float32_t> v{1.0f, 2.0f, 3.0f};
                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);
                REQUIRE_EQ(arr.size(), 3);
                CHECK_EQ(arr[0].value(), float32_t(1.0));
                CHECK_EQ(arr[1].value(), float32_t(2.0));
                CHECK_EQ(arr[2].value(), float32_t(3.0));
            }
            // arr[double] (with nulls)
            SUBCASE("float-with-nulls")
            {
                std::vector<nt<float64_t>> v{1.0, 2.0, sparrow::nullval, 3.0};
                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);
                REQUIRE_EQ(arr.size(), 4);
                REQUIRE(arr[0].has_value());
                REQUIRE(arr[1].has_value());
                REQUIRE_FALSE(arr[2].has_value());
                REQUIRE(arr[3].has_value());

                CHECK_EQ(arr[0].value(), 1.0);
                CHECK_EQ(arr[1].value(), 2.0);
                CHECK_EQ(arr[3].value(), 3.0);
            }
        }

        TEST_CASE("timestamp-layout")
        {
            SUBCASE("timestamp_milliseconds_array")
            {
                const auto* timezone = date::locate_zone("UTC");
                std::vector<timestamp_millisecond> v{
                    timestamp_millisecond{timezone, date::sys_days{date::year{2022} / 1 / 1}},
                    timestamp_millisecond{timezone, date::sys_days{date::year{2022} / 1 / 2}},
                    timestamp_millisecond{timezone, date::sys_days{date::year{2022} / 1 / 3}}
                };
                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);
                REQUIRE_EQ(arr.size(), 3);
            }
        }

        using duration_testing_types = mpl::rename<duration_types_t, std::tuple>;

        TEST_CASE_TEMPLATE_DEFINE("duration-layout", T, duration_array_id)
        {
            const std::vector<nt<T>> input_values{T(1), T(2), sparrow::nullval, T(4), T(5)};
            auto arr = sparrow::build(input_values);
            test::generic_consistency_test(arr);
            REQUIRE_EQ(arr.size(), input_values.size());
            for (size_t i = 0; i < arr.size(); ++i)
            {
                CHECK_EQ(arr[i], input_values[i]);
            }
        }
        TEST_CASE_TEMPLATE_APPLY(duration_array_id, duration_testing_types);

        TEST_CASE("list-layout")
        {
            // list[float]
            SUBCASE("list[float]")
            {
                std::vector<std::vector<float32_t>> v{{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f}};
                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);

                REQUIRE_EQ(arr.size(), 2);
                REQUIRE_EQ(arr[0].value().size(), 3);
                REQUIRE_EQ(arr[1].value().size(), 2);

                CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], float32_t(1.0f));
                CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[1], float32_t(2.0f));
                CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[2], float32_t(3.0f));
                CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], float32_t(4.0f));
                CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[1], float32_t(5.0f));

                // check that the children are **NOT** dict-encoded
                REQUIRE(!arr.raw_flat_array()->is_dictionary());
            }
            SUBCASE("list[list[float]]")
            {
                std::vector<std::vector<std::vector<float32_t>>> v{
                    {{1.2f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}},
                    {{7.0f, 8.0f, 9.0f}, {10.0f, 11.0f, 12.0f}}
                };
                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);
                REQUIRE_EQ(arr.size(), 2);
            }
            SUBCASE("options")
            {
                SUBCASE("with_large_list_flag")
                {
                    std::vector<std::vector<float32_t>> v{{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f}};
                    auto arr = sparrow::build(v, sparrow::large_list_flag);
                    using array_type = std::decay_t<decltype(arr)>;
                    static_assert(std::is_same_v<array_type, sparrow::big_list_array>);
                }
                SUBCASE("without_large_list_flag")
                {
                    std::vector<std::vector<float32_t>> v{{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f}};
                    auto arr = sparrow::build(v);
                    test::generic_consistency_test(arr);
                    using array_type = std::decay_t<decltype(arr)>;
                    static_assert(std::is_same_v<array_type, sparrow::list_array>);
                }
            }
        }

        TEST_CASE("struct-layout")
        {
            // struct<float, float>
            {
                std::vector<std::tuple<float32_t, int>> v{{1.5f, 2}, {3.5f, 4}, {5.5f, 6}};
                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);
                REQUIRE_EQ(arr.size(), 3);
            }
            // struct<float, float> (with nulls)
            {
                std::vector<nt<std::tuple<float32_t, int>>> v{
                    std::tuple<float32_t, int>{1.5f, 2},
                    sparrow::nullval,
                    std::tuple<float32_t, int>{5.5f, 6}
                };
                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);
                REQUIRE_EQ(arr.size(), 3);
            }
            // struct<list[float], uint16>
            {
                std::vector<std::tuple<std::vector<float32_t>, std::uint16_t>> v{
                    {{1.0f, 2.0f, 3.0f}, 1},
                    {{4.0f, 5.0f, 6.0f}, 2},
                    {{7.0f, 8.0f, 9.0f}, 3}
                };
                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);
                REQUIRE_EQ(arr.size(), 3);
            }
        }

        TEST_CASE("fixed-sized-list-layout")
        {
            // fixed_sized_list<float, 3>
            {
                std::vector<std::array<float32_t, 3>> v{{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}, {7.0f, 8.0f, 9.0f}};
                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);
                REQUIRE_EQ(arr.size(), 3);
            }
            // fixed_sized_list<float, 3>  with nulls
            {
                std::vector<nt<std::array<nt<float32_t>, 3>>> v{
                    std::array<nt<float32_t>, 3>{1.0f, sparrow::nullval, 3.0f},
                    sparrow::nullval,
                    std::array<nt<float32_t>, 3>{7.0f, 8.0f, sparrow::nullval}
                };
                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);
            }
        }

        TEST_CASE("variable-sized-binary")
        {
            // variable_size_binary
            {
                std::vector<std::string> v{
                    "hello",
                    " ",
                    "world",
                    "!",
                };
                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);
                REQUIRE_EQ(arr.size(), 4);
                CHECK_EQ(arr[0].value(), "hello");
                CHECK_EQ(arr[1].value(), " ");
                CHECK_EQ(arr[2].value(), "world");
                CHECK_EQ(arr[3].value(), "!");
            }
            // variable_size_binary with nulls
            {
                std::vector<nt<std::string>> v{"hello", sparrow::nullval, "world!"};
                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);
                REQUIRE_EQ(arr.size(), 3);
                REQUIRE(arr[0].has_value());
                REQUIRE_FALSE(arr[1].has_value());
                REQUIRE(arr[2].has_value());

                CHECK_EQ(arr[0].value(), "hello");
                CHECK_EQ(arr[2].value(), "world!");
            }
        }

        TEST_CASE("fixed-width-binary")
        {
            // fixed_width_binary
            {
                std::vector<std::array<byte_t, 3>> v{
                    {byte_t(1), byte_t(2), byte_t(3)},
                    {byte_t(4), byte_t(5), byte_t(6)},
                    {byte_t(7), byte_t(8), byte_t(9)}
                };
                auto arr = sparrow::build(v);
                [[maybe_unused]] const auto lol = arr.get_arrow_proxy().format();
                [[maybe_unused]] const auto lol2 = arr.get_arrow_proxy().data_type();
                test::generic_consistency_test(arr);
                REQUIRE_EQ(arr.size(), 3);
                using arr_type = std::decay_t<decltype(arr)>;
                static_assert(std::is_same_v<arr_type, sparrow::fixed_width_binary_array>);
            }
            // fixed_width_binary with nulls
            {
                std::vector<nt<std::array<byte_t, 3>>> v{
                    std::array<byte_t, 3>{byte_t(1), byte_t(2), byte_t(3)},
                    sparrow::nullval,
                    std::array<byte_t, 3>{byte_t(7), byte_t(8), byte_t(9)}
                };
                auto arr = sparrow::build(v);
                [[maybe_unused]] const auto lol = arr.get_arrow_proxy().format();
                [[maybe_unused]] const auto lol2 = arr.get_arrow_proxy().data_type();
                test::generic_consistency_test(arr);
                REQUIRE_EQ(arr.size(), 3);
                using arr_type = std::decay_t<decltype(arr)>;
                static_assert(std::is_same_v<arr_type, sparrow::fixed_width_binary_array>);
            }
        }

        TEST_CASE("sparse-union")
        {
            SUBCASE("simple")
            {
                using variant_type = std::variant<int, float32_t, std::string>;
                std::vector<variant_type> v{1, 2.0f, "hello"};

                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);
                using arr_type = std::decay_t<decltype(arr)>;
                static_assert(std::is_same_v<arr_type, sparrow::sparse_union_array>);

                REQUIRE_EQ(arr.size(), 3);
                CHECK_NULLABLE_VARIANT_EQ(arr[0], 1);
                CHECK_NULLABLE_VARIANT_EQ(arr[1], float32_t(2.0f));
                CHECK_NULLABLE_VARIANT_EQ(arr[2], std::string_view("hello"));
            }
        }

        TEST_CASE("map-layout")
        {
            SUBCASE("map")
            {
                const std::vector<std::pair<std::string, int>> values{{"a", 1}, {"b", 2}, {"c", 3}};
                const std::map<std::string, int> m{{"a", 1}, {"b", 2}, {"c", 3}};
                auto arr = sparrow::build(m);
                test::generic_consistency_test(arr);
                using arr_type = std::decay_t<decltype(arr)>;
                static_assert(std::is_same_v<arr_type, sparrow::map_array>);

                REQUIRE_EQ(arr.size(), 3);
                // Check the values
                std::size_t flat_index = 0;
                for (std::size_t i = 0; i < m.size(); ++i)
                {
                    auto val = arr[i];
                    for (const auto& v : val.value())
                    {
                        CHECK_NULLABLE_VARIANT_EQ(v.first, std::string_view(values[flat_index].first));
                        if (v.second.has_value())
                        {
                            CHECK_NULLABLE_VARIANT_EQ(v.second, values[flat_index].second);
                        }
                        ++flat_index;
                    }
                }
            }
        }
    }
}
