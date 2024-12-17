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
#include "sparrow/layout/dispatch.hpp"
#include "sparrow/layout/primitive_array.hpp"
#include "sparrow/layout/union_array.hpp"
#include "sparrow/utils/nullable.hpp"

#include "../test/external_array_data_creation.hpp"
#include "doctest/doctest.h"
#include "test_utils.hpp"

namespace sparrow
{

    namespace test
    {
        arrow_proxy
        make_sparse_union_proxy(const std::string& format_string, std::size_t n, bool altered = false)
        {
            std::vector<ArrowArray> children_arrays(2);
            std::vector<ArrowSchema> children_schemas(2);

            test::fill_schema_and_array<float>(children_schemas[0], children_arrays[0], n, 0 /*offset*/, {});
            children_schemas[0].name = "item 0";

            test::fill_schema_and_array<std::uint16_t>(children_schemas[1], children_arrays[1], n, 0 /*offset*/, {});
            children_schemas[1].name = "item 1";

            ArrowArray arr{};
            ArrowSchema schema{};

            std::vector<std::uint8_t> type_ids =
                {std::uint8_t(3), std::uint8_t(4), std::uint8_t(3), std::uint8_t(4)};
            if (altered)
            {
                type_ids[0] = std::uint8_t(4);
            }

            test::fill_schema_and_array_for_sparse_union(
                schema,
                arr,
                std::move(children_schemas),
                std::move(children_arrays),
                type_ids,
                format_string
            );

            return arrow_proxy(std::move(arr), std::move(schema));
        }

        arrow_proxy
        make_dense_union_proxy(const std::string& format_string, std::size_t n_c, bool altered = false)
        {
            std::vector<ArrowArray> children_arrays(2);
            std::vector<ArrowSchema> children_schemas(2);

            test::fill_schema_and_array<float>(children_schemas[0], children_arrays[0], n_c, 0 /*offset*/, {});
            children_schemas[0].name = "item 0";

            test::fill_schema_and_array<std::uint16_t>(
                children_schemas[1],
                children_arrays[1],
                n_c,
                0 /*offset*/,
                {}
            );
            children_schemas[1].name = "item 1";

            ArrowArray arr{};
            ArrowSchema schema{};

            std::vector<std::uint8_t> type_ids =
                {std::uint8_t(3), std::uint8_t(4), std::uint8_t(3), std::uint8_t(4)};
            if (altered)
            {
                type_ids[0] = std::uint8_t(4);
            }
            std::vector<std::int32_t> offsets = {0, 0, 1, 1};

            test::fill_schema_and_array_for_dense_union(
                schema,
                arr,
                std::move(children_schemas),
                std::move(children_arrays),
                type_ids,
                offsets,
                format_string
            );

            return arrow_proxy(std::move(arr), std::move(schema));
        }
    }

    TEST_SUITE("sparse_union")
    {
        static_assert(is_sparse_union_array_v<sparse_union_array>);
        static_assert(!is_dense_union_array_v<sparse_union_array>);

        TEST_CASE("constructor")
        {
            // the child arrays
            primitive_array<std::int16_t> arr1({{std::int16_t(2), std::int16_t(5), std::size_t(9)}});
            primitive_array<std::int32_t> arr2(
                std::vector<std::int32_t>{std::int32_t(3), std::int32_t(4), std::size_t(5)},
                std::vector<std::size_t>{1}  // INDEX 1 IS MISSING
            );

            // detyped arrays
            std::vector<array> children = {array(std::move(arr1)), array(std::move(arr2))};


            SUBCASE("with mapping")
            {
                // type ids
                sparse_union_array::type_id_buffer_type type_ids{
                    {std::uint8_t(2), std::uint8_t(3), std::uint8_t(3)}
                };

                // mapping
                std::vector<std::size_t> type_mapping{2, 3};

                // the array
                sparse_union_array arr(std::move(children), std::move(type_ids), std::move(type_mapping));

                // check the size
                REQUIRE_EQ(arr.size(), 3);

                // check elements have values
                CHECK(arr[0].has_value());
                CHECK(!arr[1].has_value());
                CHECK(arr[2].has_value());

                CHECK_NULLABLE_VARIANT_EQ(arr[0], std::int16_t(2));
                CHECK_NULLABLE_VARIANT_EQ(arr[2], std::int32_t(5));
            }
            SUBCASE("without mapping")
            {
                // type ids
                sparse_union_array::type_id_buffer_type type_ids{
                    {std::uint8_t(0), std::uint8_t(1), std::uint8_t(1)}
                };

                // the array
                sparse_union_array arr(std::move(children), std::move(type_ids));

                // check the size
                REQUIRE_EQ(arr.size(), 3);

                // check elements have values
                CHECK(arr[0].has_value());
                CHECK(!arr[1].has_value());
                CHECK(arr[2].has_value());

                CHECK_NULLABLE_VARIANT_EQ(arr[0], std::int16_t(2));
                CHECK_NULLABLE_VARIANT_EQ(arr[2], std::int32_t(5));
            }
        }
        TEST_CASE("basics")
        {
            const std::string format_string = "+us:3,4";
            const std::size_t n = 4;


            auto proxy = test::make_sparse_union_proxy(format_string, n);
            sparse_union_array uarr(std::move(proxy));

            REQUIRE(uarr.size() == n);

            SUBCASE("copy")
            {
                sparse_union_array uarr2(uarr);
                CHECK_EQ(uarr2, uarr);

                sparse_union_array uarr3(test::make_sparse_union_proxy(format_string, n, true));
                CHECK_NE(uarr3, uarr);
                uarr3 = uarr;
                CHECK_EQ(uarr3, uarr);
            }

            SUBCASE("move")
            {
                sparse_union_array uarr2(uarr);
                sparse_union_array uarr3(std::move(uarr2));
                CHECK_EQ(uarr3, uarr);

                sparse_union_array uarr4(test::make_sparse_union_proxy(format_string, n, true));
                CHECK_NE(uarr4, uarr);
                uarr4 = std::move(uarr3);
                CHECK_EQ(uarr4, uarr);
            }

            SUBCASE("operator[]")
            {
                for (std::size_t i = 0; i < n; ++i)
                {
                    const auto& val = uarr[i];
                    REQUIRE(val.has_value());
                }

                // 0
                std::visit(
                    [](auto&& arg)
                    {
                        using inner_type = std::decay_t<typename std::decay_t<decltype(arg)>::value_type>;
                        if constexpr (std::is_same_v<inner_type, float>)
                        {
                            REQUIRE_EQ(0.0f, arg.value());
                        }
                        else
                        {
                            CHECK(false);
                        }
                    },
                    uarr[0]
                );

                // 1
                std::visit(
                    [](auto&& arg)
                    {
                        using inner_type = std::decay_t<typename std::decay_t<decltype(arg)>::value_type>;
                        if constexpr (std::is_same_v<inner_type, std::uint16_t>)
                        {
                            REQUIRE_EQ(1, arg.value());
                        }
                        else
                        {
                            CHECK(false);
                        }
                    },
                    uarr[1]
                );

                // 2
                std::visit(
                    [](auto&& arg)
                    {
                        using inner_type = std::decay_t<typename std::decay_t<decltype(arg)>::value_type>;
                        if constexpr (std::is_same_v<inner_type, float>)
                        {
                            REQUIRE_EQ(2.0f, arg.value());
                        }
                        else
                        {
                            CHECK(false);
                        }
                    },
                    uarr[2]
                );

                // 3
                std::visit(
                    [](auto&& arg)
                    {
                        using inner_type = std::decay_t<typename std::decay_t<decltype(arg)>::value_type>;
                        if constexpr (std::is_same_v<inner_type, std::uint16_t>)
                        {
                            REQUIRE_EQ(3, arg.value());
                        }
                        else
                        {
                            CHECK(false);
                        }
                    },
                    uarr[3]
                );
            }
        }
    }

    TEST_SUITE("dense_union")
    {
        static_assert(is_dense_union_array_v<dense_union_array>);
        static_assert(!is_sparse_union_array_v<dense_union_array>);
        TEST_CASE("constructor")
        {
            // the child arrays
            primitive_array<std::int16_t> arr1({{std::int16_t(0), std::int16_t(1)}});
            primitive_array<std::int32_t> arr2(
                std::vector<std::int32_t>{std::int32_t(2), std::int32_t(3)},
                std::vector<std::size_t>{1}  // INDEX 1 IS MISSING
            );

            // detyped arrays
            std::vector<array> children = {array(std::move(arr1)), array(std::move(arr2))};


            // offsets
            dense_union_array::offset_buffer_type offsets{
                {std::size_t(1), std::size_t(1), std::size_t(0), std::size_t(0)}
            };

            SUBCASE("without mapping")
            {
                // type ids
                dense_union_array::type_id_buffer_type type_ids{
                    {std::uint8_t(0), std::uint8_t(1), std::uint8_t(0), std::uint8_t(1)}
                };

                // the array
                dense_union_array arr(std::move(children), std::move(type_ids), std::move(offsets));

                // check the size
                REQUIRE_EQ(arr.size(), 4);

                // check elements have values
                CHECK(arr[0].has_value());
                CHECK(!arr[1].has_value());
                CHECK(arr[2].has_value());
                CHECK(arr[3].has_value());

                CHECK_NULLABLE_VARIANT_EQ(arr[0], std::int16_t(1));
                CHECK_NULLABLE_VARIANT_EQ(arr[2], std::int16_t(0));
                CHECK_NULLABLE_VARIANT_EQ(arr[3], std::int32_t(2));
            }
            SUBCASE("with mapping")
            {
                std::vector<std::size_t> child_index_to_type_id{1, 0};
                // type ids
                dense_union_array::type_id_buffer_type type_ids{
                    {std::uint8_t(1), std::uint8_t(0), std::uint8_t(1), std::uint8_t(0)}
                };

                // the array
                dense_union_array arr(
                    std::move(children),
                    std::move(type_ids),
                    std::move(offsets),
                    std::move(child_index_to_type_id)
                );

                // check the size
                REQUIRE_EQ(arr.size(), 4);

                // check elements have values
                CHECK(arr[0].has_value());
                CHECK(!arr[1].has_value());
                CHECK(arr[2].has_value());
                CHECK(arr[3].has_value());

                CHECK_NULLABLE_VARIANT_EQ(arr[0], std::int16_t(1));
                CHECK_NULLABLE_VARIANT_EQ(arr[2], std::int16_t(0));
                CHECK_NULLABLE_VARIANT_EQ(arr[3], std::int32_t(2));
            }
        }
        TEST_CASE("basics")
        {
            const std::string format_string = "+ud:3,4";
            const std::size_t n_c = 2;
            const std::size_t n = 4;

            auto proxy = test::make_dense_union_proxy(format_string, n_c);
            dense_union_array uarr(std::move(proxy));

            REQUIRE(uarr.size() == n);

            SUBCASE("copy")
            {
                dense_union_array uarr2(uarr);
                CHECK_EQ(uarr2, uarr);

                dense_union_array uarr3(test::make_dense_union_proxy(format_string, n_c, true));
                CHECK_NE(uarr3, uarr);
                uarr3 = uarr;
                CHECK_EQ(uarr3, uarr);
            }

            SUBCASE("move")
            {
                dense_union_array uarr2(uarr);
                dense_union_array uarr3(std::move(uarr2));
                CHECK_EQ(uarr3, uarr);

                dense_union_array uarr4(test::make_dense_union_proxy(format_string, n_c, true));
                CHECK_NE(uarr4, uarr);
                uarr4 = std::move(uarr3);
                CHECK_EQ(uarr4, uarr);
            }

            SUBCASE("operator[]")
            {
                for (std::size_t i = 0; i < n; ++i)
                {
                    const auto& val = uarr[i];
                    REQUIRE(val.has_value());
                }
            }

            // 0
            std::visit(
                [](auto&& arg)
                {
                    using inner_type = std::decay_t<typename std::decay_t<decltype(arg)>::value_type>;
                    if constexpr (std::is_same_v<inner_type, float>)
                    {
                        REQUIRE_EQ(0.0f, arg.value());
                    }
                    else
                    {
                        CHECK(false);
                    }
                },
                uarr[0]
            );

            // 1
            std::visit(
                [](auto&& arg)
                {
                    using inner_type = std::decay_t<typename std::decay_t<decltype(arg)>::value_type>;
                    if constexpr (std::is_same_v<inner_type, std::uint16_t>)
                    {
                        REQUIRE_EQ(0, arg.value());
                    }
                    else
                    {
                        CHECK(false);
                    }
                },
                uarr[1]
            );

            // 2
            std::visit(
                [](auto&& arg)
                {
                    using inner_type = std::decay_t<typename std::decay_t<decltype(arg)>::value_type>;
                    if constexpr (std::is_same_v<inner_type, float>)
                    {
                        REQUIRE_EQ(1.0f, arg.value());
                    }
                    else
                    {
                        CHECK(false);
                    }
                },
                uarr[2]
            );

            // 3
            std::visit(
                [](auto&& arg)
                {
                    using inner_type = std::decay_t<typename std::decay_t<decltype(arg)>::value_type>;
                    if constexpr (std::is_same_v<inner_type, std::uint16_t>)
                    {
                        REQUIRE_EQ(1, arg.value());
                    }
                    else
                    {
                        CHECK(false);
                    }
                },
                uarr[3]
            );
        }
    }


}
