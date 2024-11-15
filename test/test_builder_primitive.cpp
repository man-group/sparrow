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

#include "sparrow/builder/builder.hpp"
#include "test_utils.hpp"

namespace sparrow
{

    template<class T>
    void sanity_check(T && /*t*/)
    {
    }

    // to keep everything very short for very deep nested types
    template<class T>
    using nt = nullable<T>;


    TEST_SUITE("builder")
    {
        TEST_CASE("primitive-layout")
        {
            // arr[float]
            SUBCASE("float")
            {   
                std::vector<float> v{1.0, 2.0, 3.0};
                auto arr = sparrow::build(v);
                sanity_check(arr);
                REQUIRE_EQ(arr.size(), 3);
                CHECK_EQ(arr[0].value(), 1.0);
                CHECK_EQ(arr[1].value(), 2.0);
                CHECK_EQ(arr[2].value(), 3.0);
            }
            // arr[double] (with nulls)
            SUBCASE("float-with-nulls")
            {   
                std::vector<nt<double>> v{1.0, 2.0, sparrow::nullval, 3.0};
                auto arr = sparrow::build(v);
                sanity_check(arr);
                REQUIRE_EQ(arr.size(), 4);
                REQUIRE(arr[0].has_value());
                REQUIRE(arr[1].has_value());
                REQUIRE_FALSE(arr[2].has_value());
                REQUIRE(arr[3].has_value());

                CHECK_EQ(arr[0].value(), 1.0);
                CHECK_EQ(arr[1].value(), 2.0);
                CHECK_EQ(arr[3].value(), 3.0);

            }
        }
    }
}

