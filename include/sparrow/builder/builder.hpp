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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or mplied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <vector>
#include <string>
#include <array>
#include <tuple>
#include <type_traits>
#include <ranges>

#include <sparrow/utils/ranges.hpp>
#include <sparrow/layout/primitive_array.hpp>
#include <sparrow/layout/list_layout/list_array.hpp>
#include <sparrow/layout/struct_layout/struct_array.hpp>
#include <sparrow/layout/variable_size_binary_array.hpp>
#include <sparrow/layout/union_array.hpp>
#include <sparrow/array.hpp>
#include "builder_utils.hpp"
#include <sparrow/utils/ranges.hpp>
#include <tuple>
#include <utility> 

namespace sparrow
{
// forward declaration
namespace detail
{
template<class T, class OPTIONS_TYPE>
struct builder;
} // namespace detail


struct dense_union_flag_t{};
struct sparse_union_flag_t{};
struct large_list_flag_t{};
struct large_binary_flag_t{};

inline constexpr dense_union_flag_t dense_union_flag;
inline constexpr sparse_union_flag_t sparse_union_flag;
inline constexpr large_list_flag_t large_list_flag;
inline constexpr large_binary_flag_t large_binary_flag;


template<class T, class ... OPTION_FLAGS>
auto build(T&& t, OPTION_FLAGS&& ... )
{
    using option_flags_type = sparrow::mpl::typelist<std::decay_t<OPTION_FLAGS>...>;
    return detail::builder<T, option_flags_type>::create(std::forward<T>(t));
}


namespace detail
{
template<class T, class ... OPTION_FLAGS>
auto build_impl(T&& t, [[maybe_unused]] sparrow::mpl::typelist<OPTION_FLAGS...> typelist)
{
    using option_flags_type = sparrow::mpl::typelist<OPTION_FLAGS...>;
    return builder<T, option_flags_type>::create(std::forward<T>(t));
}

template <class T>
concept translates_to_primitive_layout = 
    std::ranges::input_range<T> &&
    std::is_scalar_v<ensured_range_value_t<T>>;


// helper to get inner value type of smth like a vector of vector of T
// we also translate any nullable to the inner type
template<class T>
using nested_ensured_range_inner_value_t = ensured_range_value_t<ensured_range_value_t<T>>;

template<class T> 
concept translate_to_variable_sized_list_layout = 
    std::ranges::input_range<T> &&     
    std::ranges::input_range<ensured_range_value_t<T>> &&
    !tuple_like<ensured_range_value_t<T>> && // tuples go to struct layout
    // value type of inner should not be 'char-like'( char, byte, uint8), these are handled by variable_size_binary_array
    !mpl::char_like<nested_ensured_range_inner_value_t<T>>
;

template<class T>
concept translate_to_struct_layout = 
    std::ranges::input_range<T> &&
    tuple_like<ensured_range_value_t<T>> &&
    !all_elements_same<ensured_range_value_t<T>>;

template<class T>
concept translate_to_fixed_sized_list_layout =
    std::ranges::input_range<T> &&
    tuple_like<ensured_range_value_t<T>> &&
    all_elements_same<ensured_range_value_t<T>>;


template<class T> 
concept translate_to_variable_sized_binary_layout = 
    std::ranges::input_range<T> &&     
    std::ranges::input_range<ensured_range_value_t<T>> &&
    !tuple_like<ensured_range_value_t<T>> && // tuples go to struct layout
    // value type of inner must be char like ( char, byte, uint8)
    mpl::char_like<nested_ensured_range_inner_value_t<T>>
;


template<class T> 
concept translate_to_union_layout = 
    std::ranges::input_range<T> &&     
    // value type must be a variant-like type
    // *NOTE* we don't check for nullable here, as we want to handle nullable variants
    // as in the arrow spec, the nulls are handled by the elements **in** the variant
    variant_like<std::ranges::range_value_t<T>>

;


template< translates_to_primitive_layout T, class OPTION_FLAGS>
struct builder<T, OPTION_FLAGS>
{
    using type = sparrow::primitive_array<ensured_range_value_t<T>>;
    template<class U>
    static type create(U&& t)
    {
        return type(std::forward<U>(t));
    } 
};

template< translate_to_variable_sized_list_layout T, class OPTION_FLAGS>
struct builder<T, OPTION_FLAGS>
{

    using type = std::conditional_t<
        mpl::contains<large_list_flag_t>(OPTION_FLAGS{}),
        sparrow::big_list_array,
        sparrow::list_array
    >;

    template<class U>
    static type create(U && t)
    {
        auto flat_list_view = std::ranges::views::join(ensure_value_range(t));

        auto sizes = t | std::views::transform([](const auto& l){ 
            return get_size_save(l);
        });
 
        return type(
            array(build_impl(flat_list_view, OPTION_FLAGS{})), 
            type::offset_from_sizes(sizes),
            where_null(t)
        );
    }
};

template< translate_to_fixed_sized_list_layout T, class OPTION_FLAGS>
struct builder<T, OPTION_FLAGS>
{
    using type = sparrow::fixed_sized_list_array;
    constexpr static std::size_t list_size = std::tuple_size_v<mnv_t<std::ranges::range_value_t<T>>>;

    template<class U>
    static type create(U && t)
    {
        auto flat_list_view = std::ranges::views::join(ensure_value_range(t));

        return type(
            static_cast<std::uint64_t>(list_size), 
            array(build_impl(flat_list_view, OPTION_FLAGS{})),
            where_null(t)
        );
    }
};

template< translate_to_struct_layout T, class OPTION_FLAGS>
struct builder<T, OPTION_FLAGS>
{
    using type = sparrow::struct_array;
    static constexpr std::size_t n_children = std::tuple_size_v<mnv_t<std::ranges::range_value_t<T>>>;

    template<class U>
    static type create(U&& t) 
    {
        std::vector<array> detyped_children(n_children);
        for_each_index<n_children>([&](auto i)
        {
            auto tuple_i_col = t | std::views::transform([](const auto& maybe_nullable_tuple)
            {
                const auto & tuple_val = ensure_value(maybe_nullable_tuple);
                return std::get<decltype(i)::value>(tuple_val);
            }); 
            detyped_children[decltype(i)::value] = array(build_impl(tuple_i_col, OPTION_FLAGS{}));
        });
       return type(std::move(detyped_children),
       where_null(t)
       );
    }
};


template< translate_to_variable_sized_binary_layout T, class OPTION_FLAGS>
struct builder<T, OPTION_FLAGS>
{
    using type = sparrow::string_array;

    template<class U>
    static type create(U && t)
    {
        auto flat_list_view = std::ranges::views::join(ensure_value_range(t));
        u8_buffer<char> data_buffer(flat_list_view);

        auto sizes = t | std::views::transform([](const auto& l){ 
            return get_size_save(l);
        });
 
        return type(
            std::move(data_buffer),
            type::offset_from_sizes(sizes),
            where_null(t)
        );
    }
};

template< translate_to_union_layout T, class OPTION_FLAGS>
struct builder<T, OPTION_FLAGS>
{
    using type = sparrow::sparse_union_array; // TODO use options to select between sparse and dense
    using variant_type = std::ranges::range_value_t<T>;
    static constexpr std::size_t variant_size = std::variant_size_v<variant_type>;


    template<class U>
    static type create(U && t) requires(std::is_same_v<type, sparrow::sparse_union_array>)
    {
        std::vector<array> detyped_children(variant_size);
        for_each_index<variant_size>([&](auto i)
        {
            using type_at_index = std::variant_alternative_t<decltype(i)::value, variant_type>;
            auto type_i_col = t | std::views::transform([](const auto& variant)
            {   
                return variant.index() == decltype(i)::value ? 
                    std::get<type_at_index>(variant) : type_at_index{};
            });
            detyped_children[decltype(i)::value] = array(build_impl(type_i_col,  OPTION_FLAGS{}));
        });

        // type-ids
        auto type_id_range = t | std::views::transform([](const auto& v){
            return static_cast<std::uint8_t>(v.index());
        });
        u8_buffer<std::uint8_t> type_id_buffer(type_id_range);

        return type(
            std::move(detyped_children),
            std::move(type_id_buffer)
        );
    }
};



} // namespace detail
}// namespace sparrow