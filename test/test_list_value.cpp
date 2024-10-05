// Man Group Operations Limited
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

#include "sparrow_v01/layout/primitive_array.hpp"
#include "sparrow_v01/layout/list_layout/list_value.hpp"
#include "sparrow_v01/layout/struct_layout/struct_value.hpp"

#include "doctest/doctest.h"

#include "../test/external_array_data_creation.hpp"

namespace sparrow
{
    using scalar_value_type = std::int32_t;
    using test::make_arrow_proxy;

    TEST_SUITE("value_list")
    {
        TEST_CASE("size")
        {
            primitive_array<scalar_value_type> ar(make_arrow_proxy<scalar_value_type>());
            list_value2 l(&ar, 2u, 7u);

            CHECK_EQ(l.size(), 5u);
        }

        TEST_CASE("operator[]")
        {
            std::size_t begin = 2u;
            std::size_t end = 7u;
            primitive_array<scalar_value_type> ar(make_arrow_proxy<scalar_value_type>());
            list_value2 l(&ar, begin, end);
            for (std::size_t i = begin; i < end; ++i)
            {
                CHECK_EQ(l[i].has_value(), ar[begin+i].has_value());
                if (ar[begin+i].has_value())
                {
                    CHECK_EQ(std::get<primitive_array<scalar_value_type>::const_reference>(l[i]).value(), ar[begin+i].value());
                }
            }
        }
    }
}
