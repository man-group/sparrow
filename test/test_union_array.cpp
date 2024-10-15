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

#include "sparrow/layout/union_layout/union_array.hpp"

namespace sparrow
{

    TEST_SUITE("union")
    {   
        
        TEST_CASE("sparse_union")
        {

            const std::string format_string = "+us:3,4";
            const std::size_t n = 4;

            std::vector<ArrowArray> children_arrays(2);
            std::vector<ArrowSchema> children_schemas(2);

            test::fill_schema_and_array<float>(children_schemas[0], children_arrays[0], n, 0/*offset*/, {});
            children_schemas[0].name = "item 0";

            test::fill_schema_and_array<std::uint16_t>(children_schemas[1], children_arrays[1], n, 0/*offset*/, {});
            children_schemas[1].name = "item 1";


            ArrowArray arr{};
            ArrowSchema schema{};

            std::vector<std::uint8_t> type_ids = {std::uint8_t(3), std::uint8_t(4), std::uint8_t(3), std::uint8_t(4)};
            
            test::fill_schema_and_array_for_sparse_union(
                schema, arr, children_schemas, children_arrays, type_ids, format_string
            );

            arrow_proxy proxy(&arr, &schema);

            // create a sparse union array
            sparse_union_array sparse_union_arr(std::move(proxy));

            REQUIRE(sparse_union_arr.size() == n);

            SUBCASE("operator[]")
            {
                for (std::size_t i = 0; i < n; ++i)
                {
                    const auto& val = sparse_union_arr[i];
                    REQUIRE(val.has_value());
                }
            }
        }
    }


}

