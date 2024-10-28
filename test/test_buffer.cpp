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

#include <iterator>
#include <numeric>
#include <ranges>
#include <string>

#include "sparrow/buffer/buffer.hpp"
#include "sparrow/buffer/buffer_view.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    using buffer_test_type = buffer<int32_t>;
    using view_test_type = buffer_view<int32_t>;
    using const_view_test_type = buffer_view<const int32_t>;

    using non_trivial_buffer_test_type = buffer<std::string>;

    namespace
    {
        auto make_test_buffer(std::size_t size, int32_t start_value = 0) -> int32_t*
        {
            int32_t* res = new int32_t[size];
            std::iota(res, res + size, start_value);
            return res;
        }

        auto make_test_buffer_non_trivial(std::size_t size) -> std::string*
        {
            std::vector<std::string> vec(size);
            vec.insert(vec.begin(), vec.end(), vec.end());
            for (std::size_t i = 0; i < size; ++i)
            {
                vec[i] = std::to_string(i);
            }

            buffer<std::string>::allocator_type alloc;
            std::string* raw_buf = alloc.allocate(size);
            std::uninitialized_copy(vec.cbegin(), vec.cend(), raw_buf);
            return raw_buf;
        }
    }

    TEST_SUITE("buffer")
    {
        TEST_CASE("constructors")
        {
            {
                [[maybe_unused]] buffer_test_type b(8u);
            }

            {
                const std::size_t size = 8u;
                [[maybe_unused]] buffer_test_type b(make_test_buffer(size), size);
            }

            buffer_test_type b0;
            CHECK_EQ(b0.data(), nullptr);
            CHECK_EQ(b0.size(), 0u);
            CHECK_EQ(b0.capacity(), 0);

            const std::size_t expected_size = 4;
            buffer_test_type b1(expected_size);
            CHECK_NE(b1.data(), nullptr);
            CHECK_EQ(b1.size(), expected_size);
            CHECK_EQ(b1.capacity(), b1.size());

            int32_t* mem = make_test_buffer(expected_size);
            buffer_test_type b2(mem, expected_size);
            CHECK_EQ(b2.data(), mem);
            CHECK_EQ(b2.size(), expected_size);
            CHECK_EQ(b2.capacity(), b2.size());
            CHECK_EQ(b2.data()[2], 2);

            const int32_t expected_value = 3;
            buffer_test_type b3(expected_size, expected_value);
            CHECK_NE(b3.data(), nullptr);
            CHECK_EQ(b3.size(), expected_size);
            CHECK_EQ(b3.capacity(), b3.size());
            for (std::size_t i = 0; i < expected_size; ++i)
            {
                CHECK_EQ(b3[i], expected_value);
            }

            buffer_test_type b4 = {1u, 3u, 5u};
            CHECK_EQ(b4.size(), 3u);
            CHECK_EQ(b4[0], 1u);
            CHECK_EQ(b4[1], 3u);
            CHECK_EQ(b4[2], 5u);

            std::vector<buffer_test_type::value_type> exp = {2u, 4u, 6u};
            buffer_test_type b5(exp.cbegin(), exp.cend());
            CHECK_EQ(b5.size(), exp.size());
            for (size_t i = 0; i < 3u; ++i)
            {
                CHECK_EQ(b5[i], exp[i]);
            }
        }

        TEST_CASE("copy semantic")
        {
            const std::size_t size = 4u;
            buffer_test_type b1(make_test_buffer(size), size);
            buffer_test_type b2(b1);
            CHECK_EQ(b1, b2);
            CHECK_EQ(b1.capacity(), b2.capacity());

            const std::size_t size2 = 8u;
            buffer_test_type b3(make_test_buffer(size2, 4), size2);
            b2 = b3;
            CHECK_EQ(b2, b3);
            CHECK_EQ(b2.capacity(), b2.size());
            CHECK_NE(b1, b2);
        }

        TEST_CASE("move semantic")
        {
            const std::size_t size = 4u;
            buffer_test_type b1(make_test_buffer(size), size);
            buffer_test_type control(b1);
            buffer_test_type b2(std::move(b1));
            CHECK_EQ(b2, control);
            CHECK_EQ(b1.size(), 0);
            CHECK(b1.empty());
            CHECK_EQ(b1.data(), nullptr);

            const std::size_t size2 = 8u;
            buffer_test_type b4(make_test_buffer(size2, 4), size2);
            buffer_test_type control2(b4);
            b2 = std::move(b4);
            CHECK_EQ(b2, control2);
            CHECK_EQ(b2.capacity(), b2.size());
            CHECK_EQ(b4, control);
        }

        // Element access

        TEST_CASE("operator[]")
        {
            const std::size_t size = 4u;
            buffer_test_type b1(make_test_buffer(size), size);
            const buffer_test_type b2(b1);
            for (std::size_t i = 0; i < size; ++i)
            {
                CHECK_EQ(b1[i], i);
                CHECK_EQ(b2[i], i);
            }
        }

        TEST_CASE("front")
        {
            const std::size_t size = 4u;
            const std::int32_t expected_value = 3;
            buffer_test_type b1(make_test_buffer(size, expected_value), size);
            const buffer_test_type b2(b1);
            CHECK_EQ(b1.front(), expected_value);
            CHECK_EQ(b2.front(), expected_value);
        }

        TEST_CASE("back")
        {
            const std::size_t size = 4u;
            const std::int32_t expected_value = 6;
            buffer_test_type b1(make_test_buffer(size, expected_value), size);
            const buffer_test_type b2(b1);
            CHECK_EQ(b1.back(), expected_value + 3u);
            CHECK_EQ(b2.back(), expected_value + 3u);
        }

        TEST_CASE("data")
        {
            const std::size_t size = 4u;
            buffer_test_type b1(make_test_buffer(size), size);

            const int32_t expected_value = 101;
            const std::size_t idx = 3u;
            b1.data()[idx] = expected_value;
            buffer_test_type b2(b1);
            CHECK_EQ(b2.data()[idx], expected_value);

            buffer_test_type b3(std::move(b1));
            CHECK_EQ(b3.data()[idx], expected_value);
        }

        // Iterators

        static_assert(std::ranges::contiguous_range<buffer_test_type>);
        static_assert(std::contiguous_iterator<buffer_test_type::iterator>);
        static_assert(std::contiguous_iterator<buffer_test_type::const_iterator>);

        TEST_CASE("iterator")
        {
            const std::size_t size = 8u;
            buffer_test_type b(make_test_buffer(size), size);
            auto iter = b.begin();
            auto citer = b.cbegin();
            for (std::size_t i = 0; i < b.size(); ++i)
            {
                CHECK_EQ(*iter++, b[i]);
                CHECK_EQ(*citer++, b[i]);
            }
            CHECK_EQ(iter, b.end());
            CHECK_EQ(citer, b.cend());
        }

        TEST_CASE("reverse_iterator")
        {
            const std::size_t size = 8u;
            buffer_test_type b(make_test_buffer(size), size);
            auto iter = b.rbegin();
            auto citer = b.crbegin();
            for (std::size_t i = b.size(); i != 0u; --i)
            {
                CHECK_EQ(*iter++, b[i - 1]);
                CHECK_EQ(*citer++, b[i - 1]);
            }
            CHECK_EQ(iter, b.rend());
            CHECK_EQ(citer, b.crend());
        }

        TEST_CASE("iterator_consistency")
        {
            const std::size_t size = 8u;
            buffer_test_type b(make_test_buffer(size), size);

            {
                auto iter = --b.end();
                auto riter = b.rbegin();
                auto citer = --b.cend();
                auto criter = b.crbegin();

                while (iter != b.begin())
                {
                    CHECK_EQ(*iter, *riter);
                    CHECK_EQ(*citer, *criter);
                    --iter, --citer, ++riter, ++criter;
                }
            }

            {
                auto iter = b.begin();
                auto riter = --b.rend();
                auto citer = b.cbegin();
                auto criter = --b.crend();

                while (iter != b.end())
                {
                    CHECK_EQ(*iter, *riter);
                    CHECK_EQ(*citer, *criter);
                    ++iter, ++citer, --riter, --criter;
                }
            }
        }

        // capacity

        TEST_CASE("empty")
        {
            buffer_test_type b1;
            CHECK(b1.empty());

            const std::size_t size = 4u;
            buffer_test_type b2(make_test_buffer(size), size);
            CHECK(!b2.empty());
        }

        TEST_CASE("capacity")
        {
            const std::size_t size = 4u;
            buffer_test_type b(make_test_buffer(size), size);
            CHECK_EQ(b.capacity(), size);
        }

        TEST_CASE("size")
        {
            const std::size_t size = 4u;
            buffer_test_type b(make_test_buffer(size), size);
            CHECK_EQ(b.size(), size);
        }

        // modifiers

        TEST_CASE("clear")
        {
            const std::size_t size1 = 4u;
            buffer_test_type b(make_test_buffer(size1), size1);
            b.clear();
            CHECK_EQ(b.size(), 0u);
            CHECK_EQ(b.capacity(), size1);
        }

        TEST_CASE("reserve")
        {
            const std::size_t size1 = 4u;
            buffer_test_type b1(make_test_buffer(size1), size1);
            buffer_test_type b2(b1);
            const std::size_t new_cap = 8u;
            b1.reserve(new_cap);
            CHECK_EQ(b1.capacity(), new_cap);
            for (std::size_t i = 0; i < size1; ++i)
            {
                CHECK_EQ(b1.data()[i], b2.data()[i]);
            }
        }

        TEST_CASE("resize")
        {
            const std::size_t size1 = 4u;
            const std::size_t size2 = 8u;
            buffer_test_type b(make_test_buffer(size1), size1);
            b.resize(size2);
            CHECK_EQ(b.size(), size2);
            CHECK_EQ(b.capacity(), size2);
            CHECK_EQ(b.data()[2], 2);

            b.resize(size1);
            CHECK_EQ(b.size(), size1);
            CHECK_EQ(b.capacity(), size2);
            CHECK_EQ(b.data()[2], 2);

            const std::size_t size3 = 6u;
            const buffer_test_type::value_type v = 7u;
            b.resize(size3, v);
            CHECK_EQ(b.size(), size3);
            CHECK_EQ(b.capacity(), size2);
            CHECK_EQ(b.data()[2], 2);
            CHECK_EQ(b.data()[4], v);
            CHECK_EQ(b.data()[5], v);
        }

        TEST_CASE("shrink_to_fit")
        {
            const std::size_t size1 = 4u;
            buffer_test_type b1(make_test_buffer(size1), size1);
            const std::size_t new_cap = 8u;
            b1.reserve(new_cap);
            CHECK_EQ(b1.capacity(), new_cap);
            b1.shrink_to_fit();
            CHECK_EQ(b1.capacity(), size1);
        }

        TEST_CASE("swap")
        {
            const std::size_t size1 = 4u;
            const std::size_t size2 = 8u;

            buffer_test_type b1(make_test_buffer(size1), size1);
            buffer_test_type b2(make_test_buffer(size2), size2);
            auto* data1 = b1.data();
            auto* data2 = b2.data();
            b1.swap(b2);
            CHECK_EQ(b1.size(), size2);
            CHECK_EQ(b1.data(), data2);
            CHECK_EQ(b2.size(), size1);
            CHECK_EQ(b2.data(), data1);
        }

        TEST_CASE("equality comparison")
        {
            const std::size_t size = 4u;
            buffer_test_type b1(make_test_buffer(size), size);
            buffer_test_type b2(make_test_buffer(size), size);
            CHECK(b1 == b2);

            const std::size_t size2 = 8u;
            buffer_test_type b3(make_test_buffer(size2), size2);
            CHECK(b1 != b3);
        }

        TEST_CASE("emplace")
        {
            SUBCASE("in empty buffer")
            {
                buffer_test_type b;
                constexpr int32_t expected_value = 101;
                b.emplace(b.cbegin(), expected_value);
                REQUIRE_EQ(b.size(), 1);
                CHECK_EQ(b.capacity(), 2);
                CHECK_EQ(b[0], expected_value);
            }

            SUBCASE("at begin")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                constexpr int32_t expected_value = 101;
                b.emplace(b.cbegin(), expected_value);
                REQUIRE_EQ(b.size(), size + 1);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], expected_value);
                CHECK_EQ(b[1], 0);
                CHECK_EQ(b[2], 1);
                CHECK_EQ(b[3], 2);
                CHECK_EQ(b[4], 3);
            }

            SUBCASE("in the middle")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                constexpr int32_t expected_value = 101;
                b.emplace(b.cbegin() + 2, expected_value);
                REQUIRE_EQ(b.size(), size + 1);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], 0);
                CHECK_EQ(b[1], 1);
                CHECK_EQ(b[2], expected_value);
                CHECK_EQ(b[3], 2);
                CHECK_EQ(b[4], 3);
            }

            SUBCASE("at the end")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                constexpr int32_t expected_value = 101;
                b.emplace(b.cend(), expected_value);
                REQUIRE_EQ(b.size(), size + 1);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], 0);
                CHECK_EQ(b[1], 1);
                CHECK_EQ(b[2], 2);
                CHECK_EQ(b[3], 3);
                CHECK_EQ(b[4], expected_value);
            }
        }

        TEST_CASE("insert")
        {
            SUBCASE("in empty buffer")
            {
                buffer_test_type b;
                constexpr int32_t expected_value = 101;
                b.insert(b.cbegin(), expected_value);
                REQUIRE_EQ(b.size(), 1);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], expected_value);
                CHECK_EQ(b.back(), expected_value);
                CHECK_EQ(b.cend()[-1], expected_value);
                CHECK_EQ(b.crbegin()[0], expected_value);
            }

            SUBCASE("value at the beginning of the buffer")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);
                constexpr int32_t expected_value = 101;
                b.insert(b.cbegin(), expected_value);
                REQUIRE_EQ(b.size(), size + 1);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], expected_value);
                CHECK_EQ(b[1], 0);
                CHECK_EQ(b[2], 1);
                CHECK_EQ(b[3], 2);
                CHECK_EQ(b[4], 3);
            }

            SUBCASE("move value at the beginning of the buffer")
            {
                CHECK(true);
                constexpr std::size_t size = 4u;
                non_trivial_buffer_test_type b(make_test_buffer_non_trivial(size), size);
                const std::string expected_value = "9999";
                std::string movable_expected_value = expected_value;
                b.insert(b.cbegin(), std::move(movable_expected_value));
                REQUIRE_EQ(b.size(), size + 1);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], expected_value);
                CHECK_EQ(b.front(), expected_value);
                CHECK_EQ(*b.cbegin(), expected_value);
            }

            SUBCASE("value in the middle of the buffer")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);
                constexpr int32_t expected_value = 101;
                b.insert(b.cbegin() + 2, expected_value);
                REQUIRE_EQ(b.size(), size + 1);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], 0);
                CHECK_EQ(b[1], 1);
                CHECK_EQ(b[2], expected_value);
                CHECK_EQ(b[3], 2);
                CHECK_EQ(b[4], 3);
            }

            SUBCASE("move value in the middle of the buffer")
            {
                // CHECK(true);
                constexpr std::size_t size = 4u;
                non_trivial_buffer_test_type b(make_test_buffer_non_trivial(size), size);
                const std::string expected_value = "9999";
                std::string movable_expected_value = expected_value;
                b.insert(b.cbegin() + 2, std::move(movable_expected_value));
                REQUIRE_EQ(b.size(), size + 1);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], "0");
                CHECK_EQ(b[1], "1");
                CHECK_EQ(b[2], expected_value);
                CHECK_EQ(b[3], "2");
                CHECK_EQ(b[4], "3");
            }

            SUBCASE("value at the end of the buffer")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                constexpr int32_t expected_value = 101;
                b.insert(b.cend(), expected_value);
                REQUIRE_EQ(b.size(), size + 1);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], 0);
                CHECK_EQ(b[1], 1);
                CHECK_EQ(b[2], 2);
                CHECK_EQ(b[3], 3);
                CHECK_EQ(b[4], expected_value);
            }

            SUBCASE("move value at the end of the buffer")
            {
                constexpr std::size_t size = 4u;
                non_trivial_buffer_test_type b(make_test_buffer_non_trivial(size), size);
                const std::string expected_value = "9999";
                std::string movable_expected_value = expected_value;
                b.insert(b.cend(), std::move(movable_expected_value));
                REQUIRE_EQ(b.size(), size + 1);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], "0");
                CHECK_EQ(b[1], "1");
                CHECK_EQ(b[2], "2");
                CHECK_EQ(b[3], "3");
                CHECK_EQ(b[4], expected_value);
            }

            SUBCASE("count copies at the beginning")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                constexpr int32_t expected_value = 101;
                constexpr std::size_t count = 3u;
                constexpr std::size_t expected_new_size = size + count;
                b.insert(b.cbegin(), count, expected_value);
                const std::size_t new_size = b.size();
                REQUIRE_EQ(new_size, expected_new_size);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], expected_value);
                CHECK_EQ(b[1], expected_value);
                CHECK_EQ(b[2], expected_value);
                CHECK_EQ(b[3], 0);
                CHECK_EQ(b[4], 1);
                CHECK_EQ(b[5], 2);
                CHECK_EQ(b[6], 3);
            }

            SUBCASE("count copies in the middle")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                constexpr int32_t expected_value = 101;
                constexpr std::size_t count = 3u;
                constexpr std::size_t expected_new_size = size + count;
                b.insert(b.cbegin() + 2, count, expected_value);
                REQUIRE_EQ(b.size(), expected_new_size);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], 0);
                CHECK_EQ(b[1], 1);
                CHECK_EQ(b[2], expected_value);
                CHECK_EQ(b[3], expected_value);
                CHECK_EQ(b[4], expected_value);
                CHECK_EQ(b[5], 2);
                CHECK_EQ(b[6], 3);
            }

            SUBCASE("count copies at the end")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                constexpr int32_t expected_value = 101;
                constexpr std::size_t count = 3u;
                constexpr std::size_t expected_new_size = size + count;
                b.insert(b.cend(), count, expected_value);
                const std::size_t new_size = b.size();
                REQUIRE_EQ(new_size, expected_new_size);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], 0);
                CHECK_EQ(b[1], 1);
                CHECK_EQ(b[2], 2);
                CHECK_EQ(b[3], 3);
                CHECK_EQ(b[4], expected_value);
                CHECK_EQ(b[5], expected_value);
                CHECK_EQ(b[6], expected_value);
            }

            SUBCASE("elements from range [first, last) at the beginning of the buffer")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                const std::vector<int32_t> values = {101, 102, 103};
                const std::size_t expected_new_size = size + values.size();
                b.insert(b.cbegin(), values.cbegin(), values.cend());
                const std::size_t new_size = b.size();
                REQUIRE_EQ(new_size, expected_new_size);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], 101);
                CHECK_EQ(b[1], 102);
                CHECK_EQ(b[2], 103);
                CHECK_EQ(b[3], 0);
                CHECK_EQ(b[4], 1);
                CHECK_EQ(b[5], 2);
                CHECK_EQ(b[6], 3);
            }

            SUBCASE("move elements from range [first, last) at the beginning of the buffer")
            {
                constexpr std::size_t size = 4u;
                non_trivial_buffer_test_type b(make_test_buffer_non_trivial(size), size);
                std::vector<std::string> values = {"101", "102", "103"};
                const std::size_t expected_new_size = size + values.size();
                b.insert(
                    b.cbegin(),
                    std::make_move_iterator(values.begin()),
                    std::make_move_iterator(values.end())
                );
                REQUIRE_EQ(b.size(), expected_new_size);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], "101");
                CHECK_EQ(b[1], "102");
                CHECK_EQ(b[2], "103");
                CHECK_EQ(b[3], "0");
                CHECK_EQ(b[4], "1");
                CHECK_EQ(b[5], "2");
                CHECK_EQ(b[6], "3");
            }

            SUBCASE("elements from range [first, last) in the middle of the buffer")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                const std::vector<int32_t> values = {101, 102, 103};
                const std::size_t expected_new_size = size + values.size();
                b.insert(b.cbegin() + 2, values.cbegin(), values.cend());
                const std::size_t new_size = b.size();
                REQUIRE_EQ(new_size, expected_new_size);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], 0);
                CHECK_EQ(b[1], 1);
                CHECK_EQ(b[2], 101);
                CHECK_EQ(b[3], 102);
                CHECK_EQ(b[4], 103);
                CHECK_EQ(b[5], 2);
                CHECK_EQ(b[6], 3);
            }

            SUBCASE("move elements from range [first, last) in the middle of the buffer")
            {
                constexpr std::size_t size = 4u;
                non_trivial_buffer_test_type b(make_test_buffer_non_trivial(size), size);

                std::vector<std::string> values = {"101", "102", "103"};
                const std::size_t expected_new_size = size + values.size();
                b.insert(
                    b.cbegin() + 2,
                    std::make_move_iterator(values.begin()),
                    std::make_move_iterator(values.end())
                );
                REQUIRE_EQ(b.size(), expected_new_size);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], "0");
                CHECK_EQ(b[1], "1");
                CHECK_EQ(b[2], "101");
                CHECK_EQ(b[3], "102");
                CHECK_EQ(b[4], "103");
                CHECK_EQ(b[5], "2");
                CHECK_EQ(b[6], "3");
            }

            SUBCASE("elements from range [first, last) at the end of the buffer")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                const std::vector<int32_t> values = {101, 102, 103};
                const std::size_t expected_new_size = size + values.size();
                b.insert(b.cend(), values.cbegin(), values.cend());
                const std::size_t new_size = b.size();
                REQUIRE_EQ(new_size, expected_new_size);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], 0);
                CHECK_EQ(b[1], 1);
                CHECK_EQ(b[2], 2);
                CHECK_EQ(b[3], 3);
                CHECK_EQ(b[4], 101);
                CHECK_EQ(b[5], 102);
                CHECK_EQ(b[6], 103);
            }

            SUBCASE("move elements from range [first, last) at the end of the buffer")
            {
                constexpr std::size_t size = 4u;
                non_trivial_buffer_test_type b(make_test_buffer_non_trivial(size), size);

                std::vector<std::string> values = {"101", "102", "103"};
                const std::size_t expected_new_size = size + values.size();
                b.insert(b.cend(), std::make_move_iterator(values.begin()), std::make_move_iterator(values.end()));
                REQUIRE_EQ(b.size(), expected_new_size);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], "0");
                CHECK_EQ(b[1], "1");
                CHECK_EQ(b[2], "2");
                CHECK_EQ(b[3], "3");
                CHECK_EQ(b[4], "101");
                CHECK_EQ(b[5], "102");
                CHECK_EQ(b[6], "103");
            }

            SUBCASE("elements from initializer list at the beginning o the buffer")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                b.insert(b.cbegin(), {101, 102, 103});
                REQUIRE_EQ(b.size(), size + 3);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], 101);
                CHECK_EQ(b[1], 102);
                CHECK_EQ(b[2], 103);
                CHECK_EQ(b[3], 0);
                CHECK_EQ(b[4], 1);
                CHECK_EQ(b[5], 2);
                CHECK_EQ(b[6], 3);
            }

            SUBCASE("elements from initializer list in the middle of the buffer")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                b.insert(b.cbegin() + 2, {101, 102, 103});
                REQUIRE_EQ(b.size(), size + 3);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], 0);
                CHECK_EQ(b[1], 1);
                CHECK_EQ(b[2], 101);
                CHECK_EQ(b[3], 102);
                CHECK_EQ(b[4], 103);
                CHECK_EQ(b[5], 2);
                CHECK_EQ(b[6], 3);
            }

            SUBCASE("elements from initializer list at the end of the buffer")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                b.insert(b.cend(), {101, 102, 103});
                REQUIRE_EQ(b.size(), size + 3);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], 0);
                CHECK_EQ(b[1], 1);
                CHECK_EQ(b[2], 2);
                CHECK_EQ(b[3], 3);
                CHECK_EQ(b[4], 101);
                CHECK_EQ(b[5], 102);
                CHECK_EQ(b[6], 103);
            }

            SUBCASE("pos and range")
            {
                SUBCASE("range of T")
                {
                    constexpr std::size_t size = 4u;
                    buffer_test_type b(make_test_buffer(size), size);

                    const std::vector<int32_t> values = {101, 102, 103};
                    const std::size_t expected_new_size = size + values.size();
                    b.insert(b.cbegin() + 2, values);
                    const std::size_t new_size = b.size();
                    REQUIRE_EQ(new_size, expected_new_size);
                    CHECK_EQ(b[0], 0);
                    CHECK_EQ(b[1], 1);
                    CHECK_EQ(b[2], 101);
                    CHECK_EQ(b[3], 102);
                    CHECK_EQ(b[4], 103);
                    CHECK_EQ(b[5], 2);
                    CHECK_EQ(b[6], 3);
                }
            }
        }

        TEST_CASE("erase")
        {
            SUBCASE("the element at the beginning of the buffer")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                b.erase(b.cbegin());
                REQUIRE_EQ(b.size(), size - 1);
                CHECK_EQ(b.capacity(), size);
                CHECK_EQ(b[0], 1);
                CHECK_EQ(b[1], 2);
                CHECK_EQ(b[2], 3);
            }

            SUBCASE("the element in the middle of the buffer")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                b.erase(b.cbegin() + 2);
                REQUIRE_EQ(b.size(), size - 1);
                CHECK_EQ(b.capacity(), size);
                CHECK_EQ(b[0], 0);
                CHECK_EQ(b[1], 1);
                CHECK_EQ(b[2], 3);
            }

            SUBCASE("the element at the end of the buffer")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                b.erase(b.cend() - 1);
                REQUIRE_EQ(b.size(), size - 1);
                CHECK_EQ(b.capacity(), size);
                CHECK_EQ(b[0], 0);
                CHECK_EQ(b[1], 1);
                CHECK_EQ(b[2], 2);
            }

            SUBCASE("the elements in the range [first, last) at the beginning of the buffer")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                b.erase(b.cbegin(), b.cbegin() + 2);
                REQUIRE_EQ(b.size(), size - 2);
                CHECK_EQ(b.capacity(), size);
                CHECK_EQ(b[0], 2);
                CHECK_EQ(b[1], 3);
            }

            SUBCASE("the elements in the range [first, last) in the middle of the buffer")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                b.erase(b.cbegin() + 1, b.cbegin() + 3);
                REQUIRE_EQ(b.size(), size - 2);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], 0);
                CHECK_EQ(b[1], 3);
            }

            SUBCASE("the elements in the range [first, last) at the end of the buffer")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                b.erase(b.cend() - 2, b.cend());
                REQUIRE_EQ(b.size(), size - 2);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], 0);
                CHECK_EQ(b[1], 1);
            }
        }

        TEST_CASE("push_back")
        {
            SUBCASE("The new element is initialized as a copy of value")
            {
                constexpr std::size_t size = 4u;
                buffer_test_type b(make_test_buffer(size), size);

                constexpr int32_t expected_value = 101;
                b.push_back(expected_value);
                REQUIRE_EQ(b.size(), size + 1);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], 0);
                CHECK_EQ(b[1], 1);
                CHECK_EQ(b[2], 2);
                CHECK_EQ(b[3], 3);
                CHECK_EQ(b[size], expected_value);
                CHECK_EQ(b.back(), expected_value);
                CHECK_EQ(b.cend()[-1], expected_value);
                CHECK_EQ(b.crbegin()[0], expected_value);
            }

            SUBCASE("Value is moved into the new element.")
            {
                constexpr std::size_t size = 4u;
                non_trivial_buffer_test_type b(make_test_buffer_non_trivial(size), size);
                const std::string expected_value = "9999";
                std::string movable_expected_value = expected_value;
                b.push_back(std::move(movable_expected_value));
                REQUIRE_EQ(b.size(), size + 1);
                CHECK_EQ(b.capacity(), b.size() * SPARROW_BUFFER_GROWTH_FACTOR);
                CHECK_EQ(b[0], "0");
                CHECK_EQ(b[1], "1");
                CHECK_EQ(b[2], "2");
                CHECK_EQ(b[3], "3");
                CHECK_EQ(b[size], expected_value);
                CHECK_EQ(b.back(), expected_value);
                CHECK_EQ(b.cend()[-1], expected_value);
                CHECK_EQ(b.crbegin()[0], expected_value);
            }
        }
    }

    TEST_SUITE("buffer_view")
    {
        TEST_CASE("constructors")
        {
            SUBCASE("with ptr and size")
            {
                constexpr std::size_t size = 8u;
                int32_t* mem = make_test_buffer(size);
                const view_test_type v(mem, size);

                CHECK_EQ(v.data(), mem);
                CHECK_EQ(v.size(), size);
                CHECK_EQ(v.data()[2], 2);
            }

            SUBCASE("with buffer")
            {
                constexpr std::size_t size = 8u;
                buffer_test_type b(make_test_buffer(size), size);
                const view_test_type v(b);

                CHECK_EQ(v.data(), b.data());
                CHECK_EQ(v.size(), b.size());
                CHECK_EQ(v.data()[2], 2);
            }

            SUBCASE("with iterators")
            {
                const std::vector<int32_t> values{0, 1, 2, 3, 4, 5, 6, 7};
                const const_view_test_type v(values.cbegin(), values.cend());
                CHECK_EQ(v.size(), values.size());
                for (std::size_t i = 0; i < values.size(); ++i)
                {
                    CHECK_EQ(v[i], values[i]);
                }
            }
        }

        TEST_CASE("copy semantic")
        {
            const std::size_t size = 4u;
            buffer_test_type b(make_test_buffer(size), size);
            view_test_type v1(b);
            view_test_type v2(v1);
            CHECK_EQ(v1, v2);

            const std::size_t size2 = 8u;
            buffer_test_type b2(make_test_buffer(size2, 4), size2);
            view_test_type v3(b2);
            v2 = v3;
            CHECK_EQ(v2, v3);
            CHECK_NE(v1, v3);
        }

        TEST_CASE("move semantic")
        {
            const std::size_t size = 4u;
            buffer_test_type b(make_test_buffer(size), size);
            view_test_type v1(b);
            view_test_type v2(std::move(v1));
            CHECK_EQ(v1, v2);

            const std::size_t size2 = 8u;
            buffer_test_type b2(make_test_buffer(size2, 4), size2);
            view_test_type v3(b2);
            v2 = std::move(v3);
            CHECK_EQ(v2, v3);
        }

        TEST_CASE("empty")
        {
            view_test_type v1(nullptr, 0u);
            CHECK(v1.empty());

            const std::size_t size = 4u;
            buffer_test_type b(make_test_buffer(size), size);
            view_test_type v2(b);
            CHECK(!v2.empty());
        }

        TEST_CASE("size")
        {
            constexpr std::size_t size = 4u;
            buffer_test_type b(make_test_buffer(size), size);
            view_test_type v(b);
            CHECK_EQ(v.size(), size);
        }

        TEST_CASE("front")
        {
            const std::size_t size = 4u;
            const int32_t expected_value = 3;
            buffer_test_type b(make_test_buffer(size, expected_value), size);
            view_test_type v(b);
            CHECK_EQ(v.front(), expected_value);
        }

        TEST_CASE("front const")
        {
            const std::size_t size = 4u;
            const int32_t expected_value = 3;
            buffer_test_type b(make_test_buffer(size, expected_value), size);
            const view_test_type v(b);
            CHECK_EQ(v.front(), expected_value);
        }

        TEST_CASE("back")
        {
            const std::size_t size = 4u;
            const int32_t expected_value = 6;
            buffer_test_type b(make_test_buffer(size, expected_value), size);
            view_test_type v(b);
            CHECK_EQ(v.back(), expected_value + 3u);
        }

        TEST_CASE("back const")
        {
            const std::size_t size = 4u;
            const int32_t expected_value = 6;
            buffer_test_type b(make_test_buffer(size, expected_value), size);
            const view_test_type v(b);
            CHECK_EQ(v.back(), expected_value + 3u);
        }

        TEST_CASE("data")
        {
            const std::size_t size = 4u;
            buffer_test_type b(make_test_buffer(size), size);
            view_test_type v1(b);

            const int32_t expected_value = 101;
            const std::size_t idx = 3u;
            b.data()[idx] = expected_value;
            CHECK_EQ(v1.data()[idx], expected_value);

            view_test_type v2(v1);
            CHECK_EQ(v2.data()[idx], expected_value);

            view_test_type v3(std::move(v1));
            CHECK_EQ(v3.data()[idx], expected_value);
        }

        TEST_CASE("operator[]")
        {
            const std::size_t size = 4u;
            buffer_test_type b(make_test_buffer(size), size);
            view_test_type v(b);
            for (std::size_t i = 0; i < size; ++i)
            {
                CHECK_EQ(v[i], i);
            }
        }

        TEST_CASE("operator[] const")
        {
            const std::size_t size = 4u;
            buffer_test_type b(make_test_buffer(size), size);
            const view_test_type v(b);
            for (std::size_t i = 0; i < size; ++i)
            {
                CHECK_EQ(v[i], i);
            }
        }

        TEST_CASE("begin")
        {
            const std::size_t size = 8u;
            buffer_test_type b(make_test_buffer(size), size);
            view_test_type v(b);
            auto iter = v.begin();
            for (std::size_t i = 0; i < v.size(); ++i)
            {
                CHECK_EQ(*iter++, i);
            }
        }

        TEST_CASE("begin const")
        {
            const std::size_t size = 8u;
            buffer_test_type b(make_test_buffer(size), size);
            const view_test_type v(b);
            auto iter = v.begin();
            for (std::size_t i = 0; i < v.size(); ++i)
            {
                CHECK_EQ(*iter++, i);
            }
        }

        TEST_CASE("end")
        {
            const std::size_t size = 8u;
            buffer_test_type b(make_test_buffer(size), size);
            view_test_type v(b);
            auto iter = v.end();
            for (std::size_t i = 0; i < v.size(); ++i)
            {
                CHECK_EQ(*--iter, size - i - 1);
            }
        }

        TEST_CASE("end const")
        {
            const std::size_t size = 8u;
            buffer_test_type b(make_test_buffer(size), size);
            const view_test_type v(b);
            auto iter = v.end();
            for (std::size_t i = 0; i < v.size(); ++i)
            {
                CHECK_EQ(*--iter, size - i - 1);
            }
        }

        TEST_CASE("rbegin")
        {
            const std::size_t size = 8u;
            buffer_test_type b(make_test_buffer(size), size);
            view_test_type v(b);
            auto iter = v.rbegin();
            for (std::size_t i = 0; i < v.size(); ++i)
            {
                CHECK_EQ(*iter++, size - i - 1);
            }
        }

        TEST_CASE("rbegin const")
        {
            const std::size_t size = 8u;
            buffer_test_type b(make_test_buffer(size), size);
            const view_test_type v(b);
            auto iter = v.rbegin();
            for (std::size_t i = 0; i < v.size(); ++i)
            {
                CHECK_EQ(*iter++, size - i - 1);
            }
        }

        TEST_CASE("rend")
        {
            const std::size_t size = 8u;
            buffer_test_type b(make_test_buffer(size), size);
            view_test_type v(b);
            auto iter = v.rend();
            for (std::size_t i = 0; i < v.size(); ++i)
            {
                CHECK_EQ(*--iter, i);
            }
        }

        TEST_CASE("rend const")
        {
            const std::size_t size = 8u;
            buffer_test_type b(make_test_buffer(size), size);
            const view_test_type v(b);
            auto iter = v.rend();
            for (std::size_t i = 0; i < v.size(); ++i)
            {
                CHECK_EQ(*--iter, i);
            }
        }

        TEST_CASE("equality comparison")
        {
            const std::size_t size = 4u;
            buffer_test_type b1(make_test_buffer(size), size);
            buffer_test_type b2(make_test_buffer(size), size);
            view_test_type v1(b1), v2(b2);
            CHECK(v1 == v2);

            const std::size_t size2 = 8u;
            buffer_test_type b3(make_test_buffer(size2), size2);
            view_test_type v3(b3);
            CHECK(v1 != v3);
        }

        TEST_CASE("swap")
        {
            const std::size_t size1 = 4u;
            const std::size_t size2 = 8u;

            buffer_test_type b1(make_test_buffer(size1), size1);
            buffer_test_type b2(make_test_buffer(size2), size2);
            view_test_type v1(b1), v2(b2);
            auto* data1 = b1.data();
            auto* data2 = b2.data();
            v1.swap(v2);
            CHECK_EQ(v1.size(), size2);
            CHECK_EQ(v1.data(), data2);
            CHECK_EQ(v2.size(), size1);
            CHECK_EQ(v2.data(), data1);
        }

        TEST_CASE("subrange")
        {
            SUBCASE("pos and count")
            {
                const std::size_t size = 8u;
                buffer_test_type b(make_test_buffer(size), size);
                view_test_type v(b);
                const std::size_t pos = 2u;
                const std::size_t count = 3u;
                const view_test_type sv = v.subrange(pos, count);
                CHECK_EQ(sv.size(), count);
                for (std::size_t i = 0; i < count; ++i)
                {
                    CHECK_EQ(sv[i], i + pos);
                }
            }

            SUBCASE("pos")
            {
                const std::size_t size = 8u;
                buffer_test_type b(make_test_buffer(size), size);
                view_test_type v(b);
                const std::size_t pos = 2u;
                const view_test_type sv = v.subrange(pos);
                CHECK_EQ(sv.size(), size - pos);
                for (std::size_t i = 0; i < size - pos; ++i)
                {
                    CHECK_EQ(sv[i], i + pos);
                }
            }

            SUBCASE("iterators")
            {
                const std::size_t size = 8u;
                buffer_test_type b(make_test_buffer(size), size);
                view_test_type v(b);
                const std::size_t pos = 2u;
                const std::size_t count = 3u;
                const view_test_type sv(std::next(v.begin(), pos), std::next(v.begin(), pos + count));
                CHECK_EQ(sv.size(), count);
                for (std::size_t i = 0; i < count; ++i)
                {
                    CHECK_EQ(sv[i], i + pos);
                }
            }

            SUBCASE("const iterators")
            {
                const std::size_t size = 8u;
                buffer_test_type b(make_test_buffer(size), size);
                const view_test_type v(b);
                const std::size_t pos = 2u;
                const std::size_t count = 3u;
                const const_view_test_type sv(std::next(v.cbegin(), pos), std::next(v.cbegin(), pos + count));
                CHECK_EQ(sv.size(), count);
                for (std::size_t i = 0; i < count; ++i)
                {
                    CHECK_EQ(sv[i], i + pos);
                }
            }
        }
    }
}
