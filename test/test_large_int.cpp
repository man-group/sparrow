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

#include "sparrow/utils/large_int.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
#if defined(__cpp_lib_format)
    TEST_SUITE("large_int")
    {
#    ifndef SPARROW_USE_LARGE_INT_PLACEHOLDERS

        TEST_CASE("int128 formatter")
        {
            int128_t n = 123456789;
            n *= 10000000000000;
            std::string str = std::format("{}", n);
            CHECK_EQ(str, "1234567890000000000000");
        }

        TEST_CASE("int256 formatter")
        {
            int256_t n = 123456789;
            n *= 1000000000000000000;
            n *= 1000000000000000000;
            std::string str = std::format("{}", n);
            CHECK_EQ(str, "123456789000000000000000000000000000000000000");
        }
#    else
        TEST_CASE("int128 formatter")
        {
            int128_t n;
            n.words[0] = 123456789;
            n.words[1] = 100000000;
            std::string str = std::format("{}", n);
            CHECK_EQ(str, "int128_t(123456789, 100000000)");
        }

        TEST_CASE("int256 formatter")
        {
            int256_t n;
            n.words[0] = 123456789;
            n.words[1] = 100000000;
            n.words[2] = 200000000;
            n.words[3] = 300000000;
            std::string str = std::format("{}", n);
            CHECK_EQ(str, "int256_t(123456789, 100000000, 200000000, 300000000)");
        }
#    endif
    }
#endif
}
