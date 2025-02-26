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

#include <doctest/doctest.h>

#include "sparrow/buffer/u8_buffer.hpp"

TEST_SUITE("u8_buffer")
{
    TEST_CASE("constructors")
    {
        SUBCASE("with size")
        {
            constexpr std::size_t size = 8u;
            sparrow::u8_buffer<int32_t> b(size);

            CHECK_EQ(b.size(), size);
            CHECK_NE(b.data(), nullptr);
            CHECK_EQ(b.data()[2], 0);
        }

        SUBCASE("with size and value")
        {
            constexpr std::size_t size = 8u;
            constexpr std::uint8_t value = 42u;
            sparrow::u8_buffer b(size, value);

            CHECK_EQ(b.size(), size);
            CHECK_NE(b.data(), nullptr);
            CHECK_EQ(b.data()[2], value);
        }

        SUBCASE("with range")
        {
            const std::vector<std::uint32_t> values{0, 1, 2, 3, 4, 5, 6, 7};
            sparrow::u8_buffer<std::uint32_t> b(values);

            CHECK_EQ(b.size(), values.size());
            for (std::size_t i = 0; i < values.size(); ++i)
            {
                CHECK_EQ(b.data()[i], values[i]);
            }
        }

        SUBCASE("with initializer_list")
        {
            sparrow::u8_buffer<std::uint32_t> b{0, 1, 2, 3, 4, 5, 6, 7};

            CHECK_EQ(b.size(), 8u);
            for (std::size_t i = 0; i < 8u; ++i)
            {
                CHECK_EQ(b.data()[i], i);
            }
        }

        SUBCASE("taking ownership from raw pointer and size")
        {
            constexpr std::size_t size = 8u;
            int32_t* raw_buf = new int32_t[size];
            for (std::size_t i = 0; i < size; ++i)
            {
                raw_buf[i] = static_cast<int32_t>(i);
            }
            sparrow::u8_buffer<int32_t> b(raw_buf, size);

            CHECK_EQ(b.size(), size);
            CHECK_EQ(b.data(), raw_buf);
        }

        SUBCASE("copy constructor")
        {
            const std::vector<std::uint32_t> values{0, 1, 2, 3, 4, 5, 6, 7};
            sparrow::u8_buffer<std::uint32_t> b(values);

            const sparrow::u8_buffer<std::uint32_t> b_copy(b);

            CHECK_EQ(b_copy.size(), values.size());
            for (std::size_t i = 0; i < values.size(); ++i)
            {
                CHECK_EQ(b_copy.data()[i], values[i]);
            }
        }

        SUBCASE("move constructor")
        {
            const std::vector<std::uint32_t> values{0, 1, 2, 3, 4, 5, 6, 7};
            sparrow::u8_buffer<std::uint32_t> b(values);

            const sparrow::u8_buffer<std::uint32_t> b_copy(b);
            sparrow::u8_buffer<std::uint32_t> b_move(std::move(b));

            CHECK_EQ(b_move.size(), values.size());
            for (std::size_t i = 0; i < values.size(); ++i)
            {
                CHECK_EQ(b_move.data()[i], values[i]);
            }
        }
    }

    static const std::vector<std::uint32_t> values{0, 1, 2, 3, 4, 5, 6, 7};
    static const sparrow::u8_buffer<std::uint32_t> const_sample_buffer(values);
    static sparrow::u8_buffer<std::uint32_t> sample_buffer(values);

    TEST_CASE("data")
    {
        SUBCASE("const")
        {
            const std::uint32_t* data = const_sample_buffer.data();
            for (std::size_t i = 0; i < values.size(); ++i)
            {
                CHECK_EQ(data[i], values[i]);
            }
        }

        SUBCASE("non const")
        {
            std::uint32_t* data = sample_buffer.data();
            for (std::size_t i = 0; i < values.size(); ++i)
            {
                CHECK_EQ(data[i], values[i]);
            }
        }
    }

    TEST_CASE("operator []")
    {
        SUBCASE("const")
        {
            for (std::size_t i = 0; i < values.size(); ++i)
            {
                CHECK_EQ(const_sample_buffer[i], values[i]);
            }
        }

        SUBCASE("non const")
        {
            for (std::size_t i = 0; i < values.size(); ++i)
            {
                CHECK_EQ(sample_buffer[i], values[i]);
            }
        }
    }

    TEST_CASE("front")
    {
        SUBCASE("const")
        {
            CHECK_EQ(const_sample_buffer.front(), values.front());
        }

        SUBCASE("non const")
        {
            CHECK_EQ(sample_buffer.front(), values.front());
        }
    }

    TEST_CASE("back")
    {
        SUBCASE("const")
        {
            CHECK_EQ(const_sample_buffer.back(), values.back());
        }

        SUBCASE("non const")
        {
            CHECK_EQ(sample_buffer.back(), values.back());
        }
    }

    TEST_CASE("size")
    {
        SUBCASE("const")
        {
            CHECK_EQ(const_sample_buffer.size(), values.size());
        }

        SUBCASE("non const")
        {
            CHECK_EQ(sample_buffer.size(), values.size());
        }
    }

    TEST_CASE("capacity")
    {
        CHECK_EQ(const_sample_buffer.capacity(), values.size());
    }

    TEST_CASE("reserve")
    {
        const std::size_t new_capacity = 16u;
        sample_buffer.reserve(new_capacity);

        CHECK_EQ(sample_buffer.capacity(), new_capacity);
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            CHECK_EQ(sample_buffer[i], values[i]);
        }
    }

    TEST_CASE("shrink_to_fit")
    {
        sample_buffer.reserve(16u);
        CHECK_EQ(sample_buffer.capacity(), 16u);
        sample_buffer.shrink_to_fit();

        CHECK_EQ(sample_buffer.capacity(), values.size());
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            CHECK_EQ(sample_buffer[i], values[i]);
        }
    }

    TEST_CASE("clear")
    {
        sample_buffer.clear();
        CHECK_EQ(sample_buffer.size(), 0u);
        CHECK_EQ(sample_buffer.capacity(), values.size());
    }

    TEST_CASE("insert")
    {
        SUBCASE("with pos and value")
        {
            SUBCASE("at the beginning")
            {
                sparrow::u8_buffer<std::uint32_t> b(values);
                CHECK_EQ(b.size(), 8u);
                const auto pos = b.cbegin();
                const auto iter = b.insert(pos, 42u);
                CHECK_EQ(iter, b.begin());
                REQUIRE_EQ(b.size(), 9u);
                CHECK_EQ(b[0], 42u);
                CHECK_EQ(b[1], 0u);
                CHECK_EQ(b[2], 1u);
                CHECK_EQ(b[3], 2u);
                CHECK_EQ(b[4], 3u);
                CHECK_EQ(b[5], 4u);
                CHECK_EQ(b[6], 5u);
                CHECK_EQ(b[7], 6u);
                CHECK_EQ(b[8], 7u);
            }

            SUBCASE("in the middle")
            {
                sparrow::u8_buffer<std::uint32_t> b(values);
                CHECK_EQ(b.size(), 8u);
                const auto pos = b.cbegin() + 3;
                const auto iter = b.insert(pos, 42u);
                CHECK_EQ(iter, b.begin() + 3);
                REQUIRE_EQ(b.size(), 9u);
                CHECK_EQ(b[0], 0u);
                CHECK_EQ(b[1], 1u);
                CHECK_EQ(b[2], 2u);
                CHECK_EQ(b[3], 42u);
                CHECK_EQ(b[4], 3u);
                CHECK_EQ(b[5], 4u);
                CHECK_EQ(b[6], 5u);
                CHECK_EQ(b[7], 6u);
                CHECK_EQ(b[8], 7u);
            }

            SUBCASE("at the end")
            {
                sparrow::u8_buffer<std::uint32_t> b(values);
                CHECK_EQ(b.size(), 8u);
                const auto pos = b.cend();
                const auto iter = b.insert(pos, 42u);
                CHECK_EQ(iter, b.end() - 1);
                REQUIRE_EQ(b.size(), 9u);
                CHECK_EQ(b[0], 0u);
                CHECK_EQ(b[1], 1u);
                CHECK_EQ(b[2], 2u);
                CHECK_EQ(b[3], 3u);
                CHECK_EQ(b[4], 4u);
                CHECK_EQ(b[5], 5u);
                CHECK_EQ(b[6], 6u);
                CHECK_EQ(b[7], 7u);
                CHECK_EQ(b[8], 42u);
            }
        }
    }

    TEST_CASE("emplace")
    {
        SUBCASE("at the beginning")
        {
            sparrow::u8_buffer<std::uint32_t> b(values);
            CHECK_EQ(b.size(), 8u);
            const auto pos = b.cbegin();
            const auto iter = b.emplace(pos, 42u);
            CHECK_EQ(iter, b.begin());
            REQUIRE_EQ(b.size(), 9u);
            CHECK_EQ(b[0], 42u);
            CHECK_EQ(b[1], 0u);
            CHECK_EQ(b[2], 1u);
            CHECK_EQ(b[3], 2u);
            CHECK_EQ(b[4], 3u);
            CHECK_EQ(b[5], 4u);
            CHECK_EQ(b[6], 5u);
            CHECK_EQ(b[7], 6u);
            CHECK_EQ(b[8], 7u);
        }
    }

    TEST_CASE("erase")
    {
        SUBCASE("with a position")
        {
            SUBCASE("at the beginning")
            {
                sparrow::u8_buffer<std::uint32_t> b(values);
                CHECK_EQ(b.size(), 8u);
                const auto pos = b.cbegin();
                const auto iter = b.erase(pos);
                CHECK_EQ(iter, b.begin());
                REQUIRE_EQ(b.size(), 7u);
                CHECK_EQ(b[0], 1u);
                CHECK_EQ(b[1], 2u);
                CHECK_EQ(b[2], 3u);
                CHECK_EQ(b[3], 4u);
                CHECK_EQ(b[4], 5u);
                CHECK_EQ(b[5], 6u);
                CHECK_EQ(b[6], 7u);
            }

            SUBCASE("in the middle")
            {
                sparrow::u8_buffer<std::uint32_t> b(values);
                CHECK_EQ(b.size(), 8u);
                const auto pos = b.cbegin() + 2;
                const auto iter = b.erase(pos);
                CHECK_EQ(iter, b.begin() + 2);
                REQUIRE_EQ(b.size(), 7u);
                CHECK_EQ(b[0], 0u);
                CHECK_EQ(b[1], 1u);
                CHECK_EQ(b[2], 3u);
                CHECK_EQ(b[3], 4u);
                CHECK_EQ(b[4], 5u);
                CHECK_EQ(b[5], 6u);
                CHECK_EQ(b[6], 7u);
            }

            SUBCASE("at the end")
            {
                sparrow::u8_buffer<std::uint32_t> b(values);
                CHECK_EQ(b.size(), 8u);
                const auto pos = std::prev(b.cend());
                const auto iter = b.erase(pos);
                CHECK_EQ(iter, b.end());
                REQUIRE_EQ(b.size(), 7u);
                CHECK_EQ(b[0], 0u);
                CHECK_EQ(b[1], 1u);
                CHECK_EQ(b[2], 2u);
                CHECK_EQ(b[3], 3u);
                CHECK_EQ(b[4], 4u);
                CHECK_EQ(b[5], 5u);
                CHECK_EQ(b[6], 6u);
            }
        }
    }

    TEST_CASE("push_back")
    {
        sparrow::u8_buffer<std::uint32_t> b(values);
        constexpr std::uint32_t expected_value = 101;
        b.push_back(expected_value);
        REQUIRE_EQ(b.size(), 9u);
        CHECK_EQ(b[0], 0u);
        CHECK_EQ(b[1], 1u);
        CHECK_EQ(b[2], 2u);
        CHECK_EQ(b[3], 3u);
        CHECK_EQ(b[4], 4u);
        CHECK_EQ(b[5], 5u);
        CHECK_EQ(b[6], 6u);
        CHECK_EQ(b[7], 7u);
        CHECK_EQ(b[8], expected_value);
    }

    TEST_CASE("pop_back")
    {
        sparrow::u8_buffer<std::uint32_t> b(values);
        b.pop_back();
        REQUIRE_EQ(b.size(), 7u);
        CHECK_EQ(b[0], 0u);
        CHECK_EQ(b[1], 1u);
        CHECK_EQ(b[2], 2u);
        CHECK_EQ(b[3], 3u);
        CHECK_EQ(b[4], 4u);
        CHECK_EQ(b[5], 5u);
        CHECK_EQ(b[6], 6u);
    }

    TEST_CASE("resize")
    {
        SUBCASE("new size")
        {
            sparrow::u8_buffer<std::uint32_t> b(values);
            constexpr std::size_t new_size = 4u;
            b.resize(new_size);
            REQUIRE_EQ(b.size(), new_size);
            CHECK_EQ(b[0], 0u);
            CHECK_EQ(b[1], 1u);
            CHECK_EQ(b[2], 2u);
            CHECK_EQ(b[3], 3u);
        }

        SUBCASE("new size and value")
        {
            sparrow::u8_buffer<std::uint32_t> b(values);
            constexpr std::size_t new_size = 12u;
            constexpr std::uint32_t value = 101u;
            b.resize(new_size, value);
            REQUIRE_EQ(b.size(), new_size);
            CHECK_EQ(b[0], 0u);
            CHECK_EQ(b[1], 1u);
            CHECK_EQ(b[2], 2u);
            CHECK_EQ(b[3], 3u);
            CHECK_EQ(b[4], 4u);
            CHECK_EQ(b[5], 5u);
            CHECK_EQ(b[6], 6u);
            CHECK_EQ(b[7], 7u);
            CHECK_EQ(b[8], value);
            CHECK_EQ(b[9], value);
            CHECK_EQ(b[10], value);
            CHECK_EQ(b[11], value);
        }
    }
}
