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


template<std::size_t I, std::size_t SIZE>
struct for_each_index
{
    template<class F>
    static void apply(F&& f)
    {
        f(std::integral_constant<std::size_t, I>{});
        for_each_index<I + 1, SIZE>::apply(std::forward<F>(f));
    }
};

template<std::size_t SIZE>
struct for_each_index<SIZE, SIZE>
{
    template<class F>
    static void apply(F&& f)
    {
    }
};

template<class T, std::size_t N>
concept has_tuple_element =
  requires(T t) {
    typename std::tuple_element_t<N, std::remove_const_t<T>>;
    { get<N>(t) } -> std::convertible_to<const std::tuple_element_t<N, T>&>;
  };

template<class T>
concept tuple_like = !std::is_reference_v<T> 
  && requires(T t) { 
    typename std::tuple_size<T>::type; 
    requires std::derived_from<
      std::tuple_size<T>, 
      std::integral_constant<std::size_t, std::tuple_size_v<T>>
    >;
  } && []<std::size_t... N>(std::index_sequence<N...>) { 
    return (has_tuple_element<T, N> && ...); 
  }(std::make_index_sequence<std::tuple_size_v<T>>());


// concept which is true for all types which translate to a primitive
// layouts (ie range of scalars or range of nullable of scalars)
template <class T>
concept is_nullable_like = 
requires(T t)
{
    { t.has_value() } -> std::convertible_to<bool>;
    { t.get() } -> std::convertible_to<typename T::value_type>;
};

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
} // namespace sparrow