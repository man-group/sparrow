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
#include <iostream>
#include <numeric>
#include <string_view>

#include "sparrow/array_data.hpp"
#include "sparrow/array_data_factory.hpp"
#include "sparrow/contracts.hpp"
#include "sparrow/variable_size_binary_layout.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    struct vs_binary_fixture
    {
        using layout_type = variable_size_binary_layout<std::string, std::string_view, std::string_view>;

        vs_binary_fixture()
        {
            array_data::bitmap_type bitmap{words.size(), true};
            bitmap.set(2, false);
            m_data = make_default_array_data<layout_type>(words, bitmap, 1);
        }

        static constexpr std::array<std::string_view, 4> words = {"you", "are", "not", "prepared"};
        array_data m_data;
        // TODO: replace R = std::string_view with specific reference proxy

    private:

        std::int64_t* offset()
        {
            SPARROW_ASSERT_FALSE(m_data.m_buffers.empty());
            return m_data.m_buffers[0].data<std::int64_t>();
        }

        static_assert(std::same_as<layout_type::inner_value_type, std::string>);
        static_assert(std::same_as<layout_type::inner_const_reference, std::string_view>);
        using const_value_iterator = layout_type::const_value_iterator;
        static_assert(std::same_as<const_value_iterator::value_type, std::string>);
        static_assert(std::same_as<const_value_iterator::reference, std::string_view>);
    };

    TEST_SUITE("variable_size_binary_layout")
    {
        TEST_CASE_FIXTURE(vs_binary_fixture, "rebind_data")
        {
            layout_type l(m_data);
            static constexpr std::array<std::string_view, 3> new_words = {"tambourines", "and", "elephant"};
            array_data::bitmap_type new_bitmap{new_words.size(), true};
            auto data = make_default_array_data<layout_type>(new_words, new_bitmap, 0);
            l.rebind_data(data);
            for (std::size_t i = 0; i < new_words.size(); ++i)
            {
                CHECK_EQ(l[i].value(), new_words[i]);
            }
        }

        TEST_CASE_FIXTURE(vs_binary_fixture, "size")
        {
            layout_type l(m_data);
            CHECK_EQ(l.size(), m_data.m_length - m_data.m_offset);
        }

        TEST_CASE_FIXTURE(vs_binary_fixture, "operator[]")
        {
            layout_type l(m_data);
            auto cref0 = l[0];
            auto cref1 = l[1];
            auto cref2 = l[2];

            CHECK_EQ(cref0.value(), words[1]);
            CHECK(!cref1.has_value());
            CHECK_EQ(cref2.value(), words[3]);
        }

        TEST_CASE_FIXTURE(vs_binary_fixture, "const_value_iterator")
        {
            layout_type l(m_data);
            auto cref0 = l[0];
            auto cref2 = l[2];

            auto vrange = l.values();
            auto iter = vrange.begin();
            CHECK_EQ(*iter, cref0.value());
            ++iter;
            --iter;
            CHECK_EQ(*iter, cref0.value());
            iter += 2;
            CHECK_EQ(*iter, cref2.value());
            ++iter;
            CHECK_EQ(iter, vrange.end());
        }

        TEST_CASE_FIXTURE(vs_binary_fixture, "const_bitmap_iterator")
        {
            layout_type l(m_data);
            auto brange = l.bitmap();
            auto iter = brange.begin();
            CHECK(*iter);
            ++iter;
            CHECK(!*iter);
            iter += 2;
            CHECK_EQ(iter, brange.end());
        }

        TEST_CASE_FIXTURE(vs_binary_fixture, "const_iterator")
        {
            layout_type l(m_data);
            auto cref0 = l[0];
            auto cref2 = l[2];

            auto iter = l.cbegin();
            CHECK_EQ(*iter, std::make_optional(cref0.value()));

            ++iter;
            CHECK(!iter->has_value());

            ++iter;
            CHECK_EQ(iter->value(), cref2.value());

            iter++;
            CHECK_EQ(iter, l.cend());

            iter -= 3;
            CHECK_EQ(iter, l.cbegin());
        }
    }
}
