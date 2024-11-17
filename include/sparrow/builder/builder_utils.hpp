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

#include <sparrow/array.hpp>
#include <sparrow/utils/ranges.hpp>
#include <tuple>
#include <utility> 
 
namespace sparrow
{


template<class T, class KEY_TYPE = std::uint64_t>
class lazy_dict_encoded_vector : public std::vector<T>
{
    public:
        using base_type = std::vector<T>;
        using self_type = lazy_dict_encoded_vector<T, KEY_TYPE>;
        using base_type::base_type;
        using key_type = KEY_TYPE;
};

template<class T>
struct dict_encoded_key_type
{
    using type = std::uint64_t;
};

template<class T, class KEY_TYPE>
struct dict_encoded_key_type<lazy_dict_encoded_vector<T, KEY_TYPE>>
{
    using type = KEY_TYPE;
};

template<class T>
using dict_dict_encoded_key_t = typename dict_encoded_key_type<T>::type;


namespace detail
{
    template<class SOME_RANGE>
    concept is_lazy_dict_encoded_vector = sparrow::mpl::is_type_instance_of_v<std::decay_t<SOME_RANGE>, sparrow::lazy_dict_encoded_vector>;



    // only for side effects (ie lambda which is called for each index without
    //  returning anything but can have side effects)
    template <class F, std::size_t... Is>
    void for_each_index_impl(F&& f, std::index_sequence<Is...>)
    {   
        // Apply f to each index individually
        (f(std::integral_constant<std::size_t, Is>{}), ...);
    }
    template <std::size_t SIZE, class F>
    void for_each_index(F&& f)
    {
        for_each_index_impl(std::forward<F>(f), std::make_index_sequence<SIZE>());
    }


    // similar to for_each_index but with cap
    template<std::size_t INDEX, std::size_t SIZE>
    struct exitable_for_each_index_impl
    {   
        template<class F>
        static bool call(F&& f)
        {
            const bool contiunue_for_each = f(std::integral_constant<std::size_t, INDEX>{});
            if(contiunue_for_each)
            {
                return exitable_for_each_index_impl<INDEX + 1, SIZE>::call(std::forward<F>(f));
            }
            return false;
        }
    };
    template<std::size_t SIZE>
    struct exitable_for_each_index_impl<SIZE, SIZE>
    {   
        template<class F>
        static bool call(F&& )
        {
            return true;
        }
    };

    template <std::size_t SIZE, class F>
    bool exitable_for_each_index(F&& f)
    {
        return exitable_for_each_index_impl<0, SIZE>::call(std::forward<F>(f));
    }


    template<class T, std::size_t N>
    concept has_tuple_element =
    requires(T t) {
        typename std::tuple_element_t<N, std::remove_const_t<T>>;
        { get<N>(t) } -> std::convertible_to<const std::tuple_element_t<N, T>&>;
    };



    template <typename T, std::size_t... N>
    constexpr bool check_tuple_elements(std::index_sequence<N...>) {
        return (has_tuple_element<T, N> && ...);
    }

    template <typename T>
    constexpr bool is_tuple_like() {
        return check_tuple_elements<T>(std::make_index_sequence<std::tuple_size_v<T>>());
    }

    template <typename T>
    concept tuple_like = !std::is_reference_v<T>
        && requires(T t) {
            typename std::tuple_size<T>::type;
            requires std::derived_from<std::tuple_size<T>, std::integral_constant<std::size_t, std::tuple_size_v<T>>>;
        }
        && is_tuple_like<T>();


    template <typename Tuple, size_t... Is>
    constexpr bool all_elements_same_impl(std::index_sequence<Is...>) {
        return sizeof...(Is) == 0 || ((std::is_same_v<
            std::tuple_element_t<0, Tuple>, 
            std::tuple_element_t<Is, Tuple>
        >) && ...);
    }

    template <typename T>
    concept all_elements_same = tuple_like<T> && 
        all_elements_same_impl<T>(
            std::make_index_sequence<std::tuple_size_v<T>>{}
        );


    // concept which is true for all types which translate to a primitive
    // layouts (ie range of scalars or range of nullable of scalars)
    template <class T>
    concept is_nullable_like_generic = 
    requires(T t)
    {
        //typename T::value_type;
        { t.has_value() } -> std::convertible_to<bool>;
        { t.get() };// -> std::convertible_to<typename T::value_type>;
    };

    template<class T>
    concept is_nullable_like =(is_nullable_like_generic<T>  );//||  sparrow::is_nullable_v<T>);


    template<class T>
    struct maybe_nullable_value_type
    {
        using type = T;
    }; 

    template<is_nullable_like T>
    struct maybe_nullable_value_type<T>
    {
        using type = typename T::value_type;
    };



    // shorthand for maybe_nullable_value_type<T>::type
    template<class T>
    using mnv_t = typename maybe_nullable_value_type<T>::type;


    // shorhand for mnv_t<std::ranges::range_value_t<T>>
    template<class T>
    using ensured_range_value_t = mnv_t<std::ranges::range_value_t<T>>;

    // helper to get inner value type of smth like a vector of vector of T
    // we also translate any nullable to the inner type
    template<class T>
    using nested_ensured_range_inner_value_t = ensured_range_value_t<ensured_range_value_t<T>>;


    // a save way to return .size from
    // a possibly nullable object
    template<class T>
    requires(!is_nullable_like<T>)
    auto get_size_save(const T& t)
    {
        return t.size();
    }

    template<is_nullable_like T>
    auto get_size_save(const T& t)
    {
        return t.has_value() ? t.get().size() : 0;
    }

    
    template<class T>
    requires (!is_nullable_like<T>)
    auto ensure_value(T && t)
    {
        return std::forward<T>(t);
    }

    template<is_nullable_like T>
    auto ensure_value(T && t)
    {
       return std::forward<T>(t).get();
    }

    template<std::ranges::range T>
    requires(is_nullable_like< std::ranges::range_value_t<T>>)
    std::vector<std::size_t> where_null(T && t)
    {
        std::vector<std::size_t> result;
        for (std::size_t i = 0; i < t.size(); ++i)
        {
            if (!t[i].has_value())
            {
                result.push_back(i);
            }
        }
        return result;
    }
    
    template<class T>
    requires(!is_nullable_like< std::ranges::range_value_t<T>>)
    std::array<std::size_t,0> where_null(T && )
    {
        return {};
    }

    template<class T>
    requires(!is_nullable_like<std::decay_t<std::ranges::range_value_t<T>>>)
    T ensure_value_range(T && t)
    {
        return std::forward<T>(t);
    }

    template<class T>
    requires(is_nullable_like<std::decay_t<std::ranges::range_value_t<T>>>)
    auto ensure_value_range(T && t)
    {
        return t | std::views::transform([](auto && v) { return v.get(); });
    }

    template <typename T>
    concept variant_like = 
        // Must not be a reference
        !std::is_reference_v<T> && 

        // Must have an index() member function returning a size_t
        requires(const T& v) {
            { v.index() } -> std::convertible_to<std::size_t>;
        } &&

        // Must work with std::visit
        requires(T v) {
            std::visit([](auto&&) {}, v); // Use a generic lambda to test visitation
        } &&

        // Must allow std::get with an index
        requires(T v) {
            { std::get<0>(v) }; // Access by index
        } &&

        // Must allow std::get with a type
        requires(T v) {
            { std::get<typename std::variant_alternative<0, T>::type>(v) }; // Access by type
        }
    ;


} // namespace detail


} // namespace sparrow