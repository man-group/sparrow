/** @example array_builder.cpp
 * Example of usage of the generic builder
 */


#include <vector>
#include <array>
#include <tuple>
#include <list>
#include <string>
#include <cassert>

#include <sparrow/builder/builder.hpp>

void primitve_array()
{   
    //! [builder_primitive_array]
    // using initializer_list
    auto arr = sparrow::build({1, 2, 3, 4, 5});
    /////////////////////
    // using vector
    std::vector<int> v{1, 2, 3, 4, 5};
    auto arr2 = sparrow::build(v);
    /////////////////////
    // using list
    std::list<int> l{1, 2, 3, 4, 5};
    auto arr3 = sparrow::build(l);
    /////////////////////
    // using any range
    auto iota = std::views::iota(1, 6) | std::views::transform([](int i){ 
        return static_cast<int>(i);
    });
    auto arr4 = sparrow::build(iota);
    /////////////////////
    // all of the arrays above are equivalent to the manually built array
    auto arr5 = sparrow::primitive_array<int>({1, 2, 3, 4, 5});
    assert(arr == arr2);
    assert(arr == arr3);
    assert(arr == arr4);
    assert(arr == arr5);
    //! [builder_primitive_array]
}

void primitve_array_with_nulls()
{   
//! [builder_primitive_array_with_nulls]
    // using initializer_list (here we have to explicitly specify the type when using an
    // initializer list with nulls)
    auto arr = sparrow::build<sparrow::nullable<int>>({1, 2, sparrow::nullval, 4, 5});
    /////////////////////
    // using vector
    std::vector<sparrow::nullable<int>> v{1, 2, sparrow::nullval, 4, 5};
    auto arr2 = sparrow::build(v);
    /////////////////////
    // using list
    std::list<sparrow::nullable<int>> l{1, 2, sparrow::nullval, 4, 5};
    auto arr3 = sparrow::build(l);
    /////////////////////
    // using any range
    auto iota = std::views::iota(1, 6) | std::views::transform([](int i) -> sparrow::nullable<int> { 
        return i == 3 ? sparrow::nullable<int>{} : sparrow::nullable<int>{i};
    });
    auto arr4 = sparrow::build(iota);
    // all of the arrays above are equivalent to the manually built array
    std::vector<std::size_t> where_nulls{2};
    sparrow::u8_buffer<int> values{1, 2, 3, 4, 5};
    auto arr5 = sparrow::primitive_array<int>(std::move(values), where_nulls);
    assert(arr == arr2);
    assert(arr == arr3);
    assert(arr == arr4);
//! [builder_primitive_array_with_nulls]
}

void list_of_strings()
{   
    //! [builder_list_of_strings]
    // [["hello", "world","!"], ["Another", "sentence"]]
    std::vector<std::vector<std::string>> v{
        {"hello", "world", "!"}, 
        {"Another", "sentence"}
    };
    auto arr = sparrow::build(v);
    //! [builder_list_of_strings]
}

void list_of_strings_with_nulls()   
{
    //! [builder_list_of_strings_with_nulls]
    // [["hello", "world","!"],NULL , ["Another", "sentence"]]
    using string_vector = std::vector<std::string>;
    using nullable_string_vector = sparrow::nullable<string_vector>;
    std::vector<nullable_string_vector> v{
        nullable_string_vector{string_vector{"hello", "world", "!"}}, 
        nullable_string_vector{},
        nullable_string_vector{string_vector{"Another", "sentence"}}
    };
    auto arr = sparrow::build(v);
    //! [builder_list_of_strings_with_nulls]
}


void list_of_struct()
{
    //! [builder_list_of_struct]
    /*
    [
        [
            (1, 2.5), 
            (2, 3.5)
        ],
        [
            (3, 5.5), 
            (5, 6.5), 
            (6, 7.5)
        ],
        [
            (7, 8.5)
        ]
    ]
    */
    std::vector<std::vector<std::tuple<int, float>>> v{
        {
            std::tuple<int, float>{1, 2.5}, 
            std::tuple<int, float>{2, 3.5}
        },
        {	
            std::tuple<int, float>{3, 5.5}, 
            std::tuple<int, float>{5, 6.5},
            std::tuple<int, float>{6, 7.5}
        },
        {
            std::tuple<int, float>{7, 8.5}
        }
    };
    auto arr = sparrow::build(v);
    //! [builder_list_of_struct]
}

void fixed_sized_list_strings()
{	
    //! [builder_fixed_sized_list_strings]
    std::vector<std::array<std::string, 2>> v{
        {"hello", "world"},
        {"Another", "sentence"},
        {"This", "is"}
    };
    auto arr = sparrow::build(v);
    //! [builder_fixed_sized_list_strings]
}

void fixed_sized_list_of_union()
{	
    //! [builder_fixed_sized_list_of_union]
    using variant_type = std::variant<int, float>;
    using array_type = std::array<variant_type, 2>;
    std::vector<array_type> v{
        {1, 2.5f},
        {2, 3.5f},
        {3, 4.5f}
    };
    auto arr = sparrow::build(v);
    //! [builder_fixed_sized_list_of_union]
}

void dict_encoded_variable_sized_binary()
{
    //! [builder_dict_encoded_variable_sized_binary]
    sparrow::dict_encode<std::vector<std::string>> v{
        std::vector<std::string>{
            "hello", 
            "world", 
            "hello",
            "world",
            "hello",
        }
    };
    auto arr = sparrow::build(v);
    //! [builder_dict_encoded_variable_sized_binary]
}

void run_end_encoded_variable_sized_binary()
{
    //! [builder_run_end_encoded_variable_sized_binary]
    sparrow::run_end_encode<std::vector<std::string>> v{
        std::vector<std::string>{
            "hello", 
            "hello", 
            "hello",
            "world",
            "world",
        }
    };
    auto arr = sparrow::build(v);
    //! [builder_run_end_encoded_variable_sized_binary]
}

void struct_array()
{
    //! [builder_struct_array]
    using tuple_type = std::tuple<int,  std::array<std::string, 2>, sparrow::nullable<float>>;
    std::vector<tuple_type> v{
        {1, {"hello", "world"}, 2.5f},
        {2, {"Another", "sentence"}, sparrow::nullval},
        {3, {"This", "is"}, 3.5f}
    };
    auto arr = sparrow::build(v);
    //! [builder_struct_array]
}

void sparse_union_array()
{
    //! [builder_union_array]
    using variant_type = std::variant<int,  std::array<std::string, 2>, sparrow::nullable<float>>;
    std::vector<variant_type> v{
        int{1},
        std::array<std::string, 2>{{"A", "sentence"}},
        sparrow::nullable<float>{2.5f},
        sparrow::nullable<float>{}
    };
    auto arr = sparrow::build(v);
    //! [builder_union_array]
}


int main()
{
    primitve_array();
    primitve_array_with_nulls();
    list_of_strings();
    list_of_strings_with_nulls();
    list_of_struct();
    fixed_sized_list_strings();
    fixed_sized_list_of_union();
    dict_encoded_variable_sized_binary();
    run_end_encoded_variable_sized_binary();
    struct_array();
    sparse_union_array();

    return 0;
}