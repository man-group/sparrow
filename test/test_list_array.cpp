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
#include <cstring>
#include <initializer_list>
#include <stdexcept>

#include "sparrow/array.hpp"
#include "sparrow/list_array.hpp"
#include "sparrow/primitive_array.hpp"

#include "doctest/doctest.h"
#include "external_array_data_creation.hpp"
#include "test_utils.hpp"

namespace sparrow
{
    namespace test
    {
        template <class LIST_ARRAY>
        void check_sequential_lists(const LIST_ARRAY& list_arr, const std::vector<std::size_t>& sizes)
        {
            REQUIRE_EQ(list_arr.size(), sizes.size());

            for (std::size_t i = 0; i < sizes.size(); ++i)
            {
                CHECK_EQ(list_arr[i].value().size(), sizes[i]);
            }

            std::int16_t flat_index = 0;
            for (std::size_t i = 0; i < sizes.size(); ++i)
            {
                auto list = list_arr[i].value();
                for (std::size_t j = 0; j < sizes[i]; ++j)
                {
                    CHECK_NULLABLE_VARIANT_EQ(list[j], flat_index);
                    ++flat_index;
                }
            }
        }

        std::uint8_t* copy_external_buffer(const buffer<std::uint8_t>& source)
        {
            auto* destination = new std::uint8_t[source.size()];
            std::copy(source.begin(), source.end(), destination);
            return destination;
        }

        std::uint8_t* make_external_list_offsets_buffer(const std::vector<std::size_t>& sizes)
        {
            auto* raw_buffer = new std::uint8_t[(sizes.size() + 1) * sizeof(std::int32_t)];
            std::int32_t current_offset = 0;
            std::memcpy(raw_buffer, &current_offset, sizeof(current_offset));
            for (std::size_t i = 0; i < sizes.size(); ++i)
            {
                current_offset += static_cast<std::int32_t>(sizes[i]);
                std::memcpy(raw_buffer + (i + 1) * sizeof(current_offset), &current_offset, sizeof(current_offset));
            }
            return raw_buffer;
        }

        std::uint8_t* make_external_list_sizes_buffer(const std::vector<std::size_t>& sizes)
        {
            auto* raw_buffer = new std::uint8_t[sizes.size() * sizeof(std::uint32_t)];
            for (std::size_t i = 0; i < sizes.size(); ++i)
            {
                const auto list_size = static_cast<std::uint32_t>(sizes[i]);
                std::memcpy(raw_buffer + i * sizeof(list_size), &list_size, sizeof(list_size));
            }
            return raw_buffer;
        }

        template <class T>
        arrow_proxy make_list_proxy(size_t n_flat, const std::vector<size_t>& sizes)
        {
            // first we create a flat array of integers
            ArrowArray flat_arr{};
            ArrowSchema flat_schema{};
            test::fill_schema_and_array<T>(flat_schema, flat_arr, n_flat, 0 /*offset*/, {});
            flat_schema.name = "the flat array";

            ArrowArray arr{};
            ArrowSchema schema{};
            test::fill_schema_and_array_for_list_layout(
                schema,
                arr,
                std::move(flat_schema),
                std::move(flat_arr),
                sizes,
                {},
                0
            );
            return arrow_proxy(std::move(arr), std::move(schema));
        }

        template <class T>
        arrow_proxy make_external_list_proxy(size_t n_flat, const std::vector<size_t>& sizes)
        {
            ArrowArray flat_arr{};
            ArrowSchema flat_schema{};
            test::fill_schema_and_array<T>(flat_schema, flat_arr, n_flat, 0, {});
            flat_schema.name = "the flat array";

            ArrowSchema schema{};
            schema.format = "+l";
            schema.name = "test";
            schema.metadata = nullptr;
            schema.flags = static_cast<int64_t>(ArrowFlag::NULLABLE);
            schema.n_children = 1;
            schema.children = new ArrowSchema*[1]{new ArrowSchema(std::move(flat_schema))};
            schema.dictionary = nullptr;
            schema.release = &release_external_arrow_schema;

            ArrowArray arr{};
            arr.length = static_cast<std::int64_t>(sizes.size());
            arr.null_count = 0;
            arr.offset = 0;
            arr.n_buffers = 2;
            auto** buffers = new std::uint8_t*[2];
            buffers[0] = copy_external_buffer(make_bitmap_buffer(sizes.size(), std::vector<std::size_t>{}));
            buffers[1] = make_external_list_offsets_buffer(sizes);
            arr.buffers = const_cast<const void**>(reinterpret_cast<void**>(buffers));
            arr.n_children = 1;
            arr.children = new ArrowArray*[1]{new ArrowArray(std::move(flat_arr))};
            arr.dictionary = nullptr;
            arr.release = &release_external_arrow_array;

            return arrow_proxy(std::move(arr), std::move(schema));
        }

        template <class T>
        arrow_proxy make_external_list_view_proxy(size_t n_flat, const std::vector<size_t>& sizes)
        {
            ArrowArray flat_arr{};
            ArrowSchema flat_schema{};
            test::fill_schema_and_array<T>(flat_schema, flat_arr, n_flat, 0, {});
            flat_schema.name = "the flat array";

            ArrowSchema schema{};
            schema.format = "+vl";
            schema.name = "test";
            schema.metadata = nullptr;
            schema.flags = static_cast<int64_t>(ArrowFlag::NULLABLE);
            schema.n_children = 1;
            schema.children = new ArrowSchema*[1]{new ArrowSchema(std::move(flat_schema))};
            schema.dictionary = nullptr;
            schema.release = &release_external_arrow_schema;

            ArrowArray arr{};
            arr.length = static_cast<std::int64_t>(sizes.size());
            arr.null_count = 0;
            arr.offset = 0;
            arr.n_buffers = 3;
            auto** buffers = new std::uint8_t*[3];
            buffers[0] = copy_external_buffer(make_bitmap_buffer(sizes.size(), std::vector<std::size_t>{}));
            buffers[1] = make_external_list_offsets_buffer(sizes);
            buffers[2] = make_external_list_sizes_buffer(sizes);
            arr.buffers = const_cast<const void**>(reinterpret_cast<void**>(buffers));
            arr.n_children = 1;
            arr.children = new ArrowArray*[1]{new ArrowArray(std::move(flat_arr))};
            arr.dictionary = nullptr;
            arr.release = &release_external_arrow_array;

            return arrow_proxy(std::move(arr), std::move(schema));
        }

        void check_array(const list_array& list_arr, const std::vector<std::size_t>& sizes)
        {
            check_sequential_lists(list_arr, sizes);
        }

        template <class LIST_ARRAY>
        void
        check_int16_list(const LIST_ARRAY& arr, std::size_t idx, std::initializer_list<std::int16_t> expected)
        {
            REQUIRE(arr[idx].has_value());
            const auto list = arr[idx].value();
            REQUIRE_EQ(list.size(), expected.size());

            std::size_t value_idx = 0;
            for (const auto expected_value : expected)
            {
                CHECK_NULLABLE_VARIANT_EQ(list[value_idx], expected_value);
                ++value_idx;
            }
        }

        inline list_array make_int16_list_array(
            const std::vector<nullable<std::int16_t>>& flat_data,
            const std::vector<std::size_t>& sizes
        )
        {
            array flat{primitive_array<std::int16_t>(flat_data)};
            return list_array(std::move(flat), list_array::offset_from_sizes(sizes), true);
        }

        inline list_view_array make_int16_list_view_array(
            const std::vector<nullable<std::int16_t>>& flat_data,
            const std::vector<std::int32_t>& offsets,
            const std::vector<std::uint32_t>& sizes
        )
        {
            array flat{primitive_array<std::int16_t>(flat_data)};
            return list_view_array(std::move(flat), offsets, sizes, std::vector<std::size_t>{});
        }

        inline fixed_sized_list_array
        make_int16_fixed_sized_list_array(std::uint64_t list_size, const std::vector<nullable<std::int16_t>>& flat_data)
        {
            array flat{primitive_array<std::int16_t>(flat_data)};
            return fixed_sized_list_array(list_size, std::move(flat), true);
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

            auto iota = std::ranges::iota_view{std::size_t(0), std::size_t(n_flat)};

            // create flat array of integers
            primitive_array<std::int16_t> flat_arr(
                iota
                | std::views::transform(
                    [](auto i)
                    {
                        return static_cast<std::int16_t>(i);
                    }
                )
            );

            // wrap into an detyped array
            array arr(std::move(flat_arr));


            SUBCASE("from array, offset_buffer_type and nullable")
            {
                SUBCASE("nullable == true")
                {
                    // create a list array
                    list_array list_arr(std::move(arr), list_array::offset_from_sizes(sizes), true);
                    test::check_array(list_arr, sizes);
                }

                SUBCASE("nullable == false")
                {
                    // create a list array
                    list_array list_arr(std::move(arr), list_array::offset_from_sizes(sizes), false);
                    test::check_array(list_arr, sizes);
                }
            }
        }

        TEST_CASE("construct from external ArrowArray")
        {
            const std::vector<std::size_t> sizes = {1, 2, 3, 4};
            list_array list_arr(test::make_external_list_proxy<std::int16_t>(10, sizes));
            test::check_sequential_lists(list_arr, sizes);
        }

        TEST_CASE_TEMPLATE("list[T]", T, std::uint8_t, std::int32_t, float32_t, float64_t)
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

            // create a list array (keep proxy alive for mutable subcases)
            list_array list_arr(proxy);
            REQUIRE(list_arr.size() == n);

            SUBCASE("copy")
            {
#ifdef SPARROW_TRACK_COPIES
                copy_tracker::reset(copy_tracker::key<list_array>());
#endif
                list_array list_arr2(list_arr);
                CHECK_EQ(list_arr, list_arr2);
#ifdef SPARROW_TRACK_COPIES
                CHECK_EQ(copy_tracker::count(copy_tracker::key<list_array>()), 1);
#endif

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
                auto* flat_values = list_arr.raw_flat_array();

                // check the size
                REQUIRE(array_size(*flat_values) == n_flat);

                // check that flat values are "iota"
                if constexpr (std::is_integral_v<inner_scalar_type>)
                {
                    for (inner_scalar_type i = 0; i < static_cast<inner_scalar_type>(n_flat); ++i)
                    {
                        CHECK_NULLABLE_VARIANT_EQ(array_element(*flat_values, static_cast<std::size_t>(i)), i);
                    }
                }
                else
                {
                    for (inner_scalar_type i = 0; i < static_cast<inner_scalar_type>(n_flat); ++i)
                    {
                        const auto value_variant = array_element(*flat_values, static_cast<std::size_t>(i));
                        CHECK(
                            std::get<typename primitive_array<inner_scalar_type>::const_reference>(value_variant)
                                .value()
                            == doctest::Approx(static_cast<double>(i))
                        );
                    }
                }
            }

            SUBCASE("zero_null_values")
            {
                // list_array list_arr2(list_arr);
                // list_arr2.zero_null_values();
                // // check that all null values are set to 0
                // // for (std::size_t i = 0; i < n; ++i)
                // // {
                // //     if (!list_arr[i].has_value())
                // //     {
                // //         CHECK(list_arr2[i].value().size() == 1);
                // //         CHECK(list_arr2[i].value()[0] == 0);
                // //     }
                // // }

                // CHECK_EQ(list_arr2, list_arr);
            }

            SUBCASE("mutation on slice is unsupported")
            {
                auto sliced = list_arr.slice(1, 2);
                const auto val = sliced[0].value();

                CHECK_THROWS_AS(sliced.push_back(make_nullable(val)), std::logic_error);
                CHECK_THROWS_AS(sliced.erase(sliced.cbegin()), std::logic_error);
            }

            SUBCASE("push_back")
            {
                SUBCASE("element")
                {
                    list_array arr(proxy);
                    REQUIRE_EQ(arr.size(), n);

                    // push_back a copy of arr[0] = [0]
                    arr.push_back(make_nullable(arr[0].value()));

                    REQUIRE_EQ(arr.size(), n + 1);
                    CHECK(arr[n].has_value());
                    CHECK_EQ(arr[n].value().size(), 1u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[n].value()[0], static_cast<inner_scalar_type>(0));
                }

                SUBCASE("null")
                {
                    list_array arr(proxy);
                    REQUIRE_EQ(arr.size(), n);

                    arr.push_back(make_nullable(arr[0].value(), false));  // null element

                    REQUIRE_EQ(arr.size(), n + 1);
                    CHECK_FALSE(arr[n].has_value());
                }
            }

            SUBCASE("operator[]")
            {
                SUBCASE("replaces element from same array")
                {
                    list_array arr(proxy);
                    arr[0] = make_nullable(arr[1].value());

                    REQUIRE_EQ(arr.size(), n);
                    // arr[0] was [0], now replaced with arr[1]=[1,2]
                    CHECK_EQ(arr[0].value().size(), 2u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(1));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[1], static_cast<inner_scalar_type>(2));
                    // arr[1] unchanged
                    CHECK_EQ(arr[1].value().size(), 2u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(1));
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[1], static_cast<inner_scalar_type>(2));
                }

                SUBCASE("replaces element from another array")
                {
                    list_array arr(proxy);
                    // source: 3 lists of sizes {2,4,2} over iota values [0..7]
                    list_array source(test::make_list_proxy<inner_scalar_type>(n_flat2, sizes2));

                    arr[0] = make_nullable(source[1].value());  // source[1]=[2,3,4,5]

                    REQUIRE_EQ(arr.size(), n);
                    // arr[0] was [0], now [2,3,4,5]
                    CHECK_EQ(arr[0].value().size(), 4u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(2));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[1], static_cast<inner_scalar_type>(3));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[2], static_cast<inner_scalar_type>(4));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[3], static_cast<inner_scalar_type>(5));
                    // arr[1] unchanged = [1,2]
                    CHECK_EQ(arr[1].value().size(), 2u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(1));
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[1], static_cast<inner_scalar_type>(2));
                }
            }

            SUBCASE("element proxy replaces element from another array")
            {
                list_array arr(proxy);
                // source: 3 lists of sizes {2,4,2} over iota values [0..7]
                list_array source(test::make_list_proxy<inner_scalar_type>(n_flat2, sizes2));

                auto element = arr[1].value();  // arr[1]=[1,2]
                element = source[2].value();    // source[2]=[6,7]

                REQUIRE_EQ(arr.size(), n);
                // arr[0] = [0] unchanged
                CHECK_EQ(arr[0].value().size(), 1u);
                CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(0));
                // arr[1] was [1,2], now [6,7]
                CHECK_EQ(arr[1].value().size(), 2u);
                CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(6));
                CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[1], static_cast<inner_scalar_type>(7));
            }

            SUBCASE("pop_back")
            {
                list_array arr(proxy);
                REQUIRE_EQ(arr.size(), n);

                arr.pop_back();  // removes arr[3]=[6,7,8,9]

                REQUIRE_EQ(arr.size(), n - 1u);
                CHECK_EQ(arr[0].value().size(), 1u);
                CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(0));
                CHECK_EQ(arr[n - 2u].value().size(), 3u);  // was arr[2]=[3,4,5]
                CHECK_NULLABLE_VARIANT_EQ(arr[n - 2u].value()[0], static_cast<inner_scalar_type>(3));
            }

            SUBCASE("erase")
            {
                SUBCASE("first element")
                {
                    list_array arr(proxy);
                    arr.erase(arr.cbegin());  // Remove arr[0]=[0]

                    REQUIRE_EQ(arr.size(), n - 1u);
                    CHECK_EQ(arr[0].value().size(), 2u);  // was arr[1]=[1,2]
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(1));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[1], static_cast<inner_scalar_type>(2));
                    CHECK_EQ(arr[1].value().size(), 3u);  // was arr[2]=[3,4,5]
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(3));
                }

                SUBCASE("middle element")
                {
                    list_array arr(proxy);
                    arr.erase(sparrow::next(arr.cbegin(), 1));  // Remove arr[1]=[1,2]
                    REQUIRE_EQ(arr.size(), n - 1u);
                    CHECK_EQ(arr[0].value().size(), 1u);  // arr[0]=[0] unchanged
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(0));
                    CHECK_EQ(arr[1].value().size(), 3u);  // was arr[2]=[3,4,5]
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(3));
                }
            }

            SUBCASE("resize")
            {
                SUBCASE("grow")
                {
                    list_array arr(proxy);
                    arr.resize(n + 2, make_nullable(arr[0].value()));  // append 2 copies of arr[0]=[0]

                    REQUIRE_EQ(arr.size(), n + 2);
                    CHECK_EQ(arr[n].value().size(), 1u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[n].value()[0], static_cast<inner_scalar_type>(0));
                    CHECK_EQ(arr[n + 1].value().size(), 1u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[n + 1].value()[0], static_cast<inner_scalar_type>(0));
                }

                SUBCASE("shrink")
                {
                    list_array arr(proxy);

                    arr.resize(2, make_nullable(arr[0].value()));  // drop arr[2] and arr[3]

                    REQUIRE_EQ(arr.size(), 2u);
                    CHECK_EQ(arr[0].value().size(), 1u);  // arr[0]=[0]
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(0));
                    CHECK_EQ(arr[1].value().size(), 2u);  // arr[1]=[1,2]
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(1));
                }
            }

            SUBCASE("insert")
            {
                SUBCASE("at beginning")
                {
                    list_array arr(proxy);
                    const auto val = arr[n - 1].value();  // arr[3]=[6,7,8,9]

                    arr.insert(arr.cbegin(), make_nullable(val));

                    REQUIRE_EQ(arr.size(), n + 1);
                    CHECK(arr[0].has_value());
                    CHECK_EQ(arr[0].value().size(), 4u);  // newly inserted [6,7,8,9]
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(6));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[3], static_cast<inner_scalar_type>(9));
                    CHECK_EQ(arr[1].value().size(), 1u);  // was arr[0]=[0]
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(0));
                    CHECK_EQ(arr[2].value().size(), 2u);  // was arr[1]=[1,2]
                    CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[0], static_cast<inner_scalar_type>(1));
                }

                SUBCASE("in the middle")
                {
                    list_array arr(proxy);
                    const auto val = arr[2].value();  // arr[2]=[3,4,5]

                    arr.insert(sparrow::next(arr.cbegin(), 1), make_nullable(val));

                    REQUIRE_EQ(arr.size(), n + 1);
                    CHECK_EQ(arr[0].value().size(), 1u);  // arr[0]=[0] unchanged
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(0));
                    CHECK(arr[1].has_value());
                    CHECK_EQ(arr[1].value().size(), 3u);  // newly inserted [3,4,5]
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(3));
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[1], static_cast<inner_scalar_type>(4));
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[2], static_cast<inner_scalar_type>(5));
                    CHECK_EQ(arr[2].value().size(), 2u);  // was arr[1]=[1,2]
                    CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[0], static_cast<inner_scalar_type>(1));
                    CHECK_EQ(arr[3].value().size(), 3u);  // was arr[2]=[3,4,5]
                    CHECK_NULLABLE_VARIANT_EQ(arr[3].value()[0], static_cast<inner_scalar_type>(3));
                    CHECK_EQ(arr[4].value().size(), 4u);  // was arr[3]=[6,7,8,9]
                    CHECK_NULLABLE_VARIANT_EQ(arr[4].value()[0], static_cast<inner_scalar_type>(6));
                }

                SUBCASE("at end")
                {
                    list_array arr(proxy);
                    const auto val = arr[0].value();  // arr[0]=[0]

                    arr.insert(arr.cend(), make_nullable(val));

                    REQUIRE_EQ(arr.size(), n + 1);
                    CHECK_EQ(arr[n - 1].value().size(), 4u);  // arr[3]=[6,7,8,9] unchanged
                    CHECK_NULLABLE_VARIANT_EQ(arr[n - 1].value()[0], static_cast<inner_scalar_type>(6));
                    CHECK(arr[n].has_value());
                    CHECK_EQ(arr[n].value().size(), 1u);  // newly inserted [0]
                    CHECK_NULLABLE_VARIANT_EQ(arr[n].value()[0], static_cast<inner_scalar_type>(0));
                }

                SUBCASE("range in the middle")
                {
                    list_array arr(proxy);
                    // source: 3 lists of sizes {2,4,2} over iota values [0..7]
                    // source[0]=[0,1]  source[1]=[2,3,4,5]  source[2]=[6,7]
                    list_array source(test::make_list_proxy<inner_scalar_type>(n_flat2, sizes2));
                    const std::vector<list_array::value_type> to_insert = {
                        make_nullable(source[0].value()),  // [0,1]
                        make_nullable(source[1].value())   // [2,3,4,5]
                    };

                    const auto it = arr.insert(sparrow::next(arr.cbegin(), 1), to_insert.begin(), to_insert.end());

                    REQUIRE_EQ(arr.size(), n + 2);
                    CHECK_EQ(it, sparrow::next(arr.begin(), 1));
                    // arr[0]=[0] unchanged
                    CHECK_EQ(arr[0].value().size(), 1u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(0));
                    // arr[1]=source[0]=[0,1] inserted
                    CHECK_EQ(arr[1].value().size(), 2u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(0));
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[1], static_cast<inner_scalar_type>(1));
                    // arr[2]=source[1]=[2,3,4,5] inserted
                    CHECK_EQ(arr[2].value().size(), 4u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[0], static_cast<inner_scalar_type>(2));
                    CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[1], static_cast<inner_scalar_type>(3));
                    // arr[3]=[1,2] was arr[1]
                    CHECK_EQ(arr[3].value().size(), 2u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[3].value()[0], static_cast<inner_scalar_type>(1));
                    CHECK_NULLABLE_VARIANT_EQ(arr[3].value()[1], static_cast<inner_scalar_type>(2));
                    // arr[4]=[3,4,5] was arr[2]
                    CHECK_EQ(arr[4].value().size(), 3u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[4].value()[0], static_cast<inner_scalar_type>(3));
                    // arr[5]=[6,7,8,9] was arr[3]
                    CHECK_EQ(arr[5].value().size(), 4u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[5].value()[0], static_cast<inner_scalar_type>(6));
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
            test::fill_schema_and_array<T>(flat_schema, flat_arr, n_flat, 0 /*offset*/, {});
            flat_schema.name = "the flat array";

            ArrowArray arr{};
            ArrowSchema schema{};
            test::fill_schema_and_array_for_list_view_layout(
                schema,
                arr,
                std::move(flat_schema),
                std::move(flat_arr),
                sizes,
                {},
                0
            );
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

            auto iota = std::ranges::iota_view{std::size_t(0), std::size_t(n_flat)};
            primitive_array<std::int16_t> flat_arr(
                iota
                | std::views::transform(
                    [](auto i)
                    {
                        return static_cast<std::int16_t>(i);
                    }
                )
            );

            // the desired goal array is
            // [[3,4],[2,3],NAN, [0,1,2]]

            // vector of sizes
            std::vector<std::uint32_t> sizes = {2, 2, 0, 3};
            std::vector<std::int32_t> offsets = {3, 2, 0, 0};

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

        TEST_CASE("construct from external ArrowArray")
        {
            const std::vector<std::size_t> sizes = {1, 2, 3, 4};
            list_view_array list_view_arr(test::make_external_list_view_proxy<std::int16_t>(10, sizes));
            test::check_sequential_lists(list_view_arr, sizes);
        }

        TEST_CASE_TEMPLATE("list_view_array[T]", T, std::uint8_t, std::int32_t, float32_t, float64_t)
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

            // create a list array (keep proxy alive for mutable subcases)
            list_view_array list_arr(proxy);
            REQUIRE(list_arr.size() == n);

            SUBCASE("copy")
            {
#ifdef SPARROW_TRACK_COPIES
                copy_tracker::reset(copy_tracker::key<list_view_array>());
#endif
                list_view_array list_arr2(list_arr);
                CHECK_EQ(list_arr, list_arr2);
#ifdef SPARROW_TRACK_COPIES
                CHECK_EQ(copy_tracker::count(copy_tracker::key<list_view_array>()), 1);
#endif

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
                auto* flat_values = list_arr.raw_flat_array();

                // check the size
                REQUIRE(array_size(*flat_values) == n_flat);

                // check that flat values are "iota"
                if constexpr (std::is_integral_v<inner_scalar_type>)
                {
                    for (inner_scalar_type i = 0; i < static_cast<inner_scalar_type>(n_flat); ++i)
                    {
                        CHECK_NULLABLE_VARIANT_EQ(array_element(*flat_values, static_cast<std::size_t>(i)), i);
                    }
                }
                else
                {
                    for (inner_scalar_type i = 0; i < static_cast<inner_scalar_type>(n_flat); ++i)
                    {
                        const auto value_variant = array_element(*flat_values, static_cast<std::size_t>(i));
                        CHECK(
                            std::get<typename primitive_array<inner_scalar_type>::const_reference>(value_variant)
                                .value()
                            == doctest::Approx(static_cast<double>(i))
                        );
                    }
                }
            }


            SUBCASE("mutation on slice is unsupported")
            {
                auto sliced = list_arr.slice(1, 2);
                const auto val = sliced[0].value();

                CHECK_THROWS_AS(sliced.push_back(make_nullable(val)), std::logic_error);
                CHECK_THROWS_AS(sliced.erase(sliced.cbegin()), std::logic_error);
            }

            SUBCASE("push_back")
            {
                list_view_array arr(test::make_list_view_proxy<inner_scalar_type>(n_flat, sizes));
                REQUIRE_EQ(arr.size(), n);

                // push_back a copy of arr[0] = [0]
                arr.push_back(make_nullable(arr[0].value()));

                REQUIRE_EQ(arr.size(), n + 1);
                CHECK(arr[n].has_value());
                CHECK_EQ(arr[n].value().size(), 1u);
                CHECK_NULLABLE_VARIANT_EQ(arr[n].value()[0], static_cast<inner_scalar_type>(0));
            }

            SUBCASE("operator[]")
            {
                SUBCASE("replaces element from same array")
                {
                    list_view_array arr(test::make_list_view_proxy<inner_scalar_type>(n_flat, sizes));
                    arr[0] = make_nullable(arr[1].value());  // arr[1]=[1,2]

                    REQUIRE_EQ(arr.size(), n);
                    // arr[0] was [0], now [1,2]
                    CHECK_EQ(arr[0].value().size(), 2u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(1));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[1], static_cast<inner_scalar_type>(2));
                    // arr[1] unchanged
                    CHECK_EQ(arr[1].value().size(), 2u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(1));
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[1], static_cast<inner_scalar_type>(2));
                }

                SUBCASE("replaces element from another array")
                {
                    list_view_array arr(test::make_list_view_proxy<inner_scalar_type>(n_flat, sizes));
                    // source: sizes {2,4,2}: source[0]=[0,1], source[1]=[2,3,4,5], source[2]=[6,7]
                    list_view_array source(test::make_list_view_proxy<inner_scalar_type>(n_flat2, sizes2));

                    arr[0] = make_nullable(source[1].value());  // source[1]=[2,3,4,5]

                    REQUIRE_EQ(arr.size(), n);
                    // arr[0] was [0], now [2,3,4,5]
                    CHECK_EQ(arr[0].value().size(), 4u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(2));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[1], static_cast<inner_scalar_type>(3));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[2], static_cast<inner_scalar_type>(4));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[3], static_cast<inner_scalar_type>(5));
                    // arr[1] unchanged = [1,2]
                    CHECK_EQ(arr[1].value().size(), 2u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(1));
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[1], static_cast<inner_scalar_type>(2));
                }
            }

            SUBCASE("element proxy replaces element from another array")
            {
                list_view_array arr(test::make_list_view_proxy<inner_scalar_type>(n_flat, sizes));
                // source: sizes {2,4,2}: source[0]=[0,1], source[1]=[2,3,4,5], source[2]=[6,7]
                list_view_array source(test::make_list_view_proxy<inner_scalar_type>(n_flat2, sizes2));

                auto element = arr[1].value();    // arr[1]=[1,2]
                element = source[2].value();      // source[2]=[6,7]

                REQUIRE_EQ(arr.size(), n);
                // arr[0]=[0] unchanged
                CHECK_EQ(arr[0].value().size(), 1u);
                CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(0));
                // arr[1] was [1,2], now [6,7]
                CHECK_EQ(arr[1].value().size(), 2u);
                CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(6));
                CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[1], static_cast<inner_scalar_type>(7));
            }

            SUBCASE("pop_back")
            {
                list_view_array arr(test::make_list_view_proxy<inner_scalar_type>(n_flat, sizes));
                REQUIRE_EQ(arr.size(), n);

                arr.pop_back();  // removes arr[3]=[6,7,8,9]

                REQUIRE_EQ(arr.size(), n - 1u);
                CHECK_EQ(arr[0].value().size(), 1u);       // arr[0]=[0]
                CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(0));
                CHECK_EQ(arr[n - 2u].value().size(), 3u);  // was arr[2]=[3,4,5]
                CHECK_NULLABLE_VARIANT_EQ(arr[n - 2u].value()[0], static_cast<inner_scalar_type>(3));
            }

            SUBCASE("erase")
            {
                SUBCASE("first element")
                {
                    list_view_array arr(test::make_list_view_proxy<inner_scalar_type>(n_flat, sizes));
                    arr.erase(arr.cbegin());  // Remove arr[0]=[0]

                    REQUIRE_EQ(arr.size(), n - 1u);
                    CHECK_EQ(arr[0].value().size(), 2u);  // was arr[1]=[1,2]
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(1));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[1], static_cast<inner_scalar_type>(2));
                    CHECK_EQ(arr[1].value().size(), 3u);  // was arr[2]=[3,4,5]
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(3));
                }
            }

            SUBCASE("insert")
            {
                SUBCASE("at beginning")
                {
                    list_view_array arr(test::make_list_view_proxy<inner_scalar_type>(n_flat, sizes));
                    const auto val = arr[n - 1].value();  // arr[3]=[6,7,8,9]

                    arr.insert(arr.cbegin(), make_nullable(val));

                    REQUIRE_EQ(arr.size(), n + 1);
                    CHECK(arr[0].has_value());
                    CHECK_EQ(arr[0].value().size(), 4u);  // newly inserted [6,7,8,9]
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(6));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[3], static_cast<inner_scalar_type>(9));
                    CHECK_EQ(arr[1].value().size(), 1u);  // was arr[0]=[0]
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(0));
                    CHECK_EQ(arr[2].value().size(), 2u);  // was arr[1]=[1,2]
                    CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[0], static_cast<inner_scalar_type>(1));
                }

                SUBCASE("in the middle")
                {
                    // flat=[0,1,2,3,4,5], lists=[[0,1],[2,3],[4,5]]
                    const std::vector<nullable<std::int16_t>> flat_data = {
                        make_nullable<std::int16_t>(0),
                        make_nullable<std::int16_t>(1),
                        make_nullable<std::int16_t>(2),
                        make_nullable<std::int16_t>(3),
                        make_nullable<std::int16_t>(4),
                        make_nullable<std::int16_t>(5)
                    };
                    array flat{primitive_array<std::int16_t>(flat_data)};
                    list_view_array arr(
                        std::move(flat),
                        std::vector<std::int32_t>{0, 2, 4},
                        std::vector<std::uint32_t>{2, 2, 2},
                        std::vector<std::size_t>{}
                    );
                    const auto val = arr[2].value();  // [4,5]

                    arr.insert(sparrow::next(arr.cbegin(), 1), make_nullable(val));

                    REQUIRE_EQ(arr.size(), 4u);
                    CHECK_EQ(arr[0].value().size(), 2u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], std::int16_t(0));
                    CHECK(arr[1].has_value());
                    CHECK_EQ(arr[1].value().size(), 2u);  // newly inserted
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], std::int16_t(4));
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[1], std::int16_t(5));
                    CHECK_EQ(arr[2].value().size(), 2u);  // was arr[1]
                    CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[0], std::int16_t(2));
                    CHECK_EQ(arr[3].value().size(), 2u);  // was arr[2]
                    CHECK_NULLABLE_VARIANT_EQ(arr[3].value()[0], std::int16_t(4));
                }

                SUBCASE("at end")
                {
                    list_view_array arr(test::make_list_view_proxy<inner_scalar_type>(n_flat, sizes));
                    const auto val = arr[0].value();  // arr[0]=[0]

                    arr.insert(arr.cend(), make_nullable(val));

                    REQUIRE_EQ(arr.size(), n + 1);
                    CHECK_EQ(arr[n - 1].value().size(), 4u);  // arr[3]=[6,7,8,9] unchanged
                    CHECK_NULLABLE_VARIANT_EQ(arr[n - 1].value()[0], static_cast<inner_scalar_type>(6));
                    CHECK(arr[n].has_value());
                    CHECK_EQ(arr[n].value().size(), 1u);  // newly inserted [0]
                    CHECK_NULLABLE_VARIANT_EQ(arr[n].value()[0], static_cast<inner_scalar_type>(0));
                }

                SUBCASE("range at beginning")
                {
                    list_view_array arr(test::make_list_view_proxy<inner_scalar_type>(n_flat, sizes));
                    // source: sizes {2,4,2}: source[0]=[0,1], source[1]=[2,3,4,5], source[2]=[6,7]
                    list_view_array source(test::make_list_view_proxy<inner_scalar_type>(n_flat2, sizes2));
                    const std::vector<list_view_array::value_type> to_insert = {
                        make_nullable(source[0].value()),   // [0,1]
                        make_nullable(source[1].value())    // [2,3,4,5]
                    };

                    const auto it = arr.insert(arr.cbegin(), to_insert.begin(), to_insert.end());

                    REQUIRE_EQ(arr.size(), n + 2);
                    CHECK_EQ(it, arr.begin());
                    // newly inserted at front
                    CHECK_EQ(arr[0].value().size(), 2u);  // source[0]=[0,1]
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(0));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[1], static_cast<inner_scalar_type>(1));
                    CHECK_EQ(arr[1].value().size(), 4u);  // source[1]=[2,3,4,5]
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(2));
                    // original elements shifted right
                    CHECK_EQ(arr[2].value().size(), 1u);  // was arr[0]=[0]
                    CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[0], static_cast<inner_scalar_type>(0));
                    CHECK_EQ(arr[3].value().size(), 2u);  // was arr[1]=[1,2]
                    CHECK_NULLABLE_VARIANT_EQ(arr[3].value()[0], static_cast<inner_scalar_type>(1));
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
            test::fill_schema_and_array<T>(flat_schema, flat_arr, n_flat, 0 /*offset*/, {});
            flat_schema.name = "the flat array";

            ArrowArray arr{};
            ArrowSchema schema{};
            test::fill_schema_and_array_for_fixed_size_list_layout(
                schema,
                arr,
                std::move(flat_schema),
                std::move(flat_arr),
                {},
                list_size
            );
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

        TEST_CASE_TEMPLATE("fixed_sized_array_list[T]", T, std::uint8_t, std::int32_t, float32_t, float64_t)
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
            fixed_sized_list_array list_arr(proxy);   // keep proxy alive for mutable subcases

            SUBCASE("copy")
            {
#ifdef SPARROW_TRACK_COPIES
                copy_tracker::reset(copy_tracker::key<fixed_sized_list_array>());
#endif
                fixed_sized_list_array list_arr2(list_arr);
                CHECK_EQ(list_arr, list_arr2);
#ifdef SPARROW_TRACK_COPIES
                CHECK_EQ(copy_tracker::count(copy_tracker::key<fixed_sized_list_array>()), 1);
#endif

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

            SUBCASE("mutation on slice is unsupported")
            {
                auto sliced = list_arr.slice(1, 4);
                const auto val = sliced[0].value();

                CHECK_THROWS_AS(sliced.push_back(make_nullable(val)), std::logic_error);
                CHECK_THROWS_AS(sliced.erase(sliced.cbegin()), std::logic_error);
            }

            SUBCASE("push_back")
            {
                fixed_sized_list_array arr(proxy);
                REQUIRE_EQ(arr.size(), 4u);

                // push_back a copy of arr[0] = [0,1,2,3,4]
                arr.push_back(make_nullable(arr[0].value()));

                REQUIRE_EQ(arr.size(), 5u);
                CHECK(arr[4].has_value());
                CHECK_EQ(arr[4].value().size(), 5u);
                CHECK_NULLABLE_VARIANT_EQ(arr[4].value()[0], static_cast<inner_scalar_type>(0));
                CHECK_NULLABLE_VARIANT_EQ(arr[4].value()[4], static_cast<inner_scalar_type>(4));
            }

            SUBCASE("operator[]")
            {
                SUBCASE("replaces element from same array")
                {
                    fixed_sized_list_array arr(proxy);

                    arr[0] = make_nullable(arr[1].value());  // arr[1]=[5..9]

                    REQUIRE_EQ(arr.size(), 4u);
                    // arr[0] and arr[1] both equal [5..9]
                    CHECK_EQ(arr[0].value().size(), list_size);
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(5));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[4], static_cast<inner_scalar_type>(9));
                    CHECK_EQ(arr[1].value().size(), list_size);
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(5));
                    CHECK_EQ(arr[2].value().size(), list_size);
                    CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[0], static_cast<inner_scalar_type>(10));
                    CHECK_EQ(arr[3].value().size(), list_size);
                    CHECK_NULLABLE_VARIANT_EQ(arr[3].value()[0], static_cast<inner_scalar_type>(15));
                }

                SUBCASE("replaces element from another array")
                {
                    fixed_sized_list_array arr(proxy);
                    // source: 1 list of size 5 with iota [0..4]
                    fixed_sized_list_array source(
                        test::make_fixed_sized_list_proxy<inner_scalar_type>(list_size, list_size)
                    );

                    arr[2] = make_nullable(source[0].value());  // source[0]=[0..4]

                    REQUIRE_EQ(arr.size(), 4u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(0));
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(5));
                    // arr[2] now equals source[0] = [0..4]
                    CHECK_EQ(arr[2].value().size(), list_size);
                    CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[0], static_cast<inner_scalar_type>(0));
                    CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[4], static_cast<inner_scalar_type>(4));
                    CHECK_NULLABLE_VARIANT_EQ(arr[3].value()[0], static_cast<inner_scalar_type>(15));
                }
            }

            SUBCASE("element proxy replaces element from another array")
            {
                fixed_sized_list_array arr(proxy);
                // source: 1 list of size 5 with iota [0..4]
                fixed_sized_list_array source(
                    test::make_fixed_sized_list_proxy<inner_scalar_type>(list_size, list_size)
                );

                auto element = arr[3].value();       // arr[3]=[15..19]
                element = source[0].value();         // source[0]=[0..4]

                REQUIRE_EQ(arr.size(), 4u);
                CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(0));
                CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(5));
                CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[0], static_cast<inner_scalar_type>(10));
                // arr[3] was [15..19], now [0..4]
                CHECK_EQ(arr[3].value().size(), list_size);
                CHECK_NULLABLE_VARIANT_EQ(arr[3].value()[0], static_cast<inner_scalar_type>(0));
                CHECK_NULLABLE_VARIANT_EQ(arr[3].value()[4], static_cast<inner_scalar_type>(4));
            }

            SUBCASE("pop_back")
            {
                fixed_sized_list_array arr(proxy);
                REQUIRE_EQ(arr.size(), 4u);

                arr.pop_back();

                REQUIRE_EQ(arr.size(), 3u);
                CHECK_EQ(arr[2].value().size(), 5u);
                CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[0], static_cast<inner_scalar_type>(10));
            }

            SUBCASE("erase")
            {
                SUBCASE(" first element")
                {
                    fixed_sized_list_array arr(proxy);

                    arr.erase(arr.cbegin());

                    REQUIRE_EQ(arr.size(), 3u);
                    CHECK_EQ(arr[0].value().size(), 5u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(5));
                }
            }

            SUBCASE("resize")
            {
                SUBCASE("grow")
                {
                    fixed_sized_list_array arr(proxy);

                    arr.resize(6, make_nullable(arr[0].value()));

                    REQUIRE_EQ(arr.size(), 6u);
                    CHECK_EQ(arr[5].value().size(), 5u);
                }

                SUBCASE("shrink")
                {
                    fixed_sized_list_array arr(proxy);

                    arr.resize(2, make_nullable(arr[0].value()));

                    REQUIRE_EQ(arr.size(), 2u);
                    CHECK_EQ(arr[1].value().size(), 5u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(5));
                }
            }

            SUBCASE("insert")
            {
                SUBCASE("at beginning")
                {
                    fixed_sized_list_array arr(proxy);
                    const auto val = arr[0].value();  // [0,1,2,3,4]

                    arr.insert(arr.cbegin(), make_nullable(val));

                    REQUIRE_EQ(arr.size(), 5u);
                    CHECK(arr[0].has_value());
                    CHECK_EQ(arr[0].value().size(), 5u);  // newly inserted
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(0));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[4], static_cast<inner_scalar_type>(4));
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(0));   // was arr[0]
                    CHECK_NULLABLE_VARIANT_EQ(arr[4].value()[0], static_cast<inner_scalar_type>(15));  // was arr[3]
                }

                SUBCASE("in the middle")
                {
                    fixed_sized_list_array arr(proxy);
                    const auto val = arr[0].value();  // [0,1,2,3,4]

                    arr.insert(sparrow::next(arr.cbegin(), 2), make_nullable(val));

                    REQUIRE_EQ(arr.size(), 5u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(0));  // was arr[0]
                    CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], static_cast<inner_scalar_type>(5));  // was arr[1]
                    CHECK(arr[2].has_value());
                    CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[0], static_cast<inner_scalar_type>(0));   // newly inserted
                    CHECK_NULLABLE_VARIANT_EQ(arr[3].value()[0], static_cast<inner_scalar_type>(10));  // was arr[2]
                    CHECK_NULLABLE_VARIANT_EQ(arr[4].value()[0], static_cast<inner_scalar_type>(15));  // was arr[3]
                }

                SUBCASE("at end")
                {
                    fixed_sized_list_array arr(proxy);
                    const auto val = arr[0].value();  // [0,1,2,3,4]

                    arr.insert(arr.cend(), make_nullable(val));

                    REQUIRE_EQ(arr.size(), 5u);
                    CHECK_NULLABLE_VARIANT_EQ(arr[3].value()[0], static_cast<inner_scalar_type>(15));  // was arr[3]
                    CHECK(arr[4].has_value());
                    CHECK_EQ(arr[4].value().size(), 5u);  // newly inserted
                    CHECK_NULLABLE_VARIANT_EQ(arr[4].value()[0], static_cast<inner_scalar_type>(0));
                    CHECK_NULLABLE_VARIANT_EQ(arr[4].value()[4], static_cast<inner_scalar_type>(4));
                }

                SUBCASE("range at end")
                {
                    fixed_sized_list_array arr(proxy);
                    // source: 2 lists of size 5 with iota [0..9]
                    fixed_sized_list_array source(
                        test::make_fixed_sized_list_proxy<inner_scalar_type>(2 * list_size, list_size)
                    );
                    const std::vector<fixed_sized_list_array::value_type> to_insert = {
                        make_nullable(source[0].value()),  // [0..4]
                        make_nullable(source[1].value())   // [5..9]
                    };

                    const auto it = arr.insert(arr.cend(), to_insert.begin(), to_insert.end());

                    REQUIRE_EQ(arr.size(), 6u);
                    CHECK_EQ(it, sparrow::next(arr.begin(), 4));
                    // original elements unchanged
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], static_cast<inner_scalar_type>(0));
                    CHECK_NULLABLE_VARIANT_EQ(arr[3].value()[0], static_cast<inner_scalar_type>(15));
                    // newly inserted at end
                    CHECK_EQ(arr[4].value().size(), list_size);
                    CHECK_NULLABLE_VARIANT_EQ(arr[4].value()[0], static_cast<inner_scalar_type>(0));
                    CHECK_NULLABLE_VARIANT_EQ(arr[4].value()[4], static_cast<inner_scalar_type>(4));
                    CHECK_EQ(arr[5].value().size(), list_size);
                    CHECK_NULLABLE_VARIANT_EQ(arr[5].value()[0], static_cast<inner_scalar_type>(5));
                    CHECK_NULLABLE_VARIANT_EQ(arr[5].value()[4], static_cast<inner_scalar_type>(9));
                }
            }
        }
    }
}
