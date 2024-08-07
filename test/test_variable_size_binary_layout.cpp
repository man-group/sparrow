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

#include "sparrow/array/array_data.hpp"
#include "sparrow/array/array_data_factory.hpp"
#include "sparrow/layout/variable_size_binary_layout.hpp"
#include "sparrow/utils/contracts.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    struct vs_binary_fixture
    {
        using layout_type = variable_size_binary_layout<std::string, std::string_view>;

        vs_binary_fixture()
        {
            array_data::bitmap_type bitmap{words.size(), true};
            bitmap.set(2, false);
            m_data = make_default_array_data<layout_type>(words, bitmap, 1);
        }

        static constexpr std::array<std::string_view, 4> words = {"you", "are", "not", "prepared"};
        array_data m_data;

    private:

        static_assert(std::same_as<layout_type::inner_value_type, std::string>);
        static_assert(std::same_as<layout_type::inner_reference, sparrow::vs_binary_reference<layout_type>>);
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
            CHECK_EQ(l.size(), m_data.length - m_data.offset);
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

            l[0].value() = std::string("is");
            l[2].value() = std::string("unpreparedandmore");

            CHECK_EQ(cref0.value(), std::string("is"));
            CHECK(!cref1.has_value());
            CHECK_EQ(cref2.value(), std::string("unpreparedandmore"));

            l[0].value() = std::string("are");
            l[2].value() = std::string("ok");

            CHECK_EQ(cref0.value(), std::string("are"));
            CHECK(!cref1.has_value());
            CHECK_EQ(cref2.value(), std::string("ok"));
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
            const layout_type l(m_data);
            auto cref0 = l[0];
            auto cref2 = l[2];

            auto iter = l.cbegin();
            CHECK_EQ(*iter, make_nullable(cref0.value()));

            ++iter;
            CHECK(!iter->has_value());

            ++iter;
            CHECK_EQ(iter->value(), cref2.value());

            iter++;
            CHECK_EQ(iter, l.cend());

            iter -= 3;
            CHECK_EQ(iter, l.cbegin());
        }

        TEST_CASE_FIXTURE(vs_binary_fixture, "vs_binary_reference")
        {
            static_assert(std::ranges::sized_range<layout_type::inner_reference>);

            m_data.bitmap.set(2, true);
            layout_type l(m_data);
            const layout_type& cl = l;

            SUBCASE("sanity")
            {
                CHECK_EQ(cl[0].value(), words[1]);
                CHECK_EQ(cl[1].has_value(), true);
                CHECK_EQ(cl[1].value().size(), 0);
            }

            SUBCASE("size")
            {
                
                auto ref0 = l[0].value();
                CHECK_EQ(ref0.size(), 3);
            }

            SUBCASE("iterator")
            {
                
                auto ref0 = l[0].value();
                auto cref0 = cl[0].value();
                auto cref1 = cl[1].value();
                auto it = ref0.begin();
                auto it_end = ref0.end();
                std::fill(it, it_end, 'a');
                CHECK_EQ(cref0, "aaa");
                CHECK_EQ(cref1, "");
            }

            SUBCASE("const_iterator")
            {
                auto cref = cl[0].value();
                auto iter = cref.begin();
                auto citer = cref.cbegin();
                auto exp_iter = words[1].cbegin();
                CHECK_EQ(*iter, *exp_iter);
                CHECK_EQ(*citer, *exp_iter);
                ++iter, ++citer, ++exp_iter;
                CHECK_EQ(*iter, *exp_iter);
                CHECK_EQ(*citer, *exp_iter);
                ++iter, ++citer, ++exp_iter;
                CHECK_EQ(*iter, *exp_iter);
                CHECK_EQ(*citer, *exp_iter);
                ++iter, ++citer, ++exp_iter;
                CHECK_EQ(iter, cref.end());
                CHECK_EQ(citer, cref.cend());
            }

            SUBCASE("assign_same_size")
            {
                auto ref0 = l[0].value();
                ref0 = "coi";
                CHECK_EQ(cl[0].value(), "coi");
                CHECK_EQ(cl[1].value(), "");
                CHECK_EQ(cl[2].value(), words[3]);
            }

            SUBCASE("assign_larger")
            {
                auto ref0 = l[0].value();
                ref0 = "coin";
                CHECK_EQ(cl[0].value(), "coin");
                CHECK_EQ(cl[1].value(), "");
                CHECK_EQ(cl[2].value(), words[3]);
            }

            SUBCASE("assign_smaller")
            {
                auto ref0 = l[0].value();
                ref0 = "am";
                CHECK_EQ(cl[0].value(), "am");
                CHECK_EQ(cl[1].value(),"");
                CHECK_EQ(cl[2].value(), words[3]);
            }

            SUBCASE("assign_strings")
            {
                m_data.offset = 0;

                auto ref0 = l[0];
                auto ref1 = l[1];
                auto ref2 = l[2];
                auto ref3 = l[3];

                ref3.value() = std::string_view("unpreparedandmore");

                CHECK_EQ(ref0.value(), words[0]);
                CHECK_EQ(ref1.value(), words[1]);
                CHECK_EQ(ref2.value(), "");
                CHECK_EQ(ref3.value(), std::string("unpreparedandmore"));

                ref0.value() = std::string("he");
                ref1.value() = std::string("is");
                ref2.value() = std::string("");

                CHECK_EQ(ref0.value(), std::string("he"));
                CHECK_EQ(ref1.value(), std::string("is"));
                CHECK_EQ(ref2.value(), std::string_view(""));
                CHECK_EQ(ref3.value(), std::string("unpreparedandmore"));
            }

            SUBCASE("assign self_type")
            {
                m_data.offset = 0;
                auto ref0 = l[0];
                auto ref1 = l[1];
                auto ref2 = l[2];
                auto ref3 = l[3];

                constexpr std::array<std::string_view, 4> replacement_words = {"this", "is", "a", "replacement"};

                array_data::bitmap_type rpl_bitmap{replacement_words.size(), true};
                array_data rpl_vs_data = make_default_array_data<layout_type>(replacement_words, rpl_bitmap, 0);

                layout_type rpl_l(rpl_vs_data);
                CHECK_EQ(rpl_l.size(), rpl_vs_data.length - rpl_vs_data.offset);

                vs_binary_reference<layout_type> rpl_vs_ref0(&rpl_l, 0);
                vs_binary_reference<layout_type> rpl_vs_ref1(&rpl_l, 1);
                vs_binary_reference<layout_type> rpl_vs_ref2(&rpl_l, 2);
                vs_binary_reference<layout_type> rpl_vs_ref3(&rpl_l, 3);

                ref0.value() = std::move(rpl_vs_ref0);
                ref1.value() = std::move(rpl_vs_ref1);
                ref2.value() = std::move(rpl_vs_ref2);
                ref3.value() = std::move(rpl_vs_ref3);

                CHECK_EQ(ref0.value(), std::string("this"));
                CHECK_EQ(ref1.value(), std::string("is"));
                CHECK_EQ(ref2.value(), std::string("a"));
                CHECK_EQ(ref3.value(), std::string("replacement"));
            }

            SUBCASE("equality_comparison")
            {
                m_data.offset = 0;
                auto ref0 = l[0];
                auto ref3 = l[3];

                vs_binary_reference<layout_type> vs_ref0(&l, 0);
                CHECK_EQ(ref0.value(), vs_ref0);

                vs_binary_reference<layout_type> vs_ref3(&l, 3);
                CHECK_EQ(ref3.value(), vs_ref3);

                CHECK_NE(ref0.value(), vs_ref3);
                CHECK_NE(ref3.value(), vs_ref0);
            }

            SUBCASE("inequality_comparison")
            {
                m_data.offset = 0;
                auto ref0 = l[0].value();
                auto ref3 = l[3].value();

                CHECK(ref3 < ref0);
                CHECK(ref3 <= ref0);
                CHECK(ref0 >= ref3);
                CHECK(ref0 > ref3);
            }
        }
    }
}
