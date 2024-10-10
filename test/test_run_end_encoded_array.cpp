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
            using inner_value_type = std::uint8_t;


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

            auto bitmap_buffer = reinterpret_cast<std::uint8_t*>(const_cast<void*>(children_arrays[0].buffers[0]));
            bitmap_buffer[0] |= 1 << (1 % 8);
            bitmap_buffer[0] |= 1 << (3 % 8);

    


            // schema for values
            test::fill_schema_and_array<inner_value_type>(children_schemas[1], children_arrays[1], child_length, 0/*offset*/, {});
            children_schemas[1].name = "values";


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



        }
    }


}

