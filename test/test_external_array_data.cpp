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

#include "sparrow/array/external_array_data.hpp"
#include "sparrow/layout/fixed_size_layout.hpp"
#include "sparrow/layout/null_layout.hpp"
#include "sparrow/layout/variable_size_binary_layout.hpp"

#include "external_array_data_creation.hpp"
#include "doctest/doctest.h"

namespace sparrow
{
    TEST_SUITE("external_array_data")
    {
        TEST_CASE("null_layout")
        {
            auto data = test::make_test_external_array_data<sparrow::null_type>(16);
            const null_layout<external_array_data> layout(data);

            CHECK_EQ(layout.size(), std::size_t(length(data)));
            for (std::size_t i = 0; i < 16u; ++i)
            {
                CHECK(!layout[i].has_value());
            }
        }

        TEST_CASE("fixed_size_layout")
        {
            auto data = test::make_test_external_array_data<std::int32_t, true>(16);
            const fixed_size_layout<std::int32_t, external_array_data> layout(data);

            CHECK_EQ(layout.size(), std::size_t(length(data)));
            for (std::size_t i = 0; i < 16u; ++i)
            {
                CHECK_EQ(layout[i], static_cast<std::int32_t>(i));
            }

            auto iter = layout.cbegin();
            for (std::int32_t i = 0; i < 16; ++i, ++iter)
            {
                CHECK_EQ(*iter, i);
            }
            CHECK_EQ(iter, layout.cend());
        }

        TEST_CASE("variable_size_binary_layout")
        {
            constexpr std::size_t nb_words = 16u;
            auto data = test::make_test_external_array_data<std::string, false>(nb_words);
            using layout_type = variable_size_binary_layout<std::string, const std::string_view, external_array_data>;
            const layout_type layout(data);

            auto words = test::make_testing_words(nb_words);

            CHECK_EQ(layout.size(), words.size());

            for (std::size_t i = 0; i < 16u; ++i)
            {
                CHECK_EQ(layout[i], words[i]);
            }
            
            auto iter = layout.cbegin();
            auto words_iter = words.cbegin();
            auto words_end = words.cend();
            while(words_iter != words_end)
            {
                CHECK_EQ(*words_iter++, *iter++);
            }
        }
    }
}
