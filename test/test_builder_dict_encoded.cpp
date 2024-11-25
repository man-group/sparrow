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

#include "sparrow/builder/builder.hpp"
#include "test_utils.hpp"

#include <vector>
#include <tuple>
#include <variant>
#include <array>


namespace sparrow
{

    TEST_SUITE("builder")
    {
        TEST_CASE("dict-encoded")
        {
            SUBCASE("dict[int]") 
            {
                SUBCASE("no-nulls")
                {
                    dict_encode<std::vector<int>> v{
                        std::vector<int>{1, 1, 1, 2}
                    };
                    auto arr = sparrow::build(v);
                    test::generic_consistency_test(arr);
                    using array_type = std::decay_t<decltype(arr)>;
                    static_assert(std::is_same_v<array_type, sparrow::dictionary_encoded_array<std::uint64_t>>);

                    REQUIRE_EQ(arr.size(), 4);
                    CHECK_NULLABLE_VARIANT_EQ(arr[0], 1);
                    CHECK_NULLABLE_VARIANT_EQ(arr[1], 1);
                    CHECK_NULLABLE_VARIANT_EQ(arr[2], 1);
                    CHECK_NULLABLE_VARIANT_EQ(arr[3], 2);
                }

                SUBCASE("with-nulls")
                {
                    dict_encode<std::vector<nullable<int>>> v{
                        std::vector<nullable<int>>{
                            1, 1, sparrow::nullval, 2
                        }
                    }; 
                    auto arr = sparrow::build(v);
                    test::generic_consistency_test(arr);

                    using array_type = std::decay_t<decltype(arr)>;
                    static_assert(std::is_same_v<array_type, sparrow::dictionary_encoded_array<std::uint64_t>>);
                    REQUIRE_EQ(arr.size(), 4);
                    CHECK_NULLABLE_VARIANT_EQ(arr[0], 1);
                    CHECK_NULLABLE_VARIANT_EQ(arr[1], 1);
                    CHECK(!arr[2].has_value());
                    CHECK_NULLABLE_VARIANT_EQ(arr[3], 2);
                }
            }
            SUBCASE("dict[string]")
            {

                dict_encode<std::vector<nullable<std::string>>> v{
                    std::vector<nullable<std::string>>{"hello", "world", "hello", "world", nullable<std::string>{}}
                };
                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);
                using array_type = std::decay_t<decltype(arr)>;
                static_assert(std::is_same_v<array_type, sparrow::dictionary_encoded_array<std::uint64_t>>);

                REQUIRE_EQ(arr.size(), 5);
                CHECK_NULLABLE_VARIANT_EQ(arr[0], std::string_view("hello"));
                CHECK_NULLABLE_VARIANT_EQ(arr[1], std::string_view("world"));
                CHECK_NULLABLE_VARIANT_EQ(arr[2], std::string_view("hello"));
                CHECK_NULLABLE_VARIANT_EQ(arr[3], std::string_view("world"));
                CHECK(!arr[4].has_value());
                test::generic_consistency_test_impl(arr);
                
            }
            SUBCASE("dict[struct[int,float]]")
            {   
                using tuple_type = std::tuple<nullable<int>, std::uint16_t>;
                using nullable_tuple_type = nullable<tuple_type>;
                using vector_type = std::vector<nullable_tuple_type>;

                dict_encode<vector_type> v{
                    vector_type{
                        nullable_tuple_type{tuple_type{nullable<int>{1}, std::uint16_t(1)}},
                        nullable_tuple_type{},
                        nullable_tuple_type{tuple_type{nullable<int>{}, std::uint16_t(42)}},
                        nullable_tuple_type{tuple_type{nullable<int>{}, std::uint16_t(42)}}
                    }
                    
                };

                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);
                using array_type = std::decay_t<decltype(arr)>;
                static_assert(std::is_same_v<array_type, sparrow::dictionary_encoded_array<std::uint64_t>>);

                REQUIRE_EQ(arr.size(), 4);
                
                // 0
                auto arr0 = std::get<nullable<struct_value>>(arr[0]);
                REQUIRE(arr0.has_value());
                CHECK_NULLABLE_VARIANT_EQ(arr0.value()[0], int(1));
                CHECK_NULLABLE_VARIANT_EQ(arr0.value()[1], std::uint16_t(1));

                // 1
                auto arr1 = std::get<nullable<struct_value>>(arr[1]);
                REQUIRE(!arr1.has_value());

                // 2
                auto arr2 = std::get<nullable<struct_value>>(arr[2]);
                REQUIRE(arr2.has_value());
                CHECK(!arr2.value()[0].has_value());
                CHECK_NULLABLE_VARIANT_EQ(arr2.value()[1], std::uint16_t(42));

                // 3
                auto arr3 = std::get<nullable<struct_value>>(arr[3]);
                REQUIRE(arr3.has_value());
                CHECK(!arr3.value()[0].has_value());
                CHECK_NULLABLE_VARIANT_EQ(arr3.value()[1], std::uint16_t(42));

            }
            SUBCASE("dict[list[int]]")
            {
                dict_encode<std::vector<std::vector<int>>> v{
                    std::vector<std::vector<int>>{
                        {1, 2, 3},
                        {4, 5, 6}
                    }
                };
                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);
                using array_type = std::decay_t<decltype(arr)>;
                static_assert(std::is_same_v<array_type, sparrow::dictionary_encoded_array<std::uint64_t>>);

                REQUIRE_EQ(arr.size(), 2);
                CHECK_EQ(std::get<nullable<list_value>>(arr[0]).value().size(), 3);
                CHECK_NULLABLE_VARIANT_EQ(std::get<nullable<list_value>>(arr[0]).value()[0], 1);
                CHECK_NULLABLE_VARIANT_EQ(std::get<nullable<list_value>>(arr[0]).value()[1], 2);
                CHECK_NULLABLE_VARIANT_EQ(std::get<nullable<list_value>>(arr[0]).value()[2], 3);    
        }
            SUBCASE("dict[union[int, string]]")
            {
                dict_encode<std::vector<std::variant<int, std::string>>> v{
                    std::vector<std::variant<int, std::string>>{
                        int(1),
                        std::string("hello"),
                        int(2),
                        std::string("world")
                    }
                };
                auto arr = sparrow::build(v);
                test::generic_consistency_test(arr);
                using array_type = std::decay_t<decltype(arr)>;
                static_assert(std::is_same_v<array_type, sparrow::dictionary_encoded_array<std::uint64_t>>);

                REQUIRE_EQ(arr.size(), 4);
                CHECK_NULLABLE_VARIANT_EQ(arr[0], 1);
                CHECK_NULLABLE_VARIANT_EQ(arr[1], std::string_view("hello"));
                CHECK_NULLABLE_VARIANT_EQ(arr[2], 2);
                CHECK_NULLABLE_VARIANT_EQ(arr[3], std::string_view("world"));
            }

            SUBCASE("list[dict[int]]")
            {
                SUBCASE("without-nulls")
                {
                    std::vector<dict_encode<std::vector<int>>> v{
                        dict_encode<std::vector<int>>{
                            std::vector<int>{1, 2, 3}
                        },
                        dict_encode<std::vector<int>>{
                            std::vector<int>{4, 5}
                        },
                        dict_encode<std::vector<int>>{
                            std::vector<int>{6}
                        }
                    };

                    auto arr = sparrow::build(v);
                    test::generic_consistency_test(arr);
                    using array_type = std::decay_t<decltype(arr)>;
                    static_assert(std::is_same_v<array_type, sparrow::list_array>);

                    // ensure that the child is dict-encoded
                    REQUIRE(arr.raw_flat_array()->is_dictionary());

                    for(std::size_t i = 0; i < 3; ++i)
                    {
                        auto arr_val = arr[i];
                        REQUIRE(arr_val.has_value());
                        REQUIRE_EQ(arr_val.value().size(), v[i].get().size());
                        for(std::size_t j = 0; j < v[i].get().size(); ++j)
                        {
                            CHECK_NULLABLE_VARIANT_EQ(arr_val.value()[j], v[i].get()[j]);
                        }
                    }
                }
                SUBCASE("with-nulls")
                {
                    std::vector<nullable<dict_encode<std::vector<nullable<int>>>>> v{
                        dict_encode<std::vector<nullable<int>>>{
                            std::vector<nullable<int>>{1, 2, 3}
                        },
                        sparrow::nullval,
                        dict_encode<std::vector<nullable<int>>>{
                            std::vector<nullable<int>>{6}
                        },
                        dict_encode<std::vector<nullable<int>>>{
                            std::vector<nullable<int>>{nullable<int>{}}
                        },
                    };

                    auto arr = sparrow::build(v);
                    test::generic_consistency_test(arr);
                    using array_type = std::decay_t<decltype(arr)>;
                    static_assert(std::is_same_v<array_type, sparrow::list_array>);

                    // ensure that the child is dict-encoded
                    REQUIRE(arr.raw_flat_array()->is_dictionary());

                    REQUIRE(arr[0].has_value());
                    REQUIRE_FALSE(arr[1].has_value());
                    REQUIRE(arr[2].has_value());

                    // check the values
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], int(1));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[1], int(2));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[2], int(3));

                    CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[0], int(6));

                    REQUIRE(arr[3].has_value());
                    REQUIRE_EQ(arr[3].value().size(), 1);
                    CHECK(!arr[3].value()[0].has_value());
                }
            }
            SUBCASE("fixed-size-list[dict[string]]")
            {
                SUBCASE("without-nulls")
                {
                    std::vector<dict_encode<std::array<std::string, 3>>> v{
                        dict_encode<std::array<std::string, 3>>{
                            std::array<std::string, 3>{"one", "two", "three"}
                        },
                        dict_encode<std::array<std::string, 3>>{
                            std::array<std::string, 3>{"one", "two", "three"}
                        },
                        dict_encode<std::array<std::string, 3>>{
                            std::array<std::string, 3>{"one", "two", "four"}
                        }
                    };
                    auto arr = sparrow::build(v);
                    test::generic_consistency_test(arr);
                    using array_type = std::decay_t<decltype(arr)>;
                    static_assert(std::is_same_v<array_type, sparrow::fixed_sized_list_array>);

                    // ensure that the child is dict-encoded
                    REQUIRE(arr.raw_flat_array()->is_dictionary());

                    for(std::size_t i = 0; i < 3; ++i)
                    {
                        auto arr_val = arr[i];
                        REQUIRE(arr_val.has_value());
                        REQUIRE_EQ(arr_val.value().size(), 3);
                        for(std::size_t j = 0; j < 3; ++j)
                        {
                            CHECK_NULLABLE_VARIANT_EQ(arr_val.value()[j], std::string_view(v[i].get()[j]));
                        }
                    }
                }
                SUBCASE("with-nulls")
                {   
                    std::vector<nullable<dict_encode<std::array<std::string, 3>>>> v{
                        dict_encode<std::array<std::string, 3>>{
                            std::array<std::string, 3>{"one", "two", "three"}
                        },
                        sparrow::nullval,
                        dict_encode<std::array<std::string, 3>>{
                            std::array<std::string, 3>{"one", "two", "three"}
                        }
                    };
                    auto arr = sparrow::build(v);
                    test::generic_consistency_test(arr);
                    using array_type = std::decay_t<decltype(arr)>;
                    static_assert(std::is_same_v<array_type, sparrow::fixed_sized_list_array>);

                    // ensure that the child is dict-encoded
                    REQUIRE(arr.raw_flat_array()->is_dictionary());

                    REQUIRE(arr[0].has_value());
                    REQUIRE_FALSE(arr[1].has_value());
                    REQUIRE(arr[2].has_value());
                }   
            }           
            SUBCASE("struct[dict[string], int]")
            {   
                SUBCASE("with-nulls")
                {
                    std::vector<nullable<std::tuple<dict_encode<nullable<std::string>>,int>>> v 
                    {
                        std::tuple<dict_encode<nullable<std::string>>, int>{
                            dict_encode<nullable<std::string>>{"hello"}, 1
                        },
                        nullable<std::tuple<dict_encode<nullable<std::string>>, int>>{},
                        std::tuple<dict_encode<nullable<std::string>>, int>{
                            dict_encode<nullable<std::string>>{"!"}, 3
                        },
                        std::tuple<dict_encode<nullable<std::string>>, int>{
                            dict_encode<nullable<std::string>>{
                                nullable<std::string>{}
                            }, 4
                        }
                    };
                    auto arr = sparrow::build(v);
                    test::generic_consistency_test(arr);
                    using array_type = std::decay_t<decltype(arr)>;
                    static_assert(std::is_same_v<array_type, sparrow::struct_array>);
                    REQUIRE_EQ(arr.size(), 4);

                    // has values
                    REQUIRE(arr[0].has_value());
                    REQUIRE_FALSE(arr[1].has_value());
                    REQUIRE(arr[2].has_value());


                    // check the values
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[0], std::string_view("hello"));
                    CHECK_NULLABLE_VARIANT_EQ(arr[0].value()[1], int(1));

                    CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[0], std::string_view("!"));
                    CHECK_NULLABLE_VARIANT_EQ(arr[2].value()[1], int(3));

                    REQUIRE(arr[3].has_value());
                    CHECK_NULLABLE_VARIANT_EQ(arr[3].value()[1], int(4));
                    REQUIRE_FALSE(arr[3].value()[0].has_value());
                    
                }
            }
            SUBCASE("union[[dict[string], int]]")
            {
                SUBCASE("without-nulls")
                {   
                    using variant_type = std::variant<dict_encode<std::string>, int>;

                    std::vector<variant_type> v{
                        variant_type{
                            dict_encode<std::string>{"hello"}
                        },
                        variant_type{
                            int(42)
                        },
                    };
                    auto arr = sparrow::build(v);
                    test::generic_consistency_test(arr);
                    using array_type = std::decay_t<decltype(arr)>;
                    static_assert(std::is_same_v<array_type, sparrow::sparse_union_array>);
                    REQUIRE_EQ(arr.size(), 2);
                    // // check the values
                    CHECK_NULLABLE_VARIANT_EQ(arr[0], std::string_view("hello"));
                    CHECK_NULLABLE_VARIANT_EQ(arr[1], int(42));
                }
                SUBCASE("with-nulls")
                {   
                    using variant_type = std::variant<
                        dict_encode<nullable<std::string>>, 
                        nullable<int>
                    >;
                    std::vector<variant_type> v{
                        variant_type{nullable<std::string>{"hello"}},
                        variant_type{nullable<int>{}},
                        variant_type{}
                    };
                    auto arr = sparrow::build(v);
                    test::generic_consistency_test(arr);
                    using array_type = std::decay_t<decltype(arr)>;
                    static_assert(std::is_same_v<array_type, sparrow::sparse_union_array>);
                    REQUIRE_EQ(arr.size(), 3);

                    // // check the values
                    CHECK_NULLABLE_VARIANT_EQ(arr[0], std::string_view("hello"));

                    CHECK(!arr[1].has_value());
                    CHECK(!arr[2].has_value());
                    
                }
            }
        }
    }
}

