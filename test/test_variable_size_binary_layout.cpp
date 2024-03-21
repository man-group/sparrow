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

#include "doctest/doctest.h"

#include <numeric>
#include <string_view>

#include "sparrow/array_data.hpp"
#include "sparrow/variable_size_binary_layout.hpp"

#include <iostream>

namespace sparrow
{
    struct vs_binary_fixture
    {
        vs_binary_fixture()
        {
            m_data.buffers.resize(2);
            m_data.buffers[0].resize(sizeof(std::int64_t) * (nb_words + 1));
            m_data.buffers[1].resize(std::accumulate(
                words, words + nb_words, 0u, [](std::size_t res, const auto& s) { return res + s.size(); }
            ));
            m_data.buffers[0].data<std::int64_t>()[0] = 0u;
            auto iter = m_data.buffers[1].begin();
            for (size_t i = 0; i < nb_words; ++i)
            {
                offset()[i+1] = offset()[i] + words[i].size();
                std::copy(words[i].cbegin(), words[i].cend(), iter);
                iter += words[i].size();
            }

            m_data.length = 4;
            m_data.offset = 1;
        }

        static constexpr size_t nb_words = 4u;
        static constexpr std::string words[nb_words] = 
        {
            "you",
            "are",
            "not",
            "prepared"
        };

        array_data m_data;
        using layout_type = variable_size_binary_layout<std::string, std::string_view, std::string_view>;
    
    private:

        std::int64_t* offset()
        {
            return m_data.buffers[0].data<std::int64_t>();
        }
    };

    TEST_SUITE("variable_size_binary_layout")
    {
        TEST_CASE_FIXTURE(vs_binary_fixture, "types")
        {
            static_assert(std::same_as<layout_type::inner_const_reference, std::string_view>);
            using const_value_iterator = layout_type::const_value_iterator;
            static_assert(std::same_as<const_value_iterator::value_type, std::string>);
            static_assert(std::same_as<const_value_iterator::reference, std::string_view>);
        }

        TEST_CASE_FIXTURE(vs_binary_fixture, "size")
        {
            layout_type l(m_data);
            CHECK_EQ(l.size(), m_data.length - m_data.offset);
        }

        TEST_CASE_FIXTURE(vs_binary_fixture, "operator[]")
        {
            layout_type l(m_data);
            auto cref0 = l[0];
            auto cref1 = l[1];
            auto cref2 = l[2];

            CHECK_EQ(cref0.value(), words[1]);
            CHECK_EQ(cref1.value(), words[2]);
            CHECK_EQ(cref2.value(), words[3]);
        }

        TEST_CASE_FIXTURE(vs_binary_fixture, "const_value_iterator")
        {
            layout_type l(m_data);
            auto cref0 = l[0];
            auto cref1 = l[1];
            auto cref2 = l[2];

            auto iter = l.value_cbegin();
            CHECK_EQ(*iter, cref0.value());
            ++iter;
            CHECK_EQ(*iter, cref1.value());
            --iter;
            CHECK_EQ(*iter, cref0.value());
            iter += 2;
            CHECK_EQ(*iter, cref2.value());
            ++iter;
            CHECK_EQ(iter, l.value_cend());
        }
    }
}
