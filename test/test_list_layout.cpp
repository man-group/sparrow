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


template<class C>
sparrow::array_data::buffer_type build_offsets(C && values)
{
    std::vector<std::int64_t> offsets(values.size() + 1, 0);
    for(std::size_t i = 0; i < values.size(); i++)
    {
        offsets[i+1] = offsets[i] + static_cast<std::int64_t>(values[i].size());
    }

    sparrow::array_data::buffer_type offset_buffer(sizeof(std::int64_t) * (values.size() + 1), 0);
    auto offset_buffer_ptr = offset_buffer.data<std::int64_t>();
    std::copy(offsets.begin(), offsets.end(), offset_buffer_ptr);
    return offset_buffer;
}

    



TEST_SUITE("list_layout")
{
    TEST_CASE("list<int>"){
        std::vector<std::vector<int>> values = {{0, 1, 2, 3}, {4, 5}, {6, 7, 8, 9, 10}};
        auto flat_values = flatten(values);
        using data_storage = sparrow::array_data;
        using inner_layout_type = sparrow::fixed_size_layout<int, data_storage>;
        auto values_array_data = sparrow::make_default_array_data<inner_layout_type>(flat_values);
        auto list_array_data = sparrow::array_data{};
        auto offset_buffer = build_offsets(values);
        list_array_data.buffers.push_back(std::move(offset_buffer));
        list_array_data.child_data.push_back(std::move(values_array_data));
        sparrow::dynamic_bitset<uint8_t> bitmap(values.size(), true);
        list_array_data.bitmap = std::move(bitmap);
        list_array_data.length = static_cast<typename array_data::length_type>(values.size());
        using list_layout_type = sparrow::list_layout<inner_layout_type, data_storage,  std::int64_t>;
        list_layout_type list_layout(list_array_data);


        CHECK_EQ(list_layout.size(), values.size());

        auto l0 = list_layout[0];
        CHECK_EQ(l0.has_value(), true);
        auto v0 = l0.value();
        CHECK_EQ(v0.size(), values[0].size());
        CHECK_EQ(v0[0].value(), 0);
        CHECK_EQ(v0[1].value(), 1);
        CHECK_EQ(v0[2].value(), 2);
        CHECK_EQ(v0[3].value(), 3);


        auto l1 = list_layout[1];
        CHECK_EQ(l1.has_value(), true);
        auto v1 = l1.value();
        CHECK_EQ(v1.size(), values[1].size());
        CHECK_EQ(v1[0].value(), 4);
        CHECK_EQ(v1[1].value(), 5);

        auto l2 = list_layout[2];
        CHECK_EQ(l2.has_value(), true);
        auto v2 = l2.value();
        CHECK_EQ(v2.size(), values[2].size());
        CHECK_EQ(v2[0].value(), 6);
        CHECK_EQ(v2[1].value(), 7);
  
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

        // make "half"-flattened values
        std::vector<std::vector<int>> half_flat_values;
        for (auto& v : values) {
            for (auto& e : v) {
                half_flat_values.push_back(e);
            }
        }

        CHECK_EQ(half_flat_values.size(), 8);
        auto flat_values = flatten_tree(values);
        CHECK_EQ(flat_values.size(), 28);
        using data_storage = sparrow::array_data;
        using inner_layout_type = sparrow::fixed_size_layout<int, data_storage>;
        auto values_array_data = sparrow::make_default_array_data<inner_layout_type>(flat_values);

        auto inner_list_array_data = sparrow::array_data{};
        auto offset_buffer = build_offsets(half_flat_values);
        inner_list_array_data.buffers.push_back(std::move(offset_buffer));
        inner_list_array_data.child_data.push_back(values_array_data);
        sparrow::dynamic_bitset<uint8_t> bitmap(half_flat_values.size(), true);
        inner_list_array_data.bitmap = std::move(bitmap);
        inner_list_array_data.length = static_cast<typename array_data::length_type>(half_flat_values.size());
        using inner_list_layout_type = sparrow::list_layout<inner_layout_type, data_storage,  std::int64_t>;
        inner_list_layout_type inner_list_layout(inner_list_array_data);

        CHECK_EQ(inner_list_layout.size(), 8);

        auto l0 = inner_list_layout[0];
        CHECK_EQ(l0.has_value(), true);
        auto v0 = l0.value();
        CHECK_EQ(v0.size(), 4);
        CHECK_EQ(v0[0].value(), 0);
        CHECK_EQ(v0[1].value(), 1);
        CHECK_EQ(v0[2].value(), 2);
        CHECK_EQ(v0[3].value(), 3);

        auto l4 = inner_list_layout[4];
        CHECK_EQ(l4.has_value(), true);
        auto v4 = l4.value();
        CHECK_EQ(v4.size(), 2);
        CHECK_EQ(v4[0].value(), 15);
        CHECK_EQ(v4[1].value(), 16);


        auto outer_list_array_data = sparrow::array_data{};
        auto outer_offset_buffer = build_offsets(values);
        outer_list_array_data.buffers.push_back(std::move(outer_offset_buffer));
        outer_list_array_data.child_data.push_back(inner_list_array_data);
        sparrow::dynamic_bitset<uint8_t> outer_bitmap(values.size(), true);
        outer_list_array_data.bitmap = std::move(outer_bitmap);
        outer_list_array_data.length = static_cast<typename array_data::length_type>(values.size());

        using outer_list_layout_type = sparrow::list_layout<inner_list_layout_type, data_storage,  std::int64_t>;
        outer_list_layout_type outer_list_layout(outer_list_array_data);

        CHECK_EQ(outer_list_layout.size(), values.size());

        auto l0_outer = outer_list_layout[0];
        CHECK_EQ(l0_outer.has_value(), true);

        auto l0_value = l0_outer.value();
        CHECK_EQ(l0_value.size(), 3);

        auto l0_0 = l0_value[0];
        CHECK_EQ(l0_0.has_value(), true);
        auto l0_0_value = l0_0.value();
        CHECK_EQ(l0_0_value.size(), 4);
        CHECK_EQ(l0_0_value[0].value(), 0);
        CHECK_EQ(l0_0_value[1].value(), 1);
        CHECK_EQ(l0_0_value[2].value(), 2);
        CHECK_EQ(l0_0_value[3].value(), 3);

        auto l0_1 = l0_value[1];
        CHECK_EQ(l0_1.has_value(), true);
        auto l0_1_value = l0_1.value();
        CHECK_EQ(l0_1_value.size(), 2);
        CHECK_EQ(l0_1_value[0].value(), 4);
        CHECK_EQ(l0_1_value[1].value(), 5);

        auto l0_2 = l0_value[2];
        CHECK_EQ(l0_2.has_value(), true);
        auto l0_2_value = l0_2.value();
        CHECK_EQ(l0_2_value.size(), 5);
        CHECK_EQ(l0_2_value[0].value(), 6);
        CHECK_EQ(l0_2_value[1].value(), 7);
        CHECK_EQ(l0_2_value[2].value(), 8);
        CHECK_EQ(l0_2_value[3].value(), 9);
        CHECK_EQ(l0_2_value[4].value(), 10);


        auto l1_outer = outer_list_layout[1];
        CHECK_EQ(l1_outer.has_value(), true);
        auto l1_value = l1_outer.value();
        CHECK_EQ(l1_value.size(), 3);
        
        auto l1_0 = l1_value[0];
        CHECK_EQ(l1_0.has_value(), true);
        auto l1_0_value = l1_0.value();
        CHECK_EQ(l1_0_value.size(), 4);
        CHECK_EQ(l1_0_value[0].value(), 11);
        CHECK_EQ(l1_0_value[1].value(), 12);
        CHECK_EQ(l1_0_value[2].value(), 13);
        CHECK_EQ(l1_0_value[3].value(), 14);


        auto l1_1 = l1_value[1];
        CHECK_EQ(l1_1.has_value(), true);
        auto l1_1_value = l1_1.value();
        CHECK_EQ(l1_1_value.size(), 2);
        CHECK_EQ(l1_1_value[0].value(), 15);
        CHECK_EQ(l1_1_value[1].value(), 16);


        auto l1_2 = l1_value[2];
        CHECK_EQ(l1_2.has_value(), true);
        auto l1_2_value = l1_2.value();
        CHECK_EQ(l1_2_value.size(), 5);

        CHECK_EQ(l1_2_value[0].value(), 17);
        CHECK_EQ(l1_2_value[1].value(), 18);
        CHECK_EQ(l1_2_value[2].value(), 19);
        CHECK_EQ(l1_2_value[3].value(), 20);
        CHECK_EQ(l1_2_value[4].value(), 21);


        auto l2_outer = outer_list_layout[2];
        CHECK_EQ(l2_outer.has_value(), true);
        auto l2_value = l2_outer.value();
        CHECK_EQ(l2_value.size(), 2);
        
        auto l2_0 = l2_value[0];
        CHECK_EQ(l2_0.has_value(), true);
        auto l2_0_value = l2_0.value();
        CHECK_EQ(l2_0_value.size(), 4);
        CHECK_EQ(l2_0_value[0].value(), 22);
        CHECK_EQ(l2_0_value[1].value(), 23);
        CHECK_EQ(l2_0_value[2].value(), 24);
        CHECK_EQ(l2_0_value[3].value(), 25);

        auto l2_1 = l2_value[1];
        CHECK_EQ(l2_1.has_value(), true);
        auto l2_1_value = l2_1.value();
        CHECK_EQ(l2_1_value.size(), 2);
        CHECK_EQ(l2_1_value[0].value(), 26);
        CHECK_EQ(l2_1_value[1].value(), 27);

    }


} // test suite end
} // namespace sparrow
