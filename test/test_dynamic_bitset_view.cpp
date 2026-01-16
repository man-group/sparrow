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
#include <iostream>

#include "sparrow/buffer/dynamic_bitset/dynamic_bitset_view.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    struct bitmap_fixture
    {
        using buffer_type = std::uint8_t*;

        bitmap_fixture()
        {
            p_buffer = std::allocator<uint8_t>().allocate(m_block_count);
            p_buffer[0] = 38;   // 00100110
            p_buffer[1] = 85;   // 01010101
            p_buffer[2] = 53;   // 00110101
            p_buffer[3] = 7;    // 00000111
            m_null_count = 15;  // Last 3 bits of buffer[3] are unused
            p_expected_buffer = p_buffer;
        }

        ~bitmap_fixture()
        {
            std::allocator<uint8_t>().deallocate(p_buffer, m_block_count);
            p_expected_buffer = nullptr;
        }

        buffer_type release_buffer()
        {
            buffer_type res = p_buffer;
            p_buffer = nullptr;
            return res;
        }

        bitmap_fixture(const bitmap_fixture&) = delete;
        bitmap_fixture(bitmap_fixture&&) = delete;

        bitmap_fixture& operator=(const bitmap_fixture&) = delete;
        bitmap_fixture& operator=(bitmap_fixture&&) = delete;

        buffer_type p_buffer;
        buffer_type p_expected_buffer;
        static constexpr std::size_t m_block_count = 4;
        static constexpr std::size_t m_size = 29;
        std::size_t m_null_count;
    };

    TEST_SUITE("dynamic_bitset_view")
    {
        using bitmap_view = dynamic_bitset_view<const std::uint8_t>;

        TEST_CASE_FIXTURE(bitmap_fixture, "constructor")
        {
            bitmap_view b(p_buffer, m_size);
            CHECK_EQ(b.data(), p_buffer);

            const bitmap_view& b2 = b;
            CHECK_EQ(b2.data(), p_buffer);
        }

        TEST_CASE_FIXTURE(bitmap_fixture, "copy semantic")
        {
            bitmap_view b(p_buffer, m_size);
            bitmap_view b2(b);

            CHECK_EQ(b.size(), b2.size());
            CHECK_EQ(b.null_count(), b2.null_count());
            CHECK_EQ(b.data(), b2.data());
            for (size_t i = 0; i < m_block_count; ++i)
            {
                CHECK_EQ(b.data()[i], b2.data()[i]);
            }
        }

        TEST_CASE_FIXTURE(bitmap_fixture, "move semantic")
        {
            bitmap_view bref(p_buffer, m_size);
            bitmap_view b(bref);

            bitmap_view b2(std::move(b));
            CHECK_EQ(b2.size(), bref.size());
            CHECK_EQ(b2.null_count(), bref.null_count());
            for (size_t i = 0; i < m_block_count; ++i)
            {
                CHECK_EQ(b2.data()[i], bref.data()[i]);
            }
        }

        TEST_CASE_FIXTURE(bitmap_fixture, "slice_view")
        {
            bitmap_view b(p_buffer, m_size);
            
            SUBCASE("slice with both arguments")
            {
                // Slice bits [5, 15) - should get 10 bits starting at position 5
                auto slice = b.slice_view(5, 10);
                CHECK_EQ(slice.size(), 10);
                CHECK_EQ(slice.data(), b.data());  // Same underlying storage
                
                // Verify the values
                for (size_t i = 0; i < 10; ++i)
                {
                    CHECK_EQ(slice.test(i), b.test(5 + i));
                }
            }
            
            SUBCASE("slice with start only")
            {
                // Slice from position 10 to end
                auto slice = b.slice_view(10);
                CHECK_EQ(slice.size(), m_size - 10);
                CHECK_EQ(slice.data(), b.data());  // Same underlying storage
                
                // Verify the values
                for (size_t i = 0; i < slice.size(); ++i)
                {
                    CHECK_EQ(slice.test(i), b.test(10 + i));
                }
            }
            
            SUBCASE("slice at start")
            {
                auto slice = b.slice_view(0, 10);
                CHECK_EQ(slice.size(), 10);
                for (size_t i = 0; i < 10; ++i)
                {
                    CHECK_EQ(slice.test(i), b.test(i));
                }
            }
            
            SUBCASE("slice of full range")
            {
                auto slice = b.slice_view(0, m_size);
                CHECK_EQ(slice.size(), m_size);
                for (size_t i = 0; i < m_size; ++i)
                {
                    CHECK_EQ(slice.test(i), b.test(i));
                }
            }
            
            SUBCASE("out of range throws")
            {
                CHECK_THROWS_AS(b.slice_view(m_size + 1, 1), std::out_of_range);
                CHECK_THROWS_AS(b.slice_view(10, m_size), std::out_of_range);
                CHECK_THROWS_AS(b.slice_view(m_size + 1), std::out_of_range);
            }
        }
    }
}
