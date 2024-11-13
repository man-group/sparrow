#include <vector>
#include <string>
#include <array>
#include <tuple>
#include <type_traits>
#include <ranges>

#include <sparrow/layout/primitive_array.hpp>
#include <sparrow/layout/list_layout/list_array.hpp>
#include <sparrow/array.hpp>
#include "printer.hpp"
#include <sparrow/utils/ranges.hpp>

// // rangesv3
// #include <range/v3/view/join.hpp>
// #include <range/v3/view/transform.hpp>

namespace sparrow
{

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
auto build(const T& t)
{
    return builder<T>::create(t);
}

// template <class T>
// requires(!std::is_lvalue_reference_v<T>)
// cloning_ptr<array_wrapper> make_wrapper_ptr(T && ARR)
// {
//     return cloning_ptr<array_wrapper>{new array_wrapper_impl<T>(std::move(arr)) };
// }


template< translates_to_primitive_layout T>
struct builder<T>
{
    using type = primitive_array<typename maybe_nullable_value_type<std::ranges::range_value_t<T>>::type>;

    static type create(const T& t)
    {
        return type(t);
    }

};


template< translate_to_variable_sized_list_layout T>
struct builder<T>
{
    using type = big_list_array;

    static type create(const T& t)
    {

        auto flat_list_view = std::ranges::views::join(t);

        using passed_value_type =  std::ranges::range_value_t<std::ranges::range_value_t<T>>;

        using flat_list_view_type = std::decay_t<decltype(flat_list_view)>;
        using flat_list_view_value_type = std::ranges::range_value_t<flat_list_view_type>;


        // // check that value_types are matching
        static_assert( std::is_same_v<passed_value_type, flat_list_view_value_type>);


        // build offsets from sizes
        auto sizes = t | std::views::transform([](const auto& l){ return l.size(); });

        SPARROW_ASSERT_TRUE(range_size(sizes) == range_size(t));

        auto offsets = type::offset_from_sizes(sizes);
        SPARROW_ASSERT_TRUE(range_size(sizes) + 1 == offsets.size());

        // the child array
        auto flat_arr = build(flat_list_view);

        SPARROW_ASSERT_TRUE(flat_arr.size() == range_size(flat_list_view))


        // wrap the flat array into an array
        array flat_arr_detyped(std::move(flat_arr));
        
        return type(std::move(flat_arr_detyped), std::move(offsets));

    }
};






}// namespace sparrow




int main()
{
    // arr[float]
    {   
        std::vector<float> v{1.0, 2.0, 3.0, 4.0, 5.0};
        using container_type = std::decay_t<decltype(v)>;
        using builder_type = sparrow::builder<container_type>;

        auto arr = builder_type::create(v);
        std::cout<<"arr[float]"<<std::endl;
        print_arr(arr);
    }

    // list[float]
    {   
        std::vector<std::vector<float>> v{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
        using container_type = std::decay_t<decltype(v)>;
        using builder_type = sparrow::builder<container_type>;

        auto arr = builder_type::create(v);
        std::cout<<"list[float]"<<std::endl;
        print_arr(arr);
    }
    // list[list[float]]
    {   
        // vector of vector of vector of float
        std::vector<std::vector<std::vector<float>>> v{
            {{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}},
            {{7.0f, 8.0f, 9.0f}, {10.0f, 11.0f, 12.0f}}
        };

        using container_type = std::decay_t<decltype(v)>;
        using builder_type = sparrow::builder<container_type>;
        auto arr = builder_type::create(v);
        std::cout<<"list[list[float]]"<<std::endl;
        print_arr(arr);
    }

  

    return 0;

}