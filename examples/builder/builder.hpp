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
    std::ranges::input_range<mnv_t<std::ranges::range_value_t<T>>> &&
    !tuple_like<mnv_t<std::ranges::range_value_t<T>>>;

template<class T>
concept translate_to_struct_layout = 
    std::ranges::input_range<T> &&
    tuple_like<mnv_t<std::ranges::range_value_t<T>>> &&
    !all_elements_same<mnv_t<std::ranges::range_value_t<T>>> &&
    !associative_container<T>;

template<class T>
concept translate_to_fixed_sized_list_layout =
    std::ranges::input_range<T> &&
    tuple_like<mnv_t<std::ranges::range_value_t<T>>> &&
    all_elements_same<mnv_t<std::ranges::range_value_t<T>>>;

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
        auto flat_list_view = std::ranges::views::join(ensure_value_range(t));

        auto sizes = t | std::views::transform([](const auto& l){ 
            return get_size_save(l);
        });
 
        return type(
            array(build(flat_list_view)), 
            type::offset_from_sizes(sizes),
            where_null(t)
        );
    }
};

template< translate_to_fixed_sized_list_layout T>
struct builder<T>
{
    using type = fixed_sized_list_array;
    constexpr static std::size_t list_size = std::tuple_size_v<mnv_t<std::ranges::range_value_t<T>>>;

    template<class U>
    static type create(U && t)
    {
        auto flat_list_view = std::ranges::views::join(ensure_value_range(t));

        return type(
            static_cast<std::uint64_t>(list_size), 
            array(build(flat_list_view)),
            where_null(t)
        );
    }
};

template< translate_to_struct_layout T>
struct builder<T>
{
    using type = struct_array;
    static constexpr std::size_t n_children = std::tuple_size_v<mnv_t<std::ranges::range_value_t<T>>>;

    template<class U>
    static type create(U&& t) 
    {
        std::vector<array> detyped_children(n_children);
        for_each_index<n_children>([&](auto i)
        {
            auto tuple_i_col = t | std::views::transform([](const auto& maybe_nullable_tuple)
            {
                return std::get<decltype(i)::value>(ensure_value( ensure_value(maybe_nullable_tuple)));
            }); 
            detyped_children[i] = array(build(tuple_i_col));
        });
       return type(std::move(detyped_children), where_null(t));
    }
};




}// namespace sparrow