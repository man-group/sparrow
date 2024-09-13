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

#include <array>
#include <cstdint>
#include <iostream>  // For doctest
#include <string_view>

#include "sparrow/array/data_traits.hpp"
#include "sparrow/array/data_type.hpp"
#include "sparrow/arrow_array_schema_proxy.hpp"

#include "doctest/doctest.h"
#include "sparrow_v01/arrow_interface/arrow_array_schema_factory.hpp"
#include "sparrow_v01/layout/dictionary_encoded_array.hpp"
#include "sparrow_v01/layout/variable_size_binary_array.hpp"

namespace sparrow
{
    using keys_type = uint32_t;
    using sub_layout_type = variable_size_binary_array<std::string, std::string_view>;
    using layout_type = dictionary_encoded_array<keys_type, sub_layout_type>;
    static const std::array<std::string, 7> words{{"hello", "you", "are", "not", "prepared", "!", "?"}};

    arrow_proxy make_arrow_proxy()
    {
        constexpr std::array<size_t, 2> keys_nulls{1ULL, 5ULL};
        const std::vector<keys_type> keys{0, 0, 1, 2, 3, 4, 2, 5, 0, 1, 2};
        constexpr int64_t keys_offset = 1;

        constexpr std::array<size_t, 1> value_nulls{2ULL};
        constexpr int64_t values_offset = 1;

        // Indexes: 0(null), 1, 2, 3, 4(null), 2, 5, 0, 1, 2

        //// Values: you, are(null), not, prepared, !, ?

        // null, null, not, prepared, null, not, ?, you, are(null), not

        constexpr data_type keys_data_type = sparrow::arrow_traits<keys_type>::type_id;
        constexpr data_type values_data_type = sparrow::arrow_traits<std::string>::type_id;

        arrow_proxy ar{
            make_dictionary_encoded_arrow_array(keys, keys_nulls, keys_offset, words, value_nulls, values_offset),
            make_dictionary_encoded_arrow_schema(values_data_type, keys_data_type)
        };

        return ar;
    }

    TEST_SUITE("dictionary_encoded_array")
    {
        TEST_CASE("constructors")
        {
            CHECK_NOTHROW(layout_type{make_arrow_proxy()});
        }

        TEST_CASE("size")
        {
            const layout_type dict(make_arrow_proxy());
            CHECK_EQ(dict.size(), 10);
        }

        TEST_CASE("operator[]")
        {
            const layout_type dict(make_arrow_proxy());
            CHECK_FALSE(dict[0].has_value());
            CHECK_FALSE(dict[1].has_value());
            REQUIRE(dict[2].has_value());
            CHECK_EQ(dict[2].value(), words[3]);
            REQUIRE(dict[3].has_value());
            CHECK_EQ(dict[3].value(), words[4]);
            CHECK_FALSE(dict[4].has_value());
            REQUIRE(dict[5].has_value());
            CHECK_EQ(dict[5].value(), words[3]);
            REQUIRE(dict[6].has_value());
            CHECK_EQ(dict[6].value(), words[6]);
            REQUIRE(dict[7].has_value());
            CHECK_EQ(dict[7].value(), words[1]);
            CHECK_FALSE(dict[8].has_value());
            REQUIRE(dict[9].has_value());
            CHECK_EQ(dict[9].value(), words[3]);
        }

        TEST_CASE("const_iterator")
        {
            const layout_type dict(make_arrow_proxy());
            auto iter = dict.cbegin();
            CHECK_EQ(*iter, dict[0]);
            ++iter;
            CHECK_EQ(*iter, dict[1]);
            ++iter;
            CHECK_EQ(*iter, dict[2]);
            ++iter;
            --iter;
            CHECK_EQ(*iter, dict[2]);
            ++iter;
            CHECK_EQ(*iter, dict[3]);
            iter ++;
            CHECK_EQ(*iter, dict[4]);
            ++iter;
            CHECK_EQ(*iter, dict[5]);
            ++iter;
            CHECK_EQ(*iter, dict[6]);
            ++iter;
            CHECK_EQ(*iter, dict[7]);
            iter += 3;
            CHECK_EQ(iter, dict.cend());
        }

        TEST_CASE("const_value_iterator")
        {
            const layout_type dict(make_arrow_proxy());
            const auto vrange = dict.values();
            auto iter = vrange.begin();
            CHECK_EQ(*iter, "");
            iter++;
            CHECK_EQ(*iter, "");
            iter++;
            CHECK_EQ(*iter, dict[2].value());
            ++iter;
            CHECK_EQ(*iter, dict[3].value());
            iter++;
            CHECK_EQ(*iter, "");
            iter++;
            CHECK_EQ(*iter, dict[5].value());
            ++iter;
            CHECK_EQ(*iter, dict[6].value());
            ++iter;
            CHECK_EQ(*iter, dict[7].value());
            ++iter;
            CHECK_EQ(*iter, "");
            ++iter;
            CHECK_NE(*iter, words[2]);
            ++iter;
            CHECK_EQ(iter, vrange.end());
        }

        TEST_CASE("const_bitmap_iterator")
        {
            const layout_type dict(make_arrow_proxy());
            const auto brange = dict.bitmap();
            auto iter = brange.begin();
            CHECK_FALSE(*iter);
            ++iter;
            CHECK_FALSE(*iter);
            ++iter;
            CHECK(*iter);
            ++iter;
            CHECK(*iter);
            ++iter;
            CHECK_FALSE(*iter);
            ++iter;
            CHECK(*iter);
            ++iter;
            CHECK(*iter);
            ++iter;
            CHECK(*iter);
            ++iter;
            CHECK_FALSE(*iter);
            ++iter;
            CHECK(*iter);
            ++iter;
            CHECK_EQ(iter, brange.end());
        }
    }
}
