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

#include "sparrow/dynamic_bitset.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    struct bitmap_fixture
    {
        bitmap_fixture()
        {
            p_buffer = new std::uint8_t[m_block_count];
            p_buffer[0] = 38;   // 00100110
            p_buffer[1] = 85;   // 01010101
            p_buffer[2] = 53;   // 00110101
            p_buffer[3] = 7;    // 00000111
            m_null_count = 15;  // Last 3 bits of buffer[3] are unused
        }

        std::uint8_t* p_buffer;
        std::size_t m_block_count = 4;
        std::size_t m_size = 29;
        std::size_t m_null_count;
    };

    TEST_SUITE("dynamic_bitset")
    {
        using bitmap = dynamic_bitset<std::uint8_t>;

        TEST_CASE("constructor")
        {
            bitmap b1;
            CHECK_EQ(b1.size(), 0u);
            CHECK_EQ(b1.null_count(), 0u);

            const std::size_t expected_size = 13;
            bitmap b2(expected_size);
            CHECK_EQ(b2.size(), expected_size);
            CHECK_EQ(b2.null_count(), expected_size);

            bitmap b3(expected_size, true);
            CHECK_EQ(b3.size(), expected_size);
            CHECK_EQ(b3.null_count(), 0u);

            bitmap_fixture bf;
            bitmap b4(bf.p_buffer, bf.m_size);
            CHECK_EQ(b4.size(), bf.m_size);
            CHECK_EQ(b4.null_count(), bf.m_null_count);

            bitmap_fixture bf2;
            bitmap b5(bf2.p_buffer, bf2.m_size, bf2.m_null_count);
            CHECK_EQ(b5.size(), bf.m_size);
            CHECK_EQ(b5.null_count(), bf.m_null_count);
        }

        TEST_CASE_FIXTURE(bitmap_fixture, "data")
        {
            bitmap b(p_buffer, m_size);
            CHECK_EQ(b.data(), p_buffer);

            const bitmap& b2 = b;
            CHECK_EQ(b2.data(), p_buffer);
        }

        TEST_CASE_FIXTURE(bitmap_fixture, "copy semantic")
        {
            bitmap b(p_buffer, m_size);
            bitmap b2(b);

            CHECK_EQ(b.size(), b2.size());
            CHECK_EQ(b.null_count(), b2.null_count());
            CHECK_NE(b.data(), b2.data());
            for (size_t i = 0; i < m_block_count; ++i)
            {
                CHECK_EQ(b.data()[i], b2.data()[i]);
            }

            const std::size_t expected_block_count = 2;
            std::uint8_t* buf = new std::uint8_t[expected_block_count];
            buf[0] = 37;
            buf[1] = 2;
            bitmap b3(buf, expected_block_count * 8);

            b2 = b3;
            CHECK_EQ(b2.size(), b3.size());
            CHECK_EQ(b2.null_count(), b3.null_count());
            CHECK_NE(b2.data(), b3.data());
            for (size_t i = 0; i < expected_block_count; ++i)
            {
                CHECK_EQ(b2.data()[i], b3.data()[i]);
            }
        }

        TEST_CASE_FIXTURE(bitmap_fixture, "move semantic")
        {
            bitmap bref(p_buffer, m_size);
            bitmap b(bref);

            bitmap b2(std::move(b));
            CHECK_EQ(b2.size(), bref.size());
            CHECK_EQ(b2.null_count(), bref.null_count());
            for (size_t i = 0; i < m_block_count; ++i)
            {
                CHECK_EQ(b2.data()[i], bref.data()[i]);
            }

            const std::size_t expected_block_count = 2;
            std::uint8_t* buf = new std::uint8_t[expected_block_count];
            buf[0] = 37;
            buf[1] = 2;
            bitmap b4(buf, expected_block_count * 8);
            bitmap b5(b4);

            b2 = std::move(b4);
            CHECK_EQ(b2.size(), b5.size());
            CHECK_EQ(b2.null_count(), b5.null_count());
            for (size_t i = 0; i < expected_block_count; ++i)
            {
                CHECK_EQ(b2.data()[i], b5.data()[i]);
            }
        }

        TEST_CASE_FIXTURE(bitmap_fixture, "test/set")
        {
            bitmap bm(p_buffer, m_size);
            bool b1 = bm.test(2);
            CHECK(b1);
            bool b2 = bm.test(3);
            CHECK(!b2);
            bool b3 = bm.test(24);
            CHECK(b3);

            bm.set(3, true);
            CHECK_EQ(bm.data()[0], 46);
            CHECK_EQ(bm.null_count(), m_null_count - 1);

            bm.set(24, 0);
            CHECK_EQ(bm.data()[3], 6);
            CHECK_EQ(bm.null_count(), m_null_count);

            // Ensures that setting false again does not alter the null count
            bm.set(24, 0);
            CHECK_EQ(bm.data()[3], 6);
            CHECK_EQ(bm.null_count(), m_null_count);

            bm.set(2, true);
            CHECK(bm.test(2));
            CHECK_EQ(bm.null_count(), m_null_count);
        }

        TEST_CASE_FIXTURE(bitmap_fixture, "operator[]")
        {
            bitmap bm(p_buffer, m_size);
            const bitmap& cbm = bm;
            bool b1 = cbm[2];
            CHECK(b1);
            bool b2 = cbm[3];
            CHECK(!b2);
            bool b3 = cbm[24];
            CHECK(b3);

            bm.set(3, true);
            CHECK_EQ(bm.data()[0], 46);
            CHECK_EQ(bm.null_count(), m_null_count - 1);

            bm[24] = false;
            CHECK_EQ(cbm.data()[3], 6);
            CHECK_EQ(cbm.null_count(), m_null_count);

            // Ensures that setting false again does not alter the null count
            bm[24] = false;
            CHECK_EQ(bm.data()[3], 6);
            CHECK_EQ(bm.null_count(), m_null_count);

            bm[2] = true;
            CHECK(bm.test(2));
            CHECK_EQ(bm.null_count(), m_null_count);
        }

        TEST_CASE_FIXTURE(bitmap_fixture, "resize")
        {
            bitmap bref(p_buffer, m_size);
            bitmap b(bref);
            b.resize(33);
            CHECK_EQ(b.size(), 33);
            CHECK_EQ(b.null_count(), m_null_count + 4);

            // Test shrinkage
            b.resize(29);
            CHECK_EQ(b.size(), 29);
            CHECK_EQ(b.null_count(), m_null_count);
        }

        TEST_CASE_FIXTURE(bitmap_fixture, "iterator")
        {
            // Does not work because the reference is not bool&
            // static_assert(std::random_access_iterator<typename bitmap::iterator>);
            static_assert(std::random_access_iterator<typename bitmap::const_iterator>);

            bitmap b(p_buffer, m_size);
            auto iter = b.begin();
            auto citer = b.cbegin();

            ++iter;
            ++citer;
            CHECK(*iter);
            CHECK(*citer);

            iter += 14;
            citer += 14;

            CHECK(!*iter);
            CHECK(!*citer);

            auto diff = iter - b.begin();
            auto cdiff = citer - b.cbegin();

            CHECK_EQ(diff, 15);
            CHECK_EQ(cdiff, 15);

            iter -= 12;
            citer -= 12;
            diff = iter - b.begin();
            cdiff = citer - b.cbegin();
            CHECK_EQ(diff, 3);
            CHECK_EQ(cdiff, 3);

            iter += 3;
            citer += 3;
            diff = iter - b.begin();
            cdiff = citer - b.cbegin();
            CHECK_EQ(diff, 6);
            CHECK_EQ(cdiff, 6);

            iter -= 4;
            citer -= 4;
            diff = iter - b.begin();
            cdiff = citer - b.cbegin();
            CHECK_EQ(diff, 2);
            CHECK_EQ(cdiff, 2);

            auto iter_end = b.begin() + b.size();
            auto citer_end = b.cbegin() + b.size();
            CHECK_EQ(iter_end, b.end());
            CHECK_EQ(citer_end, b.cend());
        };

        TEST_CASE_FIXTURE(bitmap_fixture, "bitset_reference")
        {
            // as a reminder: p_buffer[0] = 38; // 00100110
            bitmap b(p_buffer, m_size);
            auto iter = b.begin();
            *iter = true;
            CHECK_EQ(b.null_count(), m_null_count - 1);

            ++iter;
            *iter &= false;
            CHECK_EQ(b.null_count(), m_null_count);

            iter += 2;
            *iter |= true;
            CHECK_EQ(b.null_count(), m_null_count - 1);

            ++iter;
            *iter ^= true;
            CHECK_EQ(b.null_count(), m_null_count - 2);

            CHECK_EQ(*iter, *iter);
            CHECK_NE(*iter, *++b.begin());

            CHECK_EQ(*iter, true);
            CHECK_EQ(true, *iter);

            CHECK_NE(*iter, false);
            CHECK_NE(false, *iter);
        }
    }
}
