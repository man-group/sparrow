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
#include "sparrow/layout/variable_size_binary_view_array.hpp"
#include "test_utils.hpp"

#include <vector>
#include <string>

using namespace std::literals;

namespace sparrow
{
    TEST_SUITE("variable_size_binary_view_array")
    {
        TEST_CASE("string_view_array")
        {
            std::vector<std::string> words{
                "short",
                "longer",
                "abcdefghijk",   // exactly 11
                "abcdefghijkl",   // exactly 12
                "123456789101112", // longer than 12,
                "hello world this is a long string"
            };

            std::vector<std::size_t> where_nulls{1};

            string_view_array array(words, where_nulls);


            for(std::size_t i = 0; i < words.size(); ++i)
            {
                if(i == 1)
                {
                    CHECK_FALSE(array[i].has_value());
                }
                else
                {
                    CHECK(array[i].has_value());
                    CHECK(array[i].value() == words[i]);
                }
            }
        }
    }
}
