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
#include <cstdint>

#include "sparrow/buffer_adaptor.hpp"

#include "doctest/doctest.h"

namespace sparrow
{

    TEST_SUITE("buffer_adaptor")
    {
        const std::array<uint8_t, 0> input_empty{};

        const std::array<uint8_t, 8> input{1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u};
        const std::array<uint8_t, 12> long_input{1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u};

        TEST_CASE("constructor")
        {
            SUBCASE("from non empty buffer")
            {
                buffer<uint8_t> buf(input);
                buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
            }

            SUBCASE("from empty buffer")
            {
                buffer<uint8_t> buf_empty(input);
                buffer_adaptor<uint32_t, uint8_t> buffer_adapt_empty(buf_empty);
            }
        }

        // Element access

        TEST_CASE("data")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
            auto data = buffer_adapt.data();
            CHECK_EQ(data[0], 0x04030201);
            CHECK_EQ(data[1], 0x08070605);
        }

        TEST_CASE("const data")
        {
            buffer<uint8_t> buf(input);
            const buffer_adaptor<uint32_t, uint8_t> const_buffer_adapt(buf);
            auto data = const_buffer_adapt.data();
            CHECK_EQ(data[0], 0x04030201);
            CHECK_EQ(data[1], 0x08070605);
        }

        TEST_CASE("[] operator")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
            CHECK_EQ(buffer_adapt[0], 0x04030201);
            CHECK_EQ(buffer_adapt[1], 0x08070605);
        }

        TEST_CASE("const [] operator")
        {
            buffer<uint8_t> buf(input);
            const buffer_adaptor<uint32_t, uint8_t> const_buffer_adapt(buf);
            CHECK_EQ(const_buffer_adapt[0], 0x04030201);
            CHECK_EQ(const_buffer_adapt[1], 0x08070605);
        }

        TEST_CASE("front")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
            CHECK_EQ(buffer_adapt.front(), 0x04030201);
        }

        TEST_CASE("const front")
        {
            buffer<uint8_t> buf(input);
            const buffer_adaptor<uint32_t, uint8_t> const_buffer_adapt(buf);
            CHECK_EQ(const_buffer_adapt.front(), 0x04030201);
        }

        TEST_CASE("back")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
            [[maybe_unused]] const auto lol = buffer_adapt.back();
            CHECK_EQ(buffer_adapt.back(), 0x08070605);
        }

        TEST_CASE("const back")
        {
            buffer<uint8_t> buf(input);
            const buffer_adaptor<uint32_t, uint8_t> const_buffer_adapt(buf);
            CHECK_EQ(const_buffer_adapt.back(), 0x08070605);
        }

        // Iterators
        TEST_CASE("Iterators")
        {
            buffer<uint8_t> buf(input);

            SUBCASE("begin")
            {
                buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                auto it = buffer_adapt.begin();
                CHECK_EQ(*it, 0x04030201);
            }

            SUBCASE("end")
            {
                buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                const auto it = buffer_adapt.end();
                CHECK_EQ(it, std::next(buffer_adapt.begin(), 2));
            }

            SUBCASE("const begin")
            {
                const buffer_adaptor<uint32_t, uint8_t> const_buffer_adapt(buf);
                auto it = const_buffer_adapt.begin();
                CHECK_EQ(*it, 0x04030201);
            }

            SUBCASE("const end")
            {
                const buffer_adaptor<uint32_t, uint8_t> const_buffer_adapt(buf);
                const auto it = const_buffer_adapt.end();
                CHECK_EQ(it, std::next(const_buffer_adapt.begin(), 2));
            }

            SUBCASE("const cbegin")
            {
                const buffer_adaptor<uint32_t, uint8_t> const_buffer_adapt(buf);
                auto it = const_buffer_adapt.cbegin();
                CHECK_EQ(*it, 0x04030201);
            }

            SUBCASE("const cend")
            {
                const buffer_adaptor<uint32_t, uint8_t> const_buffer_adapt(buf);
                auto it = const_buffer_adapt.cend();
                CHECK_EQ(it, std::next(const_buffer_adapt.begin(), 2));
            }

            SUBCASE("rbegin")
            {
                buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                auto it = buffer_adapt.rbegin();
                CHECK_EQ(*it, 0x08070605);
                std::advance(it, 1);
                CHECK_EQ(*it, 0x04030201);
                std::advance(it, 1);
                CHECK_EQ(it, buffer_adapt.rend());
            }

            SUBCASE("rend")
            {
                buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                auto it = buffer_adapt.rend();
                std::advance(it, -1);
                CHECK_EQ(*it, 0x04030201);
                std::advance(it, -1);
                CHECK_EQ(*it, 0x08070605);
                CHECK_EQ(it, buffer_adapt.rbegin());
            }

            SUBCASE("const rbegin")
            {
                const buffer_adaptor<uint32_t, uint8_t> const_buffer_adapt(buf);
                auto it = const_buffer_adapt.rbegin();
                CHECK_EQ(*it, 0x08070605);
                std::advance(it, 1);
                CHECK_EQ(*it, 0x04030201);
                std::advance(it, 1);
                CHECK_EQ(it, const_buffer_adapt.rend());
            }

            SUBCASE("const rend")
            {
                const buffer_adaptor<uint32_t, uint8_t> const_buffer_adapt(buf);
                auto it = const_buffer_adapt.rend();
                std::advance(it, -1);
                CHECK_EQ(*it, 0x04030201);
                std::advance(it, -1);
                CHECK_EQ(*it, 0x08070605);
                CHECK_EQ(it, const_buffer_adapt.rbegin());
            }

            SUBCASE("const crbegin")
            {
                const buffer_adaptor<uint32_t, uint8_t> const_buffer_adapt(buf);
                auto it = const_buffer_adapt.crbegin();
                CHECK_EQ(*it, 0x08070605);
                std::advance(it, 1);
                CHECK_EQ(*it, 0x04030201);
                std::advance(it, 1);
                CHECK_EQ(it, const_buffer_adapt.crend());
            }

            SUBCASE("const crend")
            {
                const buffer_adaptor<uint32_t, uint8_t> const_buffer_adapt(buf);
                auto it = const_buffer_adapt.crend();
                std::advance(it, -1);
                CHECK_EQ(*it, 0x04030201);
                std::advance(it, -1);
                CHECK_EQ(*it, 0x08070605);
                CHECK_EQ(it, const_buffer_adapt.crbegin());
            }
        }


        // Capacity

        TEST_CASE("size")
        {
            buffer<uint8_t> buf(input);
            const buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
            CHECK_EQ(buffer_adapt.size(), 2);
        }

        TEST_CASE("empty")
        {
            buffer<uint8_t> empty_buf;
            const buffer_adaptor<uint32_t, uint8_t> buffer_adapt(empty_buf);
            CHECK(buffer_adapt.empty());

            buffer<uint8_t> buf2(input);
            const buffer_adaptor<uint32_t, uint8_t> buffer_adapt2(buf2);
            CHECK(!buffer_adapt2.empty());
        }

        TEST_CASE("capacity")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
            CHECK_EQ(buffer_adapt.capacity(), 2);
        }

        TEST_CASE("reserve")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
            buffer_adapt.reserve(10);
            CHECK_EQ(buffer_adapt.capacity(), 10);
        }

        TEST_CASE("shrink_to_fit")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
            CHECK_EQ(buffer_adapt.capacity(), 2);
            buffer_adapt.reserve(50);
            CHECK_EQ(buffer_adapt.capacity(), 50);
            buffer_adapt.shrink_to_fit();
            CHECK_EQ(buffer_adapt.capacity(), 2);
        }

        // Modifiers

        TEST_CASE("clear")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
            buffer_adapt.clear();
            CHECK_EQ(buffer_adapt.size(), 0);
        }

        TEST_CASE("insert")
        {
            SUBCASE("pos and value")
            {
                SUBCASE("at the beginning")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                    const auto it = buffer_adapt.cbegin();
                    constexpr uint32_t to_insert = 0x09999999;
                    const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.insert(it, to_insert);
                    CHECK_EQ(*result, to_insert);
                    CHECK_EQ(result, buffer_adapt.begin());
                    REQUIRE_EQ(buffer_adapt.size(), 3);
                    CHECK_EQ(buffer_adapt[0], to_insert);
                    CHECK_EQ(buffer_adapt[1], 0x04030201);
                    CHECK_EQ(buffer_adapt[2], 0x08070605);
                }

                SUBCASE("in the middle")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                    const auto it = std::next(buffer_adapt.cbegin());
                    constexpr uint32_t to_insert = 0x09999999;
                    const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.insert(it, to_insert);
                    CHECK_EQ(*result, to_insert);
                    CHECK_EQ(result, std::next(buffer_adapt.begin()));
                    REQUIRE_EQ(buffer_adapt.size(), 3);
                    CHECK_EQ(buffer_adapt[0], 0x04030201);
                    CHECK_EQ(buffer_adapt[1], to_insert);
                    CHECK_EQ(buffer_adapt[2], 0x08070605);
                }

                SUBCASE("at the end")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                    const auto it = buffer_adapt.cend();
                    constexpr uint32_t to_insert = 0x09999999;
                    const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.insert(it, to_insert);
                    CHECK_EQ(*result, to_insert);
                    CHECK_EQ(result, std::prev(buffer_adapt.end()));
                    REQUIRE_EQ(buffer_adapt.size(), 3);
                    CHECK_EQ(buffer_adapt[0], 0x04030201);
                    CHECK_EQ(buffer_adapt[1], 0x08070605);
                    CHECK_EQ(buffer_adapt[2], to_insert);
                }
            }

            SUBCASE("pos, count and value")
            {
                SUBCASE("at the beginning")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                    auto it = buffer_adapt.cbegin();
                    constexpr uint32_t to_insert = 0x09999999;
                    const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.insert(it, 2, to_insert);
                    CHECK_EQ(*result, to_insert);
                    CHECK_EQ(result, buffer_adapt.begin());
                    REQUIRE_EQ(buffer_adapt.size(), 4);
                    CHECK_EQ(buffer_adapt[0], to_insert);
                    CHECK_EQ(buffer_adapt[1], to_insert);
                    CHECK_EQ(buffer_adapt[2], 0x04030201);
                    CHECK_EQ(buffer_adapt[3], 0x08070605);
                }

                SUBCASE("in the middle")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                    auto it = std::next(buffer_adapt.cbegin());
                    constexpr uint32_t to_insert = 0x09999999;
                    const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.insert(it, 2, to_insert);
                    CHECK_EQ(*result, to_insert);
                    CHECK_EQ(result, std::next(buffer_adapt.begin()));
                    REQUIRE_EQ(buffer_adapt.size(), 4);
                    CHECK_EQ(buffer_adapt[0], 0x04030201);
                    CHECK_EQ(buffer_adapt[1], to_insert);
                    CHECK_EQ(buffer_adapt[2], to_insert);
                    CHECK_EQ(buffer_adapt[3], 0x08070605);
                }

                SUBCASE("at the end")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                    auto it = buffer_adapt.cend();
                    constexpr uint32_t to_insert = 0x09999999;
                    const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.insert(it, 2, to_insert);
                    CHECK_EQ(*result, to_insert);
                    CHECK_EQ(result, std::prev(buffer_adapt.end(), 2));
                    REQUIRE_EQ(buffer_adapt.size(), 4);
                    CHECK_EQ(buffer_adapt[0], 0x04030201);
                    CHECK_EQ(buffer_adapt[1], 0x08070605);
                    CHECK_EQ(buffer_adapt[2], to_insert);
                    CHECK_EQ(buffer_adapt[3], to_insert);
                }
            }

            SUBCASE("pos, first and last")
            {
                SUBCASE("at the beginning")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                    auto it = buffer_adapt.cbegin();
                    std::vector<uint32_t> to_insert = {0x09999999, 0x08888888};
                    const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.insert(
                        it,
                        to_insert.cbegin(),
                        to_insert.cend()
                    );
                    CHECK_EQ(*result, to_insert[0]);
                    CHECK_EQ(result, buffer_adapt.begin());
                    REQUIRE_EQ(buffer_adapt.size(), 4);
                    CHECK_EQ(buffer_adapt[0], to_insert[0]);
                    CHECK_EQ(buffer_adapt[1], to_insert[1]);
                    CHECK_EQ(buffer_adapt[2], 0x04030201);
                    CHECK_EQ(buffer_adapt[3], 0x08070605);
                }

                SUBCASE("in the middle")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                    auto it = std::next(buffer_adapt.cbegin());
                    const std::vector<uint32_t> to_insert = {0x09999999, 0x08888888};
                    const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.insert(
                        it,
                        to_insert.cbegin(),
                        to_insert.cend()
                    );
                    CHECK_EQ(*result, to_insert[0]);
                    CHECK_EQ(result, std::next(buffer_adapt.begin()));
                    REQUIRE_EQ(buffer_adapt.size(), 4);
                    CHECK_EQ(buffer_adapt[0], 0x04030201);
                    CHECK_EQ(buffer_adapt[1], to_insert[0]);
                    CHECK_EQ(buffer_adapt[2], to_insert[1]);
                    CHECK_EQ(buffer_adapt[3], 0x08070605);
                }

                SUBCASE("at the end")
                {
                    buffer<uint8_t> buf(input);
                    buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                    auto it = buffer_adapt.cend();
                    std::vector<uint32_t> to_insert = {0x09999999, 0x08888888};
                    const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.insert(
                        it,
                        to_insert.cbegin(),
                        to_insert.cend()
                    );
                    CHECK_EQ(*result, to_insert[0]);
                    CHECK_EQ(result, std::prev(buffer_adapt.end(), 2));
                    REQUIRE_EQ(buffer_adapt.size(), 4);
                    CHECK_EQ(buffer_adapt[0], 0x04030201);
                    CHECK_EQ(buffer_adapt[1], 0x08070605);
                    CHECK_EQ(buffer_adapt[2], to_insert[0]);
                    CHECK_EQ(buffer_adapt[3], to_insert[1]);
                }
            }
        }

        TEST_CASE("emplace")
        {
            SUBCASE("at the beginning")
            {
                buffer<uint8_t> buf(input);
                buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                auto it = buffer_adapt.cbegin();
                constexpr uint32_t to_insert = 0x09999999;
                const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.emplace(it, to_insert);
                CHECK_EQ(*result, to_insert);
                CHECK_EQ(result, buffer_adapt.begin());
                REQUIRE_EQ(buffer_adapt.size(), 3);
                CHECK_EQ(buffer_adapt[0], to_insert);
                CHECK_EQ(buffer_adapt[1], 0x04030201);
                CHECK_EQ(buffer_adapt[2], 0x08070605);
            }

            SUBCASE("in the middle")
            {
                buffer<uint8_t> buf(input);
                buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                auto it = std::next(buffer_adapt.cbegin());
                constexpr uint32_t to_insert = 0x09999999;
                const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.emplace(it, to_insert);
                CHECK_EQ(*result, to_insert);
                CHECK_EQ(result, std::next(buffer_adapt.begin()));
                REQUIRE_EQ(buffer_adapt.size(), 3);
                CHECK_EQ(buffer_adapt[0], 0x04030201);
                CHECK_EQ(buffer_adapt[1], to_insert);
                CHECK_EQ(buffer_adapt[2], 0x08070605);
            }

            SUBCASE("at the end")
            {
                buffer<uint8_t> buf(input);
                buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                auto it = buffer_adapt.cend();
                constexpr uint32_t to_insert = 0x09999999;
                const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.emplace(it, to_insert);
                CHECK_EQ(*result, to_insert);
                CHECK_EQ(result, std::prev(buffer_adapt.end()));
                REQUIRE_EQ(buffer_adapt.size(), 3);
                CHECK_EQ(buffer_adapt[0], 0x04030201);
                CHECK_EQ(buffer_adapt[1], 0x08070605);
                CHECK_EQ(buffer_adapt[2], to_insert);
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
                        buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                        const auto it = buffer_adapt.cbegin();
                        const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.erase(it);
                        CHECK_EQ(result, buffer_adapt.begin());
                        REQUIRE_EQ(buffer_adapt.size(), 1);
                        CHECK_EQ(buffer_adapt[0], 0x08070605);
                    }

                    SUBCASE("in the middle")
                    {
                        buffer<uint8_t> buf(input);
                        buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                        const auto it = std::next(buffer_adapt.cbegin());
                        const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.erase(it);
                        CHECK_EQ(result, std::next(buffer_adapt.begin()));
                        REQUIRE_EQ(buffer_adapt.size(), 1);
                        CHECK_EQ(buffer_adapt[0], 0x04030201);
                    }

                    SUBCASE("at the end")
                    {
                        buffer<uint8_t> buf(input);
                        buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                        const auto it = std::prev(buffer_adapt.cend());
                        const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.erase(it);
                        CHECK_EQ(result, buffer_adapt.end());
                        REQUIRE_EQ(buffer_adapt.size(), 1);
                        CHECK_EQ(buffer_adapt[0], 0x04030201);
                    }
                }

                SUBCASE("with empty buffer")
                {
                    buffer<uint8_t> buf(input_empty);
                    buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                    const auto it = buffer_adapt.cbegin();
                    const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.erase(it);
                    CHECK_EQ(result, buffer_adapt.end());
                    CHECK(buffer_adapt.empty());
                }
            }

            SUBCASE("first and last")
            {
                SUBCASE("with filled buffer")
                {
                    SUBCASE("at the beginning")
                    {
                        buffer<uint8_t> buf(long_input);
                        buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                        const auto first = buffer_adapt.cbegin();
                        const auto last = buffer_adapt.cend();
                        const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.erase(first, last);
                        CHECK_EQ(result, buffer_adapt.end());
                        CHECK(buffer_adapt.empty());
                    }

                    SUBCASE("in the middle")
                    {
                        buffer<uint8_t> buf(long_input);
                        buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                        const auto first = std::next(buffer_adapt.cbegin());
                        const auto last = std::next(buffer_adapt.cend(), -1);
                        const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.erase(first, last);
                        CHECK_EQ(result, std::prev(buffer_adapt.end()));
                        REQUIRE_EQ(buffer_adapt.size(), 2);
                        CHECK_EQ(buffer_adapt[0], 0x04030201);
                        CHECK_EQ(buffer_adapt[1], 0x0C0B0A09);
                    }

                    SUBCASE("at the end")
                    {
                        buffer<uint8_t> buf(long_input);
                        buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                        const auto first = std::prev(buffer_adapt.cend());
                        const auto last = buffer_adapt.cend();
                        const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.erase(first, last);
                        CHECK_EQ(result, buffer_adapt.end());
                        REQUIRE_EQ(buffer_adapt.size(), 2);
                        CHECK_EQ(buffer_adapt[0], 0x04030201);
                        CHECK_EQ(buffer_adapt[1], 0x08070605);
                    }
                }

                SUBCASE("with empty buffer")
                {
                    buffer<uint8_t> buf(input_empty);
                    buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
                    const auto first = buffer_adapt.cbegin();
                    const auto last = buffer_adapt.cend();
                    const buffer_adaptor<uint32_t, uint8_t>::iterator result = buffer_adapt.erase(first, last);
                    CHECK_EQ(result, buffer_adapt.end());
                    CHECK(buffer_adapt.empty());
                }
            }
        }

        TEST_CASE("push_back")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
            buffer_adapt.push_back(0x05040302);
            REQUIRE_EQ(buffer_adapt.size(), 3);
            CHECK_EQ(buffer_adapt[0], 0x04030201);
            CHECK_EQ(buffer_adapt[1], 0x08070605);
            CHECK_EQ(buffer_adapt[2], 0x05040302);
        }

        TEST_CASE("pop_back")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
            buffer_adapt.pop_back();
            REQUIRE_EQ(buffer_adapt.size(), 1);
            CHECK_EQ(buffer_adapt[0], 0x04030201);
        }

        TEST_CASE("resize")
        {
            buffer<uint8_t> buf(input);
            buffer_adaptor<uint32_t, uint8_t> buffer_adapt(buf);
            SUBCASE("new_size")
            {
                buffer_adapt.resize(4);
                REQUIRE_EQ(buffer_adapt.size(), 4);
                CHECK_EQ(buffer_adapt[0], 0x04030201);
                CHECK_EQ(buffer_adapt[1], 0x08070605);
                CHECK_EQ(buffer_adapt[2], 0x00000000);
                CHECK_EQ(buffer_adapt[3], 0x00000000);
            }

            SUBCASE("new_size and value")
            {
                constexpr uint32_t value = 0x0999999;
                buffer_adapt.resize(4, value);
                REQUIRE_EQ(buffer_adapt.size(), 4);
                CHECK_EQ(buffer_adapt[0], 0x04030201);
                CHECK_EQ(buffer_adapt[1], 0x08070605);
                CHECK_EQ(buffer_adapt[2], value);
                CHECK_EQ(buffer_adapt[3], value);
            }
        }
    }
}
