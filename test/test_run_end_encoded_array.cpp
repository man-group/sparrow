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

#include "sparrow/array.hpp"

namespace sparrow
{
    namespace test{
        run_end_encoded_array make_test_run_encoded_array(bool alterate = false)
        {
            using acc_type = std::uint32_t;
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
                    alterate ? inner_value_type(43): inner_value_type(42),
                    inner_value_type(),
                    inner_value_type(9)                        
                },
                std::vector<std::size_t>{1,3} // where we have no value
            );

            // accumulated lengths
            primitive_array<acc_type> acc_lengths{{acc_type(1), acc_type(3), acc_type(6), acc_type(7), acc_type(8)}};

            array acc_lengths_array(std::move(acc_lengths));
            array encoded_values_array(std::move(encoded_values));
            
            return run_end_encoded_array(std::move(acc_lengths_array), std::move(encoded_values_array));

        }
    }
    

    TEST_SUITE("run_length_encoded")
    {   
        TEST_CASE("run_length_encoded")
        {
            using inner_value_type = std::uint64_t;
            const std::size_t n = 8;

            run_end_encoded_array rle_array = test::make_test_run_encoded_array();

            // check size
            REQUIRE(rle_array.size() == n);

            std::vector<bool> expected_bitmap{1,0,0,1,1,1,0,1};
            std::vector<inner_value_type> expected_values{1,0,0, 42,42, 42,0,9};
            
            SUBCASE("copy")
            {
                run_end_encoded_array rle_array2(rle_array);
                CHECK_EQ(rle_array2, rle_array);

                run_end_encoded_array rle_array3  = test::make_test_run_encoded_array(/*alterate=*/true);
                CHECK_NE(rle_array3, rle_array);
                rle_array3 = rle_array;
                CHECK_EQ(rle_array3, rle_array);
            }

            SUBCASE("move")
            {
                run_end_encoded_array rle_array2(rle_array);
                run_end_encoded_array rle_array3(std::move(rle_array2));
                CHECK_EQ(rle_array3, rle_array);

                run_end_encoded_array rle_array4 = test::make_test_run_encoded_array(/*alterate*/true);
                CHECK_NE(rle_array4, rle_array);
                rle_array4 = std::move(rle_array3);
                CHECK_EQ(rle_array4, rle_array);
            }

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

