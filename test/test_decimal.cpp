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

#include "doctest/doctest.h"

#include <sparrow/utils/large_int.hpp>
#include <sparrow/utils/decimal.hpp>



namespace sparrow
{

    using testing_types = std::tuple<
        int32_t
        ,int64_t
        #ifndef SPARROW_DISABLE_LARGE_INTEGER_DECIMALS
        ,int128_t
        ,int256_t
        #endif
    >;


    TEST_SUITE("decimals")
    {
        TEST_CASE_TEMPLATE_DEFINE("decimals", INTEGER_TYPE, decimal_test_id)
        {
            using integer_type = INTEGER_TYPE;
            using decimal_type = decimal<integer_type>;

            // empty
            SUBCASE("empty")
            {
                decimal_type d;
                auto storage = d.storage();
                CHECK_EQ(static_cast<int32_t>(storage), 0);
                
                // float
                auto as_float =  static_cast<float>(d);
                CHECK_EQ(as_float, doctest::Approx(0.0f));

                // double
                auto as_double =  static_cast<double>(d);
                CHECK_EQ(as_double, doctest::Approx(0.0));

                // string
                auto as_string =  std::string(d);
                CHECK_EQ(as_string, "0");
            }

            SUBCASE("scale=0")
            {
                decimal_type d(42, 0);
                auto storage = d.storage();
                CHECK_EQ(static_cast<int32_t>(storage), 42);
                
                // float
                auto as_float =  static_cast<float>(d);
                CHECK_EQ(as_float, doctest::Approx(42.0f));

                // double
                auto as_double =  static_cast<double>(d);
                CHECK_EQ(as_double, doctest::Approx(42.0));

                // string
                auto as_string =  std::string(d);
                CHECK_EQ(as_string, "42");
            }

            SUBCASE("scale=1")
            {
                SUBCASE("pos")
                {
                    decimal_type d(42, 1);
                    auto storage = d.storage();
                    CHECK_EQ(static_cast<int32_t>(storage), 42);
                    
                    // float
                    auto as_float =  static_cast<float>(d);
                    CHECK_EQ(as_float, doctest::Approx(4.2f));

                    // double
                    auto as_double =  static_cast<double>(d);
                    CHECK_EQ(as_double, doctest::Approx(4.20));

                    // string
                    auto as_string =  std::string(d);
                    CHECK_EQ(as_string, "4.2");
                }
                SUBCASE("neg")
                {
                    decimal_type d(-42, 1);
                    auto storage = d.storage();
                    CHECK_EQ(static_cast<int32_t>(storage), -42);
                    
                    // float
                    auto as_float =  static_cast<float>(d);
                    CHECK_EQ(as_float, doctest::Approx(-4.2f));

                    // double
                    auto as_double =  static_cast<double>(d);
                    CHECK_EQ(as_double, doctest::Approx(-4.20));

                    // string
                    auto as_string =  std::string(d);
                    CHECK_EQ(as_string, "-4.2");
                }
            }
            SUBCASE("scale=-1")
            {
                SUBCASE("pos")
                {
                    decimal_type d(42, -1);
                    auto storage = d.storage();
                    CHECK_EQ(static_cast<int32_t>(storage), 42);
                    
                    // float
                    auto as_float =  static_cast<float>(d);
                    CHECK_EQ(as_float, doctest::Approx(420.0f));

                    // double
                    auto as_double =  static_cast<double>(d);
                    CHECK_EQ(as_double, doctest::Approx(420.0));

                    // string
                    auto as_string =  std::string(d);
                    CHECK_EQ(as_string, "420");
                }
                SUBCASE("neg")
                {
                    decimal_type d(-42, -1);
                    auto storage = d.storage();
                    CHECK_EQ(static_cast<int32_t>(storage), -42);
                    
                    // float
                    auto as_float =  static_cast<float>(d);
                    CHECK_EQ(as_float, doctest::Approx(-420.0f));

                    // double
                    auto as_double =  static_cast<double>(d);
                    CHECK_EQ(as_double, doctest::Approx(-420.0f));

                    // string
                    auto as_string =  std::string(d);
                    CHECK_EQ(as_string, "-420");
                }
            }

            SUBCASE("generic")
            {
                std::vector<int> values = {-123,-122, -111, -100, -99, 10, 11, 100, 101, 110, 111, 122, 123};
                std::vector<int > scales    = {-3, -2, -1, 0, 1, 2, -4};

                // cross product
                for(auto value : values)
                {
                    for(auto scale : scales)
                    {
                        decimal_type d(value, scale);
                        auto storage = d.storage();
                        CHECK_EQ(static_cast<int32_t>(storage), value);
                        
                        // float
                        auto as_float =  static_cast<float>(d);
                        CHECK_EQ(as_float, doctest::Approx(static_cast<float>(value) / static_cast<float>(std::pow(10, scale))));

                        // double
                        auto as_double =  static_cast<double>(d);
                        CHECK_EQ(as_double, doctest::Approx(static_cast<double>(value) / static_cast<double>(std::pow(10, scale))));

                        auto as_string =  std::string(d);
                        if(value < 0)
                        {
                            // string must have a leading '-'
                            CHECK_EQ(as_string[0], '-');
                            
                        }
                        else{
                            // string must not have a leading '-'
                            CHECK_NE(as_string[0], '-');
                        }
                        
                        // if scale is positive!, we must have a '.' in the string
                        if(scale > 0)
                        {
                            CHECK_NE(as_string.find('.'), std::string::npos);
                        }
                        else
                        {
                            CHECK_EQ(as_string.find('.'), std::string::npos);
                        }
                        
                        // if scale is negative we must have n trailing zeros
                        if(scale < 0)
                        {
                            // training zeros of the int itself
                            auto input_trailing_zeros = 0;
                            std::string input_as_string = std::to_string(value);
                            // count trailing zeros
                            for(auto i = input_as_string.size() - 1; i >= 0; i--)
                            {
                                if(input_as_string[i] == '0')
                                {
                                    input_trailing_zeros++;
                                }
                                else
                                {
                                    break;
                                }
                            }
                            
                            auto total_trailing_zeros = 0;
                            for(auto i = as_string.size() - 1; i >= 0; i--)
                            {
                                if(as_string[i] == '0')
                                {
                                    total_trailing_zeros++;
                                }
                                else
                                {
                                    break;
                                }
                            }
                            CHECK_EQ(total_trailing_zeros - input_trailing_zeros, -scale);
                        }
                    }
                }
            }
            SUBCASE("as_string")
            {
                std::vector<std::pair<decimal_type, std::string>> data = {
                    {decimal_type(0, 0), "0"},
                    {decimal_type(0, 1), "0"},
                    {decimal_type(0, 2), "0"},
                    {decimal_type(0, 3), "0"},
                    {decimal_type(0, -1), "0"},
                    {decimal_type(0, -2), "0"},
                    {decimal_type(1, 0), "1"},
                    {decimal_type(1, 1), "0.1"},
                    {decimal_type(1, 2), "0.01"},
                    {decimal_type(1, 3), "0.001"},
                    {decimal_type(1, -1), "10"},
                    {decimal_type(1, -2), "100"},
                    {decimal_type(1, -3), "1000"},
                    {decimal_type(-1, 0), "-1"},
                    {decimal_type(-1, 1), "-0.1"},
                    {decimal_type(-1, 2), "-0.01"},
                    {decimal_type(-1, 3), "-0.001"},
                    {decimal_type(-1, -1), "-10"},
                    {decimal_type(-1, -2), "-100"},
                    {decimal_type(-1, -3), "-1000"}, 
                    {decimal_type(123456789, 0), "123456789"},
                    {decimal_type(123456789, 1), "12345678.9"},
                    {decimal_type(123456789, 2), "1234567.89"},
                    {decimal_type(123456789, 3), "123456.789"},
                    {decimal_type(123456789, 4), "12345.6789"},
                    {decimal_type(123456789, 5), "1234.56789"},
                    {decimal_type(123456789, 6), "123.456789"},
                    {decimal_type(123456789, 7), "12.3456789"},
                    {decimal_type(123456789, 8), "1.23456789"},
                    {decimal_type(123456789, 9), "0.123456789"},
                    {decimal_type(123456789, 10), "0.0123456789"},
                    {decimal_type(123456789, 11), "0.00123456789"},
                    {decimal_type(123456789, 12), "0.000123456789"},
                    {decimal_type(123456789, 13), "0.0000123456789"},
                    {decimal_type(123456789, 14), "0.00000123456789"},
                    {decimal_type(123456789, 15), "0.000000123456789"},
                    {decimal_type(123456789, 16), "0.0000000123456789"},
                    {decimal_type(123456789, 17), "0.00000000123456789"},
                    {decimal_type(123456789, 18), "0.000000000123456789"},
                    {decimal_type(123456789, 19), "0.0000000000123456789"},
                    {decimal_type(123456789, 20), "0.00000000000123456789"},
                    {decimal_type(-123456789, 0), "-123456789"},
                    {decimal_type(-123456789, 1), "-12345678.9"},
                    {decimal_type(-123456789, 2), "-1234567.89"},
                    {decimal_type(-123456789, 3), "-123456.789"},
                    {decimal_type(-123456789, 4), "-12345.6789"},
                    {decimal_type(-123456789, 5), "-1234.56789"},
                    {decimal_type(-123456789, 6), "-123.456789"},
                    {decimal_type(-123456789, 7), "-12.3456789"},
                    {decimal_type(-123456789, 8), "-1.23456789"},
                    {decimal_type(-123456789, 9), "-0.123456789"},
                    {decimal_type(-123456789, 10), "-0.0123456789"},
                    {decimal_type(-123456789, 11), "-0.00123456789"},
                    {decimal_type(-123456789, 12), "-0.000123456789"},
                    {decimal_type(-123456789, 13), "-0.0000123456789"},
                    {decimal_type(-123456789, 14), "-0.00000123456789"},
                    {decimal_type(-123456789, 15), "-0.000000123456789"},
                    {decimal_type(-123456789, 16), "-0.0000000123456789"},
                    {decimal_type(-123456789, 17), "-0.00000000123456789"},
                    {decimal_type(-123456789, 18), "-0.000000000123456789"},
                    {decimal_type(-123456789, 19), "-0.0000000000123456789"},
                    {decimal_type(-123456789, 20), "-0.00000000000123456789"},
                };

                for(auto [d, expected] : data)
                {
                    auto as_string =  std::string(d);
                    CHECK_EQ(as_string, expected);
                }
            }

        }
        TEST_CASE_TEMPLATE_APPLY(decimal_test_id, testing_types);
    }
}