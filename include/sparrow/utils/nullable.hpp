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

#include <compare>
#include <concepts>
#include <exception>
#if defined(__cpp_lib_format)
#    include <format>
#    include <ostream>
#endif
#include <type_traits>
#include <variant>

#include "sparrow/config/config.hpp"
#include "sparrow/utils/mp_utils.hpp"


#if defined(SPARROW_CONSTEXPR)
#    error "SPARROW_CONSTEXPR already defined"
#endif

// clang workaround: clang instantiates the constructor in SFINAE context,
// which is incompatible with the implementation of standard libraries which
// are not libc++. This leads to wrong compilation errors. Making the constructor
// not constexpr prevents the compiler from instantiating it.
#if defined(__clang__) && not defined(_LIBCPP_VERSION)
#    define SPARROW_CONSTEXPR
#else
#    define SPARROW_CONSTEXPR constexpr
#endif

namespace sparrow
{
    template <class T, mpl::boolean_like B>
    class nullable;

    template <class T>
    struct is_nullable : std::false_type
    {
    };

    template <class T, mpl::boolean_like B>
    struct is_nullable<nullable<T, B>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool is_nullable_v = is_nullable<T>::value;

    template <class N, class T>
    concept nullable_of = is_nullable_v<N> && std::same_as<typename N::value_type, T>;

    template <class N, class T>
    concept nullable_of_convertible_to = is_nullable_v<N> && std::convertible_to<typename N::value_type, T>;

    /*
     * Matches a range of nullables objects.
     *
     * A range is considered a range of nullables if it is a range and its value type is a nullable.
     *
     * @tparam RangeOfNullables The range to check.
     */
    template <class RangeOfNullables>
    concept range_of_nullables = std::ranges::range<RangeOfNullables>
                                 && is_nullable<std::ranges::range_value_t<RangeOfNullables>>::value;

    /*
     * Default traits for the nullable class. These traits should be specialized
     * for proxy classes whose reference and const_reference types are not
     * defined as usual. For instance:
     *
     * @code{.cpp}
     * struct nullable_traits<string_proxy>
     * {
     *     using value_type = std::string;
     *     using reference = string_proxy;
     *     using const_reference = std::string_view;
     *     using rvalue_reverence = std::string&&;
     *     using const_rvalue_reference = const std::string&&;
     * };
     * @endcode
     */
    template <class T>
    struct nullable_traits
    {
        using value_type = T;
        using reference = std::add_lvalue_reference_t<value_type>;
        using const_reference = std::add_lvalue_reference_t<std::add_const_t<value_type>>;
        using rvalue_reference = value_type&&;
        using const_rvalue_reference = const value_type&&;
    };

    template <class T>
    struct nullable_traits<T&>
    {
        using value_type = T;
        using reference = std::add_lvalue_reference_t<value_type>;
        using const_reference = std::add_lvalue_reference_t<std::add_const_t<value_type>>;
        using rvalue_reference = reference;
        using const_rvalue_reference = const_reference;
    };

    /**
     * @brief Exception thrown when accessing a null nullable value.
     *
     * This exception is thrown by nullable::value() when attempting to access
     * the underlying value of a nullable that is currently in a null state.
     */
    class bad_nullable_access : public std::exception
    {
    public:

        bad_nullable_access() noexcept = default;
        bad_nullable_access(const bad_nullable_access&) noexcept = default;
        bad_nullable_access& operator=(const bad_nullable_access&) noexcept = default;

        /**
         * @brief Gets the descriptive error message.
         *
         * @return C-string describing the error
         *
         * @post Returns non-null pointer to static error message
         */
        [[nodiscard]] const char* what() const noexcept override
        {
            return message;
        }

    private:

        static constexpr const char* message = "Invalid access to nullable underlying value";
    };

    /**
     * @brief Sentinel type to indicate a nullable value is null.
     *
     * nullval_t is used to construct or assign nullable objects to a null state.
     * It has a private constructor to prevent default construction and ensure
     * only the predefined nullval constant is used.
     */
    struct nullval_t
    {
        /**
         * @brief Private constructor to prevent default construction.
         */
        constexpr explicit nullval_t(int)
        {
        }
    };

    /**
     * @brief Global constant representing a null value for nullable objects.
     *
     * This constant is used to construct nullable objects in a null state
     * or to assign null to existing nullable objects.
     *
     * @example
     * ```cpp
     * nullable<int> n = nullval;  // null nullable
     * n = nullval;                // assign null
     * ```
     */
    inline constexpr nullval_t nullval(0);

    namespace impl
    {
        /**
         * Concepts used to disambiguate the nullable class constructors.
         */
        template <class T, class TArgs, class U, class UArgs>
        concept both_constructible_from_cref = std::constructible_from<T, mpl::add_const_lvalue_reference_t<TArgs>>
                                               and std::constructible_from<U, mpl::add_const_lvalue_reference_t<UArgs>>;

        template <class To1, class From1, class To2, class From2>
        concept both_convertible_from_cref = std::convertible_to<mpl::add_const_lvalue_reference_t<From1>, To1>
                                             and std::convertible_to<mpl::add_const_lvalue_reference_t<From2>, To2>;

        template <class T, class... Args>
        concept constructible_from_one = (std::constructible_from<T, Args> || ...);

        template <class T, class... Args>
        concept convertible_from_one = (std::convertible_to<Args, T> || ...);

        template <class T, class... Args>
        concept initializable_from_one = constructible_from_one<T, Args...> || convertible_from_one<T, Args...>;

        template <class T, class Arg>
        concept initializable_from_refs = initializable_from_one<T, Arg&, const Arg&, Arg&&, const Arg&&>;

        // We prefer std::is_assignable_v to std::assignable_from<To, From> because
        // std::assignable_from requires the existence of an implicit conversion
        // from From to To
        template <class To, class... Args>
        concept assignable_from_one = (std::is_assignable_v<std::add_lvalue_reference<To>, Args> && ...);

        template <class T, class Arg>
        concept assignable_from_refs = assignable_from_one<T, Arg&, const Arg&, Arg&&, const Arg&&>;

        template <class T, class U>
        using conditional_ref_t = std::conditional_t<std::is_reference_v<T>, const std::decay_t<U>&, std::decay_t<U>&&>;

        template <class T, class Targs, class U, class UArgs>
        concept both_constructible_from_cond_ref = std::constructible_from<T, conditional_ref_t<T, Targs>>
                                                   and std::constructible_from<U, conditional_ref_t<U, UArgs>>;

        template <class To1, class From1, class To2, class From2>
        concept both_convertible_from_cond_ref = std::convertible_to<conditional_ref_t<To1, From1>, To1>
                                                 and std::convertible_to<conditional_ref_t<To2, From2>, To2>;

        template <class To1, class From1, class To2, class From2>
        concept both_assignable_from_cref = std::is_assignable_v<
                                                std::add_lvalue_reference_t<To1>,
                                                mpl::add_const_lvalue_reference_t<From1>>
                                            and std::is_assignable_v<
                                                std::add_lvalue_reference_t<To2>,
                                                mpl::add_const_lvalue_reference_t<From2>>;

        template <class To1, class From1, class To2, class From2>
        concept both_assignable_from_cond_ref = std::is_assignable_v<
                                                    std::add_lvalue_reference_t<To1>,
                                                    conditional_ref_t<To1, From1>>
                                                and std::is_assignable_v<
                                                    std::add_lvalue_reference_t<To2>,
                                                    conditional_ref_t<To2, From2>>;
    }

    /**
     * @brief A type that models a value or reference that can be "null" or missing.
     *
     * The nullable class template provides a way to represent values that may be missing,
     * similar to std::optional but designed specifically for data science applications.
     * It stores both a value and a flag indicating whether the value should be considered
     * valid (non-null) or invalid (null).
     *
     * Key features:
     * - Value is always accessible, flag determines semantic validity
     * - Support for both value and reference semantics
     * - Efficient storage for data science workloads
     * - Compatible with range algorithms and containers
     * - Type-safe null checking and access
     *
     * When T is not a reference type, nullable has value semantics: copying moves
     * the underlying value and flag. When T is a reference type, nullable has view
     * semantics: it provides a view over external value and flag storage.
     *
     * @tparam T The type of the stored value (can be a value type or reference type)
     * @tparam B The type of the validity flag (must be boolean-like, defaults to bool)
     *
     * @pre T and B must be constructible and assignable types
     * @pre B must be convertible to and from bool
     * @post Value is always accessible regardless of flag state
     * @post Flag accurately reflects intended null/non-null semantics
     * @post Thread-safe for read operations, requires external synchronization for writes
     *
     * @example
     * ```cpp
     * // Value semantics
     * nullable<int> n1(42);           // non-null with value 42
     * nullable<int> n2 = nullval;     // null
     *
     * // Reference semantics (for container implementations)
     * int value = 10;
     * bool flag = true;
     * nullable<int&, bool&> ref_view(value, flag);
     *
     * // Safe access
     * if (n1.has_value()) {
     *     int val = n1.value();       // safe access
     * }
     * int safe_val = n1.value_or(0); // fallback value
     * ```
     */
    template <class T, mpl::boolean_like B = bool>
    class nullable
    {
    public:

        using self_type = nullable<T, B>;
        using value_traits = nullable_traits<T>;
        using value_type = typename value_traits::value_type;
        using reference = typename value_traits::reference;
        using const_reference = typename value_traits::const_reference;
        using rvalue_reference = typename value_traits::rvalue_reference;
        using const_rvalue_reference = typename value_traits::const_rvalue_reference;
        using flag_traits = nullable_traits<B>;
        using flag_type = typename flag_traits::value_type;
        using flag_reference = typename flag_traits::reference;
        using flag_const_reference = typename flag_traits::const_reference;
        using flag_rvalue_reference = typename flag_traits::rvalue_reference;
        using flag_const_rvalue_reference = typename flag_traits::const_rvalue_reference;

        /**
         * @brief Default constructor creating a null nullable.
         *
         * @tparam U Value type (deduced, must be default constructible)
         * @tparam BB Flag type (deduced, must be default constructible)
         *
         * @pre T and B must be default constructible
         * @post has_value() returns false
         * @post get() returns default-constructed value
         * @post null_flag() returns false
         */
        template <std::default_initializable U = T, std::default_initializable BB = B>
        constexpr nullable() noexcept
            : m_value()
            , m_null_flag(false)
        {
        }

        /**
         * @brief Constructor from nullval_t creating a null nullable.
         *
         * @tparam U Value type (deduced, must be default constructible)
         * @tparam BB Flag type (deduced, must be default constructible)
         *
         * @pre T and B must be default constructible
         * @post has_value() returns false
         * @post get() returns default-constructed value
         * @post null_flag() returns false
         */
        template <std::default_initializable U = T, std::default_initializable BB = B>
        constexpr nullable(nullval_t) noexcept
            : m_value()
            , m_null_flag(false)
        {
        }

        /**
         * @brief Constructor from a value creating a non-null nullable.
         *
         * @tparam U Type of the input value (must be constructible to T)
         * @param value Value to store
         *
         * @pre U must be constructible to T
         * @pre U must not be the same type as nullable (to avoid recursion)
         * @post has_value() returns true
         * @post get() returns the constructed value
         * @post null_flag() returns true
         */
        template <class U>
            requires(not std::same_as<self_type, std::decay_t<U>> and std::constructible_from<T, U &&>)
        explicit(not std::convertible_to<U&&, T>) constexpr nullable(U&& value) noexcept(
            noexcept(T(std::declval<U>()))
        )
            : m_value(std::forward<U>(value))
            , m_null_flag(true)
        {
        }

        /**
         * @brief Default copy constructor.
         *
         * @param rhs Source nullable to copy from
         *
         * @pre rhs must be in a valid state
         * @post This nullable is an exact copy of rhs
         * @post has_value() equals rhs.has_value()
         * @post If non-null, get() equals rhs.get()
         */
        constexpr nullable(const self_type& rhs) = default;

        /**
         * @brief Converting copy constructor from different nullable types.
         *
         * @tparam TO Source value type
         * @tparam BO Source flag type
         * @param rhs Source nullable to copy from
         *
         * @pre T must be constructible from TO
         * @pre B must be constructible from BO
         * @pre TO must not be initializable from this nullable type (prevents ambiguity)
         * @post This nullable contains converted copies of rhs value and flag
         * @post has_value() equals rhs.has_value()
         * @post If non-null, get() is constructed from rhs.get()
         */
        template <class TO, mpl::boolean_like BO>
            requires(impl::both_constructible_from_cref<T, TO, B, BO>
                     and not impl::initializable_from_refs<T, nullable<TO, BO>>)
        explicit(not impl::both_convertible_from_cref<T, TO, B, BO>) SPARROW_CONSTEXPR nullable(
            const nullable<TO, BO>& rhs
        )
            : m_value(rhs.get())
            , m_null_flag(rhs.null_flag())
        {
        }

#ifdef __clang__
        /**
         * @brief Converting copy constructor from different nullable types (Clang workaround for bool).
         *
         * This is a Clang-specific workaround for cases where T is bool, which requires
         * special handling due to compiler-specific template instantiation behavior.
         *
         * @tparam TO Source value type
         * @tparam BO Source flag type
         * @param rhs Source nullable to copy from
         *
         * @pre T must be constructible from TO
         * @pre B must be constructible from BO
         * @pre std::decay_t<T> must be bool (enforced by requirement)
         * @post This nullable contains converted copies of rhs value and flag
         * @post has_value() equals rhs.has_value()
         * @post If non-null, get() is constructed from rhs.get()
         *
         * @note This overload is only available on Clang and when T decays to bool
         * @note Required due to Clang's SFINAE instantiation behavior with non-libc++ standard libraries
         */
        template <class TO, mpl::boolean_like BO>
            requires(impl::both_constructible_from_cref<T, TO, B, BO> and std::same_as<std::decay_t<T>, bool>)
        explicit(not impl::both_convertible_from_cref<T, TO, B, BO>) SPARROW_CONSTEXPR nullable(
            const nullable<TO, BO>& rhs
        )
            : m_value(rhs.get())
            , m_null_flag(rhs.null_flag())
        {
        }
#endif

        /**
         * @brief Default move constructor.
         *
         * @param rhs Source nullable to move from
         *
         * @pre rhs must be in a valid state
         * @post This nullable is an exact copy of rhs
         * @post has_value() equals rhs.has_value()
         * @post If non-null, get() equals rhs.get()
         */
        constexpr nullable(self_type&& rhs) noexcept = default;

        /**
         * @brief Converting move constructor from different nullable types.
         *
         * @tparam TO Source value type
         * @tparam BO Source flag type
         * @param rhs Source nullable to move from
         *
         * @pre T must be constructible from TO
         * @pre B must be constructible from BO
         * @pre TO must not be initializable from this nullable type (prevents ambiguity)
         * @post This nullable contains converted moves of rhs value and flag
         * @post has_value() equals rhs.has_value()
         * @post If non-null, get() is constructed from std::move(rhs).get()
         */
        template <class TO, mpl::boolean_like BO>
            requires(impl::both_constructible_from_cond_ref<T, TO, B, BO>
                     and not impl::initializable_from_refs<T, nullable<TO, BO>>)
        explicit(not impl::both_convertible_from_cond_ref<T, TO, B, BO>) SPARROW_CONSTEXPR nullable(
            nullable<TO, BO>&& rhs
        )
            : m_value(std::move(rhs).get())
            , m_null_flag(std::move(rhs).null_flag())
        {
        }

#ifdef __clang__
        /**
         * @brief Converting move constructor from different nullable types (Clang workaround for bool).
         *
         * This is a Clang-specific workaround for cases where T is bool, which requires
         * special handling due to compiler-specific template instantiation behavior.
         *
         * @tparam TO Source value type
         * @tparam BO Source flag type
         * @param rhs Source nullable to move from
         *
         * @pre T must be constructible from TO
         * @pre B must be constructible from BO
         * @pre std::decay_t<T> must be bool (enforced by requirement)
         * @post This nullable contains converted moves of rhs value and flag
         * @post has_value() equals rhs.has_value()
         * @post If non-null, get() is constructed from std::move(rhs).get()
         * @post rhs is left in a valid but unspecified state
         *
         * @note This overload is only available on Clang and when T decays to bool
         * @note Required due to Clang's SFINAE instantiation behavior with non-libc++ standard libraries
         */
        template <class TO, mpl::boolean_like BO>
            requires(impl::both_constructible_from_cond_ref<T, TO, B, BO>
                     and std::same_as<std::decay_t<T>, bool>)
        explicit(not impl::both_convertible_from_cond_ref<T, TO, B, BO>) SPARROW_CONSTEXPR nullable(
            nullable<TO, BO>&& rhs
        )
            : m_value(std::move(rhs).get())
            , m_null_flag(std::move(rhs).null_flag())
        {
        }
#endif

        /**
         * @brief Constructor from value and flag.
         *
         * @param value Value to move and store
         * @param null_flag Flag to move and store
         *
         * @post get() returns the moved value
         * @post null_flag() returns the moved flag
         * @post Original value and flag are moved from
         */
        constexpr nullable(value_type&& value, flag_type&& null_flag)
            : m_value(std::move(value))
            , m_null_flag(std::move(null_flag))
        {
        }

        /**
         * @brief Constructor from lvalue references (for reference semantics).
         *
         * @param value Reference to value to store
         * @param null_flag Reference to flag to store
         *
         * @post get() refers to the provided value reference
         * @post null_flag() refers to the provided flag reference
         * @post Changes to the nullable affect the referenced objects
         */
        constexpr nullable(std::add_lvalue_reference_t<T> value, std::add_lvalue_reference_t<B> null_flag)
            : m_value(value)
            , m_null_flag(null_flag)
        {
        }

        /**
         * @brief Constructor from moved value and flag reference.
         *
         * @param value Value to move and store
         * @param null_flag Reference to flag to store
         *
         * @post get() returns the moved value
         * @post null_flag() refers to the provided flag reference
         */
        constexpr nullable(value_type&& value, std::add_lvalue_reference_t<B> null_flag)
            : m_value(std::move(value))
            , m_null_flag(null_flag)
        {
        }

        /**
         * @brief Constructor from value reference and moved flag.
         *
         * @param value Reference to value to store
         * @param null_flag Flag to move and store
         *
         * @post get() refers to the provided value reference
         * @post null_flag() returns the moved flag
         */
        constexpr nullable(std::add_lvalue_reference_t<T> value, flag_type&& null_flag)
            : m_value(value)
            , m_null_flag(std::move(null_flag))
        {
        }

        /**
         * @brief Assignment from nullval_t, setting nullable to null state.
         *
         * @param nullval_t nullval sentinel value
         * @return Reference to this nullable
         *
         * @post has_value() returns false
         * @post null_flag() returns false
         * @post get() remains accessible but semantically invalid
         */
        constexpr self_type& operator=(nullval_t) noexcept
        {
            m_null_flag = false;
            return *this;
        }

        /**
         * @brief Assignment from a value, setting nullable to non-null state.
         *
         * @tparam TO Type of the assigned value
         * @param rhs Value to assign
         * @return Reference to this nullable
         *
         * @pre TO must not be the same as nullable type
         * @pre TO must be assignable to T
         * @post has_value() returns true
         * @post get() contains the assigned value
         * @post null_flag() returns true
         */
        template <class TO>
            requires(not std::same_as<self_type, TO> and std::assignable_from<std::add_lvalue_reference_t<T>, TO>)
        constexpr self_type& operator=(TO&& rhs) noexcept
        {
            m_value = std::forward<TO>(rhs);
            m_null_flag = true;
            return *this;
        }

        /**
         * @brief Default copy assignment operator.
         *
         * @param rhs Source nullable to copy from
         * @return Reference to this nullable
         *
         * @pre rhs must be in a valid state
         * @post This nullable is an exact copy of rhs
         * @post has_value() equals rhs.has_value()
         * @post get() is assigned from rhs.get()
         */
        constexpr self_type& operator=(const self_type& rhs) noexcept
        {
            m_value = rhs.get();
            m_null_flag = rhs.null_flag();
            return *this;
        }

        /**
         * @brief Converting assignment operator from different nullable types.
         *
         * @tparam TO Source value type
         * @tparam BO Source flag type
         * @param rhs Source nullable to copy from
         * @return Reference to this nullable
         *
         * @pre T must be constructible from TO
         * @pre B must be constructible from BO
         * @pre TO must not be initializable from this nullable type (prevents ambiguity)
         * @post This nullable contains converted copies of rhs value and flag
         * @post has_value() equals rhs.has_value()
         * @post If non-null, get() is constructed from rhs.get()
         */
        template <class TO, mpl::boolean_like BO>
            requires(
                impl::both_assignable_from_cref<T, TO, B, BO>
                and not impl::initializable_from_refs<T, nullable<TO, BO>>
                and not impl::assignable_from_refs<T, nullable<TO, BO>>
            )
        constexpr self_type& operator=(const nullable<TO, BO>& rhs) noexcept
        {
            m_value = rhs.get();
            m_null_flag = rhs.null_flag();
            return *this;
        }

        /**
         * @brief Default move assignment operator.
         *
         * @param rhs Source nullable to move from
         * @return Reference to this nullable
         *
         * @pre rhs must be in a valid state
         * @post This nullable is an exact copy of rhs
         * @post has_value() equals rhs.has_value()
         * @post get() is assigned from std::move(rhs).get()
         */
        constexpr self_type& operator=(self_type&& rhs) noexcept
        {
            m_value = std::move(rhs).get();
            m_null_flag = std::move(rhs).null_flag();
            return *this;
        }

        /**
         * @brief Converting move assignment operator from different nullable types.
         *
         * @tparam TO Source value type
         * @tparam BO Source flag type
         * @param rhs Source nullable to move from
         * @return Reference to this nullable
         *
         * @pre T must be constructible from TO
         * @pre B must be constructible from BO
         * @pre TO must not be initializable from this nullable type (prevents ambiguity)
         * @post This nullable contains converted moves of rhs value and flag
         * @post has_value() equals rhs.has_value()
         * @post If non-null, get() is constructed from std::move(rhs).get()
         */
        template <class TO, mpl::boolean_like BO>
            requires(
                impl::both_assignable_from_cond_ref<T, TO, B, BO>
                and not impl::initializable_from_refs<T, nullable<TO, BO>>
                and not impl::assignable_from_refs<T, nullable<TO, BO>>
            )
        constexpr self_type& operator=(nullable<TO, BO>&& rhs) noexcept
        {
            m_value = std::move(rhs).get();
            m_null_flag = std::move(rhs).null_flag();
            return *this;
        }

        /**
         * @brief Conversion to bool indicating non-null state.
         *
         * @return true if nullable contains a valid value, false if null
         *
         * @post Return value equals has_value()
         * @post Return value equals static_cast<bool>(null_flag())
         */
        constexpr explicit operator bool() const noexcept;

        /**
         * @brief Checks whether the nullable contains a valid value.
         *
         * @return true if nullable contains a valid value, false if null
         *
         * @post Return value equals static_cast<bool>(*this)
         * @post Return value equals static_cast<bool>(null_flag())
         */
        [[nodiscard]] constexpr bool has_value() const noexcept;

        /**
         * @brief Gets mutable reference to the validity flag.
         *
         * @return Mutable reference to the flag
         *
         * @post Returned reference can be used to modify the null state
         * @post Changes to the flag affect has_value() result
         */
        [[nodiscard]] constexpr flag_reference null_flag() & noexcept;

        /**
         * @brief Gets const reference to the validity flag.
         *
         * @return Const reference to the flag
         *
         * @post Returned reference reflects current null state
         */
        [[nodiscard]] constexpr flag_const_reference null_flag() const& noexcept;

        /**
         * @brief Gets rvalue reference to the validity flag.
         *
         * @return Rvalue reference to the flag
         *
         * @post If flag is a reference type, returns the reference
         * @post If flag is a value type, returns moved flag
         */
        [[nodiscard]] constexpr flag_rvalue_reference null_flag() && noexcept;

        /**
         * @brief Gets const rvalue reference to the validity flag.
         *
         * @return Const rvalue reference to the flag
         *
         * @post If flag is a reference type, returns the reference
         * @post If flag is a value type, returns moved const flag
         */
        [[nodiscard]] constexpr flag_const_rvalue_reference null_flag() const&& noexcept;

        /**
         * @brief Gets mutable reference to the stored value.
         *
         * @return Mutable reference to the value
         *
         * @post Returned reference provides access to the stored value
         * @post Value is accessible regardless of null state
         * @post Modifications through reference affect stored value
         */
        [[nodiscard]] constexpr reference get() & noexcept;

        /**
         * @brief Gets const reference to the stored value.
         *
         * @return Const reference to the value
         *
         * @post Returned reference provides read-only access to stored value
         * @post Value is accessible regardless of null state
         */
        [[nodiscard]] constexpr const_reference get() const& noexcept;

        /**
         * @brief Gets rvalue reference to the stored value.
         *
         * @return Rvalue reference to the value
         *
         * @post If value is a reference type, returns the reference
         * @post If value is a value type, returns moved value
         */
        [[nodiscard]] constexpr rvalue_reference get() && noexcept;

        /**
         * @brief Gets const rvalue reference to the stored value.
         *
         * @return Const rvalue reference to the value
         *
         * @post If value is a reference type, returns the reference
         * @post If value is a value type, returns moved const value
         */
        [[nodiscard]] constexpr const_rvalue_reference get() const&& noexcept;

        /**
         * @brief Gets mutable reference to the value with null checking.
         *
         * @return Mutable reference to the value
         *
         * @pre has_value() must be true
         * @post Returns reference to the stored value
         *
         * @throws bad_nullable_access if has_value() is false
         */
        [[nodiscard]] constexpr reference value() &;

        /**
         * @brief Gets const reference to the value with null checking.
         *
         * @return Const reference to the value
         *
         * @pre has_value() must be true
         * @post Returns const reference to the stored value
         *
         * @throws bad_nullable_access if has_value() is false
         */
        [[nodiscard]] constexpr const_reference value() const&;

        /**
         * @brief Gets rvalue reference to the value with null checking.
         *
         * @return Rvalue reference to the value
         *
         * @pre has_value() must be true
         * @post Returns rvalue reference to the stored value
         * @post If value type, original value is moved from
         *
         * @throws bad_nullable_access if has_value() is false
         */
        [[nodiscard]] constexpr rvalue_reference value() &&;

        /**
         * @brief Gets const rvalue reference to the value with null checking.
         *
         * @return Const rvalue reference to the value
         *
         * @pre has_value() must be true
         * @post Returns const rvalue reference to the stored value
         *
         * @throws bad_nullable_access if has_value() is false
         */
        [[nodiscard]] constexpr const_rvalue_reference value() const&&;

        /**
         * @brief Gets the value or a default if null (const version).
         *
         * @tparam U Type of the default value
         * @param default_value Value to return if nullable is null
         * @return Stored value if non-null, otherwise default_value
         *
         * @pre U must be convertible to value_type
         * @post If has_value() is true, returns copy of stored value
         * @post If has_value() is false, returns converted default_value
         */
        template <class U>
        [[nodiscard]] constexpr value_type value_or(U&& default_value) const&;

        /**
         * @brief Gets the value or a default if null (rvalue version).
         *
         * @tparam U Type of the default value
         * @param default_value Value to return if nullable is null
         * @return Moved stored value if non-null, otherwise default_value
         *
         * @pre U must be convertible to value_type
         * @post If has_value() is true, returns moved stored value
         * @post If has_value() is false, returns converted default_value
         */
        template <class U>
        [[nodiscard]] constexpr value_type value_or(U&& default_value) &&;

        /**
         * @brief Swaps this nullable with another.
         *
         * @param other Nullable to swap with
         *
         * @post This nullable contains other's previous value and flag
         * @post other contains this nullable's previous value and flag
         */
        void swap(self_type& other) noexcept;

        /**
         * @brief Resets the nullable to null state.
         *
         * @post has_value() returns false
         * @post null_flag() returns false
         * @post get() remains accessible but semantically invalid
         */
        void reset() noexcept;

    private:

        /**
         * @brief Throws exception if nullable is in null state.
         *
         * @throws bad_nullable_access if has_value() is false
         */
        void throw_if_null() const;

        T m_value;      ///< The stored value (always valid)
        B m_null_flag;  ///< The validity flag (true = valid, false = null)

        template <class TO, mpl::boolean_like BO>
        friend class nullable;
    };

    /**
     * @brief Swaps two nullable objects.
     *
     * @tparam T Value type
     * @tparam B Flag type
     * @param lhs First nullable to swap
     * @param rhs Second nullable to swap
     *
     * @post lhs contains rhs's previous value and flag
     * @post rhs contains lhs's previous value and flag
     */
    template <class T, class B>
    constexpr void swap(nullable<T, B>& lhs, nullable<T, B>& rhs) noexcept;

    /**
     * @brief Equality comparison between nullable and nullval_t.
     *
     * @tparam T Value type
     * @tparam B Flag type
     * @param lhs Nullable to compare
     * @param dummy nullval sentinel
     * @return true if nullable is null, false otherwise
     *
     * @post Return value equals !lhs.has_value()
     */
    template <class T, class B>
    constexpr bool operator==(const nullable<T, B>& lhs, nullval_t) noexcept;

    /**
     * @brief Three-way comparison between nullable and nullval_t.
     *
     * @tparam T Value type
     * @tparam B Flag type
     * @param lhs Nullable to compare
     * @param dummy nullval sentinel
     * @return Ordering reflecting null state comparison
     *
     * @post Returns std::strong_ordering::greater if lhs.has_value()
     * @post Returns std::strong_ordering::equal if !lhs.has_value()
     */
    template <class T, mpl::boolean_like B>
    constexpr std::strong_ordering operator<=>(const nullable<T, B>& lhs, nullval_t dummy) noexcept;

    /**
     * @brief Equality comparison between nullable and regular value.
     *
     * @tparam T Value type
     * @tparam B Flag type
     * @tparam U Type of value to compare with
     * @param lhs Nullable to compare
     * @param rhs Value to compare with
     * @return true if nullable is non-null and values are equal
     *
     * @post Returns false if lhs is null
     * @post Returns lhs.get() == rhs if lhs is non-null
     */
    template <class T, class B, class U>
        requires(!is_nullable_v<U> && mpl::weakly_equality_comparable_with<T, U>)
    constexpr bool operator==(const nullable<T, B>& lhs, const U& rhs) noexcept;

    /**
     * @brief Three-way comparison between nullable and regular value.
     *
     * @tparam T Value type
     * @tparam B Flag type
     * @tparam U Type of value to compare with
     * @param lhs Nullable to compare
     * @param rhs Value to compare with
     * @return Ordering result
     *
     * @pre U must be three-way comparable with T
     * @post Returns std::strong_ordering::less if lhs is null
     * @post Returns lhs.get() <=> rhs if lhs is non-null
     */
    template <class T, class B, class U>
        requires(!is_nullable_v<U> && std::three_way_comparable_with<U, T>)
    constexpr std::compare_three_way_result_t<T, U>
    operator<=>(const nullable<T, B>& lhs, const U& rhs) noexcept;

    /**
     * @brief Equality comparison between two nullable objects.
     *
     * @tparam T First value type
     * @tparam B First flag type
     * @tparam U Second value type
     * @tparam UB Second flag type
     * @param lhs First nullable to compare
     * @param rhs Second nullable to compare
     * @return true if both null or both non-null with equal values
     *
     * @post Returns true if both are null
     * @post Returns false if only one is null
     * @post Returns lhs.get() == rhs.get() if both are non-null
     */
    template <class T, class B, class U, class UB>
        requires(mpl::weakly_equality_comparable_with<T, U>)
    constexpr bool operator==(const nullable<T, B>& lhs, const nullable<U, UB>& rhs) noexcept;

    /**
     * @brief Three-way comparison between two nullable objects.
     *
     * @tparam T First value type
     * @tparam B First flag type
     * @tparam U Second value type (must be three-way comparable with T)
     * @tparam UB Second flag type
     * @param lhs First nullable to compare
     * @param rhs Second nullable to compare
     * @return Ordering result
     *
     * @pre U must be three-way comparable with T
     * @post Null values compare less than non-null values
     * @post If both non-null, returns lhs.get() <=> rhs.get()
     * @post If both null, returns std::strong_ordering::equal
     */
    template <class T, class B, std::three_way_comparable_with<T> U, class UB>
    constexpr std::compare_three_way_result_t<T, U>
    operator<=>(const nullable<T, B>& lhs, const nullable<U, UB>& rhs) noexcept;

    /**
     * @brief Creates a nullable object with deduced types.
     *
     * @tparam T Value type (deduced)
     * @tparam B Flag type (deduced, defaults to bool)
     * @param value Value to store
     * @param flag Validity flag (defaults to true)
     * @return Nullable object containing the value and flag
     *
     * @post Returned nullable has specified value and flag
     * @post has_value() returns the flag value
     */
    template <class T, mpl::boolean_like B = bool>
    constexpr nullable<T, B> make_nullable(T&& value, B&& flag = true);

    /**
     * @brief Sets null values in a range to a default value.
     *
     * @tparam R Range type containing nullable values
     * @tparam T Value type for the default
     * @param range Range of nullable objects to process
     * @param default_value Value to assign to null elements
     *
     * @pre R must be a range of nullable objects with value type T
     * @post All previously null elements in range now contain default_value
     * @post All previously null elements now have has_value() == true
     * @post Non-null elements remain unchanged
     */
    template <std::ranges::range R, typename T = typename std::ranges::range_value_t<R>::value_type>
        requires(nullable_of<std::ranges::range_value_t<R>, T>)
    constexpr void zero_null_values(R& range, const T& default_value = T{});

    /**
     * @brief Variant of nullable types with has_value() convenience method.
     *
     * This class extends std::variant to work specifically with nullable types,
     * providing a uniform has_value() interface across all alternative types.
     *
     * @tparam T Pack of nullable types that can be stored in the variant
     *
     * @pre All types in T must be nullable types
     * @pre sizeof...(T) must be > 0
     * @post Provides has_value() that works regardless of active alternative
     * @post Maintains all std::variant functionality
     */
    template <class... T>
        requires(sizeof...(T) > 0 && (is_nullable_v<T> && ...))
    class nullable_variant : public std::variant<T...>
    {
    public:

        using base_type = std::variant<T...>;
        using base_type::base_type;

        constexpr nullable_variant(const nullable_variant&) = default;
        constexpr nullable_variant(nullable_variant&&) noexcept = default;

        /**
         * @brief Copy assignment operator.
         *
         * @param rhs Source variant to copy from
         * @return Reference to this variant
         *
         * @post This variant contains a copy of rhs
         * @post Active alternative matches rhs
         */
        constexpr nullable_variant& operator=(const nullable_variant&);

        /**
         * @brief Move assignment operator.
         *
         * @param rhs Source variant to move from
         * @return Reference to this variant
         *
         * @post This variant contains moved content from rhs
         * @post rhs is left in valid but unspecified state
         */
        constexpr nullable_variant& operator=(nullable_variant&&) noexcept;

        /**
         * @brief Conversion to bool indicating non-null state.
         *
         * @return true if active alternative has a valid value
         *
         * @post Return value equals has_value()
         */
        constexpr explicit operator bool() const;

        /**
         * @brief Checks whether the active alternative contains a valid value.
         *
         * @return true if active alternative is non-null, false otherwise
         *
         * @post Calls has_value() on the active nullable alternative
         */
        constexpr bool has_value() const;
    };
}

namespace std
{
    namespace mpl = sparrow::mpl;

    // Specialization of basic_common_reference for nullable proxies so
    // we can use ranges algorithm on iterators returning nullable
    template <class T, mpl::boolean_like TB, class U, mpl::boolean_like UB, template <class> class TQual, template <class> class UQual>
        requires std::common_reference_with<T, U> && std::common_reference_with<TB, UB>
    struct basic_common_reference<sparrow::nullable<T, TB>, sparrow::nullable<U, UB>, TQual, UQual>
    {
        using type = sparrow::
            nullable<std::common_reference_t<TQual<T>, UQual<U>>, std::common_reference_t<TQual<TB>, UQual<UB>>>;
    };
}

namespace sparrow
{
    /***************************
     * nullable implementation *
     ***************************/

    template <class T, mpl::boolean_like B>
    constexpr nullable<T, B>::operator bool() const noexcept
    {
        return m_null_flag;
    }

    template <class T, mpl::boolean_like B>
    constexpr bool nullable<T, B>::has_value() const noexcept
    {
        return m_null_flag;
    }

    template <class T, mpl::boolean_like B>
    constexpr auto nullable<T, B>::null_flag() & noexcept -> flag_reference
    {
        return m_null_flag;
    }

    template <class T, mpl::boolean_like B>
    constexpr auto nullable<T, B>::null_flag() const& noexcept -> flag_const_reference
    {
        return m_null_flag;
    }

    template <class T, mpl::boolean_like B>
    constexpr auto nullable<T, B>::null_flag() && noexcept -> flag_rvalue_reference
    {
        if constexpr (std::is_reference_v<B>)
        {
            return m_null_flag;
        }
        else
        {
            return flag_rvalue_reference(m_null_flag);
        }
    }

    template <class T, mpl::boolean_like B>
    constexpr auto nullable<T, B>::null_flag() const&& noexcept -> flag_const_rvalue_reference
    {
        if constexpr (std::is_reference_v<B>)
        {
            return m_null_flag;
        }
        else
        {
            return flag_const_rvalue_reference(m_null_flag);
        }
    }

    template <class T, mpl::boolean_like B>
    constexpr auto nullable<T, B>::get() & noexcept -> reference
    {
        return m_value;
    }

    template <class T, mpl::boolean_like B>
    constexpr auto nullable<T, B>::get() const& noexcept -> const_reference
    {
        return m_value;
    }

    template <class T, mpl::boolean_like B>
    constexpr auto nullable<T, B>::get() && noexcept -> rvalue_reference
    {
        if constexpr (std::is_reference_v<T>)
        {
            return m_value;
        }
        else
        {
            return rvalue_reference(m_value);
        }
    }

    template <class T, mpl::boolean_like B>
    constexpr auto nullable<T, B>::get() const&& noexcept -> const_rvalue_reference
    {
        if constexpr (std::is_reference_v<T>)
        {
            return m_value;
        }
        else
        {
            return const_rvalue_reference(m_value);
        }
    }

    template <class T, mpl::boolean_like B>
    constexpr auto nullable<T, B>::value() & -> reference
    {
        throw_if_null();
        return get();
    }

    template <class T, mpl::boolean_like B>
    constexpr auto nullable<T, B>::value() const& -> const_reference
    {
        throw_if_null();
        return get();
    }

    template <class T, mpl::boolean_like B>
    constexpr auto nullable<T, B>::value() && -> rvalue_reference
    {
        throw_if_null();
        return std::move(*this).get();
    }

    template <class T, mpl::boolean_like B>
    constexpr auto nullable<T, B>::value() const&& -> const_rvalue_reference
    {
        throw_if_null();
        return std::move(*this).get();
    }

    template <class T, mpl::boolean_like B>
    template <class U>
    constexpr auto nullable<T, B>::value_or(U&& default_value) const& -> value_type
    {
        return *this ? get() : value_type(std::forward<U>(default_value));
    }

    template <class T, mpl::boolean_like B>
    template <class U>
    constexpr auto nullable<T, B>::value_or(U&& default_value) && -> value_type
    {
        return *this ? get() : value_type(std::forward<U>(default_value));
    }

    template <class T, mpl::boolean_like B>
    void nullable<T, B>::swap(self_type& other) noexcept
    {
        using std::swap;
        swap(m_value, other.m_value);
        swap(m_null_flag, other.m_null_flag);
    }

    template <class T, mpl::boolean_like B>
    void nullable<T, B>::reset() noexcept
    {
        m_null_flag = false;
    }

    template <class T, mpl::boolean_like B>
    void nullable<T, B>::throw_if_null() const
    {
        if (!m_null_flag)
        {
            throw bad_nullable_access{};
        }
    }

    template <class T, class B>
    constexpr void swap(nullable<T, B>& lhs, nullable<T, B>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template <class T, class B>
    constexpr bool operator==(const nullable<T, B>& lhs, nullval_t) noexcept
    {
        return !lhs;
    }

    template <class T, class B>
    constexpr std::strong_ordering operator<=>(const nullable<T, B>& lhs, nullval_t) noexcept
    {
        return lhs <=> false;
    }

    template <class T, class B, class U>
        requires(!is_nullable_v<U> && mpl::weakly_equality_comparable_with<T, U>)
    constexpr bool operator==(const nullable<T, B>& lhs, const U& rhs) noexcept
    {
        return lhs && (lhs.get() == rhs);
    }

    template <class T, class B, class U>
        requires(!is_nullable_v<U> && std::three_way_comparable_with<U, T>)
    constexpr std::compare_three_way_result_t<T, U> operator<=>(const nullable<T, B>& lhs, const U& rhs) noexcept
    {
        return lhs ? lhs.get() <=> rhs : std::strong_ordering::less;
    }

    template <class T, class B, class U, class UB>
        requires(mpl::weakly_equality_comparable_with<T, U>)
    constexpr bool operator==(const nullable<T, B>& lhs, const nullable<U, UB>& rhs) noexcept
    {
        return rhs ? lhs == rhs.get() : !lhs;
    }

    template <class T, class B, std::three_way_comparable_with<T> U, class UB>
    constexpr std::compare_three_way_result_t<T, U>
    operator<=>(const nullable<T, B>& lhs, const nullable<U, UB>& rhs) noexcept
    {
        return (lhs && rhs) ? lhs.get() <=> rhs.get() : bool(lhs) <=> bool(rhs);
    }

    template <class T, mpl::boolean_like B>
    constexpr nullable<T, B> make_nullable(T&& value, B&& flag)
    {
        return nullable<T, B>(std::forward<T>(value), std::forward<B>(flag));
    }

    template <std::ranges::range R, typename T>
        requires(nullable_of<std::ranges::range_value_t<R>, T>)
    constexpr void zero_null_values(R& range, const T& default_value)
    {
        for (auto nullable_value : range)
        {
            if (!nullable_value.has_value())
            {
                nullable_value.get() = default_value;
            }
        }
    }

    /***********************************
     * nullable_variant implementation *
     ***********************************/

    template <class... T>
        requires(sizeof...(T) > 0 && (is_nullable_v<T> && ...))
    constexpr nullable_variant<T...>& nullable_variant<T...>::operator=(const nullable_variant& rhs)
    {
        base_type::operator=(rhs);
        return *this;
    }

    template <class... T>
        requires(sizeof...(T) > 0 && (is_nullable_v<T> && ...))
    constexpr nullable_variant<T...>& nullable_variant<T...>::operator=(nullable_variant&& rhs) noexcept
    {
        base_type::operator=(std::move(rhs));
        return *this;
    }

    template <class... T>
        requires(sizeof...(T) > 0 && (is_nullable_v<T> && ...))
    constexpr nullable_variant<T...>::operator bool() const
    {
        return has_value();
    }

    template <class... T>
        requires(sizeof...(T) > 0 && (is_nullable_v<T> && ...))
    constexpr bool nullable_variant<T...>::has_value() const
    {
        return std::visit(
            [](const auto& v)
            {
                return v.has_value();
            },
#if SPARROW_GCC_11_2_WORKAROUND
            static_cast<const base_type&>(*this)
#else
            *this
#endif
        );
    }
}

#if defined(__cpp_lib_format)

template <typename T, sparrow::mpl::boolean_like B>
struct std::formatter<sparrow::nullable<T, B>>
{
    constexpr auto parse(format_parse_context& ctx)
    {
        auto pos = ctx.begin();
        while (pos != ctx.end() && *pos != '}')
        {
            m_format_string.push_back(*pos);
            ++pos;
        }
        m_format_string.push_back('}');
        return pos;
    }

    auto format(const sparrow::nullable<T, B>& n, std::format_context& ctx) const
    {
        if (n.has_value())
        {
            return std::vformat_to(ctx.out(), m_format_string, std::make_format_args(n.get()));
        }
        else
        {
            return std::format_to(ctx.out(), "{}", "null");
        }
    }

    std::string m_format_string = "{:";
};

template <typename T, sparrow::mpl::boolean_like B>
std::ostream& operator<<(std::ostream& os, const sparrow::nullable<T, B>& value)
{
    os << std::format("{}", value);
    return os;
}

template <class... T>
struct std::formatter<sparrow::nullable_variant<T...>>
{
    constexpr auto parse(format_parse_context& ctx)
    {
        auto pos = ctx.begin();
        while (pos != ctx.end() && *pos != '}')
        {
            m_format_string.push_back(*pos);
            ++pos;
        }
        m_format_string.push_back('}');
        return pos;
    }

    auto format(const sparrow::nullable_variant<T...>& variant, std::format_context& ctx) const
    {
        if (variant.has_value())
        {
            return std::visit(
                [&](const auto& value)
                {
                    return std::vformat_to(ctx.out(), m_format_string, std::make_format_args(value));
                },
                variant
            );
        }
        else
        {
            return std::format_to(ctx.out(), "{}", "null");
        }
    }

    std::string m_format_string = "{:";
};

template <class... T>
std::ostream& operator<<(std::ostream& os, const sparrow::nullable_variant<T...>& value)
{
    os << std::format("{}", value);
    return os;
}

template <>
struct std::formatter<sparrow::nullval_t>
{
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::nullval_t&, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "nullval");
    }
};

#endif

inline std::ostream& operator<<(std::ostream& os, const sparrow::nullval_t&)
{
    constexpr std::string_view nullval_str = "nullval";
    os << nullval_str;
    return os;
}

#undef SPARROW_CONSTEXPR
