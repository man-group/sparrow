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

    using integer_types = std::tuple<
        std::int32_t
        ,std::int64_t
        #ifndef SPARROW_USE_LARGE_INT_PLACEHOLDERS
        ,int128_t
        ,int256_t
        #endif
    >;


    TEST_SUITE("decimal_array")
    {
        TEST_CASE_TEMPLATE_DEFINE("generic", INTEGER_TYPE, decimal_array_test_generic_id)
        {   
            using integer_type = INTEGER_TYPE;
            u8_buffer<integer_type> buffer{
                integer_type(10),
                integer_type(20),
                integer_type(33),
                integer_type(111)
            };
            std::size_t precision = 2;
            int scale = 4;
            decimal_array<decimal<integer_type>> array{std::move(buffer), precision, scale};
            CHECK_EQ(array.size(), 4);

            auto val = array[0].value();
            CHECK_EQ(val.scale(), scale);
            CHECK_EQ(static_cast<std::int64_t>(val.storage()), 10);
            CHECK_EQ(static_cast<double>(val), doctest::Approx(0.001));
            
            val = array[1].value();
            CHECK_EQ(val.scale(), scale);
            CHECK_EQ(static_cast<std::int64_t>(val.storage()), 20);
            CHECK_EQ(static_cast<double>(val), doctest::Approx(0.002));

            val = array[2].value();
            CHECK_EQ(val.scale(), scale);
            CHECK_EQ(static_cast<std::int64_t>(val.storage()), 33);
            CHECK_EQ(static_cast<double>(val), doctest::Approx(0.0033));

        }
        TEST_CASE_TEMPLATE_APPLY(decimal_array_test_generic_id, integer_types);
    }
} // namespace sparrow