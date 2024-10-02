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

#include "sparrow_v01/layout/primitive_array.hpp"
#include "sparrow_v01/layout/list_layout/list_array.hpp"

#include "doctest/doctest.h"

#include "test_utils.hpp"
#include "../test/external_array_data_creation.hpp"

namespace sparrow
{
    TEST_SUITE("list_array")
    {   
        TEST_CASE_TEMPLATE("list[T]",T, std::uint8_t, std::int32_t, float, double)
        {
            using inner_scalar_type = T;
            using inner_nullable_type = nullable<inner_scalar_type>;

            // number of elements in the flatted array
            const std::size_t n_flat = 10; //1+2+3+4
            // number of elements in the list array
            const std::size_t n = 4;
            // vector of sizes
            std::vector<std::size_t> sizes = {1, 2, 3, 4};

            // first we create a flat array of integers
            ArrowArray flat_arr{};
            ArrowSchema flat_schema{};
            test::fill_schema_and_array<inner_scalar_type>(flat_schema, flat_arr, n_flat, 0/*offset*/, {});
            flat_schema.name = "the flat array";

            ArrowArray arr{};
            ArrowSchema schema{};
            test::fill_schema_and_array_for_list_layout(schema, arr, flat_schema, flat_arr, sizes, {}, 0);
            arrow_proxy proxy(&arr, &schema);         


            // create a list array
            list_array list_arr(std::move(proxy));
            REQUIRE(list_arr.size() == n);

            SUBCASE("element-sizes")
            {
                for(std::size_t i = 0; i < n; ++i){
                    REQUIRE(list_arr[i].has_value());
                    CHECK(list_arr[i].value().size() == sizes[i]);
                }
            }   
            SUBCASE("element-values")
            {
                std::size_t flat_index = 0;
                for(std::size_t i = 0; i < n; ++i){
                    auto list = list_arr[i].value();
                    for(std::size_t j = 0; j < sizes[i]; ++j){
                       
                        auto value_variant = list[j];
                        // visit the variant
                        std::visit([&](auto && value){
                          if constexpr(std::is_same_v<std::decay_t<decltype(value)>, inner_nullable_type>){
                            CHECK(value == flat_index);
                          }
                        }, value_variant);
                        ++flat_index;
                    }
                }
            }

            SUBCASE("consitency")
            {   
                test::generic_consistency_test(list_arr);
            }

            SUBCASE("cast flat array")
            {
                // get the flat values (offset is not applied)
                array_base * flat_values = list_arr.raw_flat_array();

                // cast into a primitive array
                primitive_array<inner_scalar_type> * flat_values_casted = static_cast<primitive_array<inner_scalar_type> *>(flat_values);

                // check the size
                REQUIRE(flat_values_casted->size() == n_flat);

                // check that flat values are "iota"
                if constexpr(std::is_integral_v<inner_scalar_type>)
                {
                    for(inner_scalar_type i = 0; i < static_cast<inner_scalar_type>(n_flat); ++i){
                        CHECK((*flat_values_casted)[static_cast<std::uint64_t>(i)].value() == i);
                    }
                }
            }
              
        }
    }


}

