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
    concept is_nullable_of = is_nullable_v<N> && std::same_as<typename N::value_type, T>;

    template <class N, class T>
    concept is_nullable_of_convertible_to = is_nullable_v<N> && std::convertible_to<typename N::value_type, T>;

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

    /*
     * Defines a type of object to be thrown by nullable::value when accessing
     * a nullable object whose value is null.
     */
    class bad_nullable_access : public std::exception
    {
    public:

        bad_nullable_access() noexcept = default;
        bad_nullable_access(const bad_nullable_access&) noexcept = default;
        bad_nullable_access& operator=(const bad_nullable_access&) noexcept = default;

        const char* what() const noexcept override
        {
            return message;
        }

    private:

        static constexpr const char* message = "Invalid access to nullable underlying value";
    };

    /**
     * nullval_t is an empty class used to indicate that a nullable is null.
     */
    struct nullval_t
    {
        // This is required to disable the generation of
        // the default constructor. Otherwise, a = {} where
        // a is a nullable would lead to an ambiguous call
        // where both operator=(nullable&&) and operator=(nullval_t)
        // are valid.
        constexpr explicit nullval_t(int)
        {
        }
    };

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

        template <class T>
        static constexpr bool is_nullable_v = mpl::is_type_instance_of_v<T, nullable>;
    }

    /**
     * The nullable class models a value or a reference that can be "null", or missing,
     * like values traditionally used in data science libraries.
     * The flag indicating whether the element should be considered missing can be a
     * boolean-like value or reference.
     *
     * The value is always valid, independently from the value of the flag. The flag
     * only indicates whether the value should be considered as specified (flag is true)
     * or null (flag is false). Assigning nullval to a nullable or setting its flag to
     * flase does not trigger the destruction of the underlying value.
     *
     * When the stored object is not a reference, the nullable class has a regular value
     * semantics: copying or moving it will copy or move the underlying value and flag.
     * When the stored object is a reference, the nullable class has a view semantics:
     * copying it or moving it will copy the underlying value and flag instead of reassigining
     * the references. This allows to create nullable views over two distinct arrays (one
     * for the values, one for the flags) used to implement a stl-like contianer of nullable.
     * For instance, if you have the following class:
     *
     * @code{.cpp}
     * template <class T, class B>
     * class nullable_array
     * {
     * private:
     *     std::vector<T> m_values;
     *     std::vector<bool> m_flags;
     *
     * public:
     *
     *     using reference = nullable<double&, bool&>;
     *     using const_reference = nullable<const double&, const bool&>;
     *
     *     reference operator[](size_type i)
     *     {
     *         return reference(m_values[i], m_flags[i]);
     *     }
     *
     *     const_reference operator[](size_type i) const
     *     {
     *         return const_reference(m_values[i], m_flags[i]);
     *     }
     *     // ...
     * };
     * @endcode
     *
     * Then you want the same semantic for accessing elements of nullable_array
     * as that of std::vector, meaning that the following:
     *
     * @code{.cpp}
     * nullable_array my_array = { ... };
     * my_array[1] = my_array[0];
     * @endcode
     *
     * should copy the underlying value and flag of the first element of my_array
     * to the underlying value and flag of the second element of the array.
     *
     * @tparam T the type of the value
     * @tparam B the type of the flag. This type must be convertible to and assignable from bool
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

        template <std::default_initializable U = T, std::default_initializable BB = B>
        constexpr nullable() noexcept
            : m_value()
            , m_null_flag(false)
        {
        }

        template <std::default_initializable U = T, std::default_initializable BB = B>
        constexpr nullable(nullval_t) noexcept
            : m_value()
            , m_null_flag(false)
        {
        }

        template <class U>
            requires(not std::same_as<self_type, std::decay_t<U>> and std::constructible_from<T, U &&>)
        explicit(not std::convertible_to<U&&, T>) constexpr nullable(U&& value
        ) noexcept(noexcept(T(std::declval<U>())))
            : m_value(std::forward<U>(value))
            , m_null_flag(true)
        {
        }

        constexpr nullable(const self_type&) = default;

        template <class TO, mpl::boolean_like BO>
            requires(impl::both_constructible_from_cref<T, TO, B, BO>
                     and not impl::initializable_from_refs<T, nullable<TO, BO>>)
        explicit(not impl::both_convertible_from_cref<T, TO, B, BO>) SPARROW_CONSTEXPR
            nullable(const nullable<TO, BO>& rhs)
            : m_value(rhs.get())
            , m_null_flag(rhs.null_flag())
        {
        }

#ifdef __clang__
        template <class TO, mpl::boolean_like BO>
            requires(impl::both_constructible_from_cref<T, TO, B, BO> and std::same_as<std::decay_t<T>, bool>)
        explicit(not impl::both_convertible_from_cref<T, TO, B, BO>) SPARROW_CONSTEXPR
            nullable(const nullable<TO, BO>& rhs)
            : m_value(rhs.get())
            , m_null_flag(rhs.null_flag())
        {
        }
#endif

        constexpr nullable(self_type&&) noexcept = default;

        template <class TO, mpl::boolean_like BO>
            requires(impl::both_constructible_from_cond_ref<T, TO, B, BO>
                     and not impl::initializable_from_refs<T, nullable<TO, BO>>)
        explicit(not impl::both_convertible_from_cond_ref<T, TO, B, BO>) SPARROW_CONSTEXPR
            nullable(nullable<TO, BO>&& rhs)
            : m_value(std::move(rhs).get())
            , m_null_flag(std::move(rhs).null_flag())
        {
        }

#ifdef __clang__
        template <class TO, mpl::boolean_like BO>
            requires(impl::both_constructible_from_cond_ref<T, TO, B, BO>
                     and std::same_as<std::decay_t<T>, bool>)
        explicit(not impl::both_convertible_from_cond_ref<T, TO, B, BO>) SPARROW_CONSTEXPR
            nullable(nullable<TO, BO>&& rhs)
            : m_value(std::move(rhs).get())
            , m_null_flag(std::move(rhs).null_flag())
        {
        }
#endif

        constexpr nullable(value_type&& value, flag_type&& null_flag)
            : m_value(std::move(value))
            , m_null_flag(std::move(null_flag))
        {
        }

        constexpr nullable(std::add_lvalue_reference_t<T> value, std::add_lvalue_reference_t<B> null_flag)
            : m_value(value)
            , m_null_flag(null_flag)
        {
        }

        constexpr nullable(value_type&& value, std::add_lvalue_reference_t<B> null_flag)
            : m_value(std::move(value))
            , m_null_flag(null_flag)
        {
        }

        constexpr nullable(std::add_lvalue_reference_t<T> value, flag_type&& null_flag)
            : m_value(value)
            , m_null_flag(std::move(null_flag))
        {
        }

        constexpr self_type& operator=(nullval_t)
        {
            m_null_flag = false;
            return *this;
        }

        template <class TO>
            requires(not std::same_as<self_type, TO> and std::assignable_from<std::add_lvalue_reference_t<T>, TO>)
        constexpr self_type& operator=(TO&& rhs)
        {
            m_value = std::forward<TO>(rhs);
            m_null_flag = true;
            return *this;
        }

        constexpr self_type& operator=(const self_type& rhs)
        {
            m_value = rhs.get();
            m_null_flag = rhs.null_flag();
            return *this;
        }

        template <class TO, mpl::boolean_like BO>
            requires(impl::both_assignable_from_cref<T, TO, B, BO> and not impl::initializable_from_refs<T, nullable<TO, BO>> and not impl::assignable_from_refs<T, nullable<TO, BO>>)
        constexpr self_type& operator=(const nullable<TO, BO>& rhs)
        {
            m_value = rhs.get();
            m_null_flag = rhs.null_flag();
            return *this;
        }

        constexpr self_type& operator=(self_type&& rhs)
        {
            m_value = std::move(rhs).get();
            m_null_flag = std::move(rhs).null_flag();
            return *this;
        }

        template <class TO, mpl::boolean_like BO>
            requires(impl::both_assignable_from_cond_ref<T, TO, B, BO> and not impl::initializable_from_refs<T, nullable<TO, BO>> and not impl::assignable_from_refs<T, nullable<TO, BO>>)
        constexpr self_type& operator=(nullable<TO, BO>&& rhs)
        {
            m_value = std::move(rhs).get();
            m_null_flag = std::move(rhs).null_flag();
            return *this;
        }

        constexpr explicit operator bool() const noexcept;
        constexpr bool has_value() const noexcept;

        constexpr flag_reference null_flag() & noexcept;
        constexpr flag_const_reference null_flag() const& noexcept;
        constexpr flag_rvalue_reference null_flag() && noexcept;
        constexpr flag_const_rvalue_reference null_flag() const&& noexcept;

        constexpr reference get() & noexcept;
        constexpr const_reference get() const& noexcept;
        constexpr rvalue_reference get() && noexcept;
        constexpr const_rvalue_reference get() const&& noexcept;

        constexpr reference value() &;
        constexpr const_reference value() const&;
        constexpr rvalue_reference value() &&;
        constexpr const_rvalue_reference value() const&&;

        template <class U>
        constexpr value_type value_or(U&& default_value) const&;

        template <class U>
        constexpr value_type value_or(U&& default_value) &&;

        void swap(self_type& other) noexcept;
        void reset() noexcept;

    private:

        void throw_if_null() const;

        T m_value;
        B m_null_flag;

        template <class TO, mpl::boolean_like BO>
        friend class nullable;
    };

    template <class T, class B>
    constexpr void swap(nullable<T, B>& lhs, nullable<T, B>& rhs) noexcept;

    template <class T, class B>
    constexpr bool operator==(const nullable<T, B>& lhs, nullval_t) noexcept;

    template <class T, mpl::boolean_like B>
    constexpr std::strong_ordering operator<=>(const nullable<T, B>& lhs, nullval_t) noexcept;

    template <class T, class B, class U>
    constexpr bool operator==(const nullable<T, B>& lhs, const U& rhs) noexcept;

    template <class T, class B, class U>
        requires(!impl::is_nullable_v<U> && std::three_way_comparable_with<U, T>)
    constexpr std::compare_three_way_result_t<T, U>
    operator<=>(const nullable<T, B>& lhs, const U& rhs) noexcept;

    template <class T, class B, class U, class UB>
    constexpr bool operator==(const nullable<T, B>& lhs, const nullable<U, UB>& rhs) noexcept;

    template <class T, class B, std::three_way_comparable_with<T> U, class UB>
    constexpr std::compare_three_way_result_t<T, U>
    operator<=>(const nullable<T, B>& lhs, const nullable<U, UB>& rhs) noexcept;

    // Even if we have CTAD in C++20, some constructors add lvalue reference
    // to their argument, making the deduction impossible.
    template <class T, mpl::boolean_like B = bool>
    constexpr nullable<T, B> make_nullable(T&& value, B&& flag = true);

    /**
     * variant of nullable, exposing has_value for convenience
     *
     * @tparam T the list of nullable in the variant
     */
    template <class... T>
        requires(is_nullable_v<T> && ...)
    class nullable_variant : public std::variant<T...>
    {
    public:

        using base_type = std::variant<T...>;
        using base_type::base_type;

        constexpr nullable_variant(const nullable_variant&) = default;
        constexpr nullable_variant(nullable_variant&&) noexcept = default;

        constexpr nullable_variant& operator=(const nullable_variant&);
        constexpr nullable_variant& operator=(nullable_variant&&);

        constexpr explicit operator bool() const;
        constexpr bool has_value() const;
    };
}

namespace std
{
    namespace mpl = sparrow::mpl;

    // Specialization of basic_common_reference for nullable proxies so
    // we can use ranges algorithm on iterators returning nullable
    template <class T, mpl::boolean_like TB, class U, mpl::boolean_like UB, template <class> class TQual, template <class> class UQual>
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
    constexpr bool operator==(const nullable<T, B>& lhs, const U& rhs) noexcept
    {
        return lhs && (lhs.get() == rhs);
    }

    template <class T, class B, class U>
        requires(!impl::is_nullable_v<U> && std::three_way_comparable_with<U, T>)
    constexpr std::compare_three_way_result_t<T, U> operator<=>(const nullable<T, B>& lhs, const U& rhs) noexcept
    {
        return lhs ? lhs.get() <=> rhs : std::strong_ordering::less;
    }

    template <class T, class B, class U, class UB>
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

    /***********************************
     * nullable_variant implementation *
     ***********************************/

    template <class... T>
        requires(is_nullable_v<T> && ...)
    constexpr nullable_variant<T...>& nullable_variant<T...>::operator=(const nullable_variant& rhs)
    {
        base_type::operator=(rhs);
        return *this;
    }

    template <class... T>
        requires(is_nullable_v<T> && ...)
    constexpr nullable_variant<T...>& nullable_variant<T...>::operator=(nullable_variant&& rhs)
    {
        base_type::operator=(std::move(rhs));
        return *this;
    }

    template <class... T>
        requires(is_nullable_v<T> && ...)
    constexpr nullable_variant<T...>::operator bool() const
    {
        return has_value();
    }

    template <class... T>
        requires(is_nullable_v<T> && ...)
    constexpr bool nullable_variant<T...>::has_value() const
    {
        return std::visit(
            [](const auto& v)
            {
                return v.has_value();
            },
            *this
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


#endif

#undef SPARROW_CONSTEXPR
