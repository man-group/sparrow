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

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/c_interface.hpp"

#include "../test/external_array_data_creation.hpp"
#include "doctest/doctest.h"
#include "sparrow/layout/variable_size_binary_array.hpp"


namespace sparrow
{
    struct variable_size_binary_fixture
    {
        using layout_type = variable_size_binary_array<std::string, std::string_view>;

        variable_size_binary_fixture()
            : m_arrow_proxy(create_arrow_proxy())
        {
        }

        arrow_proxy m_arrow_proxy;
        static constexpr size_t m_length = 10;
        static constexpr size_t m_offset = 1;
        static constexpr std::array<size_t, 2> m_false_bitmap{2, 5};

    private:

        static_assert(std::same_as<layout_type::inner_value_type, std::string>);
        static_assert(std::same_as<layout_type::inner_reference, sparrow::variable_size_binary_reference<layout_type>>);
        static_assert(std::same_as<layout_type::inner_const_reference, std::string_view>);
        using const_value_iterator = layout_type::const_value_iterator;
        static_assert(std::same_as<const_value_iterator::value_type, std::string>);

        // static_assert(std::same_as<const_value_iterator::reference, std::string_view>);

        arrow_proxy create_arrow_proxy()
        {
            ArrowSchema schema{};
            ArrowArray array{};
            const std::vector<size_t> false_bitmap{m_false_bitmap.begin(), m_false_bitmap.end()};
            test::fill_schema_and_array<std::string>(schema, array, m_length, m_offset, false_bitmap);
            return arrow_proxy{std::move(array), std::move(schema)};
        }
    };

    TEST_SUITE("variable_size_binary_array")
    {
        TEST_CASE_FIXTURE(variable_size_binary_fixture, "constructor")
        {
            SUBCASE("copy arrow_proxy")
            {
                CHECK_NOTHROW(layout_type array(m_arrow_proxy));
            }

            SUBCASE("move arrow_proxy")
            {
                CHECK_NOTHROW(layout_type array(std::move(m_arrow_proxy)));
            }
        }

        TEST_CASE_FIXTURE(variable_size_binary_fixture, "copy")
        {
            layout_type ar(m_arrow_proxy);
            layout_type ar2(ar);
            CHECK_EQ(ar, ar2);

            layout_type ar3(std::move(m_arrow_proxy));
            ar3 = ar2;
            CHECK_EQ(ar2, ar3);
        }

        TEST_CASE_FIXTURE(variable_size_binary_fixture, "move")
        {
            layout_type ar(m_arrow_proxy);
            layout_type ar2(ar);
            layout_type ar3(std::move(ar));
            CHECK_EQ(ar2, ar3);

            layout_type ar4(std::move(m_arrow_proxy));
            ar4 = std::move(ar3);
            CHECK_EQ(ar2, ar4);
        }

        TEST_CASE_FIXTURE(variable_size_binary_fixture, "size")
        {
            const layout_type array(std::move(m_arrow_proxy));
            CHECK_EQ(array.size(), m_length - m_offset);
        }

        TEST_CASE_FIXTURE(variable_size_binary_fixture, "operator[]")
        {
            SUBCASE("const")
            {
                const layout_type array(std::move(m_arrow_proxy));
                REQUIRE_EQ(array.size(), m_length - m_offset);
                const auto cref0 = array[0];
                REQUIRE(cref0.has_value());
                CHECK_EQ(cref0.value(), "upon");
                const auto cref1 = array[1];
                REQUIRE_FALSE(cref1.has_value());
                const auto cref2 = array[2];
                REQUIRE(cref2.has_value());
                CHECK_EQ(cref2.value(), "time");
                const auto cref3 = array[3];
                REQUIRE(cref3.has_value());
                CHECK_EQ(cref3.value(), "I");
                const auto cref4 = array[4];
                REQUIRE_FALSE(cref4.has_value());
                const auto cref5 = array[5];
                REQUIRE(cref5.has_value());
                CHECK_EQ(cref5.value(), "writing");
                const auto cref6 = array[6];
                REQUIRE(cref6.has_value());
                CHECK_EQ(cref6.value(), "clean");
                const auto cref7 = array[7];
                REQUIRE(cref7.has_value());
                CHECK_EQ(cref7.value(), "code");
                const auto cref8 = array[8];
                REQUIRE(cref8.has_value());
                CHECK_EQ(cref8.value(), "now");
            }
        }

        // TEST_CASE("value_iterator")
        // {
        //     SUBCASE("ordering")
        //     {
        //     }

        //     SUBCASE("equality")
        //     {
        //     }
        // }

        TEST_CASE_FIXTURE(variable_size_binary_fixture, "const_value_iterator")
        {
            SUBCASE("ordering")
            {
                const layout_type array(std::move(m_arrow_proxy));
                const auto vrange = array.values();
                CHECK(vrange.begin() < vrange.end());
            }

            SUBCASE("equality")
            {
                const layout_type array(std::move(m_arrow_proxy));
                const auto vrange = array.values();
                layout_type::const_value_iterator citer = vrange.begin();
                CHECK_EQ(*citer, "upon");
                CHECK_EQ(*(++citer), "a");
                CHECK_EQ(*(++citer), "time");
                CHECK_EQ(*(++citer), "I");
                CHECK_EQ(*(++citer), "was");
                CHECK_EQ(*(++citer), "writing");
                CHECK_EQ(*(++citer), "clean");
                CHECK_EQ(*(++citer), "code");
                CHECK_EQ(*(++citer), "now");
            }
        }

        TEST_CASE_FIXTURE(variable_size_binary_fixture, "const_bitmap_iterator")
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
                CHECK_EQ(*citer, true);
                CHECK_EQ(*(++citer), false);
                CHECK_EQ(*(++citer), true);
                CHECK_EQ(*(++citer), true);
                CHECK_EQ(*(++citer), false);
                CHECK_EQ(*(++citer), true);
                CHECK_EQ(*(++citer), true);
                CHECK_EQ(*(++citer), true);
                CHECK_EQ(*(++citer), true);
            }
        }

        TEST_CASE_FIXTURE(variable_size_binary_fixture, "iterator")
        {
            const layout_type array(std::move(m_arrow_proxy));
            auto it = array.begin();

            REQUIRE(it->has_value());
            CHECK_EQ(it->value(), "upon");
            CHECK_EQ(*it, make_nullable(array[0].value()));
            ++it;

            CHECK_FALSE(it->has_value());
            ++it;

            REQUIRE(it->has_value());
            CHECK_EQ(it->value(), "time");
            ++it;

            REQUIRE(it->has_value());
            CHECK_EQ(it->value(), "I");
            ++it;

            CHECK_FALSE(it->has_value());
            ++it;

            REQUIRE(it->has_value());
            CHECK_EQ(it->value(), "writing");
            ++it;

            REQUIRE(it->has_value());
            CHECK_EQ(it->value(), "clean");
            ++it;

            REQUIRE(it->has_value());
            CHECK_EQ(it->value(), "code");
            ++it;

            REQUIRE(it->has_value());
            CHECK_EQ(it->value(), "now");
            ++it;

            CHECK_EQ(it, array.end());
        }
    }
}
