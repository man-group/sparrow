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

template<class T>
concept is_nullable_scalar = 
    is_nullable_like<T> && std::is_scalar_v<typename T::value_type>;

template <class T>
concept translates_to_primitive_layout = 
    std::ranges::input_range<T> &&
    std::is_scalar_v<std::ranges::range_value_t<mnv_t<T>>>;

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


int main()
{
    // arr[float]
    {   
        std::vector<float> v{1.0, 2.0, 3.0, 4.0, 5.0};
        std::cout<<"arr[float]:"<<std::endl;
        print_arr(sparrow::build(v));
    }
    // list[float]
    {   
        std::vector<std::vector<float>> v{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
        std::cout<<std::endl<<"list[float]:"<<std::endl;
        print_arr(sparrow::build(v));
    }
    // list[list[float]]
    {   
        std::vector<std::vector<std::vector<float>>> v{
            {{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}},
            {{7.0f, 8.0f, 9.0f}, {10.0f, 11.0f, 12.0f}}
        };
        std::cout<<std::endl<<"list[list[float]]:"<<std::endl;
        print_arr(sparrow::build(v));
    }
    // struct<float, float>
    {   
        std::vector<std::tuple<float, float>> v{
            {1.0f, 2.0f},
            {3.0f, 4.0f},
            {5.0f, 6.0f}
        };
        std::cout<<std::endl<<"struct<float, float>:"<<std::endl;
        print_arr(sparrow::build(v));
    }
    // struct<list[float], uint16>
    {   
        std::vector<std::tuple<std::vector<float>,std::uint16_t>> v{
            {{1.0f, 2.0f, 3.0f}, 1},
            {{4.0f, 5.0f, 6.0f}, 2},
            {{7.0f, 8.0f, 9.0f}, 3}
        };
        std::cout<<std::endl<<"struct<list[float], uint16>:"<<std::endl;
        print_arr(sparrow::build(v));
    }

    return 0;

}