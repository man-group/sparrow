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

#include "sparrow/layout/run_end_encoded_layout/run_end_encoded_array.hpp"

namespace sparrow
{

    TEST_SUITE("run_length_encoded")
    {   
        
        TEST_CASE("run_length_encoded")
        {
            using acc_type = std::uint32_t;
            using inner_value_type = std::uint64_t;


            // lets encode the following: (length: 8)
            // [1,null,null, 42, 42, 42, null, 9]

            // the encoded values are:
            // [1, null, 42, null, 9]

            // the accumulated lengths are:
            // [1,3,6,7,8]



            // number of elements in the run_length_encoded array
            const std::size_t n = 8;

            // size of the children
            const std::size_t child_length = 5;



            // n-children == 2
            std::vector<ArrowArray> children_arrays(2);
            std::vector<ArrowSchema> children_schemas(2);

            // schema for acc-values
            test::fill_schema_and_array<acc_type>(children_schemas[0], children_arrays[0], child_length, 0/*offset*/, {});
            children_schemas[0].name = "acc";

            // fill acc-values buffer
            std::vector<acc_type> acc_values{1,3,6,7,8};
            auto ptr = reinterpret_cast<acc_type*>(const_cast<void*>(children_arrays[0].buffers[1]));
            std::copy(acc_values.begin(), acc_values.end(), ptr);

            // schema for values
            test::fill_schema_and_array<inner_value_type>(children_schemas[1], children_arrays[1], child_length, 0/*offset*/, {1,3});
            children_schemas[1].name = "values";

            // fill values buffer
            std::vector<inner_value_type> values{1, 0, 42, 0, 9};
            auto ptr2 = reinterpret_cast<inner_value_type*>(const_cast<void*>(children_arrays[1].buffers[1]));
            std::copy(values.begin(), values.end(), ptr2);



            ArrowArray arr{};
            ArrowSchema schema{};
            test::fill_schema_and_array_for_run_end_encoded(
                schema,
                arr,
                children_schemas[0],
                children_arrays[0],
                children_schemas[1],
                children_arrays[1],
                n
            );
            arrow_proxy proxy(&arr, &schema);         

            run_end_encoded_array rle_array(std::move(proxy));


            // check size
            REQUIRE(rle_array.size() == n);

            std::vector<bool> expected_bitmap{1,0,0,1,1,1,0,1};
            std::vector<inner_value_type> expected_values{1,0,0, 42,42, 42,0,9};
            
            SUBCASE("operator[]"){
                //check elements
                for(std::size_t i=0; i<n; ++i){
                    REQUIRE(rle_array[i].has_value() == bool(expected_bitmap[i]));
                    if(expected_bitmap[i]){
                        array_traits::const_reference val = rle_array[i];
                        CHECK(val.has_value() == val.has_value());
                        // // visit the variant
                        std::visit([&]( auto && nullable) -> void {
                                using T = std::decay_t<decltype(nullable)>;
                                using inner_type = std::decay_t<typename T::value_type>;
                                if constexpr(std::is_same_v<inner_type, inner_value_type>){
                                    if(nullable.has_value()){
                                        CHECK(nullable.value() == expected_values[i]);
                                    }
                                    else{
                                        CHECK(false);
                                    }
                                }
                                else{
                                    CHECK(false);
                                }
                            },
                            val
                        );
                    }
                }
            }
            SUBCASE("iterator"){
                auto iter = rle_array.begin();
                //check elements
                for(std::size_t i=0; i<n; ++i){
                    REQUIRE(iter != rle_array.end());
                    CHECK(iter->has_value() == bool(expected_bitmap[i]));
                    if(iter->has_value()){
                        auto val = *iter;
                        std::visit([&]( auto && nullable) -> void {
                                using T = std::decay_t<decltype(nullable)>;
                                using inner_type = std::decay_t<typename T::value_type>;
                                if constexpr(std::is_same_v<inner_type, inner_value_type>){
                                    if(nullable.has_value()){
                                        CHECK(nullable.value() == expected_values[i]);
                                    }
                                    else{
                                        CHECK(false);
                                    }
                                }
                                else{
                                    CHECK(false);
                                }
                            },
                            val
                        );
                    }
                    ++iter;
                }
            }
            SUBCASE("consitency")
            {   
                test::generic_consistency_test(rle_array);
            }
        }
    }


}

