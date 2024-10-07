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


#include "sparrow/layout/dispatch.hpp"

#include "doctest/doctest.h"

#include "../test/external_array_data_creation.hpp"

namespace sparrow
{
    using test::make_arrow_proxy;
    using testing_types = std::tuple<
        null_array, 
        primitive_array<std::int8_t>,
        primitive_array<std::uint8_t>,
        primitive_array<std::int16_t>,
        primitive_array<std::uint16_t>,
        primitive_array<std::int32_t>,
        primitive_array<std::uint32_t>,
        primitive_array<std::int64_t>,
        primitive_array<std::uint64_t>,
        primitive_array<float16_t>,
        primitive_array<float32_t>,
        primitive_array<float64_t>
    >;
    
    TEST_SUITE("dispatch")
    {
        TEST_CASE_TEMPLATE_DEFINE("array_size", AR, array_size_id)
        {
            using array_type = AR;
            array_type ar(make_arrow_proxy<typename AR::inner_value_type>());

            const array_base& ar_base = ar;
            auto size = array_size(ar_base);
            CHECK_EQ(size, ar.size());
        }

        TEST_CASE_TEMPLATE_APPLY(array_size_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("array_element", AR, array_element_id)
        {
            using array_type = AR;
            array_type ar(make_arrow_proxy<typename AR::inner_value_type>());

            const array_base& ar_base = ar;
            for (std::size_t i = 0; i < ar.size(); ++i)
            {
                auto elem = array_element(ar_base, i);
                CHECK_EQ(elem.has_value(), ar[i].has_value());
                if (elem.has_value())
                {
                    CHECK_EQ(std::get<typename AR::const_reference>(elem).value(), ar[i].value());
                }
            }
        }

        TEST_CASE_TEMPLATE_APPLY(array_element_id, testing_types);
    }
}
