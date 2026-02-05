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

#include <string_view>

#include "sparrow/array.hpp"
#include "sparrow/primitive_array.hpp"
#include "sparrow/struct_array.hpp"

#include "../test/external_array_data_creation.hpp"
#include "doctest/doctest.h"
#include "test_utils.hpp"

namespace sparrow
{
    namespace test
    {
        template <class T0, class T1>
        arrow_proxy make_struct_proxy(size_t n)
        {
            // n-children == 2
            std::vector<ArrowArray> children_arrays(2);
            std::vector<ArrowSchema> children_schemas(2);

            test::fill_schema_and_array<T0>(children_schemas[0], children_arrays[0], n, 0, {});
            children_schemas[0].name = "item 0";

            test::fill_schema_and_array<T1>(children_schemas[1], children_arrays[1], n, 0, {});
            children_schemas[1].name = "item 1";

            ArrowArray arr{};
            ArrowSchema schema{};
            test::fill_schema_and_array_for_struct_layout(
                schema,
                arr,
                std::move(children_schemas),
                std::move(children_arrays),
                {}
            );
            return arrow_proxy(std::move(arr), std::move(schema));
        }
    }

    TEST_SUITE("struct_array")
    {
        static_assert(is_struc_array_v<struct_array>);

        TEST_CASE("constructors")
        {
            primitive_array<std::int16_t> flat_arr(
                {{std::int16_t(0), std::int16_t(1), std::int16_t(2), std::int16_t(3)}, true, "flat_arr1"}
            );
            primitive_array<float32_t> flat_arr2({{4.0f, 5.0f, 6.0f, 7.0f}, true, "flat_arr2"});
            primitive_array<std::int32_t> flat_arr3(
                {{std::int32_t(8), std::int32_t(9), std::int32_t(10), std::int32_t(11)}, true, "flat_arr3"}
            );

            // detyped arrays
            array arr1(std::move(flat_arr));
            array arr2(std::move(flat_arr2));
            array arr3(std::move(flat_arr3));
            std::vector<array> children{std::move(arr1), std::move(arr2), std::move(arr3)};

            SUBCASE("with whildren, nullable, name and metadata")
            {
                const struct_array arr(children, false, "name", metadata_sample_opt);

                // check the size
                REQUIRE_EQ(arr.size(), 4);

                // check the children
                REQUIRE_EQ(arr[0].value().size(), 3);
                REQUIRE_EQ(arr[1].value().size(), 3);
                REQUIRE_EQ(arr[2].value().size(), 3);
                REQUIRE_EQ(arr[3].value().size(), 3);

                // check the values
                const auto child0 = arr[0].value();
                const auto child0_names = child0.names();
                REQUIRE_EQ(child0_names.size(), 3);
                CHECK_EQ(child0_names[0], "flat_arr1");
                CHECK_EQ(child0_names[1], "flat_arr2");
                CHECK_EQ(child0_names[2], "flat_arr3");

                CHECK_NULLABLE_VARIANT_EQ(child0[0], std::int16_t(0));
                CHECK_NULLABLE_VARIANT_EQ(child0[1], float32_t(4.0f));
                CHECK_NULLABLE_VARIANT_EQ(child0[2], std::int32_t(8));

                const auto child1 = arr[1].value();
                const auto child1_names = child1.names();
                REQUIRE_EQ(child1_names.size(), 3);
                CHECK_EQ(child1_names[0], "flat_arr1");
                CHECK_EQ(child1_names[1], "flat_arr2");
                CHECK_EQ(child1_names[2], "flat_arr3");
                CHECK_NULLABLE_VARIANT_EQ(child1[0], std::int16_t(1));
                CHECK_NULLABLE_VARIANT_EQ(child1[1], float32_t(5.0f));
                CHECK_NULLABLE_VARIANT_EQ(child1[2], std::int32_t(9));

                const auto child2 = arr[2].value();
                const auto child2_names = child2.names();
                REQUIRE_EQ(child2_names.size(), 3);
                CHECK_EQ(child2_names[0], "flat_arr1");
                CHECK_EQ(child2_names[1], "flat_arr2");
                CHECK_EQ(child2_names[2], "flat_arr3");
                CHECK_NULLABLE_VARIANT_EQ(child2[0], std::int16_t(2));
                CHECK_NULLABLE_VARIANT_EQ(child2[1], float32_t(6.0f));
                CHECK_NULLABLE_VARIANT_EQ(child2[2], std::int32_t(10));

                const auto child3 = arr[3].value();
                const auto child3_names = child3.names();
                REQUIRE_EQ(child3_names.size(), 3);
                CHECK_EQ(child3_names[0], "flat_arr1");
                CHECK_EQ(child3_names[1], "flat_arr2");
                CHECK_EQ(child3_names[2], "flat_arr3");
                CHECK_NULLABLE_VARIANT_EQ(child3[0], std::int16_t(3));
                CHECK_NULLABLE_VARIANT_EQ(child3[1], float32_t(7.0f));
                CHECK_NULLABLE_VARIANT_EQ(child3[2], std::int32_t(11));
            }

            SUBCASE("with whildren, bitmap, name and metadata")
            {
                std::vector<bool> bitmap{true, false, true, false};
                const struct_array arr(children, bitmap, "name", metadata_sample_opt);

                // check the size
                REQUIRE_EQ(arr.size(), 4);

                // check the children
                REQUIRE_EQ(arr[0].value().size(), 3);
                REQUIRE_EQ(arr[2].value().size(), 3);

                // check the values
                const auto child0 = arr[0].value();
                CHECK_NULLABLE_VARIANT_EQ(child0[0], std::int16_t(0));
                CHECK_NULLABLE_VARIANT_EQ(child0[1], float32_t(4.0f));
                CHECK_NULLABLE_VARIANT_EQ(child0[2], std::int32_t(8));

                CHECK_FALSE(arr[1].has_value());

                const auto child2 = arr[2].value();
                CHECK_NULLABLE_VARIANT_EQ(child2[0], std::int16_t(2));
                CHECK_NULLABLE_VARIANT_EQ(child2[1], float32_t(6.0f));
                CHECK_NULLABLE_VARIANT_EQ(child2[2], std::int32_t(10));

                CHECK_FALSE(arr[3].has_value());
            }
        }
    };

    TEST_CASE_TEMPLATE("struct[T, uint8]", T, std::uint8_t, std::int32_t, float32_t, float64_t)
    {
        using inner_scalar_type = T;
        // using inner_nullable_type = nullable<inner_scalar_type>;

        // number of elements in the struct array
        const std::size_t n = 4;
        const std::size_t n2 = 3;

        // create a struct array
        arrow_proxy proxy = test::make_struct_proxy<inner_scalar_type, uint8_t>(n);
        struct_array struct_arr(std::move(proxy));
        REQUIRE(struct_arr.size() == n);

        SUBCASE("copy")
        {
#ifdef SPARROW_TRACK_COPIES
            copy_tracker::reset(copy_tracker::key<struct_array>());
#endif
            struct_array struct_arr2(struct_arr);
            CHECK_EQ(struct_arr, struct_arr2);
#ifdef SPARROW_TRACK_COPIES
            CHECK_EQ(copy_tracker::count(copy_tracker::key<struct_array>()), 1);
#endif

            struct_array struct_arr3(test::make_struct_proxy<inner_scalar_type, uint8_t>(n2));
            CHECK_NE(struct_arr3, struct_arr);
            struct_arr3 = struct_arr;
            CHECK_EQ(struct_arr3, struct_arr);
        }

        SUBCASE("move")
        {
            struct_array struct_arr2(struct_arr);
            struct_array struct_arr3(std::move(struct_arr2));
            CHECK_EQ(struct_arr3, struct_arr);

            struct_array struct_arr4(test::make_struct_proxy<inner_scalar_type, uint8_t>(n2));
            CHECK_NE(struct_arr4, struct_arr);
            struct_arr4 = std::move(struct_arr3);
            CHECK_EQ(struct_arr4, struct_arr);
        }

        SUBCASE("operator[]")
        {
            for (std::size_t i = 0; i < n; ++i)
            {
                auto val = struct_arr[i];
                REQUIRE(val.has_value());
                auto struct_val = val.value();
                REQUIRE_EQ(struct_val.size(), 2);

                auto val0_variant = struct_val[0];
                auto val1_variant = struct_val[1];

                REQUIRE(val0_variant.has_value());
                REQUIRE(val1_variant.has_value());

                // using const_scalar_ref = const inner_scalar_type&;
                using nullable_inner_scalar_type = nullable<const inner_scalar_type&, bool>;
                using nullable_uint8_t = nullable<const std::uint8_t&, bool>;

#if SPARROW_GCC_11_2_WORKAROUND
                using variant_type = std::decay_t<decltype(val0_variant)>;
                using base_type = typename variant_type::base_type;
#endif
                // visit the variant
                std::visit(
                    [&i](auto&& val0)
                    {
                        if constexpr (std::is_same_v<std::decay_t<decltype(val0)>, nullable_inner_scalar_type>)
                        {
                            CHECK_EQ(val0.value(), static_cast<inner_scalar_type>(i));
                        }
                        else
                        {
                            FAIL("unexpected type");
                        }
                    },
#if SPARROW_GCC_11_2_WORKAROUND
                    static_cast<const base_type&>(val0_variant)
#else
                    val0_variant
#endif
                );

                std::visit(
                    [&i](auto&& val1)
                    {
                        if constexpr (std::is_same_v<std::decay_t<decltype(val1)>, nullable_uint8_t>)
                        {
                            CHECK_EQ(val1.value(), static_cast<inner_scalar_type>(i));
                        }
                        else
                        {
                            FAIL("unexpected type");
                        }
                    },
#if SPARROW_GCC_11_2_WORKAROUND
                    static_cast<const base_type&>(val1_variant)
#else
                    val1_variant
#endif
                );
            }
        }

        SUBCASE("at")
        {
            for (std::size_t i = 0; i < n; ++i)
            {
                auto val = struct_arr[i];
                REQUIRE(val.has_value());
                auto struct_val = val.value();
                REQUIRE_EQ(struct_val.size(), 2);

                auto val0_variant = struct_val.at(0);
                auto val1_variant = struct_val.at(1);

                REQUIRE(val0_variant.has_value());
                REQUIRE(val1_variant.has_value());

                // using const_scalar_ref = const inner_scalar_type&;
                using nullable_inner_scalar_type = nullable<const inner_scalar_type&, bool>;
                using nullable_uint8_t = nullable<const std::uint8_t&, bool>;

#if SPARROW_GCC_11_2_WORKAROUND
                using variant_type = std::decay_t<decltype(val0_variant)>;
                using base_type = typename variant_type::base_type;
#endif
                // visit the variant
                std::visit(
                    [&i](auto&& val0)
                    {
                        if constexpr (std::is_same_v<std::decay_t<decltype(val0)>, nullable_inner_scalar_type>)
                        {
                            CHECK_EQ(val0.value(), static_cast<inner_scalar_type>(i));
                        }
                        else
                        {
                            FAIL("unexpected type");
                        }
                    },
#if SPARROW_GCC_11_2_WORKAROUND
                    static_cast<const base_type&>(val0_variant)
#else
                    val0_variant
#endif
                );

                std::visit(
                    [&i](auto&& val1)
                    {
                        if constexpr (std::is_same_v<std::decay_t<decltype(val1)>, nullable_uint8_t>)
                        {
                            CHECK_EQ(val1.value(), static_cast<inner_scalar_type>(i));
                        }
                        else
                        {
                            FAIL("unexpected type");
                        }
                    },
#if SPARROW_GCC_11_2_WORKAROUND
                    static_cast<const base_type&>(val1_variant)
#else
                    val1_variant
#endif
                );
            }
        }

        SUBCASE("at outside bounds")
        {
            auto val = struct_arr[0];
            REQUIRE(val.has_value());
            auto struct_val = val.value();
            REQUIRE_EQ(struct_val.size(), 2);
            CHECK_THROWS_AS(struct_val.at(2), std::out_of_range);
            CHECK_THROWS_AS(struct_val.at(100), std::out_of_range);
        }

        SUBCASE("operator==(struct_value, struct_value)")
        {
            CHECK(struct_arr[0] == struct_arr[0]);
            CHECK(struct_arr[0] != struct_arr[1]);
        }

        SUBCASE("consistency")
        {
            test::generic_consistency_test(struct_arr);
        }

        SUBCASE("add_child")
        {
            primitive_array<std::int16_t> new_child(
                {{std::int16_t(90), std::int16_t(91), std::int16_t(92), std::int16_t(93)}, true, "new_child"}
            );
            struct_arr.add_child(std::move(new_child));
            CHECK_EQ(struct_arr.children_count(), 3);
            CHECK_EQ(struct_arr.names().back(), "new_child");
            CHECK_NULLABLE_VARIANT_EQ(struct_arr[0].value().at(2), std::int16_t(90));
        }

        SUBCASE("set_child")
        {
            primitive_array<std::int16_t> new_child(
                {{std::int16_t(90), std::int16_t(91), std::int16_t(92), std::int16_t(93)}, true, "new_child"}
            );
            struct_arr.set_child(std::move(new_child), 1);
            CHECK_EQ(struct_arr.children_count(), 2);
            CHECK_EQ(struct_arr.names().back(), "new_child");
            CHECK_NULLABLE_VARIANT_EQ(struct_arr[0].value().at(1), std::int16_t(90));
        }

        SUBCASE("pop_children")
        {
            struct_arr.pop_children(1);
            CHECK_EQ(struct_arr.children_count(), 1);
        }

#if defined(__cpp_lib_format)
        SUBCASE("formatting")
        {
            const std::string formatted = std::format("{}", struct_arr);
            constexpr std::string_view expected = "|item 0|item 1|\n"
                                                  "---------------\n"
                                                  "|     0|     0|\n"
                                                  "|     1|     1|\n"
                                                  "|     2|     2|\n"
                                                  "|     3|     3|\n"
                                                  "---------------";
            CHECK_EQ(formatted, expected);
        }
#endif

        SUBCASE("struct_value iterators")
        {
            // Get a struct_value from the array
            auto val = struct_arr[0];
            REQUIRE(val.has_value());
            auto struct_val = val.value();
            REQUIRE_EQ(struct_val.size(), 2);

            // begin() and end() iteration
            int count1 = 0;
            for (auto it = struct_val.begin(); it != struct_val.end(); ++it)
            {
                CHECK(it->has_value());
                ++count1;
            }
            CHECK_EQ(count1, 2);

            // cbegin() and cend() iteration
            int count2 = 0;
            for (auto it = struct_val.cbegin(); it != struct_val.cend(); ++it)
            {
                CHECK(it->has_value());
                ++count2;
            }
            CHECK_EQ(count2, 2);

            // Range-based for loop
            int count3 = 0;
            for (const auto& elem : struct_val)
            {
                CHECK(elem.has_value());
                ++count3;
            }
            CHECK_EQ(count3, 2);
        }

        SUBCASE("struct_value begin/end iteration")
        {
            auto val = struct_arr[2];
            REQUIRE(val.has_value());
            auto struct_val = val.value();
            REQUIRE_EQ(struct_val.size(), 2);
            int i = 0;
            for (auto it = struct_val.begin(); it != struct_val.end(); ++it, ++i)
            {
                if (i == 0)
                {
                    CHECK_NULLABLE_VARIANT_EQ(*it, static_cast<inner_scalar_type>(2));
                }
                else if (i == 1)
                {
                    CHECK_NULLABLE_VARIANT_EQ(*it, static_cast<std::uint8_t>(2));
                }
            }
            CHECK_EQ(i, 2);
        }

        SUBCASE("struct_value cbegin/cend iteration")
        {
            auto val = struct_arr[2];
            REQUIRE(val.has_value());
            auto struct_val = val.value();
            REQUIRE_EQ(struct_val.size(), 2);
            int i = 0;
            for (auto it = struct_val.cbegin(); it != struct_val.cend(); ++it, ++i)
            {
                if (i == 0)
                {
                    CHECK_NULLABLE_VARIANT_EQ(*it, static_cast<inner_scalar_type>(2));
                }
                else if (i == 1)
                {
                    CHECK_NULLABLE_VARIANT_EQ(*it, static_cast<std::uint8_t>(2));
                }
            }
            CHECK_EQ(i, 2);
        }

        SUBCASE("struct_value range-based for loop")
        {
            auto val = struct_arr[2];
            REQUIRE(val.has_value());
            auto struct_val = val.value();
            REQUIRE_EQ(struct_val.size(), 2);
            int i = 0;
            for (const auto& elem : struct_val)
            {
                if (i == 0)
                {
                    CHECK_NULLABLE_VARIANT_EQ(elem, static_cast<inner_scalar_type>(2));
                }
                else if (i == 1)
                {
                    CHECK_NULLABLE_VARIANT_EQ(elem, static_cast<std::uint8_t>(2));
                }
                ++i;
            }
            CHECK_EQ(i, 2);
        }
    }
}
