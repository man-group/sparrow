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

#pragma once

#include <concepts>
#include <cstdint>
#include <iterator>
#include <memory>
#include <ranges>
#include <tuple>
#include <type_traits>

namespace sparrow::mpl
{

    /**
     * @brief Workaround to replace static_assert(false) in template code.
     *
     * This utility provides a way to trigger compilation errors in template contexts
     * where static_assert(false) would cause immediate compilation failure even in
     * non-instantiated branches.
     *
     * Reference: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2593r1.html
     *
     * @tparam T Pack of types (unused but makes the false condition dependent)
     *
     * @post Always inherits from std::false_type
     *
     * @code{.cpp}
     * template<typename T>
     * void func() {
     *     if constexpr (some_condition<T>) {
     *         // do something
     *     } else {
     *         static_assert(dependent_false<T>::value, "Unsupported type");
     *     }
     * }
     * @endcode
     */
    template <class... T>
    struct dependent_false : std::false_type
    {
    };

    /**
     * @brief Type trait to check if a type is an instantiation of a template.
     *
     * Primary template that defaults to false_type for non-matching cases.
     *
     * @tparam L The concrete type to check
     * @tparam U The template to check against
     */
    template <class L, template <class...> class U>
    struct is_type_instance_of : std::false_type
    {
    };

    /**
     * @brief Specialization for matching template instantiations.
     *
     * @tparam L The template being instantiated
     * @tparam U The template to check against
     * @tparam T Parameter pack for template arguments
     *
     * @pre L<T...> and U<T...> must be the same type
     * @post Inherits from std::true_type when types match
     */
    template <template <class...> class L, template <class...> class U, class... T>
        requires std::is_same_v<L<T...>, U<T...>>
    struct is_type_instance_of<L<T...>, U> : std::true_type
    {
    };

    /**
     * @brief Variable template for convenient access to is_type_instance_of.
     *
     * Checks if T is a concrete instantiation of template U.
     *
     * @tparam T The concrete type to check
     * @tparam U The template to check against
     *
     * @post Returns true if T is an instantiation of U, false otherwise
     *
     * @code{.cpp}
     * static_assert(is_type_instance_of_v<std::vector<int>, std::vector>);
     * static_assert(!is_type_instance_of_v<int, std::vector>);
     * @endcode
     */
    template <class T, template <class...> class U>
    constexpr bool is_type_instance_of_v = is_type_instance_of<T, U>::value;

    /**
     * @brief A sequence of types used for metaprogramming operations.
     *
     * This class template serves as a container for type lists in template
     * metaprogramming contexts. It provides the foundation for type-level
     * algorithms and operations.
     *
     * @tparam T Pack of types to store in the list
     *
     * @post Stores the given types for metaprogramming operations
     * @post Can be used with type algorithms like append, transform, etc.
     *
     * @code{.cpp}
     * using int_float_list = typelist<int, float>;
     * using empty_list = typelist<>;
     * @endcode
     */
    template <class... T>
    struct typelist
    {
    };

    /**
     * @brief Appends individual types to a typelist.
     *
     * This function takes a typelist and additional types as arguments, and returns
     * a new typelist containing all the types from the original typelist followed
     * by the additional types.
     *
     * @tparam Ts Types in the original typelist
     * @tparam Us Additional types to be appended (must not be typelists)
     * @param list The original typelist
     * @param types The additional types to append
     * @return New typelist containing all types
     *
     * @pre Us types must not be typelist instances
     * @post Returns typelist<Ts..., Us...>
     *
     * @code{.cpp}
     * auto result = append(typelist<int, float>{}, double{}, char{});
     * // result is typelist<int, float, double, char>
     * @endcode
     */
    template <class... Ts, class... Us>
        requires(!is_type_instance_of_v<Us, typelist> && ...)
    consteval auto append(typelist<Ts...>, Us...)
    {
        return typelist<Ts..., Us...>{};
    }

    /**
     * @brief Appends two typelists together.
     *
     * This function takes two typelists as input and returns a new typelist that
     * contains all the types from both input typelists in order.
     *
     * @tparam Ts Types in the first typelist
     * @tparam Us Types in the second typelist
     * @param list1 The first typelist
     * @param list2 The second typelist
     * @return New typelist containing all types from both lists
     *
     * @post Returns typelist<Ts..., Us...>
     *
     * @code{.cpp}
     * auto result = append(typelist<int, float>{}, typelist<double, char>{});
     * // result is typelist<int, float, double, char>
     * @endcode
     */
    template <class... Ts, class... Us>
    consteval auto append(typelist<Ts...>, typelist<Us...>)  // TODO: Handle several typelists
    {
        return typelist<Ts..., Us...>{};
    }

    /**
     * @brief Type alias for appending types or typelists to a given typelist.
     *
     * This template alias provides a convenient way to append one or more types
     * or typelists to an existing typelist using the append function.
     *
     * @tparam TypeList The typelist to which types will be appended
     * @tparam Us Types or typelists to be appended
     *
     * @pre TypeList must be an instance of typelist
     * @post Results in the typelist after appending operations
     *
     * @code{.cpp}
     * using result = append_t<typelist<int>, float, typelist<double>>;
     * // result is typelist<int, float, double>
     * @endcode
     */
    template <class TypeList, class... Us>
        requires mpl::is_type_instance_of_v<TypeList, typelist>
    using append_t = decltype(append(TypeList{}, Us{}...));

    /**
     * @brief Gets the count of types contained in a typelist.
     *
     * @tparam T Types in the typelist
     * @param list Typelist instance (defaulted for convenience)
     * @return Number of types in the typelist
     *
     * @post Returns sizeof...(T)
     * @post Result is available at compile time
     *
     * @code{.cpp}
     * constexpr auto count = size(typelist<int, float, double>{});
     * static_assert(count == 3);
     * @endcode
     */
    template <class... T>
    constexpr std::size_t size(typelist<T...> = {})
    {
        return sizeof...(T);
    }

    /**
     * @brief Concept that matches any typelist instantiation.
     *
     * This concept can be used to constrain template parameters to ensure
     * they are typelist instances.
     *
     * @tparam TList Type to check
     *
     * @post Evaluates to true for typelist<...> types, false otherwise
     *
     * @code{.cpp}
     * static_assert(any_typelist<typelist<int, float>>);
     * static_assert(!any_typelist<std::vector<int>>);
     * @endcode
     */
    template <typename TList>
    concept any_typelist = is_type_instance_of_v<TList, typelist>;

    namespace impl
    {
        template <class From, template <class...> class To>
        struct rename_impl;

        template <template <class...> class From, template <class...> class To, class... T>
        struct rename_impl<From<T...>, To>
        {
            using type = To<T...>;
        };
    }

    /**
     * @brief Changes the template type of a type list.
     *
     * This utility extracts the template arguments from one template instantiation
     * and applies them to a different template.
     *
     * @tparam From Source template instantiation
     * @tparam To Target template to apply arguments to
     *
     * @post Results in To<Args...> where Args are extracted from From
     *
     * @code{.cpp}
     * using result = rename<typelist<int, float>, std::variant>;
     * // result is std::variant<int, float>
     * @endcode
     */
    template <class From, template <class...> class To>
    using rename = typename impl::rename_impl<From, To>::type;

    //////////////////////////////////////////////////
    //// Type-predicates /////////////////////////////

    /**
     * @brief Concept for template types that can wrap a single type.
     *
     * This concept ensures that a template can be used as a type wrapper
     * for evaluation in type predicates, supporting either typelist or
     * std::type_identity patterns.
     *
     * @tparam W Template wrapper to check
     * @tparam T Type to be wrapped
     *
     * @post True if W<T> is either typelist<T> or std::type_identity_t<T>
     */
    template <template <class...> class W, class T>
    concept type_wrapper = std::same_as<W<T>, typelist<T>> or std::same_as<W<T>, std::type_identity_t<T>>;

    /**
     * @brief Concept for compile-time type predicates.
     *
     * This concept matches template types that can be evaluated at compile-time
     * similarly to std::true_type/std::false_type, making it possible to use
     * predicates provided by the standard library.
     *
     * @tparam P Predicate template to check
     * @tparam T Type to apply predicate to
     *
     * @post True if P<T>::value is convertible to bool
     *
     * @code{.cpp}
     * static_assert(ct_type_predicate<std::is_integral, int>);
     * @endcode
     */
    template <template <class> class P, class T>
    concept ct_type_predicate = requires {
        { P<T>::value } -> std::convertible_to<bool>;
    };

    /**
     * @brief Concept for callable type predicates.
     *
     * This concept matches types whose instances can be called with an object
     * representing a type to evaluate it. The object passed can be either a
     * typelist with one element or std::type_identity_t<T>.
     *
     * @tparam P Predicate type to check
     * @tparam T Type to apply predicate to
     *
     * @pre P must be semiregular
     * @post True if P instance can be called with type wrapper of T
     *
     * @code{.cpp}
     * auto predicate = [](auto type_wrapper) { return / some condition /; };
     * static_assert(callable_type_predicate<decltype(predicate), int>);
     * @endcode
     */
    template< class P, class T >
    concept callable_type_predicate = std::semiregular<std::decay_t<P>>
        and (
                requires(std::decay_t<P> predicate)
                {
                    { predicate(typelist<T>{}) } -> std::convertible_to<bool>;
                }
            or
                requires(std::decay_t<P> predicate)
                {
                    { predicate(std::type_identity_t<T>{}) } -> std::convertible_to<bool>;
                }
            )
        ;

    /**
     * @brief Evaluates a compile-time template predicate.
     *
     * @tparam T Type to evaluate
     * @tparam P Template predicate to apply
     * @param _ Predicate instance
     * @return Boolean result of P<T>::value
     *
     * @pre P must satisfy ct_type_predicate concept for T
     * @post Returns P<T>::value
     */
    template <class T, template <class> class P>
        requires ct_type_predicate<P, T>
    consteval bool evaluate(P<T> _)
    {
        return P<T>::value;
    }

    /**
     * @brief Evaluates a callable type predicate.
     *
     * @tparam T Type to evaluate
     * @tparam P Callable predicate type
     * @param predicate Predicate instance
     * @return Boolean result of calling predicate with type wrapper
     *
     * @pre P must satisfy callable_type_predicate concept for T
     * @post Returns result of calling predicate with appropriate type wrapper
     */
    template <class T, callable_type_predicate<T> P>
    [[nodiscard]] consteval bool evaluate(P predicate)
    {
        if constexpr (requires(std::decay_t<P> p) {
                          { p(typelist<T>{}) } -> std::same_as<bool>;
                      })
        {
            return predicate(typelist<T>{});
        }
        else
        {
            return predicate(std::type_identity_t<T>{});
        }
    }

    namespace predicate
    {

        /// Compile-time type predicate: `true` if the evaluated type is the same as `T`.
        /// Example:
        ///     static constexpr auto some_types = typelist<int, float>{}
        ///     static constexpr auto same_as_int = same_as<int>{};
        ///     static_assert( any_of(some_types, same_as_int) == true);
        ///     static_assert( all_of(some_types, same_as_int) == false);
        template <class T>
        struct same_as
        {
            template <template <class...> class W, class X>
                requires type_wrapper<W, X>
            consteval bool operator()(W<X>) const
            {
                return std::same_as<T, X>;
            }
        };
    }

    template <template <class> class P>
    struct ct_type_predicate_to_callable
    {
        template <template <class...> class W, class T>
            requires ct_type_predicate<P, T> and type_wrapper<W, T>
        consteval bool operator()(W<T>) const
        {
            return P<T>::value;
        }
    };

    /// @returns A callable type-predicate object which will return `P<T>::value` when called with `T` in a
    /// type-wrapper.
    template <template <class> class P>
    consteval auto as_predicate()
    {
        return ct_type_predicate_to_callable<P>{};
    };

    //////////////////////////////////////////////////
    //// Algorithms //////////////////////////////////

    /**
     * @brief Checks if at least one type in the typelist satisfies the predicate.
     *
     * @tparam Predicate Callable predicate type
     * @tparam L Typelist template
     * @tparam T Types in the typelist
     * @param list Typelist instance
     * @param predicate Predicate to apply (defaults to default-constructed)
     * @return true if at least one type satisfies the predicate
     *
     * @pre L<T...> must be a typelist
     * @pre Predicate must be callable for each type T
     * @post Returns true if any evaluate<T>(predicate) is true
     * @post Returns false for empty lists
     *
     * @code{.cpp}
     * auto is_integral = [](auto) { return std::is_integral_v<typename decltype(type)::type>; };
     * static_assert(any_of(typelist<int, float>{}, is_integral));
     * @endcode
     */
    template <class Predicate, template <class...> class L, class... T>
        requires any_typelist<L<T...>> and (callable_type_predicate<Predicate, T> && ...)
    [[nodiscard]] consteval bool any_of(L<T...>, [[maybe_unused]] Predicate predicate = {})
    {
        return (evaluate<T>(predicate) || ... || false);
    }

    /**
     * @brief Checks if at least one type satisfies the compile-time predicate.
     *
     * @tparam Predicate Template predicate to apply
     * @tparam L Typelist template
     * @tparam T Types in the typelist
     * @param list Typelist instance
     * @return true if at least one type satisfies the predicate
     *
     * @pre L<T...> must be a typelist
     * @pre Predicate must be a compile-time predicate for each type T
     * @post Returns true if any Predicate<T>::value is true
     * @post Returns false for empty lists
     */
    template <template <class> class Predicate, template <class...> class L, class... T>
        requires any_typelist<L<T...>> and (ct_type_predicate<Predicate, T> && ...)
    [[nodiscard]] consteval bool any_of(L<T...> list)
    {
        return any_of(list, as_predicate<Predicate>());
    }

    /**
     * @brief Checks if all types in the typelist satisfy the predicate.
     *
     * @tparam Predicate Callable predicate type
     * @tparam L Typelist template
     * @tparam T Types in the typelist
     * @param _ Typelist instance
     * @param predicate Predicate to apply
     * @return true if all types satisfy the predicate
     *
     * @pre L<T...> must be a typelist
     * @pre Predicate must be callable for each type T
     * @post Returns true if all evaluate<T>(predicate) are true
     * @post Returns true for empty lists
     */
    template <class Predicate, template <class...> class L, class... T>
        requires any_typelist<L<T...>> and (callable_type_predicate<Predicate, T> && ...)
    [[nodiscard]] consteval bool all_of(L<T...> _, [[maybe_unused]] Predicate predicate)
    {
        return (evaluate<T>(predicate) && ... && true);
    }

    /**
     * @brief Checks if all types satisfy the compile-time predicate.
     *
     * @tparam Predicate Template predicate to apply
     * @tparam L Typelist template
     * @tparam T Types in the typelist
     * @param list Typelist instance
     * @return true if all types satisfy the predicate
     *
     * @pre L<T...> must be a typelist
     * @pre Predicate must be a compile-time predicate for each type T
     * @post Returns true if all Predicate<T>::value are true
     * @post Returns true for empty lists
     */
    template <template <class> class Predicate, template <class...> class L, class... T>
        requires any_typelist<L<T...>> and (ct_type_predicate<Predicate, T> && ...)
    [[nodiscard]] consteval bool all_of(L<T...> list)
    {
        return all_of(list, as_predicate<Predicate>());
    }

    /**
     * @brief Finds the index of the first type satisfying the predicate.
     *
     * @tparam Predicate Callable predicate type
     * @tparam L Typelist template
     * @tparam T Types in the typelist
     * @param list Typelist instance
     * @param predicate Predicate to apply
     * @return Index of first matching type, or size() if not found
     *
     * @pre L<T...> must be a typelist
     * @pre Predicate must be callable for each type T
     * @post Returns index in range [0, sizeof...(T)]
     * @post Returns sizeof...(T) if no type matches
     */
    template <class Predicate, template <class...> class L, class... T>
        requires any_typelist<L<T...>> and (callable_type_predicate<Predicate, T> && ...)
    [[nodiscard]] consteval std::size_t find_if(L<T...>, Predicate predicate)
    {
        std::size_t idx = 0;
        auto check = [&](bool match_success)
        {
            if (match_success)
            {
                return true;
            }
            else
            {
                ++idx;
                return false;
            }
        };

        (check(evaluate<T>(predicate)) || ...);

        return idx;
    }

    /**
     * @brief Finds the index of the first type satisfying the compile-time predicate.
     *
     * @tparam Predicate Template predicate to apply
     * @tparam L Typelist template
     * @tparam T Types in the typelist
     * @param list Typelist instance
     * @return Index of first matching type, or size() if not found
     *
     * @pre L<T...> must be a typelist
     * @pre Predicate must be a compile-time predicate for each type T
     * @post Returns index in range [0, sizeof...(T)]
     * @post Returns sizeof...(T) if no type matches
     */
    template <template <class> class Predicate, template <class...> class L, class... T>
        requires any_typelist<L<T...>> and (ct_type_predicate<Predicate, T> && ...)
    [[nodiscard]] consteval std::size_t find_if(L<T...> list)
    {
        return find_if(list, as_predicate<Predicate>());
    }

    /**
     * @brief Finds the index of a specific type in the typelist.
     *
     * @tparam TypeToFind Type to search for
     * @tparam L Typelist type
     * @param list Typelist instance
     * @return Index of TypeToFind, or size() if not found
     *
     * @pre L must be a typelist
     * @post Returns index in range [0, size(L)]
     * @post Returns size(L) if TypeToFind is not in the list
     */
    template <class TypeToFind, any_typelist L>
    [[nodiscard]] consteval std::size_t find(L list)
    {
        return find_if(list, predicate::same_as<TypeToFind>{});
    }

    namespace impl
    {
        template <class L, class T>
        struct contains_impl;

        template <template <class...> class L, class T>
        struct contains_impl<L<>, T> : std::false_type
        {
        };

        template <template <class...> class L, class T, class... U>
        struct contains_impl<L<T, U...>, T> : std::true_type
        {
        };

        template <template <class...> class L, class V, class... U, class T>
        struct contains_impl<L<V, U...>, T> : contains_impl<L<U...>, T>
        {
        };
    }

    /**
     * @brief Checks if a typelist contains a specific type.
     *
     * @tparam L Typelist type to search in
     * @tparam V Type to search for
     * @return true if V is found in L, false otherwise
     *
     * @pre L must be a typelist
     * @post Returns true if V appears in L
     * @post Implementation avoids instantiating L for forward-declared types
     *
     * @code{.cpp}
     * static_assert(contains<typelist<int, float>, int()>);
     * static_assert(!contains<typelist<int, float>, double()>);
     * @endcode
     */
    template <any_typelist L, class V>
    [[nodiscard]] consteval bool contains()
    {
        return impl::contains_impl<L, V>::value;
    }

    namespace impl
    {
        template <template <class...> class F, class... L>
        struct transform_impl
        {
            static_assert(dependent_false<L...>::value, "transform can apply to typelist-like types only");
        };

        template <template <class...> class F, template <class...> class L, class... T>
        struct transform_impl<F, L<T...>>
        {
            using type = L<F<T>...>;
        };

        template <template <class...> class F, template <class...> class L1, class... T1, template <class...> class L2, class... T2>
        struct transform_impl<F, L1<T1...>, L2<T2...>>
        {
            using type = L1<F<T1, T2>...>;
        };
    }

    /**
     * @brief Applies a metafunction to each element of typelists.
     *
     * This template applies the metafunction F to each tuple of elements
     * in the provided typelists and returns the corresponding result list.
     *
     * @tparam F Metafunction to apply
     * @tparam L Typelist(s) to transform
     *
     * @pre L must be typelist-like types
     * @post Returns typelist with F applied to each element
     *
     * @code{.cpp}
     * // Single list transformation
     * using ptrs = transform<std::add_pointer_t, typelist<int, float>>;
     * // Result: typelist<int*, float*>
     *
     * // Binary transformation
     * using same = transform<std::is_same, typelist<int, float>, typelist<int, int>>;
     * // Result: typelist<std::is_same<int, int>, std::is_same<float, int>>
     * @endcode
     */
    template <template <class> class F, class... L>
    using transform = typename impl::transform_impl<F, L...>::type;

    namespace impl
    {
        template <class S1, class S2>
        struct merge_set_impl;

        template <template <class...> class L, class... T>
        struct merge_set_impl<L<T...>, L<>>
        {
            using type = L<T...>;
        };

        template <template <class...> class L, class... T, class U, class... V>
        struct merge_set_impl<L<T...>, L<U, V...>>
        {
            using first_arg = std::conditional_t<contains<L<T...>, U>(), L<T...>, L<T..., U>>;
            using type = typename merge_set_impl<first_arg, L<V...>>::type;
        };
    }

    /**
     * @brief Generates the union of two typelists, removing duplicates.
     *
     * This utility merges two typelists and ensures that each type appears
     * only once in the result, maintaining the order from the first list
     * and adding new types from the second list.
     *
     * @tparam L1 First typelist
     * @tparam L2 Second typelist
     *
     * @post Returns typelist containing all types from L1 and L2 without duplicates
     * @post Order preserves L1 types first, then new types from L2
     *
     * @code{.cpp}
     * using result = merge_set<typelist<int, float>, typelist<float, double>>;
     * // Result: typelist<int, float, double>
     * @endcode
     */
    template <class L1, class L2>
    using merge_set = typename impl::merge_set_impl<L1, L2>::type;

    namespace impl
    {
        template <class L>
        struct unique_impl;

        template <template <class...> class L, class... T>
        struct unique_impl<L<T...>>
        {
            using type = merge_set<L<>, L<T...>>;
        };
    }

    /// Removes all duplicated types in the given typelist
    /// Example:
    ///     unique<typelist<int, float, double, float, int> gives typelist<int, float, double>
    template <class L>
    using unique = typename impl::unique_impl<L>::type;

    //////////////////////////////////////////////////
    //// Miscellaneous ///////////////////////////////

    /**
     * @brief Adds const and lvalue reference to a type.
     *
     * This utility combines std::add_const and std::add_lvalue_reference
     * to produce const T& for most types.
     *
     * @tparam T Type to modify
     */
    template <class T>
    struct add_const_lvalue_reference : std::add_lvalue_reference<std::add_const_t<T>>
    {
    };

    /**
     * @brief Convenience alias for add_const_lvalue_reference.
     *
     * @tparam T Type to get const lvalue reference of
     *
     * @post Results in const T& for most types
     */
    template <class T>
    using add_const_lvalue_reference_t = typename add_const_lvalue_reference<T>::type;

    /**
     * @brief Conditionally adds const to a type.
     *
     * This utility allows conditional const-qualification based on a boolean
     * template parameter, with special handling for reference types.
     *
     * @tparam T Type to potentially const-qualify
     * @tparam is_const Whether to add const qualification
     */
    template <typename T, bool is_const>
    struct constify
    {
        using type = std::conditional_t<is_const, const T, T>;
    };

    /**
     * @brief Specialization for reference types.
     *
     * @tparam T Referenced type
     * @tparam is_const Whether to add const qualification to referenced type
     */
    template <typename T, bool is_const>
    struct constify<T&, is_const>
    {
        using type = std::conditional_t<is_const, const T&, T&>;
    };

    /**
     * @brief Convenience alias for constify.
     *
     * This is required since std::add_const_t<T&> is T& (not const T&).
     *
     * @tparam T Type to conditionally const-qualify
     * @tparam is_const Whether to add const qualification
     *
     * @post Results in const-qualified type if is_const is true
     * @post Handles reference types correctly
     */
    template <class T, bool is_const>
    using constify_t = typename constify<T, is_const>::type;

    /**
     * @brief Computes the const reference type of an iterator.
     *
     * This utility determines the appropriate const reference type for an iterator,
     * ensuring proper const-correctness for iterator operations.
     *
     * @tparam T Iterator type
     *
     * @post Results in the const reference type for iterator T
     */
    template <class T>
    using iter_const_reference_t = std::common_reference_t<const std::iter_value_t<T>&&, std::iter_reference_t<T>>;

    /**
     * @brief Concept for constant iterators.
     *
     * A constant iterator is an iterator whose reference type is the same as its
     * const reference type, ensuring that dereferencing always yields const access.
     *
     * @tparam T Iterator type to check
     *
     * @pre T must satisfy std::input_iterator
     * @post True if iterator provides only const access to elements
     *
     * @code{.cpp}
     * static_assert(constant_iterator<std::vector<int>::const_iterator>);
     * static_assert(!constant_iterator<std::vector<int>::iterator>);
     * @endcode
     */
    template <class T>
    concept constant_iterator = std::input_iterator<T>
                                && std::same_as<iter_const_reference_t<T>, std::iter_reference_t<T>>;

    /**
     * @brief Concept for constant ranges.
     *
     * A constant range is a range whose iterator type is a constant iterator,
     * ensuring that range-based operations provide only const access to elements.
     *
     * @tparam T Range type to check
     *
     * @pre T must satisfy std::ranges::input_range
     * @post True if range provides only const access to elements
     *
     * @code{.cpp}
     * const std::vector<int> vec = {1, 2, 3};
     * static_assert(constant_range<decltype(vec)>);
     * @endcode
     */
    template <class T>
    concept constant_range = std::ranges::input_range<T> && constant_iterator<std::ranges::iterator_t<T>>;

    /**
     * @brief Invokes undefined behavior for optimization purposes.
     *
     * This function can be used to mark code paths that should never be reached,
     * allowing compilers to optimize away impossible branches or trap execution
     * in debug builds.
     *
     * @pre This function should never actually be called
     * @post Undefined behavior occurs if called
     *
     * @note Implementation uses compiler-specific extensions when available
     * @note Documentation based on https://en.cppreference.com/w/cpp/utility/unreachable
     *
     * @code{.cpp}
     * switch (value) {
     *     case 1: return "one";
     *     case 2: return "two";
     *     default: unreachable(); // value is guaranteed to be 1 or 2
     * }
     * @endcode
     */
    [[noreturn]] inline void unreachable()
    {
        // Uses compiler specific extensions if possible.
        // Even if no extension is used, undefined behavior is still raised by
        // an empty function body and the noreturn attribute.
#if defined(_MSC_VER) && !defined(__clang__)  // MSVC
        __assume(false);
#else  // GCC, Clang
        __builtin_unreachable();
#endif
    }

    /**
     * @brief Concept for ranges whose elements are convertible to bool.
     *
     * @tparam BoolRange Range type to check
     *
     * @post True if range elements can be converted to bool
     */
    template <class BoolRange>
    concept bool_convertible_range = std::ranges::range<BoolRange>
                                     && std::convertible_to<std::ranges::range_value_t<BoolRange>, bool>;

    /**
     * @brief Concept for boolean-like types.
     *
     * This concept matches types that can be converted to and assigned from bool,
     * without requiring implicit conversion constructors.
     *
     * @tparam T Type to check
     *
     * @post True if T can be assigned from bool and converted to bool
     */
    template <class T>
    concept boolean_like = std::is_assignable_v<std::add_lvalue_reference_t<std::decay_t<T>>, bool>
                           and requires { static_cast<bool>(std::declval<T>()); };

    /**
     * @brief Concept for convertible range types.
     *
     * This concept checks if elements of range type From are convertible
     * to elements of range type To.
     *
     * @tparam From Source range type
     * @tparam To Target range type
     *
     * @post True if From elements are convertible to To elements
     */
    template <class From, class To>
    concept convertible_ranges = std::convertible_to<std::ranges::range_value_t<From>, std::ranges::range_value_t<To>>;

    /**
     * @brief Concept for unique_ptr instances.
     *
     * @tparam T Type to check
     *
     * @post True if T is a std::unique_ptr instantiation
     */
    template <typename T>
    concept unique_ptr = mpl::is_type_instance_of_v<std::remove_reference_t<T>, std::unique_ptr>;

    /**
     * @brief Concept for unique_ptr or derived types.
     *
     * @tparam T Type to check
     *
     * @post True if T is unique_ptr or derives from std::unique_ptr
     */
    template <typename T>
    concept unique_ptr_or_derived = unique_ptr<T>
                                    || std::derived_from<T, std::unique_ptr<typename T::element_type>>;

    /**
     * @brief Concept for shared_ptr instances.
     *
     * @tparam T Type to check
     *
     * @post True if T is a std::shared_ptr instantiation
     */
    template <typename T>
    concept shared_ptr = mpl::is_type_instance_of_v<std::remove_reference_t<T>, std::shared_ptr>;

    /**
     * @brief Concept for shared_ptr or derived types.
     *
     * @tparam T Type to check
     *
     * @post True if T is shared_ptr or derives from std::shared_ptr
     */
    template <typename T>
    concept shared_ptr_or_derived = shared_ptr<T>
                                    || std::derived_from<T, std::shared_ptr<typename T::element_type>>;

    /**
     * @brief Concept for any smart pointer type.
     *
     * @tparam T Type to check
     *
     * @post True if T is unique_ptr or shared_ptr
     */
    template <typename T>
    concept smart_ptr = unique_ptr<T> || shared_ptr<T>;

    /**
     * @brief Concept for smart pointers and derived types.
     *
     * @tparam T Type to check
     *
     * @post True if T is or derives from unique_ptr or shared_ptr
     */
    template <typename T>
    concept smart_ptr_and_derived = shared_ptr_or_derived<T> || unique_ptr_or_derived<T>;

    /**
     * @brief Concept for testable types in boolean contexts.
     *
     * @tparam T Type to check
     *
     * @post True if T can be used in boolean conditions
     */
    template <class T>
    concept testable = requires(T t) { t ? true : false; };

    /**
     * @brief Concept for matching qualifier requirements.
     *
     * This concept ensures that if type Y has a certain qualifier, then type T
     * must also have that qualifier.
     *
     * @tparam T First type to check
     * @tparam Y Second type to check
     * @tparam Qualifier Template predicate for the qualifier
     *
     * @post True if T has qualifier when Y has qualifier
     */
    template <typename T, typename Y, template <typename> typename Qualifier>
    concept T_matches_qualifier_if_Y_is = Qualifier<T>::value || !Qualifier<Y>::value;

    /**
     * @brief Helper to exclude copy and move constructors from variadic templates.
     *
     * This utility helps prevent copy and move constructors from being routed
     * to constructors with variadic arguments or perfect forwarding.
     *
     * @tparam CLS Class type
     * @tparam ARGS Argument types to check
     *
     * @post Value is false only for single argument that is same as CLS
     */
    template <class CLS, class... ARGS>
    struct excludes_copy_and_move_ctor
    {
        static constexpr bool value = true;
    };

    /**
     * @brief Specialization for no arguments.
     *
     * @tparam CLS Class type
     */
    template <class CLS>
    struct excludes_copy_and_move_ctor<CLS>
    {
        static constexpr bool value = true;
    };

    /**
     * @brief Specialization for single argument.
     *
     * @tparam CLS Class type
     * @tparam T Argument type
     *
     * @post Value is false if T is same as CLS (after removing cv and reference)
     */
    template <class CLS, class T>
    struct excludes_copy_and_move_ctor<CLS, T>
    {
        static constexpr bool value = !std::is_same_v<CLS, std::remove_cvref_t<T>>;
    };

    /**
     * @brief Convenience variable template for excludes_copy_and_move_ctor.
     *
     * @tparam CLS Class type
     * @tparam ARGS Argument types to check
     */
    template <class CLS, class... ARGS>
    constexpr bool excludes_copy_and_move_ctor_v = excludes_copy_and_move_ctor<CLS, ARGS...>::value;

    /**
     * @brief Concept for iterators of a specific value type.
     *
     * This concept ensures that an iterator type satisfies std::input_iterator
     * and that its value type matches the specified type exactly.
     *
     * @tparam I Iterator type to check
     * @tparam T Expected value type
     *
     * @pre I must satisfy std::input_iterator
     * @post True if iterator's value type exactly matches T
     */
    template <typename I, typename T>
    concept iterator_of_type = std::input_iterator<I>
                               && std::same_as<typename std::iterator_traits<I>::value_type, T>;

    /**
     * @brief Concept for character-like types.
     *
     * This concept matches basic character types commonly used for byte-level
     * operations and text processing.
     *
     * @tparam T Type to check
     *
     * @post True if T is char, std::byte, or uint8_t
     *
     * @note This is a simplified check based on common usage patterns
     */
    template <class T>
    concept char_like = std::same_as<T, char> || std::same_as<T, std::byte> || std::same_as<T, uint8_t>;

    /**
     * @brief Concept for std::array types.
     *
     * This concept identifies standard array types by checking for the required
     * type members and ensuring the type structure matches std::array.
     *
     * @tparam T Type to check
     *
     * @post True if T is a std::array instantiation
     */
    template <typename T>
    concept std_array = requires {
        typename std::remove_cvref_t<T>::value_type;
        requires std::same_as<
            std::array<typename std::remove_cvref_t<T>::value_type, std::tuple_size<std::remove_cvref_t<T>>::value>,
            std::remove_cvref_t<T>>;
    };

    // Concept to check if the instances of two types can be compared with operator==.
    // Notice that this concept is less restrictive than std::equality_comparable_with,
    // which requires the existence of a common refefenrece type for T and U. This additional
    // restriction makes it impossible to use it in the context of sparrow, where we want to
    // compare objects that are logically similar while being "physically" different.
    template <class T, class U>
    concept weakly_equality_comparable_with = requires(
        const std::remove_reference_t<T>& t,
        const std::remove_reference_t<U>& u
    ) {
        { t == u } -> std::convertible_to<bool>;
        { t != u } -> std::convertible_to<bool>;
        { u == t } -> std::convertible_to<bool>;
        { u != t } -> std::convertible_to<bool>;
    };
}
