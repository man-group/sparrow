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
#include "sparrow_v01/layout/list_array.hpp"

#include "doctest/doctest.h"

#include "../test/external_array_data_creation.hpp"

namespace sparrow
{




    TEST_SUITE("list_array")
    {

        
        TEST_CASE("constructor")
        {
            // number of elements in the flatted array
            const std::size_t n_flat = 10; //1+2+3+4

            // number of elements in the list array
            const std::size_t n = 4;

            // vector of sizes
            std::vector<std::size_t> sizes = {1, 2, 3, 4};


            // first we create a flat array of integers
            ArrowArray flat_arr{};
            ArrowSchema flat_schema{};
            test::fill_schema_and_array<std::int32_t>(flat_schema, flat_arr, n_flat, 0/*offset*/, {});

            ArrowArray arr{};
            ArrowSchema schema{};
            test::fill_schema_and_array_for_list_layout(schema, arr, flat_schema, flat_arr, sizes, {}, 0);

            // make an arrow proxy
            //arrow_proxy proxy(std::move(arr), std::move(schema)); // crashes on releasing the children of arr (ie flat_arr):
                                                                  // this tries to call children[0]->release(t->children[0]);
                                                                  // but at some point t->children[0] is set to a nullptr
                                                                  // -> hence this crashes

            arrow_proxy proxy(&arr, &schema);                   // works fine

            // create a list array
            list_array list(std::move(proxy));
            REQUIRE(list.size() == n);

            SUBCASE("element-sizes")
            {
                for(std::size_t i = 0; i < n; ++i){
                    CHECK(list[i].value().size() == sizes[i]);
                }
            }   


            SUBCASE("cast flat array")
            {
                // get the flat values (offset is not applied)
                array_base * flat_values = list.raw_flat_array();

                // cast into a primitive array
                primitive_array<std::int32_t> * flat_values_int = static_cast<primitive_array<int> *>(flat_values);

                // check the size
                REQUIRE(flat_values_int->size() == n_flat);

                // check that flat values are "iota"
                for(std::int32_t i = 0; i < static_cast<std::int32_t>(n_flat); ++i){
                    CHECK((*flat_values_int)[i] == i);
                }
            }
            
        }
    }


}

