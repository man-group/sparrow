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
#include <cstddef>
#include <cstdint>
#include <ranges>

#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/buffer/dynamic_bitset/non_owning_dynamic_bitset.hpp"
#include "sparrow/buffer/dynamic_bitset/null_count_policy.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    static_assert(validity_bitmap_input<validity_bitmap> == true);
    static_assert(validity_bitmap_input<const validity_bitmap&> == true);
    static_assert(validity_bitmap_input<non_owning_dynamic_bitset<std::uint8_t>> == true);

    static_assert(validity_bitmap_input<std::vector<bool>> == true);
    static_assert(validity_bitmap_input<const std::vector<bool>&> == true);
    static_assert(validity_bitmap_input<std::vector<bool>&&> == true);

    static_assert(validity_bitmap_input<std::vector<std::uint8_t>> == true);
    static_assert(validity_bitmap_input<const std::vector<std::uint8_t>&> == true);
    static_assert(validity_bitmap_input<std::vector<std::uint8_t>&&> == true);

    static_assert(validity_bitmap_input<std::vector<std::uint16_t>> == true);
    static_assert(validity_bitmap_input<const std::vector<std::uint16_t>&> == true);
    static_assert(validity_bitmap_input<std::vector<std::uint16_t>&&> == true);

    static_assert(validity_bitmap_input<std::string> == false);
    static_assert(validity_bitmap_input<const std::string&> == false);
    static_assert(validity_bitmap_input<std::string&&> == false);

    static_assert(validity_bitmap_input<std::string_view> == false);
    static_assert(validity_bitmap_input<const std::string_view&> == false);
    static_assert(validity_bitmap_input<std::string_view&&> == false);

    static_assert(validity_bitmap_input<char*> == false);
    static_assert(validity_bitmap_input<const char*> == false);
    static_assert(validity_bitmap_input<char*&&> == false);

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
            : p_buffer(std::allocator<uint8_t>().allocate(blocks.size()))
            , p_expected_buffer(p_buffer)
        {
            std::copy(blocks.begin(), blocks.end(), p_buffer);
        }

        dynamic_bitmap_fixture()
            : dynamic_bitmap_fixture(s_bitmap_blocks_values)
        {
        }

        dynamic_bitmap_fixture(std::nullptr_t)
            : p_buffer(nullptr)
            , p_expected_buffer(p_buffer)
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

        // Helper to create dynamic_bitset with correct constructor
        template <typename BitmapType>
        BitmapType make_bitmap(buffer_type buffer, std::size_t size) const
        {
            return BitmapType(buffer, size, typename BitmapType::default_allocator());
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
            : m_bitmap_buffer{blocks, buffer<uint8_t>::default_allocator()}
            , p_expected_buffer{m_bitmap_buffer.data()}
        {
        }

        non_owning_dynamic_bitset_fixture(std::nullptr_t)
            : m_bitmap_buffer{nullptr, 0, buffer<uint8_t>::default_allocator()}
            , p_expected_buffer{nullptr}
        {
        }

        buffer<uint8_t>* get_buffer()
        {
            return &m_bitmap_buffer;
        }

        // Helper to create non_owning_dynamic_bitset with correct constructor (no allocator)
        template <typename BitmapType>
        BitmapType make_bitmap(buffer<uint8_t>* buffer, std::size_t size) const
        {
            return BitmapType(buffer, size);
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
            fixture null_f(nullptr);

            // Helper lambda to create bitmap with correct constructor signature
            // For dynamic_bitset: needs allocator parameter
            // For non_owning_dynamic_bitset: no allocator parameter
            auto make_bitmap = [](auto buffer, std::size_t size, [[maybe_unused]] auto allocator)
            {
                if constexpr (std::is_same_v<bitmap, dynamic_bitset<std::uint8_t>>)
                {
                    return bitmap(buffer, size, allocator);
                }
                else
                {
                    return bitmap(buffer, size);
                }
            };

            // Helper for dynamic_bitset with null_count (only for dynamic_bitset)
            auto make_bitmap_with_null_count =
                [](auto buffer, std::size_t size, std::size_t null_count, [[maybe_unused]] auto allocator)
            {
                if constexpr (std::is_same_v<bitmap, dynamic_bitset<std::uint8_t>>)
                {
                    return bitmap(buffer, size, null_count, allocator);
                }
                else
                {
                    // non_owning_dynamic_bitset doesn't support this constructor
                    return bitmap(buffer, size);
                }
            };

            SUBCASE("constructor")
            {
                if constexpr (std::is_same_v<bitmap, dynamic_bitset<std::uint8_t>>)
                {
                    SUBCASE("default")
                    {
                        const bitmap b{typename dynamic_bitset<std::uint8_t>::default_allocator()};
                        CHECK_EQ(b.size(), 0u);
                        CHECK_EQ(b.null_count(), 0u);
                    }

                    SUBCASE("with size")
                    {
                        const std::size_t expected_size = 13;
                        const bitmap b(expected_size, typename dynamic_bitset<std::uint8_t>::default_allocator());
                        CHECK_EQ(b.size(), expected_size);
                        CHECK_EQ(b.null_count(), expected_size);
                    }

                    SUBCASE("with size and value")
                    {
                        const std::size_t expected_size = 13;
                        const bitmap b(
                            expected_size,
                            true,
                            typename dynamic_bitset<std::uint8_t>::default_allocator()
                        );
                        CHECK_EQ(b.size(), expected_size);
                        CHECK_EQ(b.null_count(), 0u);
                    }

                    SUBCASE("with buffer and size")
                    {
                        dynamic_bitmap_fixture bf;
                        const bitmap b = make_bitmap(bf.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                        CHECK_EQ(b.size(), s_bitmap_size);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count);
                    }

                    SUBCASE("with buffer, size and null count")
                    {
                        dynamic_bitmap_fixture bf2;
                        const bitmap b5 = make_bitmap_with_null_count(
                            bf2.get_buffer(),
                            s_bitmap_size,
                            s_bitmap_null_count,
                            std::allocator<uint8_t>()
                        );
                        CHECK_EQ(b5.size(), s_bitmap_size);
                        CHECK_EQ(b5.null_count(), s_bitmap_null_count);
                    }
                }
                else if constexpr (std::is_same_v<bitmap, non_owning_dynamic_bitset<std::uint8_t>>)
                {
                    non_owning_dynamic_bitset_fixture bf;
                    bitmap b = make_bitmap(bf.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    CHECK_EQ(b.size(), s_bitmap_size);
                    CHECK_EQ(b.null_count(), s_bitmap_null_count);
                }
            }

            SUBCASE("data")
            {
                SUBCASE("from non-null buffer")
                {
                    bitmap bm = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    CHECK_EQ(bm.size(), s_bitmap_size);
                    CHECK_EQ(bm.null_count(), s_bitmap_null_count);
                    CHECK_EQ(bm.data(), f.p_expected_buffer);
                }

                SUBCASE("from null buffer")
                {
                    bitmap bm = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    CHECK_EQ(bm.size(), s_bitmap_size);
                    CHECK_EQ(bm.null_count(), 0);
                    CHECK_EQ(bm.data(), nullptr);
                }

                SUBCASE("from copy")
                {
                    bitmap bm = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    const bitmap& b2 = bm;
                    CHECK_EQ(b2.data(), f.p_expected_buffer);
                }
            }

            SUBCASE("copy semantic")
            {
                const bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
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
                bitmap b3 = make_bitmap(f3.get_buffer(), blocks.size() * 8, std::allocator<uint8_t>());

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
                bitmap bref = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
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
                bitmap b4 = make_bitmap(f4.get_buffer(), blocks.size() * 8, std::allocator<uint8_t>());
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
                SUBCASE("from null buffer")
                {
                    bitmap bm = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    CHECK_EQ(bm.size(), s_bitmap_size);
                    CHECK_EQ(bm.null_count(), 0);
                    for (size_t i = 0; i < s_bitmap_size; ++i)
                    {
                        CHECK(bm.test(i));
                    }
                    CHECK_EQ(bm.data(), nullptr);
                    CHECK_EQ(bm.null_count(), 0);
                    CHECK_EQ(bm.size(), s_bitmap_size);

                    bm.set(2, true);
                    CHECK_EQ(bm.data(), nullptr);
                    if constexpr (std::is_same_v<bitmap, dynamic_bitset<std::uint8_t>>)
                    {
                        bm.set(3, false);
                        CHECK_NE(bm.data(), nullptr);
                        CHECK_EQ(bm.null_count(), 1);
                        CHECK_EQ(bm.size(), s_bitmap_size);
                        CHECK_FALSE(bm.test(3));
                    }
                }

                SUBCASE("from non-null buffer")
                {
                    bitmap bm = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());

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
            }

            SUBCASE("operator[]")
            {
                SUBCASE("from null buffer")
                {
                    bitmap bm = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    REQUIRE_EQ(bm.size(), s_bitmap_size);
                    for (size_t i = 0; i < s_bitmap_size; ++i)
                    {
                        CHECK(bm[i]);
                    }
                    bm[2] = true;
                    if constexpr (std::is_same_v<bitmap, dynamic_bitset<std::uint8_t>>)
                    {
                        bm[3] = false;
                        CHECK_NE(bm.data(), nullptr);
                        CHECK_EQ(bm.null_count(), 1);
                        CHECK_EQ(bm.size(), s_bitmap_size);
                        CHECK_FALSE(bm[3]);
                    }
                }

                SUBCASE("from non-null buffer")
                {
                    bitmap bm = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    CHECK(bm[2]);
                    CHECK_FALSE(bm[3]);
                    CHECK(bm[24]);

                    bm[3] = true;
                    CHECK_EQ(bm.data()[0], 46);
                    CHECK_EQ(bm.null_count(), s_bitmap_null_count - 1);

                    bm[24] = false;
                    CHECK_EQ(bm.data()[3], 6);
                    CHECK_EQ(bm.null_count(), s_bitmap_null_count);

                    // Ensures that setting false again does not alter the null count
                    bm[24] = false;
                    CHECK_EQ(bm.data()[3], 6);
                    CHECK_EQ(bm.null_count(), s_bitmap_null_count);

                    bm[2] = true;
                    CHECK(bm.test(2));
                    CHECK_EQ(bm.null_count(), s_bitmap_null_count);
                }
            }

            SUBCASE("resize")
            {
                SUBCASE("from null buffer")
                {
                    bitmap bm = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    CHECK_EQ(bm.size(), s_bitmap_size);
                    CHECK_EQ(bm.null_count(), 0);
                    bm.resize(40, false);
                    CHECK_EQ(bm.size(), 40);
                    CHECK_EQ(bm.null_count(), 11);
                }

                SUBCASE("from non null buffer")
                {
                    bitmap bm = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    CHECK_EQ(bm.size(), s_bitmap_size);
                    CHECK_EQ(bm.null_count(), s_bitmap_null_count);

                    // Test expansion
                    bm.resize(40, true);
                    CHECK_EQ(bm.size(), 40);
                    CHECK_EQ(bm.null_count(), s_bitmap_null_count);

                    // Test shrinkage
                    bm.resize(10, true);
                    CHECK_EQ(bm.size(), 10);
                    CHECK_EQ(bm.null_count(), 6);

                    // Test expansion
                    bm.resize(30, false);
                    CHECK_EQ(bm.size(), 30);
                    CHECK_EQ(bm.null_count(), 26);
                }
            }

            SUBCASE("iterator")
            {
                SUBCASE("increment")
                {
                    SUBCASE("from non null buffer")
                    {
                        bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                        auto iter = b.begin();
                        for (size_t i = 0; i < s_bitmap_size; ++i)
                        {
                            CHECK_EQ(*iter, b.test(i));
                            ++iter;
                        }
                    }

                    SUBCASE("from null buffer")
                    {
                        bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                        auto iter = b.begin();
                        for (size_t i = 0; i < s_bitmap_size; ++i)
                        {
                            CHECK_EQ(*iter, b.test(i));
                            ++iter;
                        }
                    }
                }

                SUBCASE("decrement")
                {
                    SUBCASE("from non null buffer")
                    {
                        bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                        auto iter = b.end();
                        for (size_t i = s_bitmap_size; i > 0; --i)
                        {
                            --iter;
                            CHECK_EQ(*iter, b.test(i - 1));
                        }
                    }
                    SUBCASE("from null buffer")
                    {
                        bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                        auto iter = b.end();
                        for (size_t i = s_bitmap_size; i > 0; --i)
                        {
                            --iter;
                            CHECK_EQ(*iter, b.test(i - 1));
                        }
                    }
                }

                SUBCASE("random increment decrement")
                {
                    // Does not work because the reference is not bool&
                    // static_assert(std::random_access_iterator<typename bitmap::iterator>);
                    static_assert(std::random_access_iterator<typename bitmap::const_iterator>);

                    bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
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
                    auto citer_end = std::next(
                        b.cbegin(),
                        static_cast<bitmap::iterator::difference_type>(b.size())
                    );
                    CHECK_EQ(iter_end, b.end());
                    CHECK_EQ(citer_end, b.cend());
                }
            }

            SUBCASE("insert")
            {
                SUBCASE("const_iterator and value_type")
                {
                    SUBCASE("begin")
                    {
                        SUBCASE("from non null buffer")
                        {
                            bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
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
                        SUBCASE("from null buffer")
                        {
                            bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                            const auto pos = b.cbegin();
                            auto iter = b.insert(pos, false);
                            CHECK_EQ(b.size(), s_bitmap_size + 1);
                            CHECK_EQ(b.null_count(), 1);
                            CHECK_EQ(*iter, false);

                            iter = b.insert(pos, true);
                            CHECK_EQ(b.size(), s_bitmap_size + 2);
                            CHECK_EQ(b.null_count(), 1);
                            CHECK_EQ(*iter, true);
                        }
                    }

                    SUBCASE("middle")
                    {
                        SUBCASE("from non null buffer")
                        {
                            bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
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
                        SUBCASE("from null buffer")
                        {
                            bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                            const auto pos = std::next(b.cbegin(), 14);
                            auto iter = b.insert(pos, false);
                            CHECK_EQ(b.size(), s_bitmap_size + 1);
                            CHECK_EQ(b.null_count(), 1);
                            CHECK_EQ(*iter, false);

                            iter = b.insert(pos, true);
                            CHECK_EQ(b.size(), s_bitmap_size + 2);
                            CHECK_EQ(b.null_count(), 1);
                            CHECK_EQ(*iter, true);
                        }
                    }

                    SUBCASE("end")
                    {
                        SUBCASE("from non null buffer")
                        {
                            bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
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
                        SUBCASE("from null buffer")
                        {
                            bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                            const auto pos = b.cend();
                            auto iter = b.insert(pos, false);
                            CHECK_EQ(b.size(), s_bitmap_size + 1);
                            CHECK_EQ(b.null_count(), 1);
                            CHECK_EQ(*iter, false);

                            iter = b.insert(pos, true);
                            CHECK_EQ(b.size(), s_bitmap_size + 2);
                            CHECK_EQ(b.null_count(), 1);
                            CHECK_EQ(*iter, true);
                        }
                    }
                }

                SUBCASE("const_iterator and count/value_type")
                {
                    SUBCASE("begin")
                    {
                        SUBCASE("from non null buffer")
                        {
                            bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
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
                        SUBCASE("from null buffer")
                        {
                            bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                            const auto pos = b.cbegin();
                            auto iter = b.insert(pos, 3, false);
                            CHECK_EQ(b.size(), s_bitmap_size + 3);
                            CHECK_EQ(b.null_count(), 3);
                            CHECK_EQ(*iter, false);
                            CHECK_EQ(*(++iter), false);
                            CHECK_EQ(*(++iter), false);

                            iter = b.insert(pos, 3, true);
                            CHECK_EQ(b.size(), s_bitmap_size + 6);
                            CHECK_EQ(b.null_count(), 3);
                            CHECK_EQ(*iter, true);
                            CHECK_EQ(*(++iter), true);
                            CHECK_EQ(*(++iter), true);
                        }
                    }

                    SUBCASE("middle")
                    {
                        SUBCASE("from non null buffer")
                        {
                            bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
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
                        SUBCASE("from null buffer")
                        {
                            bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                            const auto pos = std::next(b.cbegin(), 14);
                            auto iter = b.insert(pos, 3, false);
                            CHECK_EQ(b.size(), s_bitmap_size + 3);
                            CHECK_EQ(b.null_count(), 3);
                            CHECK_EQ(*iter, false);
                            CHECK_EQ(*(++iter), false);
                            CHECK_EQ(*(++iter), false);

                            iter = b.insert(pos, 3, true);
                            CHECK_EQ(b.size(), s_bitmap_size + 6);
                            CHECK_EQ(b.null_count(), 3);
                            CHECK_EQ(*iter, true);
                            CHECK_EQ(*(++iter), true);
                            CHECK_EQ(*(++iter), true);
                        }
                    }

                    SUBCASE("end")
                    {
                        SUBCASE("from non null buffer")
                        {
                            bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
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
                        SUBCASE("from null buffer")
                        {
                            bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                            auto iter = b.insert(b.cend(), 3, false);
                            CHECK_EQ(b.size(), s_bitmap_size + 3);
                            CHECK_EQ(b.null_count(), 3);
                            CHECK_EQ(*iter, false);
                            CHECK_EQ(*(++iter), false);
                            CHECK_EQ(*(++iter), false);

                            iter = b.insert(b.cend(), 3, true);
                            CHECK_EQ(b.size(), s_bitmap_size + 6);
                            CHECK_EQ(b.null_count(), 3);
                            CHECK_EQ(*iter, true);
                            CHECK_EQ(*(++iter), true);
                            CHECK_EQ(*(++iter), true);
                        }
                    }
                }
            }

            SUBCASE("emplace")
            {
                SUBCASE("begin")
                {
                    SUBCASE("from non null buffer")
                    {
                        bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                        auto iter = b.emplace(b.cbegin(), true);
                        CHECK_EQ(b.size(), s_bitmap_size + 1);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count);
                        CHECK_EQ(*iter, true);
                    }
                    SUBCASE("from null buffer")
                    {
                        bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                        auto iter = b.emplace(b.cbegin(), true);
                        CHECK_EQ(b.size(), s_bitmap_size + 1);
                        CHECK_EQ(b.null_count(), 0);
                        CHECK_EQ(*iter, true);
                    }
                }

                SUBCASE("middle")
                {
                    SUBCASE("from non null buffer")
                    {
                        bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                        auto iter = b.emplace(std::next(b.cbegin()), true);
                        CHECK_EQ(b.size(), s_bitmap_size + 1);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count);
                        CHECK_EQ(*iter, true);
                    }
                    SUBCASE("from null buffer")
                    {
                        bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                        auto iter = b.emplace(std::next(b.cbegin()), true);
                        CHECK_EQ(b.size(), s_bitmap_size + 1);
                        CHECK_EQ(b.null_count(), 0);
                        CHECK_EQ(*iter, true);
                    }
                }

                SUBCASE("end")
                {
                    SUBCASE("from non null buffer")
                    {
                        bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                        auto iter = b.emplace(b.cend(), true);
                        CHECK_EQ(b.size(), s_bitmap_size + 1);
                        CHECK_EQ(b.null_count(), s_bitmap_null_count);
                        CHECK_EQ(*iter, true);
                    }
                    SUBCASE("from null buffer")
                    {
                        bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                        auto iter = b.emplace(b.cend(), true);
                        CHECK_EQ(b.size(), s_bitmap_size + 1);
                        CHECK_EQ(b.null_count(), 0);
                        CHECK_EQ(*iter, true);
                        CHECK_EQ(b.data(), nullptr);

                        iter = b.emplace(b.cend(), false);
                        CHECK_EQ(b.size(), s_bitmap_size + 2);
                        CHECK_EQ(b.null_count(), 1);
                        CHECK_EQ(*iter, false);
                        CHECK_NE(b.data(), nullptr);
                    }
                }
            }

            SUBCASE("erase")
            {
                SUBCASE("const_iterator")
                {
                    SUBCASE("begin")
                    {
                        SUBCASE("from non null buffer")
                        {
                            bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                            auto iter = b.erase(b.cbegin());
                            CHECK_EQ(b.size(), s_bitmap_size - 1);
                            CHECK_EQ(b.null_count(), s_bitmap_null_count - 1);
                            CHECK_EQ(iter, b.begin());
                            CHECK_EQ(*iter, true);
                        }
                        SUBCASE("from null buffer")
                        {
                            bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                            auto iter = b.erase(b.cbegin());
                            CHECK_EQ(b.size(), s_bitmap_size - 1);
                            CHECK_EQ(b.null_count(), 0);
                            CHECK_EQ(iter, b.begin());
                            CHECK_EQ(*iter, true);
                            CHECK_EQ(b.data(), nullptr);
                        }
                    }

                    SUBCASE("middle")
                    {
                        SUBCASE("from non null buffer")
                        {
                            bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                            const auto pos = std::next(b.cbegin(), 2);
                            auto iter = b.erase(pos);
                            CHECK_EQ(b.size(), s_bitmap_size - 1);
                            CHECK_EQ(b.null_count(), s_bitmap_null_count);
                            CHECK_EQ(iter, std::next(b.begin(), 2));
                            CHECK_EQ(*iter, false);
                        }
                        SUBCASE("from null buffer")
                        {
                            bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                            const auto pos = std::next(b.cbegin(), 2);
                            auto iter = b.erase(pos);
                            CHECK_EQ(b.size(), s_bitmap_size - 1);
                            CHECK_EQ(b.null_count(), 0);
                            CHECK_EQ(iter, std::next(b.begin(), 2));
                            CHECK_EQ(*iter, true);
                            CHECK_EQ(b.data(), nullptr);
                        }
                    }
                }

                SUBCASE("const_iterator range")
                {
                    SUBCASE("begin")
                    {
                        SUBCASE("from non null buffer")
                        {
                            bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                            auto iter = b.erase(b.cbegin(), std::next(b.cbegin()));
                            CHECK_EQ(b.size(), s_bitmap_size - 1);
                            CHECK_EQ(b.null_count(), s_bitmap_null_count - 1);
                            CHECK_EQ(*iter, true);
                        }
                        SUBCASE("from null buffer")
                        {
                            bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                            auto iter = b.erase(b.cbegin(), std::next(b.cbegin()));
                            CHECK_EQ(b.size(), s_bitmap_size - 1);
                            CHECK_EQ(b.null_count(), 0);
                            CHECK_EQ(*iter, true);
                            CHECK_EQ(b.data(), nullptr);
                        }
                    }

                    SUBCASE("middle")
                    {
                        SUBCASE("from non null buffer")
                        {
                            bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                            const auto pos = std::next(b.cbegin());
                            auto iter = b.erase(pos, std::next(pos, 1));
                            CHECK_EQ(b.size(), s_bitmap_size - 1);
                            CHECK_EQ(b.null_count(), s_bitmap_null_count);
                            CHECK_EQ(*iter, true);
                        }
                        SUBCASE("from null buffer")
                        {
                            bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                            const auto pos = std::next(b.cbegin());
                            auto iter = b.erase(pos, std::next(pos, 1));
                            CHECK_EQ(b.size(), s_bitmap_size - 1);
                            CHECK_EQ(b.null_count(), 0);
                            CHECK_EQ(*iter, true);
                            CHECK_EQ(b.data(), nullptr);
                        }
                    }

                    SUBCASE("all")
                    {
                        SUBCASE("from non null buffer")
                        {
                            bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                            auto iter = b.erase(b.cbegin(), b.cend());
                            CHECK_EQ(b.size(), 0);
                            CHECK_EQ(b.null_count(), 0);
                            CHECK_EQ(iter, b.end());
                        }
                        SUBCASE("from null buffer")
                        {
                            bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                            auto iter = b.erase(b.cbegin(), b.cend());
                            CHECK_EQ(b.size(), 0);
                            CHECK_EQ(b.null_count(), 0);
                            CHECK_EQ(iter, b.end());
                            CHECK_EQ(b.data(), nullptr);
                        }
                    }
                }
            }

            SUBCASE("at")
            {
                SUBCASE("from non null buffer")
                {
                    const bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    CHECK_EQ(b.at(0), false);
                    CHECK_EQ(b.at(1), true);
                    CHECK_EQ(b.at(2), true);
                    CHECK_THROWS_AS(b.at(s_bitmap_size + 1), std::out_of_range);
                }
                SUBCASE("from null buffer")
                {
                    const bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    CHECK_EQ(b.at(0), true);
                    CHECK_EQ(b.at(1), true);
                    CHECK_EQ(b.at(2), true);
                    CHECK_THROWS_AS(b.at(s_bitmap_size + 1), std::out_of_range);
                }
            }

            SUBCASE("front")
            {
                SUBCASE("from non null buffer")
                {
                    const bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    CHECK_EQ(b.front(), false);
                }
                SUBCASE("from null buffer")
                {
                    const bitmap b = make_bitmap(
                        null_f.get_buffer(),
                        s_bitmap_size,
                        typename dynamic_bitset<std::uint8_t>::default_allocator()
                    );
                    CHECK_EQ(b.front(), true);
                }
            }

            SUBCASE("back")
            {
                SUBCASE("from non null buffer")
                {
                    const bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    CHECK_EQ(b.back(), false);
                }
                SUBCASE("from null buffer")
                {
                    const bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    CHECK_EQ(b.back(), true);
                }
            }

            SUBCASE("push_back")
            {
                SUBCASE("from non null buffer")
                {
                    bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    b.push_back(false);
                    CHECK_EQ(b.size(), s_bitmap_size + 1);
                    CHECK_EQ(b.null_count(), s_bitmap_null_count + 1);
                    CHECK_EQ(b.back(), false);
                }
                SUBCASE("from null buffer")
                {
                    bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    b.push_back(false);
                    CHECK_EQ(b.size(), s_bitmap_size + 1);
                    CHECK_EQ(b.null_count(), 1);
                    CHECK_EQ(b.back(), false);
                }
            }

            SUBCASE("pop_back")
            {
                SUBCASE("on non empty bitmap")
                {
                    bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    b.pop_back();
                    CHECK_EQ(b.size(), s_bitmap_size - 1);
                    CHECK_EQ(b.null_count(), s_bitmap_null_count - 1);
                }
                if constexpr (std::is_same_v<bitmap, dynamic_bitset<std::uint8_t>>)
                {
                    SUBCASE("on empty bitmap")
                    {
                        bitmap b{typename dynamic_bitset<std::uint8_t>::default_allocator()};
                        CHECK_NOTHROW(b.pop_back());
                    }
                }
            }

            SUBCASE("bitset_reference")
            {
                SUBCASE("from non null buffer")
                {
                    // as a reminder: p_buffer[0] = 38; // 00100110
                    bitmap b = make_bitmap(f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
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
                SUBCASE("from null buffer")
                {
                    bitmap b = make_bitmap(null_f.get_buffer(), s_bitmap_size, std::allocator<uint8_t>());
                    auto iter = b.begin();
                    *iter = true;
                    CHECK_EQ(b.null_count(), 0);

                    ++iter;
                    *iter &= false;
                    CHECK_EQ(b.null_count(), 1);

                    iter += 2;
                    *iter |= true;
                    CHECK_EQ(b.null_count(), 1);

                    ++iter;
                    *iter ^= true;
                    CHECK_EQ(b.null_count(), 2);

                    CHECK_EQ(*iter, *iter);

                    CHECK_EQ(*iter, false);
                    CHECK_EQ(false, *iter);

                    CHECK_NE(*iter, true);
                    CHECK_NE(true, *iter);
                }
            }
        }
        TEST_CASE_TEMPLATE_APPLY(dynamic_bitset_id, testing_types);
    }

    TEST_SUITE("null_count_policy")
    {
        TEST_CASE("tracking_null_count")
        {
            SUBCASE("static count_non_null")
            {
                // Test with simple buffer
                std::array<std::uint8_t, 2> buffer = {0b11110000, 0b00001111};
                const auto count = count_non_null(buffer.data(), 16, 2);
                CHECK_EQ(count, 8);  // 4 bits set in each byte = 8 total
            }

            SUBCASE("static count_non_null with partial last block")
            {
                std::array<std::uint8_t, 2> buffer = {0b11111111, 0b11111111};
                // Only count first 12 bits (8 from first byte + 4 from second)
                const auto count = count_non_null(buffer.data(), 12, 2);
                CHECK_EQ(count, 12);  // All 12 bits are set
            }

            SUBCASE("static count_non_null with nullptr")
            {
                const std::uint8_t* null_ptr = nullptr;
                const auto count = count_non_null(null_ptr, 100, 0);
                CHECK_EQ(count, 100);  // All bits assumed set for null buffer with no offset
                
                // With offset, only count from offset onwards
                const auto count_with_offset = count_non_null(null_ptr, 100, 0, 20);
                CHECK_EQ(count_with_offset, 80);  // 100 - 20 = 80 bits from offset
            }

            SUBCASE("static count_non_null with offset - no offset")
            {
                // Test that offset=0 works the same as no offset
                std::array<std::uint8_t, 2> buffer = {0b11110000, 0b00001111};
                const auto count = count_non_null(buffer.data(), 16, 2, 0);
                CHECK_EQ(count, 8);  // 4 bits set in each byte = 8 total
            }

            SUBCASE("static count_non_null with offset - byte aligned")
            {
                // Skip the first byte (8 bits), count only the second byte
                std::array<std::uint8_t, 2> buffer = {0b11110000, 0b00001111};
                const auto count = count_non_null(buffer.data(), 8, 2, 8);
                CHECK_EQ(count, 4);  // Only count second byte: 4 bits set
            }

            SUBCASE("static count_non_null with offset - bit aligned")
            {
                // Offset by 3 bits
                // Buffer: 0b11110000 0b00001111
                // LSB representation: 0 0 0 0 1 1 1 1 | 1 1 1 1 0 0 0 0
                // Starting from bit 3, count 13 bits: bits 3-15
                // Bits 3-7 of first byte: 01111 = 4 bits
                // All 8 bits of second byte: 11110000 = 4 bits
                std::array<std::uint8_t, 2> buffer = {0b11110000, 0b00001111};
                const auto count = count_non_null(buffer.data(), 13, 2, 3);
                CHECK_EQ(count, 8);  // 4 bits from first byte + 4 bits from second byte
            }

            SUBCASE("static count_non_null with offset - partial first byte")
            {
                // Test partial first byte when offset is not byte-aligned
                // Buffer: 0b11111111
                // Offset by 2, count 4 bits: bits 2-5 (0-indexed)
                std::array<std::uint8_t, 1> buffer = {0b11111111};
                const auto count = count_non_null(buffer.data(), 4, 1, 2);
                CHECK_EQ(count, 4);  // All 4 bits are set
            }

            SUBCASE("static count_non_null with offset - partial first byte mixed")
            {
                // Buffer: 0b10101010
                // Offset by 1, count 6 bits: bits 1-6 (0-indexed) = 010101
                std::array<std::uint8_t, 1> buffer = {0b10101010};
                const auto count = count_non_null(buffer.data(), 6, 1, 1);
                CHECK_EQ(count, 3);  // 3 bits are set in positions 1,3,5
            }

            SUBCASE("static count_non_null with offset - spanning multiple bytes")
            {
                // Buffer: 0b11110000 0b00001111 0b10101010
                // Offset by 4, count 16 bits
                std::array<std::uint8_t, 3> buffer = {0b11110000, 0b00001111, 0b10101010};
                const auto count = count_non_null(buffer.data(), 16, 3, 4);
                // Bits 4-7 of first byte: 1111 = 4 bits
                // All of second byte: 00001111 = 4 bits
                // Bits 0-3 of third byte: 1010 = 2 bits
                // Total: 10 bits
                CHECK_EQ(count, 10);
            }

            SUBCASE("static count_non_null with offset - offset beyond buffer")
            {
                std::array<std::uint8_t, 2> buffer = {0b11111111, 0b11111111};
                const auto count = count_non_null(buffer.data(), 8, 2, 16);
                CHECK_EQ(count, 0);  // Offset beyond buffer returns 0
            }

            SUBCASE("static count_non_null with offset - offset at buffer boundary")
            {
                std::array<std::uint8_t, 2> buffer = {0b11111111, 0b11111111};
                const auto count = count_non_null(buffer.data(), 8, 2, 15);
                CHECK_EQ(count, 1);  // Only 1 bit available
            }

            SUBCASE("static count_non_null with offset - zero bit_size")
            {
                std::array<std::uint8_t, 2> buffer = {0b11111111, 0b11111111};
                const auto count = count_non_null(buffer.data(), 0, 2, 0);
                CHECK_EQ(count, 0);  // Zero bit_size returns 0
            }

            SUBCASE("static count_non_null with offset - complex pattern")
            {
                // Test with realistic pattern
                // Buffer: 0b00100110 0b01010101 0b00110101
                std::array<std::uint8_t, 3> buffer = {0b00100110, 0b01010101, 0b00110101};
                
                // No offset, count all 24 bits
                const auto count1 = count_non_null(buffer.data(), 24, 3, 0);
                CHECK_EQ(count1, 11);  // 3 + 4 + 4 = 11 bits set
                
                // Offset by 8, count 16 bits (skip first byte)
                const auto count2 = count_non_null(buffer.data(), 16, 3, 8);
                CHECK_EQ(count2, 8);  // 4 + 4 = 8 bits set
                
                // Offset by 5, count 10 bits
                // First byte LSB: 0 1 1 0 0 1 0 0
                // Bits 5-7 of first byte: 100 = 1 bit
                // Second byte LSB: 1 0 1 0 1 0 1 0  
                // Bits 0-6 of second byte: 1010101 = 4 bits
                const auto count3 = count_non_null(buffer.data(), 10, 3, 5);
                CHECK_EQ(count3, 5);
            }

            SUBCASE("static count_non_null with offset - single bit")
            {
                // Test counting a single bit at various positions
                std::array<std::uint8_t, 1> buffer = {0b10101010};
                
                CHECK_EQ(count_non_null(buffer.data(), 1, 1, 0), 0);  // Bit 0: 0
                CHECK_EQ(count_non_null(buffer.data(), 1, 1, 1), 1);  // Bit 1: 1
                CHECK_EQ(count_non_null(buffer.data(), 1, 1, 2), 0);  // Bit 2: 0
                CHECK_EQ(count_non_null(buffer.data(), 1, 1, 3), 1);  // Bit 3: 1
                CHECK_EQ(count_non_null(buffer.data(), 1, 1, 7), 1);  // Bit 7: 1
            }

            SUBCASE("initialize")
            {
                std::array<std::uint8_t, 2> buffer = {0b10101010, 0b01010101};
                tracking_null_count<> policy;
                policy.initialize_null_count(buffer.data(), 16, 2);
                CHECK_EQ(policy.null_count(), 8);  // 8 bits are unset
            }

            SUBCASE("recompute")
            {
                std::array<std::uint8_t, 2> buffer = {0b11111111, 0b00000000};
                tracking_null_count<> policy;
                policy.initialize_null_count(buffer.data(), 16, 2);
                CHECK_EQ(policy.null_count(), 8);

                // Modify buffer and recompute
                buffer[1] = 0b11111111;
                policy.recompute_null_count(buffer.data(), 16, 2);
                CHECK_EQ(policy.null_count(), 0);
            }

            SUBCASE("update_null_count")
            {
                tracking_null_count<> policy(5);
                CHECK_EQ(policy.null_count(), 5);

                // false -> true: decrement null count
                policy.update_null_count(false, true);
                CHECK_EQ(policy.null_count(), 4);

                // true -> false: increment null count
                policy.update_null_count(true, false);
                CHECK_EQ(policy.null_count(), 5);

                // false -> false: no change
                policy.update_null_count(false, false);
                CHECK_EQ(policy.null_count(), 5);

                // true -> true: no change
                policy.update_null_count(true, true);
                CHECK_EQ(policy.null_count(), 5);
            }

            SUBCASE("swap")
            {
                tracking_null_count<> policy1(10);
                tracking_null_count<> policy2(20);

                policy1.swap_null_count(policy2);
                CHECK_EQ(policy1.null_count(), 20);
                CHECK_EQ(policy2.null_count(), 10);
            }

            SUBCASE("clear")
            {
                tracking_null_count<> policy(42);
                policy.clear_null_count();
                CHECK_EQ(policy.null_count(), 0);
            }

            SUBCASE("set_null_count")
            {
                tracking_null_count<> policy;
                policy.set_null_count(100);
                CHECK_EQ(policy.null_count(), 100);
            }
        }

        TEST_CASE("non_tracking_null_count")
        {
            SUBCASE("operations are no-ops")
            {
                non_tracking_null_count<> policy;

                // All operations should compile and do nothing
                std::array<std::uint8_t, 2> buffer = {0xFF, 0xFF};
                policy.initialize_null_count(buffer.data(), 16, 2);
                policy.recompute_null_count(buffer.data(), 16, 2);
                policy.update_null_count(false, true);
                policy.set_null_count(42);
                policy.clear_null_count();
                non_tracking_null_count<> other;
                policy.swap_null_count(other);

                // Should compile - constructor with count is available but ignores value
                non_tracking_null_count<> policy_with_count(100);
                static_cast<void>(policy_with_count);  // Suppress unused warning
            }

            SUBCASE("zero overhead")
            {
                // non_tracking_null_count should have no data members
                static_assert(sizeof(non_tracking_null_count<>) == 1);  // Empty class has size 1
            }
        }

        TEST_CASE("null_count_policy concept")
        {
            // Verify both policies satisfy the concept
            static_assert(null_count_policy<tracking_null_count<>>);
            static_assert(null_count_policy<non_tracking_null_count<>>);

            // Verify with different size types
            static_assert(null_count_policy<tracking_null_count<std::uint32_t>>);
            static_assert(null_count_policy<non_tracking_null_count<std::uint32_t>>);
        }

        TEST_CASE("non_owning_dynamic_bitset with non_tracking_null_count")
        {
            std::array<std::uint8_t, 4> buffer_data = {0b00100110, 0b01010101, 0b00110101, 0b00000111};
            buffer<std::uint8_t> buf(buffer_data, buffer<std::uint8_t>::default_allocator{});

            using non_tracking_bitset = non_owning_dynamic_bitset<std::uint8_t, non_tracking_null_count<>>;
            non_tracking_bitset bm(&buf, s_bitmap_size);

            SUBCASE("size")
            {
                CHECK_EQ(bm.size(), s_bitmap_size);
            }

            SUBCASE("null_count is not available")
            {
                // This should not compile - the null_count() method is constrained away:
                // bm.null_count();
                // We verify through the policy that track_null_count is false
                static_assert(!non_tracking_null_count<>::track_null_count);
            }

            SUBCASE("test")
            {
                CHECK(bm.test(1));
                CHECK(bm.test(2));
                CHECK_FALSE(bm.test(0));
                CHECK_FALSE(bm.test(3));
            }

            SUBCASE("set")
            {
                bm.set(0, true);
                CHECK(bm.test(0));

                bm.set(0, false);
                CHECK_FALSE(bm.test(0));
            }

            SUBCASE("operator[]")
            {
                CHECK(bm[1]);
                CHECK_FALSE(bm[0]);

                bm[0] = true;
                CHECK(bm[0]);

                bm[0] = false;
                CHECK_FALSE(bm[0]);
            }

            SUBCASE("iterator")
            {
                auto iter = bm.begin();
                CHECK_FALSE(*iter);  // bit 0 is false
                ++iter;
                CHECK(*iter);  // bit 1 is true
                ++iter;
                CHECK(*iter);  // bit 2 is true
                ++iter;
                CHECK_FALSE(*iter);  // bit 3 is false
            }

            SUBCASE("const_iterator")
            {
                const auto& const_bm = bm;
                auto iter = const_bm.cbegin();
                CHECK_FALSE(*iter);
                ++iter;
                CHECK(*iter);
            }

            SUBCASE("resize")
            {
                bm.resize(40, true);
                CHECK_EQ(bm.size(), 40);
                // Verify new bits are set to true
                for (std::size_t i = s_bitmap_size; i < 40; ++i)
                {
                    CHECK(bm.test(i));
                }
            }

            SUBCASE("insert")
            {
                const auto pos = bm.cbegin();
                auto iter = bm.insert(pos, true);
                CHECK_EQ(bm.size(), s_bitmap_size + 1);
                CHECK(*iter);
            }

            SUBCASE("erase")
            {
                const auto pos = bm.cbegin();
                bm.erase(pos);
                CHECK_EQ(bm.size(), s_bitmap_size - 1);
            }

            SUBCASE("push_back")
            {
                bm.push_back(true);
                CHECK_EQ(bm.size(), s_bitmap_size + 1);
                CHECK(bm.test(s_bitmap_size));
            }

            SUBCASE("pop_back")
            {
                bm.pop_back();
                CHECK_EQ(bm.size(), s_bitmap_size - 1);
            }
        }

        TEST_CASE("comparing tracking vs non_tracking behavior")
        {
            std::array<std::uint8_t, 4> buffer_data = {0b00100110, 0b01010101, 0b00110101, 0b00000111};

            buffer<std::uint8_t> buf1(buffer_data, buffer<std::uint8_t>::default_allocator{});
            buffer<std::uint8_t> buf2(buffer_data, buffer<std::uint8_t>::default_allocator{});

            using tracking_bitset = non_owning_dynamic_bitset<std::uint8_t, tracking_null_count<>>;
            using non_tracking_bitset = non_owning_dynamic_bitset<std::uint8_t, non_tracking_null_count<>>;

            tracking_bitset tracking_bm(&buf1, s_bitmap_size);
            non_tracking_bitset non_tracking_bm(&buf2, s_bitmap_size);

            SUBCASE("same test results")
            {
                for (std::size_t i = 0; i < s_bitmap_size; ++i)
                {
                    CHECK_EQ(tracking_bm.test(i), non_tracking_bm.test(i));
                }
            }

            SUBCASE("same iteration results")
            {
                auto t_iter = tracking_bm.cbegin();
                auto nt_iter = non_tracking_bm.cbegin();
                for (std::size_t i = 0; i < s_bitmap_size; ++i)
                {
                    CHECK_EQ(*t_iter, *nt_iter);
                    ++t_iter;
                    ++nt_iter;
                }
            }

            SUBCASE("same set behavior")
            {
                tracking_bm.set(0, true);
                non_tracking_bm.set(0, true);
                CHECK_EQ(tracking_bm.test(0), non_tracking_bm.test(0));
                CHECK(tracking_bm.test(0));

                tracking_bm.set(0, false);
                non_tracking_bm.set(0, false);
                CHECK_EQ(tracking_bm.test(0), non_tracking_bm.test(0));
                CHECK_FALSE(tracking_bm.test(0));
            }
        }
    }
}
