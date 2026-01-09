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

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ranges>

#include "sparrow/buffer/bit_vector/bit_vector.hpp"
#include "sparrow/buffer/bit_vector/bit_vector_view.hpp"
#include "sparrow/buffer/bit_vector/non_owning_bit_vector.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    static constexpr std::size_t s_bit_vector_size = 29;
    static constexpr std::array<uint8_t, 4> s_bit_vector_blocks{{0b00100110, 0b01010101, 0b00110101, 0b00000111}};

    using testing_types = std::tuple<bit_vector<std::uint8_t>>;

    struct bit_vector_fixture
    {
        using buffer_type = std::uint8_t*;

        template <std::ranges::input_range R>
        bit_vector_fixture(const R& blocks)
            : p_buffer(std::allocator<uint8_t>().allocate(blocks.size()))
        {
            std::copy(blocks.begin(), blocks.end(), p_buffer);
        }

        bit_vector_fixture()
            : bit_vector_fixture(s_bit_vector_blocks)
        {
        }

        ~bit_vector_fixture()
        {
            if (p_buffer != nullptr)
            {
                std::allocator<uint8_t>().deallocate(p_buffer, s_bit_vector_blocks.size());
            }
        }

        buffer_type get_buffer()
        {
            buffer_type res = p_buffer;
            p_buffer = nullptr;
            return res;
        }

        buffer_type p_buffer;
    };

    struct null_buffer_fixture
    {
        using buffer_type = std::uint8_t*;

        null_buffer_fixture()
            : p_buffer(nullptr)
        {
        }

        buffer_type get_buffer()
        {
            return p_buffer;
        }

        buffer_type p_buffer;
    };

    // Removed make_bit_vector helper - use direct construction instead

    TEST_SUITE("bit_vector")
    {
        TEST_CASE_TEMPLATE_DEFINE("bit_vector owning tests", BitmapType, bit_vector_test_id)
        {
            bit_vector_fixture f;
            null_buffer_fixture null_f;

            using bitmap = BitmapType;
            using alloc_type = std::allocator<std::uint8_t>;

            SUBCASE("constructor")
            {
                SUBCASE("default")
                {
                    bitmap bm(alloc_type{});
                    CHECK_EQ(bm.size(), 0);
                    CHECK(bm.empty());
                }

                SUBCASE("from buffer")
                {
                    bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});
                    CHECK_EQ(bm.size(), s_bit_vector_size);
                    CHECK_FALSE(bm.empty());

                    // Verify data semantics: null buffer returns false
                    if (bm.data() == nullptr)
                    {
                        CHECK_EQ(bm.test(0), false);
                        CHECK_EQ(bm.test(10), false);
                    }
                }

                SUBCASE("from null buffer")
                {
                    bitmap bm(null_f.get_buffer(), s_bit_vector_size, alloc_type{});
                    CHECK_EQ(bm.size(), s_bit_vector_size);
                    CHECK_FALSE(bm.empty());
                    CHECK_EQ(bm.data(), nullptr);
                }
            }

            SUBCASE("size and empty")
            {
                bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});
                CHECK_EQ(bm.size(), s_bit_vector_size);
                CHECK_FALSE(bm.empty());

                bitmap bm_empty(alloc_type{});
                CHECK_EQ(bm_empty.size(), 0);
                CHECK(bm_empty.empty());
            }

            SUBCASE("test")
            {
                SUBCASE("from non-null buffer")
                {
                    bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});

                    // Test first byte: 0b00100110
                    CHECK_EQ(bm.test(0), false);  // bit 0
                    CHECK_EQ(bm.test(1), true);   // bit 1
                    CHECK_EQ(bm.test(2), true);   // bit 2
                    CHECK_EQ(bm.test(3), false);  // bit 3
                    CHECK_EQ(bm.test(4), false);  // bit 4
                    CHECK_EQ(bm.test(5), true);   // bit 5
                    CHECK_EQ(bm.test(6), false);  // bit 6
                    CHECK_EQ(bm.test(7), false);  // bit 7

                    // Test second byte: 0b01010101
                    CHECK_EQ(bm.test(8), true);   // bit 8
                    CHECK_EQ(bm.test(9), false);  // bit 9
                    CHECK_EQ(bm.test(10), true);  // bit 10
                }

                SUBCASE("from null buffer")
                {
                    bitmap bm(null_f.get_buffer(), s_bit_vector_size, alloc_type{});

                    // Data semantics: null buffer means all bits are false
                    CHECK_EQ(bm.test(0), false);
                    CHECK_EQ(bm.test(10), false);
                    CHECK_EQ(bm.test(s_bit_vector_size - 1), false);
                }
            }

            SUBCASE("operator[]")
            {
                SUBCASE("const access")
                {
                    const bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});

                    CHECK_EQ(bm[0], false);
                    CHECK_EQ(bm[1], true);
                    CHECK_EQ(bm[2], true);
                }

                SUBCASE("from null buffer")
                {
                    const bitmap bm(null_f.get_buffer(), s_bit_vector_size, alloc_type{});

                    CHECK_EQ(bm[0], false);
                    CHECK_EQ(bm[10], false);
                }
            }

            SUBCASE("set")
            {
                SUBCASE("from non-null buffer")
                {
                    bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});

                    // Change bit from false to true
                    CHECK_EQ(bm.test(0), false);
                    bm.set(0, true);
                    CHECK_EQ(bm.test(0), true);

                    // Change bit from true to false
                    CHECK_EQ(bm.test(1), true);
                    bm.set(1, false);
                    CHECK_EQ(bm.test(1), false);

                    // Set to same value
                    bm.set(2, true);
                    CHECK_EQ(bm.test(2), true);
                }

                SUBCASE("from null buffer")
                {
                    bitmap bm(null_f.get_buffer(), s_bit_vector_size, alloc_type{});

                    // Setting false on null buffer does nothing (stays null)
                    CHECK_EQ(bm.data(), nullptr);
                    bm.set(0, false);
                    CHECK_EQ(bm.data(), nullptr);
                    CHECK_EQ(bm.test(0), false);

                    // Setting true allocates buffer
                    bm.set(5, true);
                    CHECK_NE(bm.data(), nullptr);
                    CHECK_EQ(bm.test(5), true);
                    // Other bits should be false (data semantics)
                    CHECK_EQ(bm.test(0), false);
                    CHECK_EQ(bm.test(4), false);
                }
            }

            SUBCASE("count")
            {
                SUBCASE("from non-null buffer")
                {
                    bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});

                    // Count set bits in: 0b00100110, 0b01010101, 0b00110101, 0b00000111 (29 bits total)
                    // First byte: 3 bits set
                    // Second byte: 4 bits set
                    // Third byte: 4 bits set
                    // Fourth byte (5 bits used): 3 bits set
                    // Total: 14 bits set
                    CHECK_EQ(bm.count(), 14);
                }

                SUBCASE("from null buffer")
                {
                    bitmap bm(null_f.get_buffer(), s_bit_vector_size, alloc_type{});

                    // Data semantics: null buffer has no set bits
                    CHECK_EQ(bm.count(), 0);
                }

                SUBCASE("after modifications")
                {
                    bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});
                    auto initial_count = bm.count();

                    // Set a false bit to true
                    bm.set(0, true);  // was false
                    CHECK_EQ(bm.count(), initial_count + 1);

                    // Set a true bit to false
                    bm.set(1, false);  // was true
                    CHECK_EQ(bm.count(), initial_count);
                }
            }

            SUBCASE("at")
            {
                SUBCASE("valid access")
                {
                    const bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});

                    CHECK_EQ(bm.at(0), false);
                    CHECK_EQ(bm.at(1), true);
                    CHECK_EQ(bm.at(s_bit_vector_size - 1), false);  // bit 28 = bit 4 of byte 3
                }

                SUBCASE("out of range")
                {
                    const bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});

                    CHECK_THROWS_AS(bm.at(s_bit_vector_size), std::out_of_range);
                    CHECK_THROWS_AS(bm.at(s_bit_vector_size + 10), std::out_of_range);
                }
            }

            SUBCASE("front and back")
            {
                SUBCASE("from non-null buffer")
                {
                    bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});

                    // First bit of 0b00100110 is false
                    CHECK_EQ(bm.front(), false);

                    // Last bit (bit 28, which is bit 4 of fourth byte 0b00000111)
                    // Bit 4 in 0b00000111 is 0, so back() should be false
                    CHECK_EQ(bm.back(), false);
                }

                SUBCASE("from null buffer")
                {
                    const bitmap bm(null_f.get_buffer(), s_bit_vector_size, alloc_type{});

                    // Data semantics: null buffer returns false
                    CHECK_EQ(bm.front(), false);
                    CHECK_EQ(bm.back(), false);
                }
            }

            SUBCASE("data and buffer")
            {
                SUBCASE("non-null buffer")
                {
                    bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});

                    CHECK_NE(bm.data(), nullptr);
                    CHECK_FALSE(bm.buffer().empty());
                }

                SUBCASE("null buffer")
                {
                    bitmap bm(null_f.get_buffer(), s_bit_vector_size, alloc_type{});

                    CHECK_EQ(bm.data(), nullptr);
                }
            }

            SUBCASE("block_count")
            {
                bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});

                // 29 bits / 8 bits per byte = 3.625, so we need 4 blocks
                CHECK_EQ(bm.block_count(), 4);

                bitmap bm_empty(alloc_type{});
                CHECK_EQ(bm_empty.block_count(), 0);
            }

            SUBCASE("iterators")
            {
                SUBCASE("begin and end")
                {
                    bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});

                    auto it = bm.begin();
                    auto end_it = bm.end();

                    CHECK_NE(it, end_it);
                    CHECK_EQ(std::distance(it, end_it), s_bit_vector_size);
                }

                SUBCASE("iteration")
                {
                    bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});

                    std::size_t count = 0;
                    for (auto it = bm.begin(); it != bm.end(); ++it)
                    {
                        ++count;
                    }
                    CHECK_EQ(count, s_bit_vector_size);
                }

                SUBCASE("const iterators")
                {
                    const bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});

                    auto it = bm.cbegin();
                    auto end_it = bm.cend();

                    CHECK_EQ(std::distance(it, end_it), s_bit_vector_size);
                }
            }

            SUBCASE("resize")
            {
                SUBCASE("grow with false")
                {
                    bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});

                    bm.resize(40, false);
                    CHECK_EQ(bm.size(), 40);

                    // New bits should be false
                    for (std::size_t i = s_bit_vector_size; i < 40; ++i)
                    {
                        CHECK_EQ(bm.test(i), false);
                    }
                }

                SUBCASE("grow with true")
                {
                    bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});

                    bm.resize(40, true);
                    CHECK_EQ(bm.size(), 40);

                    // New bits should be true
                    for (std::size_t i = s_bit_vector_size; i < 40; ++i)
                    {
                        CHECK_EQ(bm.test(i), true);
                    }
                }

                SUBCASE("shrink")
                {
                    bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});

                    bm.resize(10);
                    CHECK_EQ(bm.size(), 10);
                }

                SUBCASE("from null buffer with false")
                {
                    bitmap bm(null_f.get_buffer(), s_bit_vector_size, alloc_type{});

                    bm.resize(40, false);
                    CHECK_EQ(bm.size(), 40);
                    CHECK_EQ(bm.data(), nullptr);  // Should stay null for false
                }

                SUBCASE("from null buffer with true")
                {
                    bitmap bm(null_f.get_buffer(), s_bit_vector_size, alloc_type{});

                    bm.resize(40, true);
                    CHECK_EQ(bm.size(), 40);
                    // Buffer may be allocated for true values
                }
            }

            SUBCASE("clear")
            {
                bitmap bm(f.get_buffer(), s_bit_vector_size, alloc_type{});
                CHECK_FALSE(bm.empty());

                bm.clear();
                CHECK(bm.empty());
                CHECK_EQ(bm.size(), 0);
            }

            SUBCASE("swap")
            {
                bitmap bm1(f.get_buffer(), s_bit_vector_size, alloc_type{});
                bitmap bm2(alloc_type{});

                auto bm1_size = bm1.size();
                auto bm2_size = bm2.size();

                bm1.swap(bm2);

                CHECK_EQ(bm1.size(), bm2_size);
                CHECK_EQ(bm2.size(), bm1_size);
            }
        }

        TEST_CASE_TEMPLATE_APPLY(bit_vector_test_id, testing_types);

        TEST_CASE("bit_vector_view")
        {
            bit_vector_fixture f;
            null_buffer_fixture null_f;

            using bitmap_view = bit_vector_view<std::uint8_t>;

            SUBCASE("constructor")
            {
                SUBCASE("from buffer")
                {
                    bitmap_view bm(f.p_buffer, s_bit_vector_size);
                    CHECK_EQ(bm.size(), s_bit_vector_size);
                    CHECK_FALSE(bm.empty());
                }

                SUBCASE("from null buffer")
                {
                    bitmap_view bm(null_f.p_buffer, s_bit_vector_size);
                    CHECK_EQ(bm.size(), s_bit_vector_size);
                    CHECK_FALSE(bm.empty());
                    CHECK_EQ(bm.data(), nullptr);
                }
            }

            SUBCASE("test")
            {
                SUBCASE("from non-null buffer")
                {
                    bitmap_view bm(f.p_buffer, s_bit_vector_size);

                    // Test first byte: 0b00100110
                    CHECK_EQ(bm.test(0), false);
                    CHECK_EQ(bm.test(1), true);
                    CHECK_EQ(bm.test(2), true);
                }

                SUBCASE("from null buffer")
                {
                    bitmap_view bm(null_f.p_buffer, s_bit_vector_size);

                    // Data semantics: null buffer means all bits are false
                    CHECK_EQ(bm.test(0), false);
                    CHECK_EQ(bm.test(10), false);
                }
            }

            SUBCASE("count")
            {
                bitmap_view bm(f.p_buffer, s_bit_vector_size);
                CHECK_EQ(bm.count(), 14);

                bitmap_view bm_null(null_f.p_buffer, s_bit_vector_size);
                CHECK_EQ(bm_null.count(), 0);
            }

            SUBCASE("const access")
            {
                const bitmap_view bm(f.p_buffer, s_bit_vector_size);

                CHECK_EQ(bm[0], false);
                CHECK_EQ(bm[1], true);
                CHECK_EQ(bm.test(2), true);
                CHECK_EQ(bm.front(), false);
                CHECK_EQ(bm.back(), false);  // bit 28
            }

            SUBCASE("data and buffer")
            {
                bitmap_view bm(f.p_buffer, s_bit_vector_size);
                CHECK_EQ(bm.data(), f.p_buffer);
                CHECK_FALSE(bm.buffer().empty());

                bitmap_view bm_null(null_f.p_buffer, s_bit_vector_size);
                CHECK_EQ(bm_null.data(), nullptr);
            }

            SUBCASE("iterators")
            {
                bitmap_view bm(f.p_buffer, s_bit_vector_size);

                auto it = bm.begin();
                auto end_it = bm.end();

                CHECK_EQ(std::distance(it, end_it), s_bit_vector_size);

                std::size_t count = 0;
                for (auto bit : bm)
                {
                    (void) bit;
                    ++count;
                }
                CHECK_EQ(count, s_bit_vector_size);
            }
        }

        TEST_CASE("bit semantics vs validity semantics")
        {
            null_buffer_fixture null_f;

            SUBCASE("bit_vector has data semantics")
            {
                // Data semantics: null buffer = all bits false
                bit_vector_view<std::uint8_t> bv(null_f.p_buffer, 10);

                CHECK_EQ(bv.test(0), false);
                CHECK_EQ(bv.test(5), false);
                CHECK_EQ(bv.test(9), false);
                CHECK_EQ(bv.count(), 0);
            }
        }
    }
}
