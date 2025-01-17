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

#include "sparrow/layout/fixed_width_binary_layout/fixed_width_binary_array_utils.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    TEST_SUITE("fixed_width_binary_array_utils")
    {
        TEST_CASE("num_bytes_for_fixed_sized_binary")
        {
            CHECK_EQ(num_bytes_for_fixed_sized_binary("w:42"), 42);
            CHECK_EQ(num_bytes_for_fixed_sized_binary("w:1"), 1);
            CHECK_THROWS(num_bytes_for_fixed_sized_binary("w:-56"));
            CHECK_THROWS(num_bytes_for_fixed_sized_binary("w:323.56"));
            CHECK_THROWS(num_bytes_for_fixed_sized_binary("w:dqsd"));
            CHECK_THROWS(num_bytes_for_fixed_sized_binary("u:45"));
        }
    }
}
