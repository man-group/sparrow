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

#include "sparrow/array.hpp"
#include "sparrow/debug/copy_tracker.hpp"
#include "sparrow/decimal_array.hpp"

#include "test_utils.hpp"
#include "zero_copy_test_utils.hpp"

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
#ifdef SPARROW_TRACK_COPIES
                    copy_tracker::reset(copy_tracker::key<decimal_array<decimal<INTEGER_TYPE>>>());
#endif
                    u8_buffer<INTEGER_TYPE> buffer{values};
                    decimal_array<decimal<INTEGER_TYPE>> array{std::move(buffer), bitmaps, precision, scale};
#ifdef SPARROW_TRACK_COPIES
                    CHECK_EQ(copy_tracker::count(copy_tracker::key<decimal_array<decimal<INTEGER_TYPE>>>()), 0);
#endif
                    CHECK_EQ(array.size(), 4);
                    for (std::size_t i = 0; i < array.size(); ++i)
                    {
                        CHECK_EQ(array[i].has_value(), bitmaps[i]);
                    }
                }

                SUBCASE("data_buffer, precision, scale")
                {
#ifdef SPARROW_TRACK_COPIES
                    copy_tracker::reset(copy_tracker::key<decimal_array<decimal<INTEGER_TYPE>>>());
#endif
                    u8_buffer<INTEGER_TYPE> buffer{values};
                    decimal_array<decimal<INTEGER_TYPE>> array{std::move(buffer), precision, scale};
#ifdef SPARROW_TRACK_COPIES
                    CHECK_EQ(copy_tracker::count(copy_tracker::key<decimal_array<decimal<INTEGER_TYPE>>>()), 0);
#endif
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
                SUBCASE("larger")
                {
                    array.resize(6, make_nullable(decimal<INTEGER_TYPE>(42, scale)));
                    CHECK_EQ(array.size(), 6);
                    CHECK_EQ(array[4].value().storage(), 42);
                    CHECK_EQ(array[5].value().storage(), 42);
                }

                SUBCASE("smaller")
                {
                    array.resize(3, make_nullable(decimal<INTEGER_TYPE>(0, scale)));
                    REQUIRE_EQ(array.size(), 3);
                    CHECK_EQ(array[0].value().storage(), 10);
                    CHECK_EQ(array[1].value().storage(), 20);
                    CHECK_EQ(array[2].has_value(), false);
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
                SUBCASE("at beginning")
                {
                    decimal_array<decimal<INTEGER_TYPE>> array{values, bitmaps, precision, scale};
                    CHECK_EQ(array.size(), 4);

                    auto it = array.insert(array.cbegin(), make_nullable(decimal<INTEGER_TYPE>(77, scale)));
                    CHECK_EQ(array.size(), 5);
                    CHECK_EQ((*it).value().storage(), 77);
                    CHECK_EQ(array[0].value().storage(), 77);
                }

                SUBCASE("insert in middle")
                {
                    decimal_array<decimal<INTEGER_TYPE>> array{values, bitmaps, precision, scale};
                    CHECK_EQ(array.size(), 4);

                    auto it = array.insert(array.cbegin() + 2, make_nullable(decimal<INTEGER_TYPE>(77, scale)));
                    CHECK_EQ(array.size(), 5);
                    CHECK_EQ((*it).value().storage(), 77);
                    CHECK_EQ(array[2].value().storage(), 77);
                }

                SUBCASE("at end")
                {
                    decimal_array<decimal<INTEGER_TYPE>> array{values, bitmaps, precision, scale};
                    CHECK_EQ(array.size(), 4);

                    array.insert(array.cend(), make_nullable(decimal<INTEGER_TYPE>(77, scale)));
                    CHECK_EQ(array.size(), 5);
                    CHECK_EQ(array[4].value().storage(), 77);
                }
            }

            SUBCASE("erase")
            {
                SUBCASE("at the beginning")
                {
                    decimal_array<decimal<INTEGER_TYPE>> array{values, bitmaps, precision, scale};
                    CHECK_EQ(array.size(), 4);

                    auto it = array.erase(array.cbegin());
                    CHECK_EQ(array.size(), 3);
                    // After erasing index 0, iterator points to what was index 1
                    CHECK_EQ((*it).value().storage(), values[1]);
                    // Check that the last element (originally index 3) is now at index 2
                    CHECK_EQ(array[2].value().storage(), values[3]);

                    CHECK_EQ(array[0].value().storage(), values[1]);
                    CHECK_EQ(array[1].has_value(), false);
                }

                SUBCASE("in middle")
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

                    CHECK_EQ(array[0].value().storage(), values[0]);
                    CHECK_EQ(array[1].has_value(), false);
                }

                SUBCASE("at end")
                {
                    decimal_array<decimal<INTEGER_TYPE>> array{values, bitmaps, precision, scale};
                    CHECK_EQ(array.size(), 4);

                    auto it = array.erase(array.cbegin() + 3);
                    CHECK_EQ(array.size(), 3);
                    CHECK_EQ(it, array.end());
                    CHECK_EQ(array[0].value().storage(), values[0]);
                    CHECK_EQ(array[1].value().storage(), values[1]);
                    CHECK_EQ(array[2].has_value(), false);
                }
            }
        }

        TEST_CASE_TEMPLATE_APPLY(decimal_array_test_generic_id, integer_types);

        TEST_CASE("copy and move")
        {
            using integer_type = std::int64_t;
            using decimal_type = decimal<integer_type>;

            const std::vector<integer_type> values{
                integer_type(10),
                integer_type(20),
                integer_type(33),
                integer_type(111)
            };
            const std::vector<bool> bitmaps{true, true, false, true};
            constexpr std::size_t precision = 2;
            constexpr int scale = 4;

            decimal_array<decimal_type> arr{values, bitmaps, precision, scale};

            SUBCASE("copy")
            {
#ifdef SPARROW_TRACK_COPIES
                copy_tracker::reset(copy_tracker::key<decimal_array<decimal_type>>());
                copy_tracker::reset(copy_tracker::key<buffer<uint8_t>>());
#endif
                decimal_array<decimal_type> arr2(arr);
                CHECK_EQ(arr, arr2);
#ifdef SPARROW_TRACK_COPIES
                CHECK_EQ(copy_tracker::count(copy_tracker::key<decimal_array<decimal_type>>()), 1);
                CHECK_EQ(copy_tracker::count(copy_tracker::key<buffer<uint8_t>>()), 0);
#endif

                decimal_array<decimal_type> arr3{std::vector<integer_type>{5, 10}, precision, scale};
                CHECK_NE(arr, arr3);
#ifdef SPARROW_TRACK_COPIES
                copy_tracker::reset(copy_tracker::key<decimal_array<decimal_type>>());
                copy_tracker::reset(copy_tracker::key<buffer<uint8_t>>());
#endif
                arr3 = arr;
                CHECK_EQ(arr, arr3);
#ifdef SPARROW_TRACK_COPIES
                CHECK_EQ(copy_tracker::count(copy_tracker::key<decimal_array<decimal_type>>()), 1);
                CHECK_EQ(copy_tracker::count(copy_tracker::key<buffer<uint8_t>>()), 0);
#endif
            }

            SUBCASE("move")
            {
                decimal_array<decimal_type> arr2(arr);
#ifdef SPARROW_TRACK_COPIES
                copy_tracker::reset(copy_tracker::key<decimal_array<decimal_type>>());
                copy_tracker::reset(copy_tracker::key<buffer<uint8_t>>());
#endif
                decimal_array<decimal_type> arr3(std::move(arr));
                CHECK_EQ(arr2, arr3);
#ifdef SPARROW_TRACK_COPIES
                CHECK_EQ(copy_tracker::count(copy_tracker::key<decimal_array<decimal_type>>()), 0);
                CHECK_EQ(copy_tracker::count(copy_tracker::key<buffer<uint8_t>>()), 0);
#endif

                decimal_array<decimal_type> arr4{std::vector<integer_type>{5, 10}, precision, scale};
                CHECK_NE(arr2, arr4);
#ifdef SPARROW_TRACK_COPIES
                copy_tracker::reset(copy_tracker::key<decimal_array<decimal_type>>());
                copy_tracker::reset(copy_tracker::key<buffer<uint8_t>>());
#endif
                arr4 = std::move(arr2);
                CHECK_EQ(arr3, arr4);
#ifdef SPARROW_TRACK_COPIES
                CHECK_EQ(copy_tracker::count(copy_tracker::key<decimal_array<decimal_type>>()), 0);
                CHECK_EQ(copy_tracker::count(copy_tracker::key<buffer<uint8_t>>()), 0);
#endif
            }
        }

#ifndef SPARROW_USE_LARGE_INT_PLACEHOLDERS

        TEST_CASE("zero copy with std allocator")
        {
#    ifdef __GNUC__
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wcast-align"
#    endif
            using decimal_type = decimal<int128_t>;
            using storage_type = int128_t;
            constexpr std::size_t precision = 10;
            constexpr int scale = 2;
            size_t num_rows = 10;
            auto allocator = std::allocator<uint8_t>{};
            auto [typed_ptr, data_buffer] = sparrow::test::make_zero_copy_data_buffer<storage_type>(
                num_rows,
                allocator
            );

            decimal_array<decimal_type> arr(
                std::move(data_buffer),
                sparrow::validity_bitmap(nullptr, num_rows, allocator),
                precision,
                scale
            );
            sparrow::array array{std::move(arr)};

            auto arrow_structures = sparrow::get_arrow_structures(array);
            auto arrow_array_buffers = sparrow::get_arrow_array_buffers(
                *arrow_structures.first,
                *arrow_structures.second
            );
            const auto* roundtripped_ptr = reinterpret_cast<const storage_type*>(
                arrow_array_buffers.at(1).data<uint8_t>()
            );

            CHECK_EQ(roundtripped_ptr, typed_ptr);
#    ifdef __GNUC__
#        pragma GCC diagnostic pop
#    endif
        }

        TEST_CASE("zero copy with default allocator")
        {
#    ifdef __GNUC__
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wcast-align"
#    endif
            using decimal_type = decimal<int128_t>;
            using storage_type = int128_t;
            constexpr std::size_t precision = 10;
            constexpr int scale = 2;
            size_t num_rows = 10;
            using SparrowAllocator = sparrow::buffer<std::uint8_t>::default_allocator;
            auto allocator = SparrowAllocator{};
            auto [typed_ptr, data_buffer] = sparrow::test::make_zero_copy_data_buffer<storage_type>(
                num_rows,
                allocator
            );

            decimal_array<decimal_type> arr(
                std::move(data_buffer),
                sparrow::validity_bitmap(nullptr, num_rows, allocator),
                precision,
                scale
            );
            sparrow::array array{std::move(arr)};

            auto arrow_structures = sparrow::get_arrow_structures(array);
            auto arrow_array_buffers = sparrow::get_arrow_array_buffers(
                *arrow_structures.first,
                *arrow_structures.second
            );
            const auto* roundtripped_ptr = reinterpret_cast<const storage_type*>(
                arrow_array_buffers.at(1).data<uint8_t>()
            );

            CHECK_EQ(roundtripped_ptr, typed_ptr);
#    ifdef __GNUC__
#        pragma GCC diagnostic pop
#    endif
        }

        TEST_CASE("zero copy bitmap with std allocator")
        {
#    ifdef __GNUC__
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wcast-align"
#    endif
            using storage_type = sparrow::int128_t;
            size_t num_rows{10};
            auto data_allocator = std::allocator<uint8_t>{};
            auto bitmap_allocator = std::allocator<uint8_t>{};

            auto [typed_ptr, data_buffer] = sparrow::test::make_zero_copy_data_buffer<storage_type>(
                num_rows,
                data_allocator
            );
            auto [original_bitmap_ptr, validity_bitmap] = sparrow::test::make_zero_copy_validity_bitmap(
                num_rows,
                bitmap_allocator
            );

            sparrow::decimal_array<sparrow::decimal<storage_type>>
            arr(std::move(data_buffer), std::move(validity_bitmap), std::size_t{38}, int{0});

            const auto& proxy = sparrow::detail::array_access::get_arrow_proxy(arr);
            const ArrowArray& c_array = proxy.array();

            CHECK_EQ(static_cast<const uint8_t*>(c_array.buffers[0]), original_bitmap_ptr);
#    ifdef __GNUC__
#        pragma GCC diagnostic pop
#    endif
        }

        TEST_CASE("zero copy bitmap with default allocator")
        {
#    ifdef __GNUC__
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wcast-align"
#    endif
            using storage_type = sparrow::int128_t;
            size_t num_rows{10};
            using SparrowAllocator = sparrow::buffer<std::uint8_t>::default_allocator;
            auto allocator = SparrowAllocator{};

            auto [typed_ptr, data_buffer] = sparrow::test::make_zero_copy_data_buffer<storage_type>(
                num_rows,
                allocator
            );
            auto [original_bitmap_ptr, validity_bitmap] = sparrow::test::make_zero_copy_validity_bitmap(
                num_rows,
                allocator
            );

            sparrow::decimal_array<sparrow::decimal<storage_type>>
            arr(std::move(data_buffer), std::move(validity_bitmap), std::size_t{38}, int{0});

            const auto& proxy = sparrow::detail::array_access::get_arrow_proxy(arr);
            const ArrowArray& c_array = proxy.array();

            CHECK_EQ(static_cast<const uint8_t*>(c_array.buffers[0]), original_bitmap_ptr);
#    ifdef __GNUC__
#        pragma GCC diagnostic pop
#    endif
        }

#endif
    }
}  // namespace sparrow
