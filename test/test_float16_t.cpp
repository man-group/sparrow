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

#ifndef SPARROW_STD_FIXED_FLOAT_SUPPORT
#    include "sparrow/details/3rdparty/float16_t.hpp"
#    include "doctest/doctest.h"

namespace sparrow
{
    using float16_t = half_float::half;

    using testing_types = std::tuple<
        bool,
        std::int8_t,
        std::uint8_t,
        std::int16_t,
        std::uint16_t,
        std::int32_t,
        std::uint32_t,
        std::int64_t,
        std::uint64_t,
        float,
        double>;

    TEST_SUITE("float16_t")
    {
        TEST_CASE_TEMPLATE_DEFINE("", T, float16_t_id)
        {
            SUBCASE("equality")
            {
                T init = 1;
                float16_t f(1);
                CHECK(f == init);
            }

            SUBCASE("Constructor")
            {
                T init = 1;
                float16_t f(init);
                CHECK_EQ(f, init);
            }

            SUBCASE("assignment")
            {
                T init = 3;
                float16_t f(2);
                f = init;
                CHECK_EQ(f, init);
            }

            SUBCASE("comparison")
            {
                T init = 1;
                float16_t f = 2;
                CHECK(init < f);
                CHECK(init <= f);
                CHECK(f > init);
                CHECK(f >= init);
                CHECK(f != init);
            }
        }
        TEST_CASE_TEMPLATE_APPLY(float16_t_id, testing_types);
    }
}

#endif
