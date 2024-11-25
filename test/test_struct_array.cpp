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
#include "sparrow/layout/struct_layout/struct_array.hpp"
#include "sparrow/utils/nullable.hpp"

#include "doctest/doctest.h"

#include "test_utils.hpp"
#include "../test/external_array_data_creation.hpp"
#include "sparrow/array.hpp"

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

            test::fill_schema_and_array<T0>(children_schemas[0], children_arrays[0], n, 0/*offset*/, {});
            children_schemas[0].name = "item 0";

            test::fill_schema_and_array<T1>(children_schemas[1], children_arrays[1], n, 0/*offset*/, {});
            children_schemas[1].name = "item 1";

            ArrowArray arr{};
            ArrowSchema schema{};
            test::fill_schema_and_array_for_struct_layout(schema, arr, std::move(children_schemas), std::move(children_arrays), {});
            return arrow_proxy(std::move(arr), std::move(schema));
        }
    }

    TEST_SUITE("struct_array")
    {   
        static_assert(is_struc_array_v<struct_array>);

        TEST_CASE("constructors")
        {
            primitive_array<std::int16_t> flat_arr({{std::int16_t(0), std::int16_t(1), std::int16_t(2), std::int16_t(3)}});
            primitive_array<float> flat_arr2({{4.0f, 5.0f, 6.0f, 7.0f}});
            primitive_array<std::int32_t> flat_arr3({{std::int32_t(8), std::int32_t(9), std::int32_t(10), std::int32_t(11)}});

            // detyped arrays
            std::vector<array> children = {array(std::move(flat_arr)), array(std::move(flat_arr2)), array(std::move(flat_arr3))};

            struct_array arr(std::move(children));

            // check the size
            REQUIRE_EQ(arr.size(), 4);
            
            // check the children
            REQUIRE_EQ(arr[0].value().size(), 3);
            REQUIRE_EQ(arr[1].value().size(), 3);
            REQUIRE_EQ(arr[2].value().size(), 3);
            REQUIRE_EQ(arr[3].value().size(), 3);

            // check the values
            CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], std::int16_t(0));
            CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[1], float(4.0f));
            CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[2], std::int32_t(8));

            CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[0], std::int16_t(1));
            CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[1], float(5.0f));
            CHECK_NULLABLE_VARIANT_EQ(arr[1].value()[2], std::int32_t(9));
            
            CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[0], std::int16_t(2));
            CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[1], float(6.0f));
            CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[2], std::int32_t(10));


        };


        TEST_CASE_TEMPLATE("struct[T, uint8]",T,  std::uint8_t, std::int32_t, float, double)
        {
            using inner_scalar_type = T;
            //using inner_nullable_type = nullable<inner_scalar_type>;

            // number of elements in the struct array
            const std::size_t n = 4;
            const std::size_t n2 = 3;

            // create a struct array
            arrow_proxy proxy = test::make_struct_proxy<inner_scalar_type, uint8_t>(n);
            struct_array struct_arr(std::move(proxy));
            REQUIRE(struct_arr.size() == n);

            SUBCASE("copy")
            {
                struct_array struct_arr2(struct_arr);
                CHECK_EQ(struct_arr, struct_arr2);

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


                    //using const_scalar_ref = const inner_scalar_type&;
                    using nullable_inner_scalar_type = nullable<const inner_scalar_type &, bool >;
                    using nullable_uint8_t = nullable<const  std::uint8_t & , bool >;

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
                        val0_variant
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
                        val1_variant
                    );

                }
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
        }
    }


}

