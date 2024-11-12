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
#include <string_view>

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_factory.hpp"
#include "sparrow/layout/dictionary_encoded_array.hpp"
#include "sparrow/layout/variable_size_binary_array.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/types/data_type.hpp"
#include "sparrow/array.hpp"

#include "test_utils.hpp"
#include "doctest/doctest.h"

namespace sparrow
{
    using keys_type = uint32_t;
    using layout_type = dictionary_encoded_array<keys_type>;
    using layout_type_cref = layout_type::const_reference;

    static const std::array<std::string, 7> words{{"hello", "you", "are", "not", "prepared", "!", "?"}};

    inline arrow_proxy make_arrow_proxy()
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

    nullable<std::string_view> get_dict_value(layout_type_cref r)
    {
        return std::get<nullable<std::string_view>>(r);
    }

    TEST_SUITE("dictionary_encoded_array")
    {
        TEST_CASE("constructors")
        {
            CHECK_NOTHROW(layout_type{make_arrow_proxy()});
        }

        TEST_CASE("convinience_constructors")
        {   
            using key_type = std::uint32_t;
            using array_type = dictionary_encoded_array<key_type>;
            using keys_buffer_type = typename array_type::keys_buffer_type;

            // the value array
            primitive_array<float> values{0.0f, 1.0f, 2.0f, 3.0f};

            // detyped array
            array values_arr(std::move(values));

            // the keys **data**
            keys_buffer_type keys{3,3,2,1,0};

            // where nulls are 
            std::vector<std::size_t> where_null{2};

            // create the array
            auto arr = array_type(std::move(keys), std::move(values_arr), std::move(where_null));

            // check the size
            REQUIRE_EQ(arr.size(), 5);

            // check bitmap
            REQUIRE_EQ(arr[0].has_value(), true);
            REQUIRE_EQ(arr[1].has_value(), true);
            REQUIRE_EQ(arr[2].has_value(), false);
            REQUIRE_EQ(arr[3].has_value(), true);
            REQUIRE_EQ(arr[4].has_value(), true);

            // check the values
            CHECK_NULLABLE_VARIANT_EQ(arr[0], 3.0f);
            CHECK_NULLABLE_VARIANT_EQ(arr[1], 3.0f);
            CHECK_NULLABLE_VARIANT_EQ(arr[3], 1.0f);
            CHECK_NULLABLE_VARIANT_EQ(arr[4], 0.0f);
        }

        TEST_CASE("copy")
        {
            layout_type ar(make_arrow_proxy());
            layout_type ar2(ar);
            CHECK_EQ(ar, ar2);

            layout_type ar3(make_arrow_proxy());
            ar3 = ar;
            CHECK_EQ(ar, ar3);
        }

        TEST_CASE("move")
        {
            layout_type ar(make_arrow_proxy());
            layout_type ar2(ar);
            layout_type ar3(std::move(ar));
            CHECK_EQ(ar2, ar3);

            layout_type ar4(make_arrow_proxy());
            ar4 = std::move(ar3);
            CHECK_EQ(ar2, ar4);
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
            CHECK_EQ(get_dict_value(dict[2]).value(), words[3]);
            REQUIRE(dict[3].has_value());
            CHECK_EQ(get_dict_value(dict[3]).value(), words[4]);
            CHECK_FALSE(dict[4].has_value());
            REQUIRE(dict[5].has_value());
            CHECK_EQ(get_dict_value(dict[5]).value(), words[3]);
            REQUIRE(dict[6].has_value());
            CHECK_EQ(get_dict_value(dict[6]).value(), words[6]);
            REQUIRE(dict[7].has_value());
            CHECK_EQ(get_dict_value(dict[7]).value(), words[1]);
            CHECK_FALSE(dict[8].has_value());
            REQUIRE(dict[9].has_value());
            CHECK_EQ(get_dict_value(dict[9]).value(), words[3]);
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
            iter++;
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

        /*TEST_CASE("const_value_iterator")
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
        }*/
    }
}
