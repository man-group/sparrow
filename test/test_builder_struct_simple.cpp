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
        
        TEST_CASE("struct-layout-simple")
        {
            // struct<float, float>
            {   
                std::vector<std::tuple<float, int>> v{
                    {1.5f, 2},
                    {3.5f, 4},
                    {5.5f, 6}
                };
                sanity_check(sparrow::build(v));
            }
            // struct<float, float> (with nulls)
            {   
                std::vector<nt<std::tuple<float, int>>> v{
                    std::tuple<float, int>{1.5f, 2},
                    sparrow::nullval,
                    std::tuple<float, int>{5.5f, 6}
                };
                sanity_check(sparrow::build(v));
            }
        }
    }
}

