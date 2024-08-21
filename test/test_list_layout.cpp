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
#include <array>
#include <cstdint>
#include <iostream>  // For doctest
#include <numeric>
#include <string_view>

#include "sparrow/array/array_data_factory.hpp"
#include "sparrow/layout/list_layout/list_layout.hpp"
#include "sparrow/layout/fixed_size_layout.hpp"
#include "doctest/doctest.h"

#include "array_data_creation.hpp"
#include "layout_tester.hpp"

namespace sparrow
{
   
TEST_SUITE("list_layout")
{
    // this test will be invoked for each **scalar** type.
    // Since this test uses  a "fixed_size_layout" as the inner layout, 
    // it will **not** work for non-scalar types (ie std::string)
    TEST_CASE_TEMPLATE_DEFINE("generic-scalar-test", T, generic_scalar_test)
    {
        SUBCASE("list<T>")
        { 

            auto d = test::iota_vector<T>(11);
            std::vector<std::vector<T>> values = {
                {d[0],d[1], d[2], d[3]}, 
                {d[4], d[5]}, 
                {d[6], d[7], d[8], d[9], d[10]}
            };

            auto list_array_data = test::make_array_data_for_list_of_scalars(values);

            // auto flat_values = flatten(values);
            using data_storage = sparrow::array_data;
            using inner_layout_type = sparrow::fixed_size_layout<T, data_storage>;
            using list_layout_type = sparrow::list_layout<inner_layout_type, data_storage,  std::int64_t>;
            list_layout_type list_layout(list_array_data);


            REQUIRE_EQ(list_layout.size(), values.size());
            for(std::size_t i = 0; i < values.size(); i++)
            {
                auto maybe_list = list_layout[i];
                CHECK_EQ(maybe_list.has_value(), true);
                auto list = maybe_list.value();
                CHECK_EQ(list.size(), values[i].size());
                for(std::size_t j = 0; j < values[i].size(); j++)
                {
                    auto maybe_value = list[j];
                    CHECK_EQ(maybe_value.has_value(), true);
                    CHECK_EQ(maybe_value.value(), values[i][j]);
                }
            }
    
        }
        SUBCASE("list<list<T>>")
        {
            auto d = test::iota_vector<T>(28);
            std::vector<std::vector<std::vector<T>>> values = {
                {
                    {d[0],d[1], d[2], d[3]}, 
                    {d[4], d[5], d[6]},
                    {d[7], d[8], d[9], d[10]}
                },
                {
                    {d[11],d[12], d[13], d[14]}, 
                    {d[15], d[16]}, 
                    {d[17], d[18], d[19], d[20], d[21]}
                },
                {
                    {d[22],d[23], d[24], d[25]}, 
                    {d[26], d[27]}
                }
            };

            auto outer_list_array_data = test::make_array_data_for_list_of_list_of_scalars(values);
        
            using data_storage = sparrow::array_data;
            using inner_layout_type = sparrow::fixed_size_layout<T, data_storage>;
            using inner_list_layout_type = sparrow::list_layout<inner_layout_type, data_storage,  std::int64_t>;
            using outer_list_layout_type = sparrow::list_layout<inner_list_layout_type, data_storage,  std::int64_t>;
            
            outer_list_layout_type outer_list_layout(outer_list_array_data);
            REQUIRE_EQ(outer_list_layout.size(), values.size());
            
            SUBCASE("operator[]")
            {
                for(std::size_t i = 0; i < values.size(); i++)
                {
                    auto maybe_list = outer_list_layout[i];
                    CHECK_EQ(maybe_list.has_value(), true);
                    auto list = maybe_list.value();
                    REQUIRE_EQ(list.size(), values[i].size());
                    for(std::size_t j = 0; j < values[i].size(); j++)
                    {
                        auto maybe_inner_list = list[j];
                        CHECK_EQ(maybe_inner_list.has_value(), true);
                        auto inner_list = maybe_inner_list.value();
                        CHECK_EQ(inner_list.size(), values[i][j].size());
                        for(std::size_t k = 0; k < values[i][j].size(); k++)
                        {
                            auto maybe_value = inner_list[k];
                            CHECK_EQ(maybe_value.has_value(), true);
                            CHECK_EQ(maybe_value.value(), values[i][j][k]);
                        }
                    }
                }
            }
            SUBCASE("consistency")
            {
                test::layout_tester(outer_list_layout);
            }
        }
    }

    TEST_CASE_TEMPLATE_INVOKE(
        generic_scalar_test,
        //bool,
        std::uint8_t//,
        // std::int8_t,
        // std::uint16_t,
        // std::int16_t,
        // std::uint32_t,
        // std::int32_t,
        // std::uint64_t,
        // std::int64_t,
        // float16_t,
        // float32_t,
        // float64_t
    );


} // test suite end
} // namespace sparrow
