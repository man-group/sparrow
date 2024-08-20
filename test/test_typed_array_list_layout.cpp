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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or mplied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <compare>
#include <cstddef>
#include <cstdint>
#include <iostream>  // For Doctest
#include <sstream>
#include <string>
#include <type_traits>

#include "sparrow/array/typed_array.hpp"
#include "sparrow/layout/list_layout/list_layout.hpp"
#include "array_data_creation.hpp"
#include "doctest/doctest.h"

using namespace sparrow;


TEST_SUITE("typed_array")
{
   TEST_CASE("list-layout")
   {
        std::vector<std::vector<int>> values = {{0, 1, 2, 3}, {4, 5}, {6, 7, 8, 9, 10}};
        auto list_array_data = test::make_array_data_for_list_of_scalars(values);

        using data_storage = sparrow::array_data;
        using inner_layout_type = sparrow::fixed_size_layout<int, data_storage>;
        using list_layout_type = sparrow::list_layout<inner_layout_type, data_storage,  std::int64_t>;
        using typed_array_type = typed_array<list_value_t<list_layout_type, true>, list_layout_type>;

        typed_array_type array{list_array_data};

        REQUIRE(array.size() == 3);
        for(std::size_t i = 0; i < array.size(); ++i)
        {   
            CHECK(array[i].has_value());
            auto value = array[i].value();
            for(std::size_t j = 0; j <value.size(); ++j)
            {
                CHECK(value[j].has_value());
                CHECK(value[j].value() == values[i][j]);
            }
        }

        

   }
}
