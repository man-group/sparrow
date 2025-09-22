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
            p_buffer = new std::uint8_t[m_block_count];
            p_buffer[0] = 38;   // 00100110
            p_buffer[1] = 85;   // 01010101
            p_buffer[2] = 53;   // 00110101
            p_buffer[3] = 7;    // 00000111
            m_null_count = 15;  // Last 3 bits of buffer[3] are unused
            p_expected_buffer = p_buffer;
        }

        ~bitmap_fixture()
        {
            delete[] p_buffer;
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
            bitmap_view b(p_buffer, m_size, 0u);
            CHECK_EQ(b.data(), p_buffer);

            const bitmap_view& b2 = b;
            CHECK_EQ(b2.data(), p_buffer);
        }

        TEST_CASE_FIXTURE(bitmap_fixture, "copy semantic")
        {
            bitmap_view b(p_buffer, m_size, 0u);
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
            bitmap_view bref(p_buffer, m_size, 0u);
            bitmap_view b(bref);

            bitmap_view b2(std::move(b));
            CHECK_EQ(b2.size(), bref.size());
            CHECK_EQ(b2.null_count(), bref.null_count());
            for (size_t i = 0; i < m_block_count; ++i)
            {
                CHECK_EQ(b2.data()[i], bref.data()[i]);
            }
        }
    }
}
