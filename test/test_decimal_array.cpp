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

#include <cstdint>
#include <ranges>
#include <vector>


#include "sparrow/layout/decimal_array.hpp"
#include "test_utils.hpp"

namespace sparrow
{
    TEST_SUITE("decimal_array")
    {
        TEST_CASE("basics")
        {
            u8_buffer<std::int32_t> buffer{10,20,33,111};
            std::size_t precision = 2;
            int scale = 4;
            decimal_32_array array{std::move(buffer), precision, scale};
            CHECK_EQ(array.size(), 4);

            auto val = array[0].value();
            CHECK_EQ(val.scale(), scale);
            CHECK_EQ(val.storage(), 10);


        }
    }
} // namespace sparrow