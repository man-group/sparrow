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
#include <numeric>
#include <vector>

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/variable_size_binary_array.hpp"

#include "../test/external_array_data_creation.hpp"
#include "../test/metadata_sample.hpp"
#include "doctest/doctest.h"
#include "test_utils.hpp"

namespace sparrow
{
    // Type list for testing both binary_array and big_binary_array
    using binary_array_types = std::tuple<binary_array, big_binary_array>;

    template <class T>
    struct binary_array_fixture
    {
        using layout_type = T;

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

        static_assert(is_binary_array_v<layout_type> || is_big_binary_array_v<layout_type>);
        static_assert(std::same_as<typename layout_type::inner_value_type, value_type>);
        static_assert(
            std::same_as<typename layout_type::inner_reference, sparrow::variable_size_binary_reference<layout_type>>
        );
        static_assert(std::same_as<typename layout_type::inner_const_reference, const_reference>);
        using const_value_iterator = typename layout_type::const_value_iterator;
        static_assert(std::same_as<typename const_value_iterator::value_type, value_type>);

        static_assert(std::same_as<typename const_value_iterator::reference, const_reference>);

        arrow_proxy create_arrow_proxy()
        {
            ArrowSchema schema{};
            ArrowArray array{};
            const std::vector<size_t> false_bitmap{m_false_bitmap.begin(), m_false_bitmap.end()};

            if constexpr (std::same_as<layout_type, binary_array>)
            {
                test::fill_schema_and_array<std::vector<byte_t>>(schema, array, m_length, m_offset, false_bitmap);
            }
            else if constexpr (std::same_as<layout_type, big_binary_array>)
            {
                fill_big_binary_schema_and_array(schema, array, m_length, m_offset, false_bitmap);
            }

            return arrow_proxy{std::move(array), std::move(schema)};
        }

    private:

        void fill_big_binary_schema_and_array(
            ArrowSchema& schema,
            ArrowArray& arr,
            size_t size,
            size_t offset,
            const std::vector<size_t>& false_bitmap
        )
        {
            const repeat_view<bool> children_ownership(true, 0);

            sparrow::fill_arrow_schema(
                schema,
                std::string_view("Z"),  // Large binary format
                "test",
                metadata_sample_opt,
                std::nullopt,
                nullptr,
                children_ownership,
                nullptr,
                true
            );

            using buffer_type = sparrow::buffer<std::uint8_t>;

            auto bytes = test::make_testing_bytes(size);
            std::size_t value_size = std::accumulate(
                bytes.cbegin(),
                bytes.cbegin() + std::ptrdiff_t(size),
                std::size_t(0),
                [](std::size_t res, const auto& s)
                {
                    return res + s.size();
                }
            );

            buffer_type offset_buf(sizeof(std::int64_t) * (size + 1));  // Use int64_t for big binary
            buffer_type value_buf(sizeof(char) * value_size);
            {
                std::int64_t* offset_data = offset_buf.data<std::int64_t>();  // Use int64_t
                offset_data[0] = 0;
                byte_t* ptr = value_buf.data<byte_t>();
                for (std::size_t i = 0; i < size; ++i)
                {
                    offset_data[i + 1] = offset_data[i] + static_cast<std::int64_t>(bytes[i].size());  // Use
                                                                                                       // int64_t
                    sparrow::ranges::copy(bytes[i], ptr);
                    ptr += bytes[i].size();
                }
            }

            std::vector<buffer_type> arr_buffs = {
                sparrow::test::make_bitmap_buffer(size, false_bitmap),
                std::move(offset_buf),
                std::move(value_buf)
            };

            sparrow::fill_arrow_array(
                arr,
                static_cast<std::int64_t>(size - offset),
                static_cast<std::int64_t>(false_bitmap.size()),
                static_cast<std::int64_t>(offset),
                std::move(arr_buffs),
                nullptr,
                children_ownership,
                nullptr,
                true
            );
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

        TEST_CASE_TEMPLATE_DEFINE("constructor", T, constructor_id)
        {
            binary_array_fixture<T> fixture;
            using layout_type = T;

            SUBCASE("copy arrow_proxy")
            {
                CHECK_NOTHROW(layout_type(fixture.m_arrow_proxy));
            }

            SUBCASE("move arrow_proxy")
            {
                CHECK_NOTHROW(layout_type(std::move(fixture.m_arrow_proxy)));
            }
        }

        TEST_CASE_TEMPLATE_DEFINE("copy", T, copy_id)
        {
            binary_array_fixture<T> fixture;
            using layout_type = T;

            layout_type ar(fixture.m_arrow_proxy);
            layout_type ar2(ar);
            CHECK_EQ(ar, ar2);

            layout_type ar3(std::move(fixture.m_arrow_proxy));
            ar3 = ar2;
            CHECK_EQ(ar2, ar3);
        }

        TEST_CASE_TEMPLATE_DEFINE("move", T, move_id)
        {
            binary_array_fixture<T> fixture;
            using layout_type = T;

            layout_type ar(fixture.m_arrow_proxy);
            layout_type ar2(ar);
            layout_type ar3(std::move(ar));
            CHECK_EQ(ar2, ar3);

            layout_type ar4(std::move(fixture.m_arrow_proxy));
            ar4 = std::move(ar3);
            CHECK_EQ(ar2, ar4);
        }

        TEST_CASE_TEMPLATE_DEFINE("size", T, size_id)
        {
            binary_array_fixture<T> fixture;
            using layout_type = T;

            const layout_type array(std::move(fixture.m_arrow_proxy));
            CHECK_EQ(array.size(), fixture.m_length - fixture.m_offset);
        }

        TEST_CASE_TEMPLATE_DEFINE("operator[]", T, operator_bracket_id)
        {
            binary_array_fixture<T> fixture;
            using layout_type = T;

            std::vector<std::vector<byte_t>> words = test::make_testing_bytes(fixture.m_length);

            SUBCASE("const")
            {
                const layout_type array(std::move(fixture.m_arrow_proxy));
                REQUIRE_EQ(array.size(), fixture.m_length - fixture.m_offset);
                const auto cref0 = array[0];
                REQUIRE(cref0.has_value());
                CHECK_EQ(cref0.get(), words[fixture.m_offset]);
                const auto cref1 = array[1];
                REQUIRE_FALSE(cref1.has_value());
                const auto cref2 = array[2];
                REQUIRE(cref2.has_value());
                CHECK_EQ(cref2.get(), words[fixture.m_offset + 2]);
                const auto cref3 = array[3];
                REQUIRE(cref3.has_value());
                CHECK_EQ(cref3.get(), words[fixture.m_offset + 3]);
                const auto cref4 = array[4];
                REQUIRE_FALSE(cref4.has_value());
                const auto cref5 = array[5];
                REQUIRE(cref5.has_value());
                CHECK_EQ(cref5.get(), words[fixture.m_offset + 5]);
                const auto cref6 = array[6];
                REQUIRE(cref6.has_value());
                CHECK_EQ(cref6.get(), words[fixture.m_offset + 6]);
                const auto cref7 = array[7];
                REQUIRE(cref7.has_value());
                CHECK_EQ(cref7.get(), words[fixture.m_offset + 7]);
                const auto cref8 = array[8];
                REQUIRE(cref8.has_value());
                CHECK_EQ(cref8.get(), words[fixture.m_offset + 8]);
            }

            SUBCASE("mutable")
            {
                layout_type array(std::move(fixture.m_arrow_proxy));
                REQUIRE_EQ(array.size(), fixture.m_length - fixture.m_offset);
                auto ref0 = array[0];
                REQUIRE(ref0.has_value());
                CHECK_EQ(ref0.get(), words[fixture.m_offset]);
                auto ref1 = array[1];
                REQUIRE_FALSE(ref1.has_value());
                auto ref2 = array[2];
                REQUIRE(ref2.has_value());
                CHECK_EQ(ref2.get(), words[fixture.m_offset + 2]);
                auto ref3 = array[3];
                REQUIRE(ref3.has_value());
                CHECK_EQ(ref3.get(), words[fixture.m_offset + 3]);
                auto ref4 = array[4];
                REQUIRE_FALSE(ref4.has_value());
                auto ref5 = array[5];
                REQUIRE(ref5.has_value());
                CHECK_EQ(ref5.get(), words[fixture.m_offset + 5]);
                auto ref6 = array[6];
                REQUIRE(ref6.has_value());
                CHECK_EQ(ref6.get(), words[fixture.m_offset + 6]);
                auto ref7 = array[7];
                REQUIRE(ref7.has_value());
                CHECK_EQ(ref7.get(), words[fixture.m_offset + 7]);
                auto ref8 = array[8];
                REQUIRE(ref8.has_value());
                CHECK_EQ(ref8.get(), words[fixture.m_offset + 8]);

                using bytes_type = std::vector<byte_t>;
                bytes_type word61 = {byte_t(14), byte_t(15)};
                array[6] = make_nullable<bytes_type>(bytes_type(word61));
                CHECK_EQ(ref6.get(), word61);
                CHECK_EQ(ref7.get(), words[fixture.m_offset + 7]);
                CHECK_EQ(ref8.get(), words[fixture.m_offset + 8]);

                bytes_type word62 = {byte_t(17)};
                array[6] = make_nullable<bytes_type>(bytes_type(word62));
                CHECK_EQ(ref6.get(), word62);
                CHECK_EQ(ref7.get(), words[fixture.m_offset + 7]);
                CHECK_EQ(ref8.get(), words[fixture.m_offset + 8]);
            }
        }

        TEST_CASE_TEMPLATE_DEFINE("value", T, value_id)
        {
            binary_array_fixture<T> fixture;
            using layout_type = T;

            std::vector<std::vector<byte_t>> words = test::make_testing_bytes(fixture.m_length);

            SUBCASE("const")
            {
                const layout_type array(std::move(fixture.m_arrow_proxy));
                CHECK_EQ(array.value(0), words[fixture.m_offset]);
                CHECK_EQ(array.value(1), words[fixture.m_offset + 1]);
                CHECK_EQ(array.value(2), words[fixture.m_offset + 2]);
                CHECK_EQ(array.value(3), words[fixture.m_offset + 3]);
                CHECK_EQ(array.value(4), words[fixture.m_offset + 4]);
                CHECK_EQ(array.value(5), words[fixture.m_offset + 5]);
                CHECK_EQ(array.value(6), words[fixture.m_offset + 6]);
            }

            SUBCASE("mutable")
            {
                layout_type array(std::move(fixture.m_arrow_proxy));
                CHECK_EQ(array.value(0), words[fixture.m_offset]);
                CHECK_EQ(array.value(1), words[fixture.m_offset + 1]);
                CHECK_EQ(array.value(2), words[fixture.m_offset + 2]);
                CHECK_EQ(array.value(3), words[fixture.m_offset + 3]);
                CHECK_EQ(array.value(4), words[fixture.m_offset + 4]);
                CHECK_EQ(array.value(5), words[fixture.m_offset + 5]);
                CHECK_EQ(array.value(6), words[fixture.m_offset + 6]);
                CHECK_EQ(array.value(7), words[fixture.m_offset + 7]);
                CHECK_EQ(array.value(8), words[fixture.m_offset + 8]);

                using bytes_type = std::vector<byte_t>;
                bytes_type word61 = {byte_t(14), byte_t(15)};
                array.value(6) = word61;
                CHECK_EQ(array.value(6), word61);
                CHECK_EQ(array.value(7), words[fixture.m_offset + 7]);
                CHECK_EQ(array.value(8), words[fixture.m_offset + 8]);

                bytes_type word62 = {byte_t(17)};
                array.value(6) = word62;
                CHECK_EQ(array.value(6), word62);
                CHECK_EQ(array.value(7), words[fixture.m_offset + 7]);
                CHECK_EQ(array.value(8), words[fixture.m_offset + 8]);
            }
        }

        TEST_CASE_TEMPLATE_DEFINE("const_bitmap_iterator", T, const_bitmap_iterator_id)
        {
            binary_array_fixture<T> fixture;
            using layout_type = T;

            SUBCASE("ordering")
            {
                const layout_type array(std::move(fixture.m_arrow_proxy));
                const auto array_bitmap = array.bitmap();
                CHECK(array_bitmap.begin() < array_bitmap.end());
            }

            SUBCASE("equality")
            {
                const layout_type array(std::move(fixture.m_arrow_proxy));
                const auto array_bitmap = array.bitmap();

                typename layout_type::const_bitmap_iterator citer = array_bitmap.begin();
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

        TEST_CASE_TEMPLATE_DEFINE("iterator", T, iterator_id)
        {
            binary_array_fixture<T> fixture;
            using layout_type = T;

            std::vector<std::vector<byte_t>> words = test::make_testing_bytes(fixture.m_length);

            SUBCASE("const")
            {
                const layout_type array(std::move(fixture.m_arrow_proxy));
                auto it = array.cbegin();

                REQUIRE(it->has_value());
                CHECK_EQ(it->value(), words[fixture.m_offset]);
                CHECK_EQ(*it, make_nullable(array[0].value()));
                ++it;

                CHECK_FALSE(it->has_value());
                CHECK_EQ(it->get(), words[fixture.m_offset + 1]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[fixture.m_offset + 2]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[fixture.m_offset + 3]);
                ++it;

                CHECK_FALSE(it->has_value());
                CHECK_EQ(it->get(), words[fixture.m_offset + 4]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[fixture.m_offset + 5]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[fixture.m_offset + 6]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[fixture.m_offset + 7]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[fixture.m_offset + 8]);
                ++it;

                CHECK_EQ(it, array.end());
            }

            SUBCASE("non const")
            {
                layout_type array(std::move(fixture.m_arrow_proxy));
                auto it = array.begin();

                REQUIRE(it->has_value());
                CHECK_EQ(it->value(), words[fixture.m_offset]);
                CHECK_EQ(*it, make_nullable(array[0].value()));
                ++it;

                CHECK_FALSE(it->has_value());
                CHECK_EQ(it->get(), words[fixture.m_offset + 1]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[fixture.m_offset + 2]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[fixture.m_offset + 3]);
                ++it;

                CHECK_FALSE(it->has_value());
                CHECK_EQ(it->get(), words[fixture.m_offset + 4]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[fixture.m_offset + 5]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[fixture.m_offset + 6]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[fixture.m_offset + 7]);
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), words[fixture.m_offset + 8]);
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
                CHECK_EQ(it->get(), words[fixture.m_offset + 8]);
            }
        }

        TEST_CASE_TEMPLATE_DEFINE("zero_null_values", T, zero_null_values_id)
        {
            binary_array_fixture<T> fixture;
            using layout_type = T;

            layout_type array(std::move(fixture.m_arrow_proxy));
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
        TEST_CASE_TEMPLATE_DEFINE("formatting", T, formatting_id)
        {
            binary_array_fixture<T> fixture;
            using layout_type = T;

            const layout_type array(std::move(fixture.m_arrow_proxy));
            const std::string formatted = std::format("{}", array);

            if constexpr (std::same_as<layout_type, binary_array>)
            {
                constexpr std::string_view
                    expected = "`Binary [name=test | size=9] <<0x01, 0x01, 0xff, 0x00>, null, <0x02, 0x03>, <0x03, 0x05, 0xff>, null, <0x08, 0x0d>, <0x0d, 0x15, 0xfb, 0x08>, <0x15, 0x22, 0xf8>, <0x22, 0x37>>";
                CHECK_EQ(formatted, expected);
            }
            else if constexpr (std::same_as<layout_type, big_binary_array>)
            {
                constexpr std::string_view
                    expected = "`Large binary [name=test | size=9] <<0x01, 0x01, 0xff, 0x00>, null, <0x02, 0x03>, <0x03, 0x05, 0xff>, null, <0x08, 0x0d>, <0x0d, 0x15, 0xfb, 0x08>, <0x15, 0x22, 0xf8>, <0x22, 0x37>>";
                CHECK_EQ(formatted, expected);
            }
        }
#endif

        // Apply the template tests to both binary_array and big_binary_array
        TEST_CASE_TEMPLATE_APPLY(constructor_id, binary_array_types);
        TEST_CASE_TEMPLATE_APPLY(copy_id, binary_array_types);
        TEST_CASE_TEMPLATE_APPLY(move_id, binary_array_types);
        TEST_CASE_TEMPLATE_APPLY(size_id, binary_array_types);
        TEST_CASE_TEMPLATE_APPLY(operator_bracket_id, binary_array_types);
        TEST_CASE_TEMPLATE_APPLY(value_id, binary_array_types);
        TEST_CASE_TEMPLATE_APPLY(const_bitmap_iterator_id, binary_array_types);
        TEST_CASE_TEMPLATE_APPLY(iterator_id, binary_array_types);
        TEST_CASE_TEMPLATE_APPLY(zero_null_values_id, binary_array_types);
#if defined(__cpp_lib_format)
        TEST_CASE_TEMPLATE_APPLY(formatting_id, binary_array_types);
#endif
    }
}
