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

#include <array>
#include <bit>
#include <cstdint>
#include <list>

#include "sparrow/buffer/buffer.hpp"
#include "sparrow/buffer/buffer_adaptor.hpp"
#include "sparrow/utils/bit.hpp"
#include "sparrow/utils/memory_alignment.hpp"

#include "doctest/doctest.h"

constexpr uint8_t operator""_u8(unsigned long long int value)
{
    return static_cast<uint8_t>(value);
}

namespace sparrow
{
    TEST_SUITE("buffer_adaptor")
    {
        const std::array<uint8_t, 0> input_empty{};

        const std::array<uint8_t, 8> input{1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u};
        const std::array<uint8_t, 12> long_input{1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u};

        static_assert(T_is_const_if_FromBufferRef_is_const<std::vector<uint16_t>, uint32_t>);
        static_assert(T_is_const_if_FromBufferRef_is_const<std::vector<uint16_t>, const uint32_t>);
        static_assert(T_is_const_if_FromBufferRef_is_const<const std::vector<uint16_t>, const uint32_t>);
        static_assert(not T_is_const_if_FromBufferRef_is_const<const std::vector<uint16_t>, uint32_t>);

        static_assert(BufferReference<std::vector<uint16_t>&, uint32_t>);
        static_assert(BufferReference<const std::vector<uint16_t>&, uint32_t>);
        static_assert(BufferReference<std::vector<uint16_t>&, const uint32_t>);
        static_assert(BufferReference<const std::vector<uint16_t>&, const uint32_t>);
        static_assert(not BufferReference<std::vector<uint16_t>&, uint8_t>);
        static_assert(not BufferReference<std::vector<uint16_t>, uint32_t>);
        static_assert(not BufferReference<std::list<uint16_t>&, uint32_t>);

        TEST_CASE("constructor")
        {
            SUBCASE("from mutable non empty buffer")
            {
                buffer<uint8_t> buf(input);
                buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
            }

            SUBCASE("from mutable empty buffer")
            {
                buffer<uint8_t> buf_empty(input);
                buffer_adaptor<uint32_t, decltype(buf_empty)&> buffer_adapt_empty(buf_empty);
            }

            SUBCASE("from const non empty buffer")
            {
                [[maybe_unused]] buffer_adaptor<const uint32_t, decltype(input)&> buffer_adapt(input);
            }

            SUBCASE("from const empty buffer")
            {
                [[maybe_unused]] buffer_adaptor<const uint32_t, decltype(input_empty)&> buffer_adapt_empty(
                    input_empty
                );
            }
        }

        // Element access

        TEST_CASE("data")
        {
            SUBCASE("from mutable data")
            {
                buffer<uint8_t> buf(input);
                buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                auto data = buffer_adapt.data();
                CHECK_EQ(data[0], to_native_endian<std::endian::little>(0x04030201u));
                CHECK_EQ(data[1], to_native_endian<std::endian::little>(0x08070605u));
            }

            SUBCASE("from const data")
            {
                const buffer<uint8_t> buf(input);
                const buffer_adaptor<const uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                auto data = const_buffer_adapt.data();
                CHECK_EQ(data[0], to_native_endian<std::endian::little>(0x04030201u));
                CHECK_EQ(data[1], to_native_endian<std::endian::little>(0x08070605u));
            }
        }

        TEST_CASE("const data")
        {
            SUBCASE("from mutable data")
            {
                buffer<uint8_t> buf(input);
                const buffer_adaptor<uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                auto data = const_buffer_adapt.data();
                CHECK_EQ(data[0], to_native_endian<std::endian::little>(0x04030201u));
                CHECK_EQ(data[1], to_native_endian<std::endian::little>(0x08070605u));
            }

            SUBCASE("from const data")
            {
                const buffer<uint8_t> buf(input);
                const buffer_adaptor<const uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                auto data = const_buffer_adapt.data();
                CHECK_EQ(data[0], to_native_endian<std::endian::little>(0x04030201u));
                CHECK_EQ(data[1], to_native_endian<std::endian::little>(0x08070605u));
            }
        }

        TEST_CASE("[] operator")
        {
            SUBCASE("from mutable data")
            {
                buffer<uint8_t> buf(input);
                buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                CHECK_EQ(buffer_adapt[1], to_native_endian<std::endian::little>(0x08070605u));

                buffer_adapt[0] = 0x11111111u;
                CHECK_EQ(buf[0], 0x11);
            }

            SUBCASE("from const data")
            {
                const buffer<uint8_t> buf(input);
                const buffer_adaptor<const uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                CHECK_EQ(const_buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                CHECK_EQ(const_buffer_adapt[1], to_native_endian<std::endian::little>(0x08070605u));
            }
        }

        TEST_CASE("const [] operator")
        {
            SUBCASE("from mutable data")
            {
                buffer<uint8_t> buf(input);
                const buffer_adaptor<uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                CHECK_EQ(const_buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                CHECK_EQ(const_buffer_adapt[1], to_native_endian<std::endian::little>(0x08070605u));
            }

            SUBCASE("from const data")
            {
                const buffer<uint8_t> buf(input);
                const buffer_adaptor<const uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                CHECK_EQ(const_buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                CHECK_EQ(const_buffer_adapt[1], to_native_endian<std::endian::little>(0x08070605u));
            }
        }

        TEST_CASE("front")
        {
            SUBCASE("from mutable data")
            {
                buffer<uint8_t> buf(input);
                buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                CHECK_EQ(buffer_adapt.front(), to_native_endian<std::endian::little>(0x04030201u));
            }

            SUBCASE("from const data")
            {
                const buffer<uint8_t> buf(input);
                const buffer_adaptor<const uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                CHECK_EQ(const_buffer_adapt.front(), to_native_endian<std::endian::little>(0x04030201u));
            }
        }

        TEST_CASE("const front")
        {
            SUBCASE("from mutable data")
            {
                buffer<uint8_t> buf(input);
                const buffer_adaptor<uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                CHECK_EQ(const_buffer_adapt.front(), to_native_endian<std::endian::little>(0x04030201u));
            }

            SUBCASE("from const data")
            {
                const buffer<uint8_t> buf(input);
                const buffer_adaptor<const uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                CHECK_EQ(const_buffer_adapt.front(), to_native_endian<std::endian::little>(0x04030201u));
            }
        }

        TEST_CASE("back")
        {
            SUBCASE("from mutable data")
            {
                buffer<uint8_t> buf(input);
                buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                CHECK_EQ(buffer_adapt.back(), to_native_endian<std::endian::little>(0x08070605u));
            }

            SUBCASE("from const data")
            {
                const buffer<uint8_t> buf(input);
                const buffer_adaptor<const uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                CHECK_EQ(const_buffer_adapt.back(), to_native_endian<std::endian::little>(0x08070605u));
            }
        }

        TEST_CASE("const back")
        {
            SUBCASE("from mutable data")
            {
                buffer<uint8_t> buf(input);
                const buffer_adaptor<uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                CHECK_EQ(const_buffer_adapt.back(), to_native_endian<std::endian::little>(0x08070605u));
            }

            SUBCASE("from const data")
            {
                const buffer<uint8_t> buf(input);
                const buffer_adaptor<const uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                CHECK_EQ(const_buffer_adapt.back(), to_native_endian<std::endian::little>(0x08070605u));
            }
        }

        // Iterators
        TEST_CASE("Iterators")
        {
            buffer<uint8_t> buf(input);

            SUBCASE("begin")
            {
                buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                auto it = buffer_adapt.begin();
                CHECK_EQ(*it, to_native_endian<std::endian::little>(0x04030201u));
            }

            SUBCASE("end")
            {
                buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                const auto it = buffer_adapt.end();
                CHECK_EQ(it, std::next(buffer_adapt.begin(), 2));
            }

            SUBCASE("const begin")
            {
                const buffer_adaptor<uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                auto it = const_buffer_adapt.begin();
                CHECK_EQ(*it, to_native_endian<std::endian::little>(0x04030201u));
            }

            SUBCASE("const end")
            {
                const buffer_adaptor<uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                const auto it = const_buffer_adapt.end();
                CHECK_EQ(it, std::next(const_buffer_adapt.begin(), 2));
            }

            SUBCASE("const cbegin")
            {
                const buffer_adaptor<uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                auto it = const_buffer_adapt.cbegin();
                CHECK_EQ(*it, to_native_endian<std::endian::little>(0x04030201u));
            }

            SUBCASE("const cend")
            {
                const buffer_adaptor<uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                auto it = const_buffer_adapt.cend();
                CHECK_EQ(it, std::next(const_buffer_adapt.begin(), 2));
            }

            SUBCASE("rbegin")
            {
                buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                auto it = buffer_adapt.rbegin();
                CHECK_EQ(*it, to_native_endian<std::endian::little>(0x08070605u));
                std::advance(it, 1);
                CHECK_EQ(*it, to_native_endian<std::endian::little>(0x04030201u));
                std::advance(it, 1);
                CHECK_EQ(it, buffer_adapt.rend());
            }

            SUBCASE("rend")
            {
                buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                auto it = buffer_adapt.rend();
                std::advance(it, -1);
                CHECK_EQ(*it, to_native_endian<std::endian::little>(0x04030201u));
                std::advance(it, -1);
                CHECK_EQ(*it, to_native_endian<std::endian::little>(0x08070605u));
                CHECK_EQ(it, buffer_adapt.rbegin());
            }

            SUBCASE("const rbegin")
            {
                const buffer_adaptor<uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                auto it = const_buffer_adapt.rbegin();
                CHECK_EQ(*it, to_native_endian<std::endian::little>(0x08070605u));
                std::advance(it, 1);
                CHECK_EQ(*it, to_native_endian<std::endian::little>(0x04030201u));
                std::advance(it, 1);
                CHECK_EQ(it, const_buffer_adapt.rend());
            }

            SUBCASE("const rend")
            {
                const buffer_adaptor<uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                auto it = const_buffer_adapt.rend();
                std::advance(it, -1);
                CHECK_EQ(*it, to_native_endian<std::endian::little>(0x04030201u));
                std::advance(it, -1);
                CHECK_EQ(*it, to_native_endian<std::endian::little>(0x08070605u));
                CHECK_EQ(it, const_buffer_adapt.rbegin());
            }

            SUBCASE("const crbegin")
            {
                const buffer_adaptor<uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                auto it = const_buffer_adapt.crbegin();
                CHECK_EQ(*it, to_native_endian<std::endian::little>(0x08070605u));
                std::advance(it, 1);
                CHECK_EQ(*it, to_native_endian<std::endian::little>(0x04030201u));
                std::advance(it, 1);
                CHECK_EQ(it, const_buffer_adapt.crend());
            }

            SUBCASE("const crend")
            {
                const buffer_adaptor<uint32_t, decltype(buf)&> const_buffer_adapt(buf);
                auto it = const_buffer_adapt.crend();
                std::advance(it, -1);
                CHECK_EQ(*it, to_native_endian<std::endian::little>(0x04030201u));
                std::advance(it, -1);
                CHECK_EQ(*it, to_native_endian<std::endian::little>(0x08070605u));
                CHECK_EQ(it, const_buffer_adapt.crbegin());
            }
        }

        // Capacity

        TEST_CASE("size")
        {
            buffer<uint8_t> buf(input);
            const buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
            CHECK_EQ(buffer_adapt.size(), 2);
            CHECK_EQ(buf.size(), 8);
        }

        TEST_CASE("empty")
        {
            buffer<uint8_t> empty_buf;
            const buffer_adaptor<uint32_t, decltype(empty_buf)&> buffer_adapt(empty_buf);
            CHECK(buffer_adapt.empty());
            CHECK(empty_buf.empty());

            buffer<uint8_t> buf2(input);
            const buffer_adaptor<uint32_t, decltype(buf2)&> buffer_adapt2(buf2);
            CHECK_FALSE(buffer_adapt2.empty());
            CHECK_FALSE(buf2.empty());
        }

        TEST_CASE("capacity")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
            CHECK_EQ(buffer_adapt.capacity(), 16);
            CHECK_EQ(buf.capacity(), sparrow::calculate_aligned_size<uint32_t>(8));
        }

        TEST_CASE("reserve")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
            buffer_adapt.reserve(10);
            CHECK_EQ(buffer_adapt.capacity(), 16);
            CHECK_EQ(buf.capacity(), 64);
        }

        TEST_CASE("shrink_to_fit")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
            CHECK_EQ(buffer_adapt.capacity(), 16);
            CHECK_EQ(buf.capacity(), 64);
            buffer_adapt.reserve(50);
            CHECK_EQ(buffer_adapt.capacity(), 50);
            CHECK_EQ(buf.capacity(), 200);
            buffer_adapt.shrink_to_fit();
            CHECK_EQ(buffer_adapt.capacity(), 16);
            CHECK_EQ(buf.capacity(), 64);
        }

        // Modifiers

        TEST_CASE("clear")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
            buffer_adapt.clear();
            CHECK(buffer_adapt.empty());
            CHECK(buf.empty());
        }

        TEST_CASE("insert")
        {
            SUBCASE("pos and value")
            {
                SUBCASE("at the beginning")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                    const auto it = buffer_adapt.cbegin();
                    constexpr uint32_t to_insert = 0x09999999;
                    const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.insert(
                        it,
                        to_insert
                    );
                    CHECK_EQ(*result, to_insert);
                    CHECK_EQ(result, buffer_adapt.begin());
                    REQUIRE_EQ(buffer_adapt.size(), 3);
                    CHECK_EQ(buffer_adapt[0], to_insert);
                    CHECK_EQ(buffer_adapt[1], to_native_endian<std::endian::little>(0x04030201u));
                    CHECK_EQ(buffer_adapt[2], to_native_endian<std::endian::little>(0x08070605u));
                    CHECK_EQ(buf.size(), 12);
                }

                SUBCASE("in the middle")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                    const auto it = std::next(buffer_adapt.cbegin());
                    constexpr uint32_t to_insert = to_native_endian<std::endian::little>(0x09999999u);
                    const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.insert(
                        it,
                        to_insert
                    );
                    CHECK_EQ(*result, to_insert);
                    CHECK_EQ(result, std::next(buffer_adapt.begin()));
                    REQUIRE_EQ(buffer_adapt.size(), 3);
                    CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                    CHECK_EQ(buffer_adapt[1], to_insert);
                    CHECK_EQ(buffer_adapt[2], to_native_endian<std::endian::little>(0x08070605u));
                    CHECK_EQ(buf.size(), 12);
                }

                SUBCASE("at the end")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                    const auto it = buffer_adapt.cend();
                    constexpr uint32_t to_insert = 0x09999999;
                    const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.insert(
                        it,
                        to_insert
                    );
                    CHECK_EQ(*result, to_insert);
                    CHECK_EQ(result, std::prev(buffer_adapt.end()));
                    REQUIRE_EQ(buffer_adapt.size(), 3);
                    CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                    CHECK_EQ(buffer_adapt[1], to_native_endian<std::endian::little>(0x08070605u));
                    CHECK_EQ(buffer_adapt[2], to_insert);
                    CHECK_EQ(buf.size(), 12);
                }
            }

            SUBCASE("pos, count and value")
            {
                SUBCASE("at the beginning")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                    auto it = buffer_adapt.cbegin();
                    constexpr uint32_t to_insert = 0x09999999;
                    const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.insert(
                        it,
                        2,
                        to_insert
                    );
                    CHECK_EQ(*result, to_insert);
                    CHECK_EQ(result, buffer_adapt.begin());
                    REQUIRE_EQ(buffer_adapt.size(), 4);
                    CHECK_EQ(buffer_adapt[0], to_insert);
                    CHECK_EQ(buffer_adapt[1], to_insert);
                    CHECK_EQ(buffer_adapt[2], to_native_endian<std::endian::little>(0x04030201u));
                    CHECK_EQ(buffer_adapt[3], to_native_endian<std::endian::little>(0x08070605u));
                    CHECK_EQ(buf.size(), 16);
                }

                SUBCASE("in the middle")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                    auto it = std::next(buffer_adapt.cbegin());
                    constexpr uint32_t to_insert = 0x09999999;
                    const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.insert(
                        it,
                        2,
                        to_insert
                    );
                    CHECK_EQ(*result, to_insert);
                    CHECK_EQ(result, std::next(buffer_adapt.begin()));
                    REQUIRE_EQ(buffer_adapt.size(), 4);
                    CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                    CHECK_EQ(buffer_adapt[1], to_insert);
                    CHECK_EQ(buffer_adapt[2], to_insert);
                    CHECK_EQ(buffer_adapt[3], to_native_endian<std::endian::little>(0x08070605u));
                    CHECK_EQ(buf.size(), 16);
                }

                SUBCASE("at the end")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                    auto it = buffer_adapt.cend();
                    constexpr uint32_t to_insert = 0x09999999;
                    const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.insert(
                        it,
                        2,
                        to_insert
                    );
                    CHECK_EQ(*result, to_insert);
                    CHECK_EQ(result, std::prev(buffer_adapt.end(), 2));
                    REQUIRE_EQ(buffer_adapt.size(), 4);
                    CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                    CHECK_EQ(buffer_adapt[1], to_native_endian<std::endian::little>(0x08070605u));
                    CHECK_EQ(buffer_adapt[2], to_insert);
                    CHECK_EQ(buffer_adapt[3], to_insert);
                    CHECK_EQ(buf.size(), 16);
                }
            }

            SUBCASE("pos, first and last")
            {
                SUBCASE("at the beginning")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                    auto it = buffer_adapt.cbegin();
                    std::vector<uint32_t> to_insert = {0x09999999, 0x08888888};
                    const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.insert(
                        it,
                        to_insert.cbegin(),
                        to_insert.cend()
                    );
                    CHECK_EQ(*result, to_insert[0]);
                    CHECK_EQ(result, buffer_adapt.begin());
                    REQUIRE_EQ(buffer_adapt.size(), 4);
                    CHECK_EQ(buffer_adapt[0], to_insert[0]);
                    CHECK_EQ(buffer_adapt[1], to_insert[1]);
                    CHECK_EQ(buffer_adapt[2], to_native_endian<std::endian::little>(0x04030201u));
                    CHECK_EQ(buffer_adapt[3], to_native_endian<std::endian::little>(0x08070605u));
                    CHECK_EQ(buf.size(), 16);
                }

                SUBCASE("in the middle")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                    auto it = std::next(buffer_adapt.cbegin());
                    const std::vector<uint32_t> to_insert = {0x09999999, 0x08888888};
                    const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.insert(
                        it,
                        to_insert.cbegin(),
                        to_insert.cend()
                    );
                    CHECK_EQ(*result, to_insert[0]);
                    CHECK_EQ(result, std::next(buffer_adapt.begin()));
                    REQUIRE_EQ(buffer_adapt.size(), 4);
                    CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                    CHECK_EQ(buffer_adapt[1], to_insert[0]);
                    CHECK_EQ(buffer_adapt[2], to_insert[1]);
                    CHECK_EQ(buffer_adapt[3], to_native_endian<std::endian::little>(0x08070605u));
                    CHECK_EQ(buf.size(), 16);
                }

                SUBCASE("at the end")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                    auto it = buffer_adapt.cend();
                    std::vector<uint32_t> to_insert = {0x09999999, 0x08888888};
                    const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.insert(
                        it,
                        to_insert.cbegin(),
                        to_insert.cend()
                    );
                    CHECK_EQ(*result, to_insert[0]);
                    CHECK_EQ(result, std::prev(buffer_adapt.end(), 2));
                    REQUIRE_EQ(buffer_adapt.size(), 4);
                    CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                    CHECK_EQ(buffer_adapt[1], to_native_endian<std::endian::little>(0x08070605u));
                    CHECK_EQ(buffer_adapt[2], to_insert[0]);
                    CHECK_EQ(buffer_adapt[3], to_insert[1]);
                    CHECK_EQ(buf.size(), 16);
                }
            }
        }

        TEST_CASE("emplace")
        {
            SUBCASE("at the beginning")
            {
                buffer<uint8_t> buf(input);
                buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                auto it = buffer_adapt.cbegin();
                constexpr uint32_t to_insert = 0x09999999;
                const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.emplace(
                    it,
                    to_insert
                );
                CHECK_EQ(*result, to_insert);
                CHECK_EQ(result, buffer_adapt.begin());
                REQUIRE_EQ(buffer_adapt.size(), 3);
                CHECK_EQ(buffer_adapt[0], to_insert);
                CHECK_EQ(buffer_adapt[1], to_native_endian<std::endian::little>(0x04030201u));
                CHECK_EQ(buffer_adapt[2], to_native_endian<std::endian::little>(0x08070605u));
                CHECK_EQ(buf.size(), 12);
            }

            SUBCASE("in the middle")
            {
                buffer<uint8_t> buf(input);
                buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                auto it = std::next(buffer_adapt.cbegin());
                constexpr uint32_t to_insert = 0x09999999u;
                const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.emplace(
                    it,
                    to_insert
                );
                CHECK_EQ(*result, to_insert);
                CHECK_EQ(result, std::next(buffer_adapt.begin()));
                REQUIRE_EQ(buffer_adapt.size(), 3);
                CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                CHECK_EQ(buffer_adapt[1], to_insert);
                CHECK_EQ(buffer_adapt[2], to_native_endian<std::endian::little>(0x08070605u));
                CHECK_EQ(buf.size(), 12);
            }

            SUBCASE("at the end")
            {
                buffer<uint8_t> buf(input);
                buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                auto it = buffer_adapt.cend();
                constexpr uint32_t to_insert = 0x09999999u;
                const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.emplace(
                    it,
                    to_insert
                );
                CHECK_EQ(*result, to_insert);
                CHECK_EQ(result, std::prev(buffer_adapt.end()));
                REQUIRE_EQ(buffer_adapt.size(), 3);
                CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                CHECK_EQ(buffer_adapt[1], to_native_endian<std::endian::little>(0x08070605u));
                CHECK_EQ(buffer_adapt[2], to_insert);
                CHECK_EQ(buf.size(), 12);
            }
        }

        TEST_CASE("erase")
        {
            SUBCASE("pos")
            {
                SUBCASE("with filled buffer")
                {
                    SUBCASE("at the beginning")
                    {
                        buffer<uint8_t> buf(input);
                        buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                        const auto it = buffer_adapt.cbegin();
                        const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.erase(it);
                        CHECK_EQ(result, buffer_adapt.begin());
                        REQUIRE_EQ(buffer_adapt.size(), 1);
                        CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x08070605u));
                        CHECK_EQ(buf.size(), 4);
                        CHECK_EQ(buf[0], 0x05_u8);
                        CHECK_EQ(buf[1], 0x06_u8);
                        CHECK_EQ(buf[2], 0x07_u8);
                        CHECK_EQ(buf[3], 0x08_u8);
                    }

                    SUBCASE("in the middle")
                    {
                        buffer<uint8_t> buf(input);
                        buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                        const auto it = std::next(buffer_adapt.cbegin());
                        const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.erase(it);
                        CHECK_EQ(result, std::next(buffer_adapt.begin()));
                        REQUIRE_EQ(buffer_adapt.size(), 1);
                        CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                        REQUIRE_EQ(buf.size(), 4);
                        CHECK_EQ(buf[0], 0x01_u8);
                        CHECK_EQ(buf[1], 0x02_u8);
                        CHECK_EQ(buf[2], 0x03_u8);
                        CHECK_EQ(buf[3], 0x04_u8);
                    }

                    SUBCASE("at the end")
                    {
                        buffer<uint8_t> buf(input);
                        buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                        const auto it = std::prev(buffer_adapt.cend());
                        const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.erase(it);
                        CHECK_EQ(result, buffer_adapt.end());
                        REQUIRE_EQ(buffer_adapt.size(), 1);
                        CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                        REQUIRE_EQ(buf.size(), 4);
                        CHECK_EQ(buf[0], 0x01_u8);
                        CHECK_EQ(buf[1], 0x02_u8);
                        CHECK_EQ(buf[2], 0x03_u8);
                        CHECK_EQ(buf[3], 0x04_u8);
                    }
                }

                SUBCASE("with empty buffer")
                {
                    buffer<uint8_t> buf(input_empty);
                    buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                    const auto it = buffer_adapt.cbegin();
                    const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.erase(it);
                    CHECK_EQ(result, buffer_adapt.end());
                    CHECK(buffer_adapt.empty());
                    CHECK(buf.empty());
                }
            }

            SUBCASE("first and last")
            {
                SUBCASE("with filled buffer")
                {
                    SUBCASE("at the beginning")
                    {
                        buffer<uint8_t> buf(long_input);
                        buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                        const auto first = buffer_adapt.cbegin();
                        const auto last = buffer_adapt.cend();
                        const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.erase(
                            first,
                            last
                        );
                        CHECK_EQ(result, buffer_adapt.end());
                        CHECK(buffer_adapt.empty());
                        CHECK_EQ(buf.size(), 0);
                    }

                    SUBCASE("in the middle")
                    {
                        buffer<uint8_t> buf(long_input);
                        buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                        const auto first = std::next(buffer_adapt.cbegin());
                        const auto last = std::next(buffer_adapt.cend(), -1);
                        const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.erase(
                            first,
                            last
                        );
                        CHECK_EQ(result, std::prev(buffer_adapt.end()));
                        REQUIRE_EQ(buffer_adapt.size(), 2);
                        CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                        CHECK_EQ(buffer_adapt[1], to_native_endian<std::endian::little>(0x0C0B0A09u));
                        CHECK_EQ(buf.size(), 8);
                    }

                    SUBCASE("at the end")
                    {
                        buffer<uint8_t> buf(long_input);
                        buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                        const auto first = std::prev(buffer_adapt.cend());
                        const auto last = buffer_adapt.cend();
                        const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.erase(
                            first,
                            last
                        );
                        CHECK_EQ(result, buffer_adapt.end());
                        REQUIRE_EQ(buffer_adapt.size(), 2);
                        CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                        CHECK_EQ(buffer_adapt[1], to_native_endian<std::endian::little>(0x08070605u));
                        CHECK_EQ(buf.size(), 8);
                    }
                }

                SUBCASE("with empty buffer")
                {
                    buffer<uint8_t> buf(input_empty);
                    buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
                    const auto first = buffer_adapt.cbegin();
                    const auto last = buffer_adapt.cend();
                    const buffer_adaptor<uint32_t, decltype(buf)&>::iterator result = buffer_adapt.erase(
                        first,
                        last
                    );
                    CHECK_EQ(result, buffer_adapt.end());
                    CHECK(buffer_adapt.empty());
                }
            }
        }

        TEST_CASE("push_back")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
            constexpr uint32_t to_push = to_native_endian<std::endian::little>(0x05040302u);
            buffer_adapt.push_back(to_push);
            REQUIRE_EQ(buffer_adapt.size(), 3);
            CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
            CHECK_EQ(buffer_adapt[1], to_native_endian<std::endian::little>(0x08070605u));
            CHECK_EQ(buffer_adapt[2], to_push);

            CHECK_EQ(buf[8], 0x02u);
            CHECK_EQ(buf[9], 0x03u);
            CHECK_EQ(buf[10], 0x04u);
            CHECK_EQ(buf[11], 0x05u);
        }

        TEST_CASE("pop_back")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
            buffer_adapt.pop_back();
            REQUIRE_EQ(buffer_adapt.size(), 1);
            CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
            CHECK_EQ(buf.size(), 4);
        }

        TEST_CASE("resize")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, decltype(buf)&> buffer_adapt(buf);
            SUBCASE("new_size")
            {
                buffer_adapt.resize(4);
                REQUIRE_EQ(buffer_adapt.size(), 4);
                CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                CHECK_EQ(buffer_adapt[1], to_native_endian<std::endian::little>(0x08070605u));
                CHECK_EQ(buffer_adapt[2], to_native_endian<std::endian::little>(0x00000000u));
                CHECK_EQ(buffer_adapt[3], to_native_endian<std::endian::little>(0x00000000u));
                CHECK_EQ(buf.size(), 16);
            }

            SUBCASE("new_size and value")
            {
                constexpr uint32_t value = 0x0999999;
                buffer_adapt.resize(4, value);
                REQUIRE_EQ(buffer_adapt.size(), 4);
                CHECK_EQ(buffer_adapt[0], to_native_endian<std::endian::little>(0x04030201u));
                CHECK_EQ(buffer_adapt[1], to_native_endian<std::endian::little>(0x08070605u));
                CHECK_EQ(buffer_adapt[2], value);
                CHECK_EQ(buffer_adapt[3], value);
                CHECK_EQ(buf.size(), 16);
            }
        }

        TEST_CASE("make_buffer_adaptor")
        {
            auto buffer_adaptor = make_buffer_adaptor<uint32_t>(input);
            const auto size = buffer_adaptor.size();
            CHECK_EQ(size, 2);
        }
    }
}
