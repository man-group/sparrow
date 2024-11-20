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

#include "sparrow/builder/builder_utils.hpp"
#include "test_utils.hpp"

#include <vector>
#include <tuple>
#include <variant>
#include <array>

namespace sparrow
{   
    class my_variant : public std::variant<int, double>
    {
    public:
        using std::variant<int, double>::variant;
    };
}


// specialize std::variant_alternative_t for my_variant
template<std::size_t I>
struct std::variant_alternative<I, sparrow::my_variant>
{
    using type = std::variant_alternative_t<I, std::variant<int, double>>;
};

namespace sparrow{

    TEST_SUITE("builder-utils")
    {   
        TEST_CASE("for_each_index")
        {
            SUBCASE("empty")
            {
                // the return type of for_each_index indicates if the loop was run to completion
                detail::for_each_index<0>([](auto i) 
                {
                    // should not be called
                    CHECK_FALSE(decltype(i)::value, 0);
                    CHECK(false);
                    return true; // should not be called
                });
            }
            SUBCASE("non-empt")
            {
                int c = 0;
                detail::for_each_index<3>([&](auto i)
                {
                    CHECK_EQ(decltype(i)::value, c);
                    ++c;
                    return true;
                });
                CHECK_EQ(c, 3);
            }
        }

        TEST_CASE("exitable_for_each_index")
        {   
            SUBCASE("empty")
            {
                // the return type of exitable_for_each_index indicates if the loop was run to completion
                CHECK(detail::exitable_for_each_index<0>([](auto  i)
                {
                    // should not be called
                    CHECK_FALSE(decltype(i)::value, 0);
                    CHECK(false);
                    return true; // should not be called
                }));
            }
            SUBCASE("exit right away")
            {
                // the return type of exitable_for_each_index indicates if the loop was run to completion
                CHECK_FALSE(detail::exitable_for_each_index<3>([](auto i)
                {
                    // exit right away
                    CHECK(decltype(i)::value == 0);
                    return false;
                }));
            }
            
            SUBCASE("full")
            {
                int c=0;
                CHECK(detail::exitable_for_each_index<3>([&](auto i)
                {
                    // run to completion
                    CHECK_EQ(decltype(i)::value, c);
                    ++c;
                    return true;
                }));
                CHECK_EQ(c, 3);
            }

            SUBCASE("half")
            {
                int c = 0;
                CHECK_FALSE(detail::exitable_for_each_index<4>([&](auto i)
                {
                    CHECK_EQ(decltype(i)::value, c);
                    ++c;
                    return decltype(i)::value < 2;
                }));
                CHECK_EQ(c, 3);
            }
        }

        // concepts and types
        TEST_CASE("types")
        {
            // nullable
            {
                static_assert(detail::is_nullable_like<nullable<int>>);
                static_assert(!detail::is_nullable_like<std::tuple<int>>);
            }

            // is_express_layout_desire
            static_assert(detail::is_express_layout_desire<run_end_encode<int>>);
            static_assert(detail::is_express_layout_desire<run_end_encode<int>>);
            static_assert(!detail::is_express_layout_desire<nullable<run_end_encode<int>>>);


            // decayed_range_value_t
            static_assert(std::is_same_v<detail::decayed_range_value_t<std::vector<int>>, int>);


            // is tuple_like
            static_assert(detail::tuple_like<std::tuple<int>>);
            static_assert(detail::tuple_like<std::tuple<int,int>>);
            static_assert(detail::tuple_like<std::tuple<>>);
            static_assert(detail::tuple_like<std::pair<int, int>>);
            static_assert(detail::tuple_like<std::array<int, 3>>);
            static_assert(!detail::tuple_like<std::vector<int>>);
            static_assert(!detail::tuple_like<int>);


            // all elements the same
            static_assert(detail::all_elements_same<std::tuple<int, int, int>>);
            static_assert(detail::all_elements_same<std::tuple<int, int>>);
            static_assert(detail::all_elements_same<std::tuple<int>>);
            static_assert(!detail::all_elements_same<std::tuple<int, double>>);
    

            // variant like
            static_assert(detail::variant_like<std::variant<int>>);
            static_assert(detail::variant_like<std::variant<int, float>>);
            static_assert(detail::variant_like<std::variant<int, bool>>);
            static_assert(!detail::variant_like<int>);
            static_assert(!detail::variant_like<std::tuple<int>>);
            static_assert(detail::variant_like<my_variant>);


            // maybe_nullable_value_type
            static_assert(std::is_same_v<detail::mnv_t<nullable<int>>, int>);
            static_assert(std::is_same_v<detail::mnv_t<int>, int>);
            static_assert(std::is_same_v<detail::mnv_t<nullable<nullable<int>>>, nullable<int>>);


            // maybe_express_layout_desire_value_type
            static_assert(std::is_same_v<detail::meldv_t<run_end_encode<int>>, int>);
            static_assert(std::is_same_v<detail::meldv_t<int>, int>);
            static_assert(std::is_same_v<detail::meldv_t<nullable<int>>, nullable<int>>);
            static_assert(std::is_same_v<detail::meldv_t<nullable<run_end_encode<int>>> , nullable<run_end_encode<int>>>);

            // layout_flag_t
            static_assert(std::is_same_v<detail::layout_flag_t<int>, detail::dont_enforce_layout>);
            static_assert(std::is_same_v<detail::layout_flag_t<run_end_encode<int>>, detail::enforce_run_end_encoded_layout>);
            static_assert(std::is_same_v<detail::layout_flag_t<nullable<run_end_encode<int>>>, detail::enforce_run_end_encoded_layout>);
            static_assert(std::is_same_v<detail::layout_flag_t<dict_encode<int>>, detail::enforce_dict_encoded_layout>);
            static_assert(std::is_same_v<detail::layout_flag_t<nullable<dict_encode<int>>>, detail::enforce_dict_encoded_layout>);


            // look trough
            static_assert(std::is_same_v<detail::look_trough_t<std::vector<nullable<int>>>, std::vector<nullable<int>>>);
            static_assert(std::is_same_v<detail::look_trough_t<nullable<std::vector<int>>>, std::vector<int>>);
            static_assert(std::is_same_v<detail::look_trough_t<nullable<int>>, int>);
            static_assert(std::is_same_v<detail::look_trough_t<nullable<nullable<int>>>, nullable<int>>);
            static_assert(std::is_same_v<detail::look_trough_t<dict_encode<std::vector<int>>>, std::vector<int>>);
            static_assert(std::is_same_v<detail::look_trough_t<dict_encode<nullable<int>>>, int>);

        } 
        TEST_CASE("get_size_save")
        {
            CHECK_EQ(detail::get_size_save(std::vector<int>{1, 2, 3}), 3);
            CHECK_EQ(detail::get_size_save(std::vector<int>{}), 0);
            CHECK_EQ(detail::get_size_save(nullable<std::vector<int>>{std::vector<int>{1, 2, 3}}), 3);
            CHECK_EQ(detail::get_size_save(nullable<std::vector<int>>{std::vector<int>{}}), 0);
            CHECK_EQ(detail::get_size_save(dict_encode<std::vector<int>>{std::vector<int>{1, 2, 3}}), 3);
        }
        TEST_CASE("ensure_value")
        {
            CHECK_EQ(detail::ensure_value(1), 1);
            CHECK_EQ(detail::ensure_value(nullable<int>{1}), 1);
            CHECK_EQ(detail::ensure_value(dict_encode<int>{1}), 1);
            CHECK_EQ(detail::ensure_value(run_end_encode<int>{1}), 1);
            CHECK_EQ(detail::ensure_value(nullable<dict_encode<int>>{dict_encode<int>{1}}), 1);
        }       
        TEST_CASE("where-null")
        {

            SUBCASE("vector-of-nullables")
            {
                std::vector<nullable<int>> v{1, 2, sparrow::nullval, 4};
                std::vector<std::size_t> res = detail::where_null(v);
                CHECK_EQ(res.size(), 1);
                CHECK_EQ(res[0], 2);
            }
            SUBCASE("vector-of-scalar")
            {
                std::vector<int> v{1, 2, 3, 4};
                std::array<std::size_t,0> res = detail::where_null(v);
                CHECK_EQ(res.size(), 0); // pointless but uses the return value
            }
            SUBCASE("vector-of-nullabels-dict-encode")
            {
                std::vector<nullable<dict_encode<int>>> v{
                    dict_encode<int>{1},
                    sparrow::nullval,
                    dict_encode<int>{3},
                    dict_encode<int>{4}
                };
                std::vector<std::size_t> res = detail::where_null(v);
                CHECK_EQ(res.size(), 1);
                CHECK_EQ(res[0], 1);
            }
            SUBCASE("vector-of-nullabels-dict-encode")
            {
                std::vector<dict_encode<nullable<int>>> v{
                    dict_encode<nullable<int>>{nullable<int>{int(1)}},   
                    dict_encode<nullable<int>>{nullable<int>{}},   
                    dict_encode<nullable<int>>{nullable<int>{int(2)}},   
                    dict_encode<nullable<int>>{nullable<int>{int(3)}}
                };
                std::vector<std::size_t> res = detail::where_null(v);
                CHECK_EQ(res.size(), 1);
                CHECK_EQ(res[0], 1);
            }
            
        }
    }
}

