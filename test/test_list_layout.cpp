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


namespace sparrow
{
   

// flatten a vector of vectors
template <class T>
std::vector<T> flatten(std::vector<std::vector<T>> values)
{
    std::vector<T> flat_values;
    for (auto& v : values) {
        for (auto& e : v) {
            flat_values.push_back(e);
        }
    }
    return flat_values;
}

// flatten tree nested vectors
template <class T>
std::vector<T> flatten_tree(std::vector<std::vector<std::vector<T>>> values)
{
    std::vector<T> flat_values;
    for (auto& v : values) {
        for (auto& e : v) {
            for (auto& f : e) {
                flat_values.push_back(f);
            }
        }
    }
    return flat_values;
}





TEST_SUITE("list_layout")
{
    TEST_CASE("list<int>"){ 
        std::vector<std::vector<int>> values = {{0, 1, 2, 3}, {4, 5}, {6, 7, 8, 9, 10}};

        auto list_array_data = test::make_array_data_for_list_of_scalars(values);

        // auto flat_values = flatten(values);
        using data_storage = sparrow::array_data;
        using inner_layout_type = sparrow::fixed_size_layout<int, data_storage>;
        using list_layout_type = sparrow::list_layout<inner_layout_type, data_storage,  std::int64_t>;
        list_layout_type list_layout(list_array_data);


        CHECK_EQ(list_layout.size(), values.size());
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
    TEST_CASE("list<list<int>>")
    {
        std::vector<std::vector<std::vector<int>>> values = {
            {
                {0, 1, 2, 3}, {4, 5}, {6, 7, 8, 9, 10}
            },
            {
                {11, 12, 13, 14}, {15, 16}, {17, 18, 19, 20, 21}
            },
            {   
                {22, 23, 24, 25}, 
                {26, 27}
            }
        };

        auto outer_list_array_data = test::make_array_data_for_list_of_list_of_scalars(values);
     
        using data_storage = sparrow::array_data;
        using inner_layout_type = sparrow::fixed_size_layout<int, data_storage>;
        using inner_list_layout_type = sparrow::list_layout<inner_layout_type, data_storage,  std::int64_t>;
        using outer_list_layout_type = sparrow::list_layout<inner_list_layout_type, data_storage,  std::int64_t>;
        
        SUBCASE("operator[]")
        {
            outer_list_layout_type outer_list_layout(outer_list_array_data);
            CHECK_EQ(outer_list_layout.size(), values.size());
            for(std::size_t i = 0; i < values.size(); i++)
            {
                auto maybe_list = outer_list_layout[i];
                CHECK_EQ(maybe_list.has_value(), true);
                auto list = maybe_list.value();
                CHECK_EQ(list.size(), values[i].size());
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
        SUBCASE("iterator")
        {
            outer_list_layout_type outer_list_layout(outer_list_array_data);
            using difference_type = typename outer_list_layout_type::iterator::difference_type;
            auto layout_iter  = outer_list_layout.begin();
            for(std::size_t i = 0; i < values.size(); i++)
            {
                auto maybe_list = layout_iter[static_cast<difference_type>(i)];
                CHECK_EQ(maybe_list.has_value(), true);
                auto list = maybe_list.value();
                auto iter = list.begin();
                CHECK_EQ(list.size(), values[i].size());
                for(std::size_t j = 0; j < values[i].size(); j++)
                {
                    auto maybe_inner_list = iter[static_cast<difference_type>(j)];
                    CHECK_EQ(maybe_inner_list.has_value(), true);
                    auto inner_list = maybe_inner_list.value();
                    auto inner_iter = inner_list.begin();
                    CHECK_EQ(inner_list.size(), values[i][j].size());
                    for(std::size_t k = 0; k < values[i][j].size(); k++)
                    {
                        auto maybe_value = inner_iter[static_cast<difference_type>(k)];
                        CHECK_EQ(maybe_value.has_value(), true);
                        CHECK_EQ(maybe_value.value(), values[i][j][k]);
                    }
                }
            }
        }
    }


} // test suite end
} // namespace sparrow
