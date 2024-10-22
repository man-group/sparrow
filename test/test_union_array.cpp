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

#include "sparrow/layout/primitive_array.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/layout/dispatch.hpp"
#include "doctest/doctest.h"

#include "test_utils.hpp"
#include "../test/external_array_data_creation.hpp"

#include "sparrow/layout/union_array.hpp"

namespace sparrow
{

    namespace test
    {
        arrow_proxy make_sparse_union_proxy(const std::string& format_string, std::size_t n, bool altered = false)
        {
            std::vector<ArrowArray> children_arrays(2);
            std::vector<ArrowSchema> children_schemas(2);

            test::fill_schema_and_array<float>(children_schemas[0], children_arrays[0], n, 0/*offset*/, {});
            children_schemas[0].name = "item 0";

            test::fill_schema_and_array<std::uint16_t>(children_schemas[1], children_arrays[1], n, 0/*offset*/, {});
            children_schemas[1].name = "item 1";

            ArrowArray arr{};
            ArrowSchema schema{};

            std::vector<std::uint8_t> type_ids = {std::uint8_t(3), std::uint8_t(4), std::uint8_t(3), std::uint8_t(4)};
            if (altered)
            {
                type_ids[0] = std::uint8_t(4);
            }
            
            test::fill_schema_and_array_for_sparse_union(
                schema, arr, std::move(children_schemas), std::move(children_arrays), type_ids, format_string
            );

            return arrow_proxy(std::move(arr), std::move(schema));
        }

        arrow_proxy make_dense_union_proxy(const std::string& format_string, std::size_t n_c, bool altered = false)
        {
            std::vector<ArrowArray> children_arrays(2);
            std::vector<ArrowSchema> children_schemas(2);

            test::fill_schema_and_array<float>(children_schemas[0], children_arrays[0], n_c, 0/*offset*/, {});
            children_schemas[0].name = "item 0";

            test::fill_schema_and_array<std::uint16_t>(children_schemas[1], children_arrays[1], n_c, 0/*offset*/, {});
            children_schemas[1].name = "item 1";

            ArrowArray arr{};
            ArrowSchema schema{};

            std::vector<std::uint8_t> type_ids = {std::uint8_t(3), std::uint8_t(4), std::uint8_t(3), std::uint8_t(4)};
            if (altered)
            {
                type_ids[0] = std::uint8_t(4);
            }
            std::vector<std::int32_t> offsets = {0,0,1,1};
            
            test::fill_schema_and_array_for_dense_union(
                schema, arr, std::move(children_schemas), std::move(children_arrays), type_ids, offsets, format_string
            );

            return arrow_proxy(std::move(arr), std::move(schema));
        }
    }

    TEST_SUITE("union")
    {   
        TEST_CASE("sparse_union")
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
                std::visit([](auto&& arg) {
                    using inner_type = std::decay_t< typename std::decay_t<decltype(arg)>::value_type>;
                    if constexpr (std::is_same_v<inner_type, float>)
                    {
                        REQUIRE_EQ(0.0f, arg.value());
                    }
                    else
                    {
                        CHECK(false);
                    }
                
                }, uarr[0]);

                // 1
                std::visit([](auto&& arg) {
                    using inner_type = std::decay_t< typename std::decay_t<decltype(arg)>::value_type>;
                    if constexpr (std::is_same_v<inner_type, std::uint16_t>)
                    {
                        REQUIRE_EQ(1, arg.value());
                    }
                    else
                    {
                        CHECK(false);
                    }
                
                }, uarr[1]);

                // 2 
                std::visit([](auto&& arg) {
                    using inner_type = std::decay_t< typename std::decay_t<decltype(arg)>::value_type>;
                    if constexpr (std::is_same_v<inner_type, float>)
                    {
                        REQUIRE_EQ(2.0f, arg.value());
                    }
                    else
                    {
                        CHECK(false);
                    }
                
                }, uarr[2]);

                // 3 
                std::visit([](auto&& arg) {
                    using inner_type = std::decay_t< typename std::decay_t<decltype(arg)>::value_type>;
                    if constexpr (std::is_same_v<inner_type, std::uint16_t>)
                    {
                        REQUIRE_EQ(3, arg.value());
                    }
                    else
                    {
                        CHECK(false);
                    }
                
                }, uarr[3]);
                
            }
        }

        TEST_CASE("dense_union")
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
            std::visit([](auto&& arg) {
                using inner_type = std::decay_t< typename std::decay_t<decltype(arg)>::value_type>;
                if constexpr (std::is_same_v<inner_type, float>)
                {
                    REQUIRE_EQ(0.0f, arg.value());
                }
                else
                {
                    CHECK(false);
                }
            
            }, uarr[0]);

            // 1
            std::visit([](auto&& arg) {
                using inner_type = std::decay_t< typename std::decay_t<decltype(arg)>::value_type>;
                if constexpr (std::is_same_v<inner_type, std::uint16_t>)
                {
                    REQUIRE_EQ(0, arg.value());
                }
                else
                {
                    CHECK(false);
                }
            
            }, uarr[1]);

            // 2 
            std::visit([](auto&& arg) {
                using inner_type = std::decay_t< typename std::decay_t<decltype(arg)>::value_type>;
                if constexpr (std::is_same_v<inner_type, float>)
                {
                    REQUIRE_EQ(1.0f, arg.value());
                }
                else
                {
                    CHECK(false);
                }
            
            }, uarr[2]);

            // 3
            std::visit([](auto&& arg) {
                using inner_type = std::decay_t< typename std::decay_t<decltype(arg)>::value_type>;
                if constexpr (std::is_same_v<inner_type, std::uint16_t>)
                {
                    REQUIRE_EQ(1, arg.value());
                }
                else
                {
                    CHECK(false);
                }
            
            }, uarr[3]);
        }
    }


}

