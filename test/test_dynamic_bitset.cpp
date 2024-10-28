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
#include <ranges>

#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/buffer/dynamic_bitset/non_owning_dynamic_bitset.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    static constexpr std::size_t s_bitmap_size = 29;
    static constexpr std::size_t s_bitmap_null_count = 15;
    static constexpr std::array<uint8_t, 4> s_bitmap_blocks_values{
        {0b00100110, 0b01010101, 0b00110101, 0b00000111}
    };

    using testing_types = std::tuple<dynamic_bitset<std::uint8_t>, non_owning_dynamic_bitset<std::uint8_t>>;

    struct dynamic_bitmap_fixture
    {
        using buffer_type = std::uint8_t*;

        template <std::ranges::input_range R>
        dynamic_bitmap_fixture(const R& blocks)
            : p_buffer(new std::uint8_t[blocks.size()])
            , p_expected_buffer(p_buffer)
        {
            std::copy(blocks.begin(), blocks.end(), p_buffer);
        }

        dynamic_bitmap_fixture()
            : dynamic_bitmap_fixture(s_bitmap_blocks_values)
        {
        }

        ~dynamic_bitmap_fixture()
        {
            delete[] p_buffer;
            p_expected_buffer = nullptr;
        }

        buffer_type get_buffer()
        {
            buffer_type res = p_buffer;
            p_buffer = nullptr;
            return res;
        }

        dynamic_bitmap_fixture(const dynamic_bitmap_fixture&) = delete;
        dynamic_bitmap_fixture(dynamic_bitmap_fixture&&) = delete;

        dynamic_bitmap_fixture& operator=(const dynamic_bitmap_fixture&) = delete;
        dynamic_bitmap_fixture& operator=(dynamic_bitmap_fixture&&) = delete;

        buffer_type p_buffer;
        buffer_type p_expected_buffer;
    };

    struct non_owning_dynamic_bitset_fixture
    {
        non_owning_dynamic_bitset_fixture()
            : non_owning_dynamic_bitset_fixture(s_bitmap_blocks_values)
        {
        }

        template <std::ranges::input_range R>
        non_owning_dynamic_bitset_fixture(const R& blocks)
            : m_bitmap_buffer{blocks}
            , p_expected_buffer{m_bitmap_buffer.data()}
        {
        }

        buffer<uint8_t>* get_buffer()
        {
            return &m_bitmap_buffer;
        }

        buffer<uint8_t> m_bitmap_buffer;
        std::uint8_t* p_expected_buffer;
    };

    TEST_SUITE("dynamic_bitset")
    {
        TEST_CASE_TEMPLATE_DEFINE("", bitmap, dynamic_bitset_id)
        {
            // type "fixture" is dynamic_bitmap_fixture if "bitmap" is dynamic_bitset<std::uint8_t> else
            // non_owning_dynamic_bitset_fixture
            using fixture = std::conditional_t<
                std::is_same_v<bitmap, dynamic_bitset<std::uint8_t>>,
                dynamic_bitmap_fixture,
                non_owning_dynamic_bitset_fixture>;

            fixture f;

            SUBCASE("constructor")
            {
                if constexpr (std::is_same_v<bitmap, dynamic_bitset<std::uint8_t>>)
                {
                    const bitmap b1;
                    CHECK_EQ(b1.size(), 0u);
                    CHECK_EQ(b1.null_count(), 0u);

                    const std::size_t expected_size = 13;
                    const bitmap b2(expected_size);
                    CHECK_EQ(b2.size(), expected_size);
                    CHECK_EQ(b2.null_count(), expected_size);

                    const bitmap b3(expected_size, true);
                    CHECK_EQ(b3.size(), expected_size);
                    CHECK_EQ(b3.null_count(), 0u);

                    dynamic_bitmap_fixture bf;
                    const bitmap b4(bf.get_buffer(), s_bitmap_size);
                    CHECK_EQ(b4.size(), s_bitmap_size);
                    CHECK_EQ(b4.null_count(), s_bitmap_null_count);

                    dynamic_bitmap_fixture bf2;
                    const bitmap b5(bf2.get_buffer(), s_bitmap_size, s_bitmap_null_count);
                    CHECK_EQ(b5.size(), s_bitmap_size);
                    CHECK_EQ(b5.null_count(), s_bitmap_null_count);
                }
                else if constexpr (std::is_same_v<bitmap, non_owning_dynamic_bitset<std::uint8_t>>)
                {
                    non_owning_dynamic_bitset_fixture bf;
                    bitmap b(bf.get_buffer(), s_bitmap_size);
                    CHECK_EQ(b.size(), s_bitmap_size);
                    CHECK_EQ(b.null_count(), s_bitmap_null_count);
                }
            }

            SUBCASE("data")
            {
                const bitmap b(f.get_buffer(), s_bitmap_size);
                CHECK_EQ(b.data(), f.p_expected_buffer);

                const bitmap& b2 = b;
                CHECK_EQ(b2.data(), f.p_expected_buffer);
            }


            SUBCASE("copy semantic")
            {
                const bitmap b(f.get_buffer(), s_bitmap_size);
                bitmap b2(b);

                REQUIRE_EQ(b.size(), b2.size());
                CHECK_EQ(b.null_count(), b2.null_count());

                if constexpr (std::is_same_v<bitmap, dynamic_bitset<std::uint8_t>>)
                {
                    CHECK_NE(b.data(), b2.data());
                }
                else if constexpr (std::is_same_v<bitmap, non_owning_dynamic_bitset<std::uint8_t>>)
                {
                    CHECK_EQ(b.data(), b2.data());
                }

                for (size_t i = 0; i < s_bitmap_blocks_values.size(); ++i)
                {
                    CHECK_EQ(b.data()[i], b2.data()[i]);
                }

                const std::array<std::uint8_t, 2> blocks{37, 2};
                fixture f3{blocks};
                bitmap b3(f3.get_buffer(), blocks.size() * 8);

                b2 = b3;
                REQUIRE_EQ(b2.size(), b3.size());
                CHECK_EQ(b2.null_count(), b3.null_count());

                if constexpr (std::is_same_v<bitmap, dynamic_bitset<std::uint8_t>>)
                {
                    CHECK_NE(b2.data(), b3.data());
                }
                else if constexpr (std::is_same_v<bitmap, non_owning_dynamic_bitset<std::uint8_t>>)
                {
                    CHECK_EQ(b2.data(), b3.data());
                }

                for (size_t i = 0; i < blocks.size(); ++i)
                {
                    CHECK_EQ(b2.data()[i], b3.data()[i]);
                }
            }

            SUBCASE("move semantic")
            {
                bitmap bref(f.get_buffer(), s_bitmap_size);
                bitmap b(bref);

                bitmap b2(std::move(b));
                REQUIRE_EQ(b2.size(), bref.size());
                CHECK_EQ(b2.null_count(), bref.null_count());
                for (size_t i = 0; i < s_bitmap_blocks_values.size(); ++i)
                {
                    CHECK_EQ(b2.data()[i], bref.data()[i]);
                }

                const std::array<std::uint8_t, 2> blocks{37, 2};
                fixture f4{blocks};
                bitmap b4(f4.get_buffer(), blocks.size() * 8);
                bitmap b5(b4);

                b2 = std::move(b4);
                REQUIRE_EQ(b2.size(), b5.size());
                CHECK_EQ(b2.null_count(), b5.null_count());
                for (size_t i = 0; i < blocks.size(); ++i)
                {
                    CHECK_EQ(b2.data()[i], b5.data()[i]);
                }
            }

            SUBCASE("test/set")
            {
                bitmap bm(f.get_buffer(), s_bitmap_size);

                CHECK(bm.test(2));
                CHECK_FALSE(bm.test(3));
                CHECK(bm.test(24));

                bm.set(3, true);
                CHECK_EQ(bm.data()[0], 46);
                CHECK_EQ(bm.null_count(), s_bitmap_null_count - 1);

                bm.set(24, 0);
                CHECK_EQ(bm.data()[3], 6);
                CHECK_EQ(bm.null_count(), s_bitmap_null_count);

                // Ensures that setting false again does not alter the null count
                bm.set(24, 0);
                CHECK_EQ(bm.data()[3], 6);
                CHECK_EQ(bm.null_count(), s_bitmap_null_count);

                bm.set(2, true);
                CHECK(bm.test(2));
                CHECK_EQ(bm.null_count(), s_bitmap_null_count);
            }

            SUBCASE("operator[]")
            {
                bitmap bm(f.get_buffer(), s_bitmap_size);
                const bitmap& cbm = bm;
                CHECK(cbm[2]);
                CHECK_FALSE(cbm[3]);
                CHECK(cbm[24]);

                bm.set(3, true);
                CHECK_EQ(bm.data()[0], 46);
                CHECK_EQ(bm.null_count(), s_bitmap_null_count - 1);

                bm[24] = false;
                CHECK_EQ(cbm.data()[3], 6);
                CHECK_EQ(cbm.null_count(), s_bitmap_null_count);

                // Ensures that setting false again does not alter the null count
                bm[24] = false;
                CHECK_EQ(bm.data()[3], 6);
                CHECK_EQ(bm.null_count(), s_bitmap_null_count);

                bm[2] = true;
                CHECK(bm.test(2));
                CHECK_EQ(bm.null_count(), s_bitmap_null_count);
            }

            SUBCASE("resize")
            {
                bitmap b(f.get_buffer(), s_bitmap_size);
                b.resize(33);
                CHECK_EQ(b.size(), 33);
                CHECK_EQ(b.null_count(), s_bitmap_null_count + 4);

                // Test shrinkage
                b.resize(29);
                CHECK_EQ(b.size(), 29);
                CHECK_EQ(b.null_count(), s_bitmap_null_count);

                // Test expansion
                b.resize(35);
                CHECK_EQ(b.size(), 35);
                CHECK_EQ(b.null_count(), s_bitmap_null_count + 6);

                // Test expansion with a value
                b.resize(40, true);
                CHECK_EQ(b.size(), 40);
                CHECK_EQ(b.null_count(), s_bitmap_null_count + 6);
            }

            SUBCASE("iterator")
            {
                // Does not work because the reference is not bool&
                // static_assert(std::random_access_iterator<typename bitmap::iterator>);
                static_assert(std::random_access_iterator<typename bitmap::const_iterator>);

                bitmap b(f.get_buffer(), s_bitmap_size);
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

                auto iter_end = std::next(b.begin(), static_cast<bitmap::iterator::difference_type>(b.size()));
                auto citer_end = std::next(b.cbegin(), static_cast<bitmap::iterator::difference_type>(b.size()));
                CHECK_EQ(iter_end, b.end());
                CHECK_EQ(citer_end, b.cend());
            };

            SUBCASE("insert")
            {
                SUBCASE("const_iterator and value_type")
                {
                    SUBCASE("begin")
                    {
                        bitmap b(f.get_buffer(), s_bitmap_size);
                        const auto pos = b.cbegin();
                        auto iter = b.insert(pos, false);
                        CHECK_EQ(b.size(), s_bitmap_size + 1);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count + 1);
                        CHECK_EQ(*iter, false);

                        iter = b.insert(pos, true);
                        CHECK_EQ(b.size(), s_bitmap_size + 2);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count + 1);
                        CHECK_EQ(*iter, true);
                    }

                    SUBCASE("middle")
                    {
                        bitmap b(f.get_buffer(), s_bitmap_size);
                        const auto pos = std::next(b.cbegin(), 14);
                        auto iter = b.insert(pos, false);
                        CHECK_EQ(b.size(), s_bitmap_size + 1);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count + 1);
                        CHECK_EQ(*iter, false);

                        iter = b.insert(pos, true);
                        CHECK_EQ(b.size(), s_bitmap_size + 2);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count + 1);
                        CHECK_EQ(*iter, true);
                    }

                    SUBCASE("end")
                    {
                        bitmap b(f.get_buffer(), s_bitmap_size);
                        const auto pos = b.cend();
                        auto iter = b.insert(pos, false);
                        CHECK_EQ(b.size(), s_bitmap_size + 1);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count + 1);
                        CHECK_EQ(*iter, false);

                        iter = b.insert(pos, true);
                        CHECK_EQ(b.size(), s_bitmap_size + 2);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count + 1);
                        CHECK_EQ(*iter, true);
                    }
                }

                SUBCASE("const_iterator and count/value_type")
                {
                    SUBCASE("begin")
                    {
                        bitmap b(f.get_buffer(), s_bitmap_size);
                        const auto pos = b.cbegin();
                        auto iter = b.insert(pos, 3, false);
                        CHECK_EQ(b.size(), s_bitmap_size + 3);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count + 3);
                        CHECK_EQ(*iter, false);
                        CHECK_EQ(*(++iter), false);
                        CHECK_EQ(*(++iter), false);

                        iter = b.insert(pos, 3, true);
                        CHECK_EQ(b.size(), s_bitmap_size + 6);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count + 3);
                        CHECK_EQ(*iter, true);
                        CHECK_EQ(*(++iter), true);
                        CHECK_EQ(*(++iter), true);
                    }

                    SUBCASE("middle")
                    {
                        bitmap b(f.get_buffer(), s_bitmap_size);
                        const auto pos = std::next(b.cbegin(), 14);
                        auto iter = b.insert(pos, 3, false);
                        CHECK_EQ(b.size(), s_bitmap_size + 3);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count + 3);
                        CHECK_EQ(*iter, false);
                        CHECK_EQ(*(++iter), false);
                        CHECK_EQ(*(++iter), false);

                        iter = b.insert(pos, 3, true);
                        CHECK_EQ(b.size(), s_bitmap_size + 6);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count + 3);
                        CHECK_EQ(*iter, true);
                        CHECK_EQ(*(++iter), true);
                        CHECK_EQ(*(++iter), true);
                    }

                    SUBCASE("end")
                    {
                        bitmap b(f.get_buffer(), s_bitmap_size);
                        auto iter = b.insert(b.cend(), 3, false);
                        CHECK_EQ(b.size(), s_bitmap_size + 3);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count + 3);
                        CHECK_EQ(*iter, false);
                        CHECK_EQ(*(++iter), false);
                        CHECK_EQ(*(++iter), false);

                        iter = b.insert(b.cend(), 3, true);
                        CHECK_EQ(b.size(), s_bitmap_size + 6);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count + 3);
                        CHECK_EQ(*iter, true);
                        CHECK_EQ(*(++iter), true);
                        CHECK_EQ(*(++iter), true);
                    }
                }
            }

            SUBCASE("emplace")
            {
                SUBCASE("begin")
                {
                    bitmap b(f.get_buffer(), s_bitmap_size);
                    auto iter = b.emplace(b.cbegin(), true);
                    CHECK_EQ(b.size(), s_bitmap_size + 1);
                    CHECK_EQ(b.null_count(), s_bitmap_null_count);
                    CHECK_EQ(*iter, true);
                }

                SUBCASE("middle")
                {
                    bitmap b(f.get_buffer(), s_bitmap_size);
                    auto iter = b.emplace(std::next(b.cbegin()), true);
                    CHECK_EQ(b.size(), s_bitmap_size + 1);
                    CHECK_EQ(b.null_count(), s_bitmap_null_count);
                    CHECK_EQ(*iter, true);
                }

                SUBCASE("end")
                {
                    bitmap b(f.get_buffer(), s_bitmap_size);
                    auto iter = b.emplace(b.cend(), true);
                    CHECK_EQ(b.size(), s_bitmap_size + 1);
                    CHECK_EQ(b.null_count(), s_bitmap_null_count);
                    CHECK_EQ(*iter, true);
                }
            }

            SUBCASE("erase")
            {
                SUBCASE("const_iterator")
                {
                    SUBCASE("begin")
                    {
                        bitmap b(f.get_buffer(), s_bitmap_size);
                        auto iter = b.erase(b.cbegin());
                        CHECK_EQ(b.size(), s_bitmap_size - 1);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count - 1);
                        CHECK_EQ(iter, b.begin());
                        CHECK_EQ(*iter, true);
                    }

                    SUBCASE("middle")
                    {
                        bitmap b(f.get_buffer(), s_bitmap_size);
                        const auto pos = std::next(b.cbegin(), 2);
                        auto iter = b.erase(pos);
                        CHECK_EQ(b.size(), s_bitmap_size - 1);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count);
                        CHECK_EQ(iter, std::next(b.begin(), 2));
                        CHECK_EQ(*iter, false);
                    }
                }

                SUBCASE("const_iterator range")
                {
                    SUBCASE("begin")
                    {
                        bitmap b(f.get_buffer(), s_bitmap_size);
                        auto iter = b.erase(b.cbegin(), std::next(b.cbegin()));
                        CHECK_EQ(b.size(), s_bitmap_size - 1);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count - 1);
                        CHECK_EQ(*iter, true);
                    }

                    SUBCASE("middle")
                    {
                        bitmap b(f.get_buffer(), s_bitmap_size);
                        const auto pos = std::next(b.cbegin());
                        auto iter = b.erase(pos, std::next(pos, 1));
                        CHECK_EQ(b.size(), s_bitmap_size - 1);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count);
                        CHECK_EQ(*iter, true);
                    }

                    SUBCASE("all")
                    {
                        bitmap b(f.get_buffer(), s_bitmap_size);
                        auto iter = b.erase(b.cbegin(), b.cend());
                        CHECK_EQ(b.size(), 0);
                        CHECK_EQ(b.null_count(), 0);
                        CHECK_EQ(iter, b.end());
                    }
                }
            }

            SUBCASE("at")
            {
                const bitmap b(f.get_buffer(), s_bitmap_size);
                CHECK_EQ(b.at(0), false);
                CHECK_EQ(b.at(1), true);
                CHECK_EQ(b.at(2), true);
                CHECK_THROWS_AS(b.at(s_bitmap_size + 1), std::out_of_range);
            }

            SUBCASE("front")
            {
                const bitmap b(f.get_buffer(), s_bitmap_size);
                CHECK_EQ(b.front(), false);
            }

            SUBCASE("back")
            {
                const bitmap b(f.get_buffer(), s_bitmap_size);
                CHECK_EQ(b.back(), false);
            }

            SUBCASE("push_back")
            {
                bitmap b(f.get_buffer(), s_bitmap_size);
                b.push_back(false);
                CHECK_EQ(b.size(), s_bitmap_size + 1);
                CHECK_EQ(b.null_count(), s_bitmap_null_count + 1);
                CHECK_EQ(b.back(), false);
            }

            SUBCASE("pop_back")
            {
                bitmap b(f.get_buffer(), s_bitmap_size);
                b.pop_back();
                CHECK_EQ(b.size(), s_bitmap_size - 1);
                CHECK_EQ(b.null_count(), s_bitmap_null_count - 1);
            }

            SUBCASE("bitset_reference")
            {
                // as a reminder: p_buffer[0] = 38; // 00100110
                bitmap b(f.get_buffer(), s_bitmap_size);
                auto iter = b.begin();
                *iter = true;
                CHECK_EQ(b.null_count(), s_bitmap_null_count - 1);

                ++iter;
                *iter &= false;
                CHECK_EQ(b.null_count(), s_bitmap_null_count);

                iter += 2;
                *iter |= true;
                CHECK_EQ(b.null_count(), s_bitmap_null_count - 1);

                ++iter;
                *iter ^= true;
                CHECK_EQ(b.null_count(), s_bitmap_null_count - 2);

                CHECK_EQ(*iter, *iter);
                CHECK_NE(*iter, *++b.begin());

                CHECK_EQ(*iter, true);
                CHECK_EQ(true, *iter);

                CHECK_NE(*iter, false);
                CHECK_NE(false, *iter);
            }
        }

        TEST_CASE_TEMPLATE_APPLY(dynamic_bitset_id, testing_types);
    }
}
