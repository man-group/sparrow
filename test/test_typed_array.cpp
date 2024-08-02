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

#include <compare>
#include <cstddef>
#include <cstdint>
#include <iostream>  // For Doctest
#include <sstream>
#include <string>
#include <type_traits>

#include "sparrow/array/typed_array.hpp"

#include "array_data_creation.hpp"
#include "doctest/doctest.h"

using namespace sparrow;
using sparrow::test::to_value_type;

namespace
{
    constexpr size_t array_size = 10;
    constexpr size_t array_offset = 1;
    const std::vector<size_t> false_bitmap = {9};
}

namespace
{
    // typed_array traits

    using testing_array = typed_array<double>;

    static_assert(std::is_same_v<testing_array::value_type, array_value_type_t<testing_array>>);
    static_assert(std::is_same_v<testing_array::reference, array_reference_t<testing_array>>);
    static_assert(std::is_same_v<testing_array::const_reference, array_const_reference_t<testing_array>>);
    static_assert(std::is_same_v<testing_array::size_type, array_size_type_t<testing_array>>);
    static_assert(std::is_same_v<testing_array::iterator, array_iterator_t<testing_array>>);
    static_assert(std::is_same_v<testing_array::const_iterator, array_const_iterator_t<testing_array>>);
    static_assert(std::is_same_v<testing_array::const_bitmap_range, array_const_bitmap_range_t<testing_array>>);
    static_assert(std::is_same_v<testing_array::const_value_range, array_const_value_range_t<testing_array>>);
}

TEST_SUITE("typed_array")
{
    TEST_CASE("default constructor for variable_size_binary_layout")
    {
        using Layout = variable_size_binary_layout<std::string, const std::string_view>;
        const typed_array<std::string, Layout> ta_for_vsbl;
        CHECK_EQ(ta_for_vsbl.size(), 0);
    }

    TEST_CASE("default constructor for dictionary_encoded_layout")
    {
        using SubLayout = variable_size_binary_layout<std::string, const std::string_view>;
        using Layout = dictionary_encoded_layout<std::uint32_t, SubLayout>;
        typed_array<uint32_t, Layout> ta_for_dels;
        CHECK_EQ(ta_for_dels.size(), 0);
    }

    /*TEST_CASE("constructor from range of nullable fixed_size_layout"){
        using nt = nullable<int32_t>;
        SUBCASE("some missing")
        {
            std::vector<nt> data = { nt{0}, nullval, nt{2}, nullval};
            typed_array<int32_t> ta{data};

            CHECK_EQ(ta.size(), 4);

            // 0
            CHECK(ta[0].has_value());
            CHECK_EQ(ta[0].value(), 0);

            // 1
            CHECK_FALSE(ta[1].has_value());
            // 2
            CHECK(ta[2].has_value());
            CHECK_EQ(ta[2].value(), 2);
            // 3
            CHECK_FALSE(ta[3].has_value());
        }
        SUBCASE("all missing")
        {
            std::vector<nt> data = { nullval, nullval, nullval, nullval};
            typed_array<int32_t> ta{data};

            CHECK_EQ(ta.size(), 4);

            // 0
            CHECK_FALSE(ta[0].has_value());
            // 1
            CHECK_FALSE(ta[1].has_value());
            // 2
            CHECK_FALSE(ta[2].has_value());
            // 3
            CHECK_FALSE(ta[3].has_value());
        }
        SUBCASE("non missing")
        {
            std::vector<nt> data = { nt{0}, nt{1}, nt{2}, nt{3}};
            typed_array<int32_t> ta{data};

            CHECK_EQ(ta.size(), 4);

            // 0
            CHECK(ta[0].has_value());
            CHECK_EQ(ta[0].value(), 0);

            // 1
            CHECK(ta[1].has_value());
            CHECK_EQ(ta[1].value(), 1);

            // 2
            CHECK(ta[2].has_value());
            CHECK_EQ(ta[2].value(), 2);

            // 3
            CHECK(ta[3].has_value());
            CHECK_EQ(ta[3].value(), 3);
        }
    }
    TEST_CASE("constructor from range of nullable variable_size_binary_layout"){
        using nt = nullable<std::string>;
        SUBCASE("some missing")
        {
            std::vector<nt> data = { nt{"aa"}, nullval, nt{"bbb"}, nullval};
            typed_array<std::string> ta{data};

            CHECK_EQ(ta.size(), 4);
            // 0
            CHECK(ta[0].has_value());
            CHECK_EQ(ta[0].value(), "aa");
            // 1
            CHECK_FALSE(ta[1].has_value());
            // 2
            CHECK(ta[2].has_value());
            CHECK_EQ(ta[2].value(), "bbb");
            // 3
            CHECK_FALSE(ta[3].has_value());
        }
        SUBCASE("all missing")
        {
            std::vector<nt> data = { nullval, nullval, nullval, nullval};
            typed_array<std::string> ta{data};

            CHECK_EQ(ta.size(), 4);
            // 0
            CHECK_FALSE(ta[0].has_value());
            // 1
            CHECK_FALSE(ta[1].has_value());
            // 2
            CHECK_FALSE(ta[2].has_value());
            // 3
            CHECK_FALSE(ta[3].has_value());
        }
        SUBCASE("non missing"){
            std::vector<nt> data = { nt{"aa"}, nt{"bbb"}, nt{"c"}, nt{"dddd"}};
            typed_array<std::string> ta{data};

            CHECK_EQ(ta.size(), 4);
            // 0
            CHECK(ta[0].has_value());
            CHECK_EQ(ta[0].value(), "aa");
            // 1
            CHECK(ta[1].has_value());
            CHECK_EQ(ta[1].value(), "bbb");
            // 2
            CHECK(ta[2].has_value());
            CHECK_EQ(ta[2].value(), "c");
            // 3
            CHECK(ta[3].has_value());
            CHECK_EQ(ta[3].value(), "dddd");
        }
    }*/

    TEST_CASE_TEMPLATE_DEFINE("all", T, all)
    {
        SUBCASE("default constructor for fixed_size_layout")
        {
            using Layout = fixed_size_layout<int32_t>;
            const typed_array<int32_t, Layout> ta_for_fsl;
            CHECK_EQ(ta_for_fsl.size(), 0);
        }

        SUBCASE("constructor with parameter")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset);
            const typed_array<T> ta{array_data};
            CHECK_EQ(ta.size(), array_size - array_offset);
        }

        SUBCASE("copy constructor")
        {
            auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset);
            typed_array<T> ta1{array_data};
            typed_array<T> ta2(ta1);

            CHECK_EQ(ta1, ta2);
        }

        SUBCASE("move constructor")
        {
            auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset);
            typed_array<T> ta1{array_data};
            typed_array<T> ta2(ta1);
            typed_array<T> ta3(std::move(ta2));

            CHECK_EQ(ta1, ta3);
        }
        /*SUBCASE("construct from range")
        {
            auto data = test::iota_vector<T>(10);
            typed_array<T> ta{data};

            for (size_t i = 0; i < ta.size(); ++i)
            {
                CHECK_EQ(ta[i].value(), data[i]);
            }
        }*/
        SUBCASE("copy assignment")
        {
            auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset);
            typed_array<T> ta1{array_data};

            auto array_data2 = sparrow::test::make_test_array_data<T>(array_size + 8u, array_offset);
            typed_array<T> ta2{array_data2};

            CHECK_NE(ta1, ta2);

            ta2 = ta1;
            CHECK_EQ(ta1, ta2);
        }

        SUBCASE("move assignment")
        {
            auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset);
            typed_array<T> ta1{array_data};
            typed_array<T> ta3{ta1};

            auto array_data2 = sparrow::test::make_test_array_data<T>(array_size + 8u, array_offset);
            typed_array<T> ta2{array_data2};

            CHECK_NE(ta1, ta2);

            ta2 = std::move(ta1);
            CHECK_EQ(ta3, ta2);
        }

        // Element access

        SUBCASE("at")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            typed_array<T> ta{array_data};
            for (typename typed_array<T>::size_type i = 0; i < ta.size() - 1; ++i)
            {
                CHECK_EQ(ta.at(i).value(), to_value_type<T>(i + array_offset));
            }
            CHECK_FALSE(ta.at(false_bitmap[0] - array_offset).has_value());

            CHECK_THROWS_AS(ta.at(ta.size()), std::out_of_range);
        }

        SUBCASE("const at")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            const typed_array<T> ta{array_data};
            for (typename typed_array<T>::size_type i = 0; i < ta.size() - 1; ++i)
            {
                CHECK_EQ(ta.at(i).value(), to_value_type<T>(i + array_offset));
            }
            CHECK_FALSE(ta.at(false_bitmap[0] - array_offset).has_value());

            CHECK_THROWS_AS(ta.at(ta.size()), std::out_of_range);
        }

        SUBCASE("operator[]")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            typed_array<T> ta{array_data};
            for (typename typed_array<T>::size_type i = 0; i < ta.size() - 1; ++i)
            {
                CHECK_EQ(ta[i].value(), to_value_type<T>(i + 1));
            }
            CHECK_FALSE(ta[ta.size() - 1].has_value());
        }

        SUBCASE("const operator[]")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            const typed_array<T> ta{array_data};
            for (typename typed_array<T>::size_type i = 0; i < ta.size() - 1; ++i)
            {
                CHECK_EQ(ta[i].value(), to_value_type<T>(i + array_offset));
            }
            CHECK_FALSE(ta[false_bitmap[0] - array_offset].has_value());
        }

        SUBCASE("front")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            typed_array<T> ta{array_data};
            CHECK_EQ(ta.front().value(), to_value_type<T>(1));
        }

        SUBCASE("const front")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            const typed_array<T> ta{array_data};
            CHECK_EQ(ta.front().value(), to_value_type<T>(1));
        }

        SUBCASE("back")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            typed_array<T> ta{array_data};
            CHECK_FALSE(ta.back().has_value());
        }

        SUBCASE("const back")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            const typed_array<T> ta{array_data};
            CHECK_FALSE(ta.back().has_value());
        }

        // Iterators

        SUBCASE("const iterators")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            const typed_array<T> ta{array_data};

            auto iter = ta.cbegin();
            CHECK(std::is_const_v<typename std::remove_reference_t<decltype(iter->value())>>);
            auto iter_bis = ta.begin();
            CHECK(std::is_const_v<typename std::remove_reference_t<decltype(iter_bis->value())>>);
            CHECK_EQ(iter, iter_bis);

            const auto end = ta.cend();
            CHECK(std::is_const_v<typename std::remove_reference_t<decltype(end->value())>>);
            const auto end_bis = ta.end();
            CHECK(std::is_const_v<typename std::remove_reference_t<decltype(end_bis->value())>>);
            CHECK_EQ(end, end_bis);

            for (typename typed_array<T>::size_type i = 0; i < ta.size() - 1; ++iter, ++i)
            {
                REQUIRE(iter->has_value());
                CHECK_EQ(*iter, make_nullable(ta[i].value()));
            }

            CHECK_EQ(++iter, end);

            const auto array_data_empty = sparrow::test::make_test_array_data<T>(0, 0);
            const typed_array<T> typed_array_empty(array_data_empty);
            CHECK_EQ(typed_array_empty.cbegin(), typed_array_empty.cend());
        }

        SUBCASE("bitmap")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            const typed_array<T> ta{array_data};
            const auto bitmap = ta.bitmap();
            REQUIRE_EQ(bitmap.size(), array_size - array_offset);
            for (int32_t i = 0; i < static_cast<int32_t>(bitmap.size()) - 1; ++i)
            {
                CHECK(bitmap[i]);
            }
            CHECK_FALSE(bitmap[8]);
        }

        SUBCASE("values")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            const typed_array<T> ta{array_data};
            const auto values = ta.values();
            CHECK_EQ(values.size(), array_size - array_offset);
            for (int32_t i = 0; i < static_cast<int32_t>(values.size()); ++i)
            {
                CHECK_EQ(values[i], to_value_type<T>(i + 1));
            }
        }

        // Capacity

        SUBCASE("empty")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            const typed_array<T> ta{array_data};
            CHECK_FALSE(ta.empty());

            const auto array_data_empty = sparrow::test::make_test_array_data<T>(0, 0);
            const typed_array<T> typed_array_empty(array_data_empty);
            CHECK(typed_array_empty.empty());
        }

        SUBCASE("size")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            const typed_array<T> ta{array_data};
            CHECK_EQ(ta.size(), array_size - array_offset);
        }

        // Operators

        SUBCASE("<=>")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            const typed_array<T> ta{array_data};
            CHECK_EQ(ta <=> ta, std::strong_ordering::equal);

            const auto array_data_less = sparrow::test::make_test_array_data<T>(array_size - 1, array_offset - 1, {8});
            const typed_array<T> typed_array_less(array_data_less);
            CHECK_EQ(ta <=> typed_array_less, std::strong_ordering::greater);
            CHECK_EQ(typed_array_less <=> ta, std::strong_ordering::less);
        }

        SUBCASE("==")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            const typed_array<T> ta{array_data};
            const typed_array<T> ta_same{array_data};
            CHECK(ta == ta);
            CHECK(ta == ta_same);

            const auto array_data_less = sparrow::test::make_test_array_data<T>(array_size - 1, array_offset - 1, {8});
            const ::typed_array<T> ta_less{array_data_less};
            CHECK_FALSE(ta == ta_less);
            CHECK_FALSE(ta_less == ta);
        }

        SUBCASE("!=")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            const typed_array<T> ta{array_data};
            const typed_array<T> ta_same{array_data};
            CHECK_FALSE(ta != ta);
            CHECK_FALSE(ta != ta_same);

            const auto array_data_less = sparrow::test::make_test_array_data<T>(array_size - 1, array_offset - 1, {8});
            const typed_array<T> ta_less{array_data_less};
            CHECK(ta != ta_less);
            CHECK(ta_less != ta);
        }

        SUBCASE("<")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            const typed_array<T> ta{array_data};
            const typed_array<T> ta_same{array_data};
            CHECK_FALSE(ta < ta);
            CHECK_FALSE(ta < ta_same);

            const auto array_data_less = sparrow::test::make_test_array_data<T>(array_size - 1, array_offset - 1, {8});
            const typed_array<T> ta_less{array_data_less};
            CHECK_FALSE(ta < ta_less);
            CHECK(ta_less < ta);
        }

        SUBCASE(">")
        {
            const auto array_data = sparrow::test::make_test_array_data<T>(array_size, array_offset, false_bitmap);
            const typed_array<T> ta{array_data};
            const typed_array<T> ta_same{array_data};
            CHECK_FALSE(ta > ta);
            CHECK_FALSE(ta > ta_same);

            const auto array_data_less = sparrow::test::make_test_array_data<T>(array_size - 1, array_offset - 1, {8});
            const ::typed_array<T> ta_less{array_data_less};
            CHECK(ta > ta_less);
            CHECK_FALSE(ta_less > ta);
        }
    }

    TEST_CASE_TEMPLATE_DEFINE("all_except_string",  T, all_except_string)
    {
        // this constructor is not available for string on purpose
        // because std::string is auto mapped to a variable_size_binary_layout 
        // which defeats the purpose of this constructor
        SUBCASE("constructor_with_n_inital_value")
        {   
            // get a "one" in a generic way with iota_vector of length 2 at index 1 =)
            const auto data = test::iota_vector<T>(2);
            const auto one = data[1];

            const typed_array<T> ta{10, one};
            CHECK_EQ(ta.size(), 10);

            for (size_t i = 0; i < ta.size(); ++i)
            {
                CHECK_EQ(ta[i].value(), one);
            }
        }
    }

    TEST_CASE_TEMPLATE_INVOKE(
        all,
        bool,
        std::uint8_t,
        std::int8_t,
        std::uint16_t,
        std::int16_t,
        std::uint32_t,
        std::int32_t,
        std::uint64_t,
        std::int64_t,
        std::string,
        float16_t,
        float32_t,
        float64_t
    );

    TEST_CASE_TEMPLATE_INVOKE(
        all_except_string,
        bool,
        std::uint8_t,
        std::int8_t,
        std::uint16_t,
        std::int16_t,
        std::uint32_t,
        std::int32_t,
        std::uint64_t,
        std::int64_t,
        float16_t,
        float32_t,
        float64_t
    );

}
