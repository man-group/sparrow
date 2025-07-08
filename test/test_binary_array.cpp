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

#include <cstddef>
#include <vector>

#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/variable_size_binary_array.hpp"

#include "../test/external_array_data_creation.hpp"
#include "doctest/doctest.h"
#include "test_utils.hpp"

namespace sparrow
{
    struct binary_array_fixture
    {
        using layout_type = binary_array;

        binary_array_fixture()
            : m_arrow_proxy(create_arrow_proxy())
        {
        }

        arrow_proxy m_arrow_proxy;
        static constexpr size_t m_length = 10;
        static constexpr size_t m_offset = 1;
        static constexpr std::array<size_t, 2> m_false_bitmap{2, 5};

        using value_type = std::vector<byte_t>;
        using const_reference = arrow_traits<value_type>::const_reference;

    private:

        static_assert(is_binary_array_v<layout_type>);
        static_assert(std::same_as<layout_type::inner_value_type, value_type>);
        static_assert(
            std::same_as<layout_type::inner_reference, sparrow::variable_size_binary_reference<layout_type>>
        );
        static_assert(std::same_as<layout_type::inner_const_reference, const_reference>);
        using const_value_iterator = layout_type::const_value_iterator;
        static_assert(std::same_as<const_value_iterator::value_type, value_type>);

        static_assert(std::same_as<const_value_iterator::reference, const_reference>);

        arrow_proxy create_arrow_proxy()
        {
            ArrowSchema schema{};
            ArrowArray array{};
            const std::vector<size_t> false_bitmap{m_false_bitmap.begin(), m_false_bitmap.end()};
            test::fill_schema_and_array<std::vector<byte_t>>(schema, array, m_length, m_offset, false_bitmap);
            return arrow_proxy{std::move(array), std::move(schema)};
        }
    };

    template <class T>
    void print_bytes(const T& vec)
    {
        for (auto b : vec)
        {
            std::cout << int(b) << ' ';
        }
        std::cout << '\n';
    }

    TEST_SUITE("binary_array")
    {
        TEST_CASE("convenience")
        {
            SUBCASE("high-level")
            {
                std::vector<byte_t> word0 = {byte_t(0), byte_t(1)};
                std::vector<byte_t> word1 = {byte_t(2)};
                std::vector<byte_t> word4 = {byte_t(8), byte_t(9), byte_t(10)};
                std::vector<std::vector<byte_t>>
                    words{word0, word1, {byte_t(3), byte_t(4), byte_t(5)}, {byte_t(6), byte_t(7)}, word4};
                std::vector<std::size_t> where_nulls{2, 3};
                const binary_array array(words, std::move(where_nulls));

                REQUIRE_EQ(array.size(), words.size());

                // check nulls
                CHECK(array[0].has_value());
                CHECK(array[1].has_value());
                CHECK_FALSE(array[2].has_value());
                CHECK_FALSE(array[3].has_value());
                CHECK(array[4].has_value());

                // check values
                CHECK_EQ(array[0].value(), word0);
                CHECK_EQ(array[1].value(), word1);
                CHECK_EQ(array[4].value(), word4);
            }
        }

        TEST_CASE_FIXTURE(binary_array_fixture, "constructor")
        {
            SUBCASE("copy arrow_proxy")
            {
                CHECK_NOTHROW(layout_type(m_arrow_proxy));
            }

            SUBCASE("move arrow_proxy")
            {
                CHECK_NOTHROW(layout_type(std::move(m_arrow_proxy)));
            }
        }

        TEST_CASE_FIXTURE(binary_array_fixture, "copy")
        {
            layout_type ar(m_arrow_proxy);
            layout_type ar2(ar);
            CHECK_EQ(ar, ar2);

            layout_type ar3(std::move(m_arrow_proxy));
            ar3 = ar2;
            CHECK_EQ(ar2, ar3);
        }

        TEST_CASE_FIXTURE(binary_array_fixture, "move")
        {
            layout_type ar(m_arrow_proxy);
            layout_type ar2(ar);
            layout_type ar3(std::move(ar));
            CHECK_EQ(ar2, ar3);

            layout_type ar4(std::move(m_arrow_proxy));
            ar4 = std::move(ar3);
            CHECK_EQ(ar2, ar4);
        }

        TEST_CASE_FIXTURE(binary_array_fixture, "size")
        {
            const layout_type array(std::move(m_arrow_proxy));
            CHECK_EQ(array.size(), m_length - m_offset);
        }

        TEST_CASE_FIXTURE(binary_array_fixture, "operator[]")
        {
            std::vector<std::vector<byte_t>> words = test::make_testing_bytes(m_length);

            SUBCASE("const")
            {
                const layout_type array(std::move(m_arrow_proxy));
                REQUIRE_EQ(array.size(), m_length - m_offset);
                const auto cref0 = array[0];
                REQUIRE(cref0.has_value());
                CHECK_EQ(cref0.get(), words[m_offset]);
                const auto cref1 = array[1];
                REQUIRE_FALSE(cref1.has_value());
                const auto cref2 = array[2];
                REQUIRE(cref2.has_value());
                CHECK_EQ(cref2.get(), words[m_offset + 2]);
                const auto cref3 = array[3];
                REQUIRE(cref3.has_value());
                CHECK_EQ(cref3.get(), words[m_offset + 3]);
                const auto cref4 = array[4];
                REQUIRE_FALSE(cref4.has_value());
                const auto cref5 = array[5];
                REQUIRE(cref5.has_value());
                CHECK_EQ(cref5.get(), words[m_offset + 5]);
                const auto cref6 = array[6];
                REQUIRE(cref6.has_value());
                CHECK_EQ(cref6.get(), words[m_offset + 6]);
                const auto cref7 = array[7];
                REQUIRE(cref7.has_value());
                CHECK_EQ(cref7.get(), words[m_offset + 7]);
                const auto cref8 = array[8];
                REQUIRE(cref8.has_value());
                CHECK_EQ(cref8.get(), words[m_offset + 8]);
            }

            SUBCASE("mutable")
            {
                layout_type array(std::move(m_arrow_proxy));
                REQUIRE_EQ(array.size(), m_length - m_offset);
                auto ref0 = array[0];
                REQUIRE(ref0.has_value());
                CHECK_EQ(ref0.get(), words[m_offset]);
                auto ref1 = array[1];
                REQUIRE_FALSE(ref1.has_value());
                auto ref2 = array[2];
                REQUIRE(ref2.has_value());
                CHECK_EQ(ref2.get(), words[m_offset + 2]);
                auto ref3 = array[3];
                REQUIRE(ref3.has_value());
                CHECK_EQ(ref3.get(), words[m_offset + 3]);
                auto ref4 = array[4];
                REQUIRE_FALSE(ref4.has_value());
                auto ref5 = array[5];
                REQUIRE(ref5.has_value());
                CHECK_EQ(ref5.get(), words[m_offset + 5]);
                auto ref6 = array[6];
                REQUIRE(ref6.has_value());
                CHECK_EQ(ref6.get(), words[m_offset + 6]);
                auto ref7 = array[7];
                REQUIRE(ref7.has_value());
                CHECK_EQ(ref7.get(), words[m_offset + 7]);
                auto ref8 = array[8];
                REQUIRE(ref8.has_value());
                CHECK_EQ(ref8.get(), words[m_offset + 8]);

                using bytes_type = std::vector<byte_t>;
                bytes_type word61 = {byte_t(14), byte_t(15)};
                array[6] = make_nullable<bytes_type>(bytes_type(word61));
                CHECK_EQ(ref6.get(), word61);
                CHECK_EQ(ref7.get(), words[m_offset + 7]);
                CHECK_EQ(ref8.get(), words[m_offset + 8]);

                bytes_type word62 = {byte_t(17)};
                array[6] = make_nullable<bytes_type>(bytes_type(word62));
                CHECK_EQ(ref6.get(), word62);
                CHECK_EQ(ref7.get(), words[m_offset + 7]);
                CHECK_EQ(ref8.get(), words[m_offset + 8]);
            }
        }

        TEST_CASE_FIXTURE(binary_array_fixture, "value")
        {
            std::vector<std::vector<byte_t>> words = test::make_testing_bytes(m_length);

            SUBCASE("const")
            {
                const layout_type array(std::move(m_arrow_proxy));
                CHECK_EQ(array.value(0), words[m_offset]);
                CHECK_EQ(array.value(1), words[m_offset + 1]);
                CHECK_EQ(array.value(2), words[m_offset + 2]);
                CHECK_EQ(array.value(3), words[m_offset + 3]);
                CHECK_EQ(array.value(4), words[m_offset + 4]);
                CHECK_EQ(array.value(5), words[m_offset + 5]);
                CHECK_EQ(array.value(6), words[m_offset + 6]);
            }

            SUBCASE("mutable")
            {
                layout_type array(std::move(m_arrow_proxy));
                CHECK_EQ(array.value(0), words[m_offset]);
                CHECK_EQ(array.value(1), words[m_offset + 1]);
                CHECK_EQ(array.value(2), words[m_offset + 2]);
                CHECK_EQ(array.value(3), words[m_offset + 3]);
                CHECK_EQ(array.value(4), words[m_offset + 4]);
                CHECK_EQ(array.value(5), words[m_offset + 5]);
                CHECK_EQ(array.value(6), words[m_offset + 6]);
                CHECK_EQ(array.value(7), words[m_offset + 7]);
                CHECK_EQ(array.value(8), words[m_offset + 8]);

                using bytes_type = std::vector<byte_t>;
                bytes_type word61 = {byte_t(14), byte_t(15)};
                array.value(6) = word61;
                CHECK_EQ(array.value(6), word61);
                CHECK_EQ(array.value(7), words[m_offset + 7]);
                CHECK_EQ(array.value(8), words[m_offset + 8]);

                bytes_type word62 = {byte_t(17)};
                array.value(6) = word62;
                CHECK_EQ(array.value(6), word62);
                CHECK_EQ(array.value(7), words[m_offset + 7]);
                CHECK_EQ(array.value(8), words[m_offset + 8]);
            }
        }

        TEST_CASE_FIXTURE(binary_array_fixture, "const_bitmap_iterator")
        {
            SUBCASE("ordering")
            {
                const layout_type array(std::move(m_arrow_proxy));
                const auto array_bitmap = array.bitmap();
                CHECK(array_bitmap.begin() < array_bitmap.end());
            }

            SUBCASE("equality")
            {
                const layout_type array(std::move(m_arrow_proxy));
                const auto array_bitmap = array.bitmap();

                layout_type::const_bitmap_iterator citer = array_bitmap.begin();
                CHECK(*citer);
                CHECK_FALSE(*(++citer));
                CHECK(*(++citer));
                CHECK(*(++citer));
                CHECK_FALSE(*(++citer));
                CHECK(*(++citer));
                CHECK(*(++citer));
                CHECK(*(++citer));
                CHECK(*(++citer));
            }
        }

        TEST_CASE_FIXTURE(binary_array_fixture, "iterator")
        {
            std::vector<std::vector<byte_t>> words = test::make_testing_bytes(m_length);

            SUBCASE("const")
            {
                const layout_type array(std::move(m_arrow_proxy));
                auto it = array.cbegin();

                REQUIRE(it->has_value());
                CHECK_EQ(it->value(), words[m_offset]);
                CHECK_EQ(*it, make_nullable(array[0].value()));
                ++it;

                CHECK_FALSE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 1]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 2]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 3]);
                ++it;

                CHECK_FALSE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 4]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 5]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 6]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 7]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 8]);
                ++it;

                CHECK_EQ(it, array.end());
            }

            SUBCASE("non const")
            {
                layout_type array(std::move(m_arrow_proxy));
                auto it = array.begin();

                REQUIRE(it->has_value());
                CHECK_EQ(it->value(), words[m_offset]);
                CHECK_EQ(*it, make_nullable(array[0].value()));
                ++it;

                CHECK_FALSE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 1]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 2]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 3]);
                ++it;

                CHECK_FALSE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 4]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 5]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 6]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 7]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 8]);
                ++it;

                CHECK_EQ(it, array.end());

                --it;
                --it;
                using bytes_type = std::vector<byte_t>;
                bytes_type word61 = {byte_t(14), byte_t(15)};
                *it = make_nullable<bytes_type>(bytes_type(word61));
                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), word61);
                ++it;
                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[m_offset + 8]);
            }
        }

        TEST_CASE_FIXTURE(binary_array_fixture, "zero_null_values")
        {
            layout_type array(std::move(m_arrow_proxy));
            array.zero_null_values();
            // CHECK that all null values are set to empty vector
            for (auto&& i : array)
            {
                if (!i.has_value())
                {
                    CHECK(i.get().empty());
                }
            }
        }
#if defined(__cpp_lib_format)
        TEST_CASE_FIXTURE(binary_array_fixture, "formatting")
        {
            const layout_type array(std::move(m_arrow_proxy));
            const std::string formatted = std::format("{}", array);
            constexpr std::string_view
                expected = "Binary [name=test | size=9] <<1, 1, 255, 0>, null, <2, 3>, <3, 5, 255>, null, <8, 13>, <13, 21, 251, 8>, <21, 34, 248>, <34, 55>>";
            CHECK_EQ(formatted, expected);
        }
#endif
    }
}
