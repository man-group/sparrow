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

#include "sparrow/decimal_array.hpp"

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

            SUBCASE("operator[]")
            {
                SUBCASE("const")
                {
                    const decimal_array<decimal<INTEGER_TYPE>> array{values, bitmaps, precision, scale};
                    CHECK_EQ(array.size(), 4);
                    for (std::size_t i = 0; i < array.size(); ++i)
                    {
                        CHECK_EQ(array[i].has_value(), bitmaps[i]);
                        if (array[i].has_value())
                        {
                            const auto val = array[i].value();
                            CHECK_EQ(val.scale(), scale);
                            CHECK_EQ(val.storage(), values[i]);
                        }
                    }
                }

                SUBCASE("mutable")
                {
                    decimal_array<decimal<INTEGER_TYPE>> array{values, bitmaps, precision, scale};
                    CHECK_EQ(array.size(), 4);
                    for (std::size_t i = 0; i < array.size(); ++i)
                    {
                        CHECK_EQ(array[i].has_value(), bitmaps[i]);
                        auto ref = array[i];
                        if (ref.has_value())
                        {
                            auto new_value = ref.value().storage();
                            new_value += 1;
                            ref = make_nullable(decimal<INTEGER_TYPE>(new_value, scale));

                            const auto new_decimal = array[i].value();
                            CHECK_EQ(new_decimal.scale(), scale);
                            const auto storage = new_decimal.storage();
                            CHECK_EQ(storage, values[i] + 1);
                        }
                    }
                }
            }

            SUBCASE("modify an element with a different scale")
            {
                decimal_array<decimal<INTEGER_TYPE>> array{values, bitmaps, precision, scale};
                CHECK_EQ(array.size(), 4);
                decimal<INTEGER_TYPE> new_value(100, 2);  // Different scale
                array[0] = make_nullable(new_value);
                CHECK_EQ(array[0].has_value(), true);
                const auto val = array[0].value();
                CHECK_EQ(val.scale(), scale);
                CHECK_EQ(static_cast<std::int64_t>(val.storage()), 10000);
                CHECK_EQ(static_cast<double>(val), doctest::Approx(1.0));
            }

            SUBCASE("zero_null_values")
            {
                decimal_array<decimal<INTEGER_TYPE>> array{values, bitmaps, precision, scale};
                array.zero_null_values();
                CHECK_EQ(array.size(), 4);
                for (std::size_t i = 0; i < array.size(); ++i)
                {
                    if (!array[i].has_value())
                    {
                        CHECK_EQ(array[i].get().storage(), 0);
                    }
                }
            }

            SUBCASE("resize")
            {
                decimal_array<decimal<INTEGER_TYPE>> array{values, bitmaps, precision, scale};
                CHECK_EQ(array.size(), 4);
                
                // Resize larger
                SUBCASE("larger")
                {
                    array.resize(6, make_nullable(decimal<INTEGER_TYPE>(42, scale)));
                    CHECK_EQ(array.size(), 6);
                    CHECK_EQ(array[4].value().storage(), 42);
                    CHECK_EQ(array[5].value().storage(), 42);
                }
                
                SUBCASE("smaller")
                {
                    // Resize smaller
                    array.resize(3, make_nullable(decimal<INTEGER_TYPE>(0, scale)));
                    CHECK_EQ(array.size(), 3);
                }
            }

            SUBCASE("push_back")
            {
                decimal_array<decimal<INTEGER_TYPE>> array{values, bitmaps, precision, scale};
                CHECK_EQ(array.size(), 4);
                
                array.push_back(make_nullable(decimal<INTEGER_TYPE>(99, scale)));
                CHECK_EQ(array.size(), 5);
                CHECK_EQ(array[4].value().storage(), 99);
            }

            SUBCASE("insert")
            {
                decimal_array<decimal<INTEGER_TYPE>> array{values, bitmaps, precision, scale};
                CHECK_EQ(array.size(), 4);
                
                auto it = array.insert(array.cbegin() + 2, make_nullable(decimal<INTEGER_TYPE>(77, scale)));
                CHECK_EQ(array.size(), 5);
                CHECK_EQ((*it).value().storage(), 77);
                CHECK_EQ(array[2].value().storage(), 77);
            }

            SUBCASE("erase")
            {
                decimal_array<decimal<INTEGER_TYPE>> array{values, bitmaps, precision, scale};
                CHECK_EQ(array.size(), 4);
                
                auto it = array.erase(array.cbegin() + 1);
                CHECK_EQ(array.size(), 3);
                // After erasing index 1, iterator points to what was index 2 (which is null)
                CHECK_FALSE((*it).has_value());
                // Check that the last element (originally index 3) is now at index 2
                CHECK(array[2].has_value());
                CHECK_EQ(array[2].value().storage(), values[3]);
            }
        }

        TEST_CASE_TEMPLATE_APPLY(decimal_array_test_generic_id, integer_types);
    }
}  // namespace sparrow
