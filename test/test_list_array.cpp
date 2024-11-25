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

#include "sparrow/layout/list_layout/list_array.hpp"
#include "sparrow/layout/primitive_array.hpp"

#include "doctest/doctest.h"
#include "external_array_data_creation.hpp"
#include "test_utils.hpp"
#include "sparrow/array.hpp"

namespace sparrow
{
    namespace test
    {
        template <class T>
        arrow_proxy make_list_proxy(size_t n_flat, const std::vector<size_t>& sizes)
        {
            // first we create a flat array of integers
            ArrowArray flat_arr{};
            ArrowSchema flat_schema{};
            test::fill_schema_and_array<T>(flat_schema, flat_arr, n_flat, 0/*offset*/, {});
            flat_schema.name = "the flat array";

            ArrowArray arr{};
            ArrowSchema schema{};
            test::fill_schema_and_array_for_list_layout(schema, arr, std::move(flat_schema), std::move(flat_arr), sizes, {}, 0);
            return arrow_proxy(std::move(arr), std::move(schema));
        }
    }

    TEST_SUITE("list_array")
    {
        static_assert(is_list_array_v<list_array>);
        static_assert(!is_big_list_array_v<list_array>);
        static_assert(!is_list_view_array_v<list_array>);
        static_assert(!is_big_list_view_array_v<list_array>);
        static_assert(!is_fixed_sized_list_array_v<list_array>);

        TEST_CASE("constructors")
        {
            // from sizes
            std::vector<std::size_t> sizes = {2, 2, 3, 4};

            // number of elements in the flatted array
            std::size_t n_flat = 11;  // 2+2+3+4

            // create flat array of integers
            primitive_array<std::int16_t> flat_arr(std::ranges::iota_view{std::size_t(0), std::size_t(n_flat)} | std::views::transform([](auto i){
                return static_cast<std::int16_t>(i);})
            );

            // wrap into an detyped array
            array arr(std::move(flat_arr));

            // create a list array
            list_array list_arr(std::move(arr), list_array::offset_from_sizes(sizes));

            // check the size
            REQUIRE_EQ(list_arr.size(), sizes.size());
            
            // check the sizes
            for(std::size_t i = 0; i < sizes.size(); ++i)
            {
                CHECK_EQ(list_arr[i].value().size(), sizes[i]);
            }

            // check the values
            std::int16_t flat_index = 0;
            for(std::size_t i = 0; i < sizes.size(); ++i)
            {
                auto list = list_arr[i].value();
                for(std::size_t j = 0; j < sizes[i]; ++j)
                {
                    CHECK_NULLABLE_VARIANT_EQ(list[j], flat_index);
                    ++flat_index;
                }
            }
        }
        TEST_CASE_TEMPLATE("list[T]", T, std::uint8_t, std::int32_t, float, double)
        {
            using inner_scalar_type = T;

            // number of elements in the flatted array
            const std::size_t n_flat = 10;  // 1+2+3+4
            // number of elements in the list array
            const std::size_t n = 4;
            // vector of sizes
            std::vector<std::size_t> sizes = {1, 2, 3, 4};

            const std::size_t n_flat2 = 8;
            std::vector<std::size_t> sizes2 = {2, 4, 2};

            arrow_proxy proxy = test::make_list_proxy<inner_scalar_type>(n_flat, sizes);

            // create a list array
            list_array list_arr(std::move(proxy));
            REQUIRE(list_arr.size() == n);

            SUBCASE("copy")
            {
                list_array list_arr2(list_arr);
                CHECK_EQ(list_arr, list_arr2);

                list_array list_arr3(test::make_list_proxy<T>(n_flat2, sizes2));
                CHECK_NE(list_arr3, list_arr);
                list_arr3 = list_arr;
                CHECK_EQ(list_arr3, list_arr);
            }

            SUBCASE("move")
            {
                list_array list_arr2(list_arr);
                list_array list_arr3(std::move(list_arr2));
                CHECK_EQ(list_arr3, list_arr);

                list_array list_arr4(test::make_list_proxy<T>(n_flat2, sizes2));
                CHECK_NE(list_arr4, list_arr);
                list_arr4 = std::move(list_arr3);
                CHECK_EQ(list_arr4, list_arr);
            }

            SUBCASE("element-sizes")
            {
                for (std::size_t i = 0; i < n; ++i)
                {
                    REQUIRE(list_arr[i].has_value());
                    CHECK(list_arr[i].value().size() == sizes[i]);
                }
            }
            SUBCASE("element-values")
            {
                std::size_t flat_index = 0;
                for (std::size_t i = 0; i < n; ++i)
                {
                    auto list = list_arr[i].value();
                    for (std::size_t j = 0; j < sizes[i]; ++j)
                    {
                        auto value_variant = list[j];
                        CHECK_NULLABLE_VARIANT_EQ(value_variant,  static_cast<inner_scalar_type>(flat_index));
                        ++flat_index;
                    }
                }
            }

            SUBCASE("consistency")
            {   
                test::generic_consistency_test(list_arr);
            }

            SUBCASE("cast flat array")
            {
                // get the flat values (offset is not applied)
                array_wrapper* flat_values = list_arr.raw_flat_array();

                // cast into a primitive array
                auto& flat_values_casted = unwrap_array<primitive_array<inner_scalar_type>>(*flat_values);

                using primitive_size_type = typename primitive_array<inner_scalar_type>::size_type;
                // check the size
                REQUIRE(flat_values_casted.size() == n_flat);

                // check that flat values are "iota"
                if constexpr (std::is_integral_v<inner_scalar_type>)
                {
                    for(inner_scalar_type i = 0; i < static_cast<inner_scalar_type>(n_flat); ++i){
                        CHECK(flat_values_casted[static_cast<primitive_size_type>(i)].value() == i);
                    }
                }
                else
                {
                    for(inner_scalar_type i = 0; i < static_cast<inner_scalar_type>(n_flat); ++i){
                        CHECK(flat_values_casted[static_cast<primitive_size_type>(i)].value() == doctest::Approx(static_cast<double>(i)));
                    }
                }
            }
        }
    }

    namespace test
    {
        template <class T>
        arrow_proxy make_list_view_proxy(size_t n_flat, const std::vector<size_t>& sizes)
        {
            // first we create a flat array of integers
            ArrowArray flat_arr{};
            ArrowSchema flat_schema{};
            test::fill_schema_and_array<T>(flat_schema, flat_arr, n_flat, 0/*offset*/, {});
            flat_schema.name = "the flat array";

            ArrowArray arr{};
            ArrowSchema schema{};
            test::fill_schema_and_array_for_list_view_layout(schema, arr, std::move(flat_schema), std::move(flat_arr), sizes, {}, 0);
            return arrow_proxy(std::move(arr), std::move(schema));
        }
    }

    TEST_SUITE("list_view_array")
    {
        static_assert(!is_list_array_v<list_view_array>);
        static_assert(!is_big_list_array_v<list_view_array>);
        static_assert(is_list_view_array_v<list_view_array>);
        static_assert(!is_big_list_view_array_v<list_view_array>);
        static_assert(!is_fixed_sized_list_array_v<list_view_array>);

        TEST_CASE("constructors")
        {
           // flat data is [0,1,2,3,4]
            std::size_t n_flat = 5;

            // create flat array of integers
            primitive_array<std::int16_t> flat_arr(std::ranges::iota_view{std::size_t(0), std::size_t(n_flat)} | std::views::transform([](auto i){
                return static_cast<std::int16_t>(i);})
            );

            // the desired goal array is
            // [[3,4],[2,3],NAN, [0,1,2]]

            // vector of sizes
            std::vector<std::uint32_t> sizes = {2, 2,0, 3};
            std::vector<std::uint32_t> offsets = {3, 2,0,0};

            std::vector<std::uint32_t> where_missing = {2};


            // wrap into an detyped array
            array arr(std::move(flat_arr));

            list_view_array list_view_arr(std::move(arr), offsets, sizes, where_missing);

            // check the size
            REQUIRE_EQ(list_view_arr.size(), sizes.size());

            // checkm has_value
            CHECK(list_view_arr[0].has_value());
            CHECK(list_view_arr[1].has_value());
            CHECK_FALSE(list_view_arr[2].has_value());
            CHECK(list_view_arr[3].has_value());


            // check the sizes
            CHECK_EQ(list_view_arr[0].value().size(), sizes[0]);
            CHECK_EQ(list_view_arr[1].value().size(), sizes[1]);
            CHECK_EQ(list_view_arr[3].value().size(), sizes[3]);



            // check the values
            CHECK_NULLABLE_VARIANT_EQ(list_view_arr[0].value()[0], std::int16_t(3));
            CHECK_NULLABLE_VARIANT_EQ(list_view_arr[0].value()[1], std::int16_t(4));

            CHECK_NULLABLE_VARIANT_EQ(list_view_arr[1].value()[0], std::int16_t(2));
            CHECK_NULLABLE_VARIANT_EQ(list_view_arr[1].value()[1], std::int16_t(3));

            CHECK_NULLABLE_VARIANT_EQ(list_view_arr[3].value()[0], std::int16_t(0));
            CHECK_NULLABLE_VARIANT_EQ(list_view_arr[3].value()[1], std::int16_t(1));
            CHECK_NULLABLE_VARIANT_EQ(list_view_arr[3].value()[2], std::int16_t(2));
        }

        TEST_CASE_TEMPLATE("list_view_array[T]", T, std::uint8_t, std::int32_t, float, double)
        {
            using inner_scalar_type = T;

            // number of elements in the flatted array
            const std::size_t n_flat = 10;  // 1+2+3+4
            // number of elements in the list array
            const std::size_t n = 4;
            // vector of sizes
            std::vector<std::size_t> sizes = {1, 2, 3, 4};

            const std::size_t n_flat2 = 8;
            std::vector<std::size_t> sizes2 = {2, 4, 2};
            
            arrow_proxy proxy = test::make_list_view_proxy<inner_scalar_type>(n_flat, sizes);

            // create a list array
            list_view_array list_arr(std::move(proxy));
            REQUIRE(list_arr.size() == n);

            SUBCASE("copy")
            {
                list_view_array list_arr2(list_arr);
                CHECK_EQ(list_arr, list_arr2);

                list_view_array list_arr3(test::make_list_view_proxy<T>(n_flat2, sizes2));
                CHECK_NE(list_arr3, list_arr);
                list_arr3 = list_arr;
                CHECK_EQ(list_arr3, list_arr);
            }

            SUBCASE("move")
            {
                list_view_array list_arr2(list_arr);
                list_view_array list_arr3(std::move(list_arr2));
                CHECK_EQ(list_arr3, list_arr);

                list_view_array list_arr4(test::make_list_view_proxy<T>(n_flat2, sizes2));
                CHECK_NE(list_arr4, list_arr);
                list_arr4 = std::move(list_arr3);
                CHECK_EQ(list_arr4, list_arr);
            }

            SUBCASE("element-sizes")
            {
                for (std::size_t i = 0; i < n; ++i)
                {
                    REQUIRE(list_arr[i].has_value());
                    CHECK(list_arr[i].value().size() == sizes[i]);
                }
            }

            SUBCASE("element-values")
            {
                std::size_t flat_index = 0;
                for (std::size_t i = 0; i < n; ++i)
                {
                    auto list = list_arr[i].value();
                    for (std::size_t j = 0; j < sizes[i]; ++j)
                    {
                        auto value_variant = list[j];
                        CHECK_NULLABLE_VARIANT_EQ(value_variant, static_cast<inner_scalar_type>(flat_index));
                        ++flat_index;
                    }
                }
            }

            SUBCASE("consistency")
            {   
                test::generic_consistency_test(list_arr);
            }

            SUBCASE("cast flat array")
            {
                // get the flat values (offset is not applied)
                array_wrapper* flat_values = list_arr.raw_flat_array();

                // cast into a primitive array
                auto& flat_values_casted = unwrap_array<primitive_array<inner_scalar_type>>(*flat_values);

                using primitive_size_type = typename primitive_array<inner_scalar_type>::size_type;
                // check the size
                REQUIRE(flat_values_casted.size() == n_flat);

                // check that flat values are "iota"
                if constexpr (std::is_integral_v<inner_scalar_type>)
                {
                    for(inner_scalar_type i = 0; i < static_cast<inner_scalar_type>(n_flat); ++i){
                        CHECK(flat_values_casted[static_cast<primitive_size_type>(i)].value() == i);
                    }
                }
                else
                {
                    for(inner_scalar_type i = 0; i < static_cast<inner_scalar_type>(n_flat); ++i){
                        CHECK(flat_values_casted[static_cast<primitive_size_type>(i)].value() == doctest::Approx(static_cast<double>(i)));
                    }
                }
            }
        }
    }

    namespace test
    {
        template <class T>
        arrow_proxy make_fixed_sized_list_proxy(size_t n_flat, size_t list_size)
        {
            // first we create a flat array of integers
            ArrowArray flat_arr{};
            ArrowSchema flat_schema{};
            test::fill_schema_and_array<T>(flat_schema, flat_arr, n_flat, 0/*offset*/, {});
            flat_schema.name = "the flat array";

            ArrowArray arr{};
            ArrowSchema schema{};
            test::fill_schema_and_array_for_fixed_size_list_layout(schema, arr, std::move(flat_schema), std::move(flat_arr), {}, list_size);
            return arrow_proxy(std::move(arr), std::move(schema));
        }
    }

    TEST_SUITE("fixed_sized_list_array")
    {
        static_assert(!is_list_array_v<fixed_sized_list_array>);
        static_assert(!is_big_list_array_v<fixed_sized_list_array>);
        static_assert(!is_list_view_array_v<fixed_sized_list_array>);
        static_assert(!is_big_list_view_array_v<fixed_sized_list_array>);
        static_assert(is_fixed_sized_list_array_v<fixed_sized_list_array>);

        TEST_CASE_TEMPLATE("fixed_sized_array_list[T]", T, std::uint8_t, std::int32_t, float, double)
        {
            using inner_scalar_type = T;

            // number of elements in the flatted array
            const std::size_t n_flat = 20;
            // the size of each list =
            const std::size_t list_size = 5;

            const std::size_t n = n_flat / list_size;

            CHECK(n == 4);

            const std::size_t n_flat2 = 10;
            const std::size_t list_size2 = 4;
            
            arrow_proxy proxy = test::make_fixed_sized_list_proxy<inner_scalar_type>(n_flat, list_size);
            fixed_sized_list_array list_arr(std::move(proxy));

            SUBCASE("copy")
            {
                fixed_sized_list_array list_arr2(list_arr);
                CHECK_EQ(list_arr, list_arr2);

                fixed_sized_list_array list_arr3(test::make_fixed_sized_list_proxy<T>(n_flat2, list_size2));
                CHECK_NE(list_arr3, list_arr);
                list_arr3 = list_arr;
                CHECK_EQ(list_arr3, list_arr);
            }

            SUBCASE("move")
            {
                fixed_sized_list_array list_arr2(list_arr);
                fixed_sized_list_array list_arr3(std::move(list_arr2));
                CHECK_EQ(list_arr3, list_arr);

                fixed_sized_list_array list_arr4(test::make_fixed_sized_list_proxy<T>(n_flat2, list_size2));
                CHECK_NE(list_arr4, list_arr);
                list_arr4 = std::move(list_arr3);
                CHECK_EQ(list_arr4, list_arr);
            }

            SUBCASE("consistency")
            {   
                test::generic_consistency_test(list_arr);
            }
            REQUIRE(list_arr.size() == n);

            SUBCASE("element-sizes")
            {
                for (std::size_t i = 0; i < list_arr.size(); ++i)
                {
                    REQUIRE(list_arr[i].has_value());
                    REQUIRE(list_arr[i].value().size() == list_size);
                }
            }

            SUBCASE("element-values")
            {
                std::size_t flat_index = 0;
                for (std::size_t i = 0; i < n; ++i)
                {
                    auto list = list_arr[i].value();
                    for (std::size_t j = 0; j < list.size(); ++j)
                    {
                        auto value_variant = list[j];
                        CHECK_NULLABLE_VARIANT_EQ(value_variant, static_cast<inner_scalar_type>(flat_index));
                        ++flat_index;
                    }
                }
            }
        }
    }
}
