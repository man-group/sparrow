#pragma once

#include <vector>
#include <string>
#include <array>
#include <tuple>
#include <type_traits>
#include <ranges>

#include <sparrow/layout/primitive_array.hpp>
#include <sparrow/layout/list_layout/list_array.hpp>
#include <sparrow/layout/struct_layout/struct_array.hpp>
#include <sparrow/array.hpp>
#include "printer.hpp"
#include "builder_utils.hpp"
#include <sparrow/utils/ranges.hpp>
#include <tuple>
#include <utility> 


namespace sparrow
{

template <class T>
concept translates_to_primitive_layout = 
    std::ranges::input_range<T> &&
    std::is_scalar_v<mnv_t<std::ranges::range_value_t<T>>>;

template<class T>
concept translate_to_variable_sized_list_layout = 
    std::ranges::input_range<T> &&
    std::ranges::input_range<mnv_t<std::ranges::range_value_t<T>>>;


template<class T>
concept translate_to_struct_layout = 
    std::ranges::input_range<T> &&
    // the value_type of the range is a tuple
    tuple_like<mnv_t<std::ranges::range_value_t<T>>>;

template<class T>
struct builder;

template<class T>
auto build(T&& t)
{
    return builder<T>::create(std::forward<T>(t));
}

template< translates_to_primitive_layout T>
struct builder<T>
{
    using type = primitive_array<typename maybe_nullable_value_type<std::ranges::range_value_t<T>>::type>;

    template<class U>
    static type create(U&& t)
    {
        return type(std::forward<U>(t));
    }

};

template< translate_to_variable_sized_list_layout T>
struct builder<T>
{
    using type = big_list_array;

    template<class U>
    static type create(U && t)
    {

        auto flat_list_view = std::ranges::views::join(t);

        using passed_value_type =  std::ranges::range_value_t<std::ranges::range_value_t<T>>;

        using flat_list_view_type = std::decay_t<decltype(flat_list_view)>;
        using flat_list_view_value_type = std::ranges::range_value_t<flat_list_view_type>;

        // // check that value_types are matching
        static_assert( std::is_same_v<passed_value_type, flat_list_view_value_type>);

        // build offsets from sizes
        auto sizes = t | std::views::transform([](const auto& l){ return l.size(); });


        auto offsets = type::offset_from_sizes(sizes);
        // the child array
        auto flat_arr = build(flat_list_view);

        // wrap the flat array into an array
        array flat_arr_detyped(std::move(flat_arr));
        
        return type(std::move(flat_arr_detyped), std::move(offsets));

    }
};


template< translate_to_struct_layout T>
struct builder<T>
{
    using type = struct_array;

    template<class U>
    static type create(U&& t)
    {
        using tuple_type = mnv_t<std::ranges::range_value_t<T>>;

        // length of tuple ==> number of children
        constexpr std::size_t n_children = std::tuple_size_v<tuple_type>;

        std::vector<array> detyped_children(n_children);

        for_each_index<0, n_children>::apply([&](auto i)
        {
            constexpr std::size_t I = decltype(i)::value;

            // create a view of the i-th element of the tuple
            auto tuple_i_col = t | std::views::transform([](const auto& tuple)
            {
                return get<I>(tuple);
            }); 

            using tuple_i_col_type = std::ranges::range_value_t<decltype(tuple_i_col)>;
            using builder_type = builder<tuple_i_col_type>;

            // the child array
            auto col_arr = build(tuple_i_col);

            // wrap the flat array into an array
            array detyped_col_arr(std::move(col_arr));

            // move the array into the vector
            detyped_children[i] = std::move(detyped_col_arr);
        });
        return type(std::move(detyped_children));
    }
};

}// namespace sparrow