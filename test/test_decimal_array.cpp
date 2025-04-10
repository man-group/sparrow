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

#include "sparrow/layout/decimal_array.hpp"

#include "test_utils.hpp"

namespace sparrow
{

    using integer_types = std::tuple<
        std::int32_t,
        std::int64_t
#ifndef SPARROW_USE_LARGE_INT_PLACEHOLDERS
        ,
        int128_t,
        int256_t
#endif
        >;


    TEST_SUITE("decimal_array")
    {
        TEST_CASE_TEMPLATE_DEFINE("generic", INTEGER_TYPE, decimal_array_test_generic_id)
        {
            const std::vector<INTEGER_TYPE> values{
                INTEGER_TYPE(10),
                INTEGER_TYPE(20),
                INTEGER_TYPE(33),
                INTEGER_TYPE(111)
            };

            const std::vector<bool> bitmaps{true, true, false, true};

            constexpr std::size_t precision = 2;
            constexpr int scale = 4;

            SUBCASE("constructors")
            {
                SUBCASE("range, bitmaps, precision, scale")
                {
                    decimal_array<decimal<INTEGER_TYPE>> array{values, bitmaps, precision, scale};
                    CHECK_EQ(array.size(), 4);
                    for (std::size_t i = 0; i < array.size(); ++i)
                    {
                        CHECK_EQ(array[i].has_value(), bitmaps[i]);
                    }
                }

                SUBCASE("range, precision, scale")
                {
                    decimal_array<decimal<INTEGER_TYPE>> array{values, precision, scale};
                    CHECK_EQ(array.size(), 4);
                    for (std::size_t i = 0; i < array.size(); ++i)
                    {
                        [[maybe_unused]] auto lol = array[i];
                        CHECK(array[i].has_value());
                    }
                }

                SUBCASE("data_buffer, bitmaps, precision, scale")
                {
                    u8_buffer<INTEGER_TYPE> buffer{values};
                    decimal_array<decimal<INTEGER_TYPE>> array{std::move(buffer), bitmaps, precision, scale};
                    CHECK_EQ(array.size(), 4);
                    for (std::size_t i = 0; i < array.size(); ++i)
                    {
                        CHECK_EQ(array[i].has_value(), bitmaps[i]);
                    }
                }

                SUBCASE("data_buffer, precision, scale")
                {
                    u8_buffer<INTEGER_TYPE> buffer{values};
                    decimal_array<decimal<INTEGER_TYPE>> array{std::move(buffer), precision, scale};
                    CHECK_EQ(array.size(), 4);
                    for (std::size_t i = 0; i < array.size(); ++i)
                    {
                        CHECK(array[i].has_value());
                    }
                }

                SUBCASE("data_buffer, precision, scale, nullable")
                {
                    SUBCASE("nullable")
                    {
                        u8_buffer<INTEGER_TYPE> buffer{values};
                        decimal_array<decimal<INTEGER_TYPE>> array{std::move(buffer), precision, scale, true};
                        CHECK_EQ(array.size(), 4);
                        for (std::size_t i = 0; i < array.size(); ++i)
                        {
                            CHECK(array[i].has_value());
                        }
                    }
                    SUBCASE("not nullable")
                    {
                        u8_buffer<INTEGER_TYPE> buffer{values};
                        decimal_array<decimal<INTEGER_TYPE>> array{std::move(buffer), precision, scale, false};
                        CHECK_EQ(array.size(), 4);
                        for (std::size_t i = 0; i < array.size(); ++i)
                        {
                            CHECK(array[i].has_value());
                        }
                    }
                }
            }

            SUBCASE("full")
            {
                using integer_type = INTEGER_TYPE;
                auto buffer = u8_buffer<integer_type>{values};
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
        }

        TEST_CASE_TEMPLATE_APPLY(decimal_array_test_generic_id, integer_types);
    }
}  // namespace sparrow