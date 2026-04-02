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
#include "sparrow/layout/array_registry.hpp"
#include "sparrow/primitive_array.hpp"
#include "sparrow/run_end_encoded_array.hpp"
#include "sparrow/utils/nullable.hpp"

#include "../test/external_array_data_creation.hpp"
#include "doctest/doctest.h"
#include "test_utils.hpp"

namespace sparrow
{
    namespace test
    {
        inline auto make_u64_value(std::uint64_t value, bool valid = true) -> array_traits::value_type
        {
            return array_traits::value_type(nullable<std::uint64_t>(value, valid));
        }

        inline primitive_array<std::int32_t> get_run_ends_child(const run_end_encoded_array& array)
        {
            return primitive_array<std::int32_t>(
                ::sparrow::detail::array_access::get_arrow_proxy(array).children()[0].view()
            );
        }

        inline primitive_array<std::uint64_t> get_encoded_values_child(const run_end_encoded_array& array)
        {
            return primitive_array<std::uint64_t>(
                ::sparrow::detail::array_access::get_arrow_proxy(array).children()[1].view()
            );
        }

        run_end_encoded_array make_test_run_encoded_array(bool alterate = false)
        {
            using acc_type = std::int32_t;
            using inner_value_type = std::uint64_t;

            // lets encode the following: (length: 8)
            // [1,null,null, 42, 42, 42, null, 9]

            // the encoded values are:
            // [1, null, 42, null, 9]

            // the accumulated lengths are:
            // [1,3,6,7,8]

            // if alterate is true, all 42s will be replaced by 43s
            // this is to test EQ/NEQ after copy/move

            // encoded values
            primitive_array<inner_value_type> encoded_values(
                std::vector<inner_value_type>{
                    inner_value_type(1),
                    inner_value_type(),
                    // to check if arrays differ / are the same
                    alterate ? inner_value_type(43) : inner_value_type(42),
                    inner_value_type(),
                    inner_value_type(9)
                },
                std::vector<std::size_t>{1, 3}  // where we have no value
            );

            // accumulated lengths
            primitive_array<acc_type> acc_lengths{
                {acc_type(1), acc_type(3), acc_type(6), acc_type(7), acc_type(8)}
            };

            array acc_lengths_array(std::move(acc_lengths));
            array encoded_values_array(std::move(encoded_values));

            return run_end_encoded_array(std::move(acc_lengths_array), std::move(encoded_values_array));
        }
    }

    TEST_SUITE("run_length_encoded")
    {
        static_assert(is_run_end_encoded_array_v<run_end_encoded_array>);

        TEST_CASE("run_length_encoded")
        {
            using inner_value_type = std::uint64_t;
            const std::size_t n = 8;

            run_end_encoded_array rle_array = test::make_test_run_encoded_array();

            // check size
            REQUIRE(rle_array.size() == n);

            std::vector<bool> expected_bitmap{1, 0, 0, 1, 1, 1, 0, 1};
            std::vector<inner_value_type> expected_values{1, 0, 0, 42, 42, 42, 0, 9};

            SUBCASE("copy")
            {
#ifdef SPARROW_TRACK_COPIES
                copy_tracker::reset(copy_tracker::key<run_end_encoded_array>());
#endif
                run_end_encoded_array rle_array2(rle_array);
                CHECK_EQ(rle_array2, rle_array);
#ifdef SPARROW_TRACK_COPIES
                CHECK_EQ(copy_tracker::count(copy_tracker::key<run_end_encoded_array>()), 1);
#endif

                run_end_encoded_array rle_array3 = test::make_test_run_encoded_array(/*alterate=*/true);
                CHECK_NE(rle_array3, rle_array);
                rle_array3 = rle_array;
                CHECK_EQ(rle_array3, rle_array);
            }

            SUBCASE("move")
            {
                run_end_encoded_array rle_array2(rle_array);
                run_end_encoded_array rle_array3(std::move(rle_array2));
                CHECK_EQ(rle_array3, rle_array);

                run_end_encoded_array rle_array4 = test::make_test_run_encoded_array(/*alterate*/ true);
                CHECK_NE(rle_array4, rle_array);
                rle_array4 = std::move(rle_array3);
                CHECK_EQ(rle_array4, rle_array);
            }

            SUBCASE("operator[]")
            {
                SUBCASE("const")
                {
                    // check elements
                    for (std::size_t i = 0; i < n; ++i)
                    {
                        REQUIRE(rle_array[i].has_value() == bool(expected_bitmap[i]));
                        if (expected_bitmap[i])
                        {
                            array_traits::const_reference val = rle_array[i];
                            CHECK(val.has_value() == val.has_value());
                            // // visit the variant
                            std::visit(
                                [&](auto&& nullable) -> void
                                {
                                    using T = std::decay_t<decltype(nullable)>;
                                    using inner_type = std::decay_t<typename T::value_type>;
                                    if constexpr (std::is_same_v<inner_type, inner_value_type>)
                                    {
                                        if (nullable.has_value())
                                        {
                                            CHECK(nullable.value() == expected_values[i]);
                                        }
                                        else
                                        {
                                            CHECK(false);
                                        }
                                    }
                                    else
                                    {
                                        CHECK(false);
                                    }
                                },
#if SPARROW_GCC_11_2_WORKAROUND
                                sparrow::test::unwrap_gcc11_variant_base(val)
#else
                                val
#endif
                            );
                        }
                    }
                }

                SUBCASE("mutable")
                {
                    rle_array[3] = test::make_u64_value(7);

                    REQUIRE_EQ(rle_array.size(), 8u);
                    CHECK_NULLABLE_VARIANT_EQ(rle_array[3], std::uint64_t(7));
                    CHECK_NULLABLE_VARIANT_EQ(rle_array[4], std::uint64_t(42));
                    CHECK_NULLABLE_VARIANT_EQ(rle_array[5], std::uint64_t(42));

                    const auto run_ends = test::get_run_ends_child(rle_array);
                    const auto encoded_values = test::get_encoded_values_child(rle_array);
                    REQUIRE_EQ(run_ends.size(), 6u);
                    REQUIRE_EQ(encoded_values.size(), 6u);
                    CHECK_EQ(run_ends[0].value(), 1);
                    CHECK_EQ(run_ends[1].value(), 3);
                    CHECK_EQ(run_ends[2].value(), 4);
                    CHECK_EQ(run_ends[3].value(), 6);
                    CHECK_EQ(run_ends[4].value(), 7);
                    CHECK_EQ(run_ends[5].value(), 8);
                }
            }

            SUBCASE("iterator")
            {
                auto iter = rle_array.begin();
                // check elements
                for (std::size_t i = 0; i < n; ++i)
                {
                    REQUIRE(iter != rle_array.end());
                    CHECK(iter->has_value() == bool(expected_bitmap[i]));
                    if (iter->has_value())
                    {
                        auto val = *iter;
                        std::visit(
                            [&](auto&& nullable) -> void
                            {
                                using T = std::decay_t<decltype(nullable)>;
                                using inner_type = std::decay_t<typename T::value_type>;
                                if constexpr (std::is_same_v<inner_type, inner_value_type>)
                                {
                                    if (nullable.has_value())
                                    {
                                        CHECK(nullable.value() == expected_values[i]);
                                    }
                                    else
                                    {
                                        CHECK(false);
                                    }
                                }
                                else
                                {
                                    CHECK(false);
                                }
                            },
#if SPARROW_GCC_11_2_WORKAROUND
                            sparrow::test::unwrap_gcc11_variant_base(val)
#else
                            val
#endif
                        );
                    }
                    ++iter;
                }
            }

            SUBCASE("mutable iterator reuses cached run across split mutation")
            {
                auto iter = sparrow::next(rle_array.begin(), 4);
                auto ref = *iter;

                CHECK_NULLABLE_VARIANT_EQ(ref, std::uint64_t(42));

                ref = test::make_u64_value(7);

                CHECK_NULLABLE_VARIANT_EQ(ref, std::uint64_t(7));
                CHECK_NULLABLE_VARIANT_EQ(static_cast<array_traits::const_reference>(ref), std::uint64_t(7));
                CHECK_NULLABLE_VARIANT_EQ(rle_array[4], std::uint64_t(7));

                ref = test::make_u64_value(8);

                CHECK_NULLABLE_VARIANT_EQ(ref, std::uint64_t(8));
                CHECK_NULLABLE_VARIANT_EQ(static_cast<array_traits::const_reference>(ref), std::uint64_t(8));
                CHECK_NULLABLE_VARIANT_EQ(rle_array[4], std::uint64_t(8));
                CHECK_NULLABLE_VARIANT_EQ(rle_array[3], std::uint64_t(42));
                CHECK_NULLABLE_VARIANT_EQ(rle_array[5], std::uint64_t(42));
            }

            SUBCASE("reverse_iterator")
            {
                auto iter = rle_array.rbegin();
                // check elements
                for (std::size_t i = 0; i < n; ++i)
                {
                    REQUIRE(iter != rle_array.rend());
                    const auto idx = rle_array.size() - i - 1;
                    CHECK(iter->has_value() == bool(expected_bitmap[idx]));
                    if (iter->has_value())
                    {
                        auto val = *iter;
                        std::visit(
                            [&](auto&& nullable) -> void
                            {
                                using T = std::decay_t<decltype(nullable)>;
                                using inner_type = std::decay_t<typename T::value_type>;
                                if constexpr (std::is_same_v<inner_type, inner_value_type>)
                                {
                                    if (nullable.has_value())
                                    {
                                        CHECK(nullable.value() == expected_values[idx]);
                                    }
                                    else
                                    {
                                        CHECK(false);
                                    }
                                }
                                else
                                {
                                    CHECK(false);
                                }
                            },
#if SPARROW_GCC_11_2_WORKAROUND
                            sparrow::test::unwrap_gcc11_variant_base(val)
#else
                            val
#endif
                        );
                    }
                    ++iter;
                }
            }

            SUBCASE("consistency")
            {
                test::generic_consistency_test(rle_array);
            }

            SUBCASE("push_back merges adjacent run")
            {
                rle_array.push_back(test::make_u64_value(9));

                REQUIRE_EQ(rle_array.size(), 9u);
                CHECK_NULLABLE_VARIANT_EQ(rle_array[7], std::uint64_t(9));
                CHECK_NULLABLE_VARIANT_EQ(rle_array[8], std::uint64_t(9));

                const auto run_ends = test::get_run_ends_child(rle_array);
                const auto encoded_values = test::get_encoded_values_child(rle_array);
                REQUIRE_EQ(run_ends.size(), 5u);
                REQUIRE_EQ(encoded_values.size(), 5u);
                CHECK_EQ(run_ends[4].value(), 9);
                CHECK_EQ(encoded_values[4].value(), std::uint64_t(9));
            }

            SUBCASE("insert inside run splits encoding")
            {
                static_cast<void>(
                    rle_array.insert(sparrow::next(rle_array.cbegin(), 4), test::make_u64_value(7))
                );

                REQUIRE_EQ(rle_array.size(), 9u);
                CHECK_NULLABLE_VARIANT_EQ(rle_array[3], std::uint64_t(42));
                CHECK_NULLABLE_VARIANT_EQ(rle_array[4], std::uint64_t(7));
                CHECK_NULLABLE_VARIANT_EQ(rle_array[5], std::uint64_t(42));
                CHECK_NULLABLE_VARIANT_EQ(rle_array[6], std::uint64_t(42));

                const auto run_ends = test::get_run_ends_child(rle_array);
                REQUIRE_EQ(run_ends.size(), 7u);
                CHECK_EQ(run_ends[0].value(), 1);
                CHECK_EQ(run_ends[1].value(), 3);
                CHECK_EQ(run_ends[2].value(), 4);
                CHECK_EQ(run_ends[3].value(), 5);
                CHECK_EQ(run_ends[4].value(), 7);
                CHECK_EQ(run_ends[5].value(), 8);
                CHECK_EQ(run_ends[6].value(), 9);
            }

            SUBCASE("erase range merges adjacent runs")
            {
                static_cast<void>(
                    rle_array.erase(sparrow::next(rle_array.cbegin(), 3), sparrow::next(rle_array.cbegin(), 6))
                );

                REQUIRE_EQ(rle_array.size(), 5u);
                CHECK_NULLABLE_VARIANT_EQ(rle_array[0], std::uint64_t(1));
                CHECK_FALSE(rle_array[1].has_value());
                CHECK_FALSE(rle_array[2].has_value());
                CHECK_FALSE(rle_array[3].has_value());
                CHECK_NULLABLE_VARIANT_EQ(rle_array[4], std::uint64_t(9));

                const auto run_ends = test::get_run_ends_child(rle_array);
                const auto encoded_values = test::get_encoded_values_child(rle_array);
                REQUIRE_EQ(run_ends.size(), 3u);
                REQUIRE_EQ(encoded_values.size(), 3u);
                CHECK_EQ(run_ends[0].value(), 1);
                CHECK_EQ(run_ends[1].value(), 4);
                CHECK_EQ(run_ends[2].value(), 5);
                CHECK_FALSE(encoded_values[1].has_value());
            }

            SUBCASE("mutation on slice is unsupported")
            {
                run_end_encoded_array sliced(detail::array_access::get_arrow_proxy(rle_array).slice(1, 3));

                CHECK_THROWS_AS(sliced.push_back(test::make_u64_value(11)), std::logic_error);
                CHECK_THROWS_AS(sliced.erase(sliced.cbegin()), std::logic_error);
            }

#if defined(__cpp_lib_format)
            SUBCASE("formatter")
            {
                const std::string formatted = std::format("{}", rle_array);
                constexpr std::string_view expected = "Run end encoded [size=8] <1, null, null, 42, 42, 42, null, 9>";
                CHECK_EQ(formatted, expected);
            }
#endif
        }
    }
}
