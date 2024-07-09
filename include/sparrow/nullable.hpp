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
#include <type_traits>

#include "sparrow/mp_utils.hpp"

namespace sparrow
{
    template <class T>
    concept boolean_like = std::constructible_from<T, bool> and std::convertible_to<T, bool>;

    template <class T, boolean_like B>
    class nullable;
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

        virtual const char* what() const noexcept
        {
            return message;
        }

    private:

        static constexpr const char* message = "bad nullable access";
    };

    /**
     * nullval_t is an empty class used to indicate that a nullable is null.
     */
    struct nullval_t
    {
        constexpr explicit nullval_t(int) {}
    };

    inline constexpr nullval_t nullval(0);

    namespace impl
    {
        /**
         * Concepts used to disambiguate the nullable class constructors.
         */
        template <class T, class TArgs, class U, class UArgs>
        concept both_constructible_from_cref =
            std::constructible_from<T, mpl::add_const_lvalue_reference_t<TArgs>> and
            std::constructible_from<U, mpl::add_const_lvalue_reference_t<UArgs>>;

        template <class To1, class From1, class To2, class From2>
        concept both_convertible_from_cref =
            std::convertible_to<mpl::add_const_lvalue_reference_t<From1>, To1> and
            std::convertible_to<mpl::add_const_lvalue_reference_t<From2>, To2>;

        template <class T, class... Args>
        concept constructible_from_one =
            (std::constructible_from<T, Args> || ...);

        template <class T, class... Args>
        concept convertible_from_one =
            (std::convertible_to<Args, T> || ...);

        template <class T, class... Args>
        concept initializable_from_one =
            constructible_from_one<T, Args...> ||
            convertible_from_one<T, Args...>;

        template <class T, class Arg>
        concept initializable_from_refs =
            initializable_from_one<T, Arg&, const Arg&, Arg&&, const Arg&&>;

        template <class To, class... Args>
        concept assignable_from_one =
            (std::assignable_from<std::add_lvalue_reference_t<To>, Args> && ...);

        template <class T, class Arg>
        concept assignable_from_refs =
            assignable_from_one<T, Arg&, const Arg&, Arg&&, const Arg&&>;

        template <class T, class U>
        using conditional_ref_t = std::conditional_t<std::is_reference_v<T>, const std::decay_t<U>&, std::decay_t<U>&&>;

        template <class T, class Targs, class U, class UArgs>
        concept both_constructible_from_cond_ref =
            std::constructible_from<T, conditional_ref_t<T, Targs>> and
            std::constructible_from<U, conditional_ref_t<U, UArgs>>;

        template <class To1, class From1, class To2, class From2>
        concept both_convertible_from_cond_ref =
            std::convertible_to<conditional_ref_t<To1, From1>, To1> and
            std::convertible_to<conditional_ref_t<To2, From2>, To2>;

        template <class To1, class From1, class To2, class From2>
        concept both_assignable_from_cref =
            std::assignable_from<std::add_lvalue_reference_t<To1>, mpl::add_const_lvalue_reference_t<From1>> and
            std::assignable_from<std::add_lvalue_reference_t<To2>, mpl::add_const_lvalue_reference_t<From2>&>;

        template <class To1, class From1, class To2, class From2>
        concept both_assignable_from_cond_ref =
            std::assignable_from<std::add_lvalue_reference_t<To1>, conditional_ref_t<To1, From1>> and
            std::assignable_from<std::add_lvalue_reference_t<To2>, conditional_ref_t<To2, From2>>;

        template <class T>
        static constexpr bool is_nullable_v = mpl::is_type_instance_of_v<T, nullable>;
    }

    /**
     * The nullable class.
     */
    template <class T, boolean_like B = bool>
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
        
        constexpr nullable() noexcept
            : m_value()
            , m_flag(false)
        {
        }
        
        constexpr nullable(nullval_t) noexcept
            : m_value()
            , m_flag(false)
        {
        }
        
        template <class U>
        requires (
            not std::same_as<self_type, std::decay_t<U>> and
            std::constructible_from<T, U&&>
        )
        explicit (not std::convertible_to<U&&, T>)
        constexpr nullable(U&& value)
            : m_value(std::forward<U>(value))
            , m_flag(true)
        {
        }

        constexpr nullable(const self_type&) = default;

        template <class TO, boolean_like BO>
        requires (
            impl::both_constructible_from_cref<T, TO, B, BO> and
            not impl::initializable_from_refs<T, nullable<TO, BO>>
        )
        explicit(not impl::both_convertible_from_cref<T, TO, B, BO>)
        constexpr nullable(const nullable<TO, BO>& rhs)
            : m_value(rhs.get())
            , m_flag(rhs.has_value())
        {
        }

        constexpr nullable(self_type&&) noexcept = default;

        template <class TO, boolean_like BO>
        requires (
            impl::both_constructible_from_cond_ref<T, TO, B, BO> and
            not impl::initializable_from_refs<T, nullable<TO, BO>>
        )
        explicit(not impl::both_convertible_from_cond_ref<T, TO, B, BO>)
        constexpr nullable(nullable<TO, BO>&& rhs)
            : m_value(std::move(rhs).get())
            , m_flag(std::move(rhs).has_value())
        {
        }

        constexpr nullable(value_type&& value, flag_type&& flag)
            : m_value(std::move(value))
            , m_flag(std::move(flag))
        {
        }

        constexpr nullable(std::add_lvalue_reference_t<T> value, std::add_lvalue_reference_t<B> flag)
            : m_value(value)
            , m_flag(flag)
        {
        }

        constexpr nullable(value_type&& value, std::add_lvalue_reference_t<B> flag)
            : m_value(std::move(value))
            , m_flag(flag)
        {
        }

        constexpr nullable(std::add_lvalue_reference_t<T> value, flag_type&& flag)
            : m_value(value)
            , m_flag(std::move(flag))
        {
        }

        constexpr self_type& operator=(nullval_t)
        {
            m_flag = false;
            return *this;
        }

        template <class TO>
        requires(
            not std::same_as<self_type, TO> and
            std::assignable_from<std::add_lvalue_reference_t<T>, TO>
        )
        constexpr self_type& operator=(TO&& rhs)
        {
            m_value = std::forward<TO>(rhs);
            m_flag = true;
            return *this;
        }

        constexpr self_type& operator=(const self_type& rhs)
        {
            m_value = rhs.get();
            m_flag = rhs.has_value();
            return *this;
        }

        template <class TO, boolean_like BO>
        requires(
            impl::both_assignable_from_cref<T, TO, B, BO> and
            not impl::initializable_from_refs<T, nullable<TO, BO>> and
            not impl::assignable_from_refs<T, nullable<TO, BO>>
        )
        constexpr self_type& operator=(const nullable<TO, BO>& rhs)
        {
            m_value = rhs.get();
            m_flag = rhs.has_value();
            return *this;
        }

        constexpr self_type& operator=(self_type&& rhs)
        {
            m_value = std::move(rhs).get();
            m_flag = std::move(rhs).has_value();
            return *this;
        }

        template <class TO, boolean_like BO>
        requires(
            impl::both_assignable_from_cond_ref<T, TO, B, BO> and
            not impl::initializable_from_refs<T, nullable<TO, BO>> and
            not impl::assignable_from_refs<T, nullable<TO, BO>>
        )
        constexpr self_type& operator=(nullable<TO, BO>&& rhs)
        {
            m_value = std::move(rhs).get();
            m_flag = std::move(rhs).has_value();
            return *this;
        }

        constexpr explicit operator bool() const noexcept;

        constexpr flag_reference has_value() & noexcept;
        constexpr flag_const_reference has_value() const & noexcept;
        constexpr flag_rvalue_reference has_value() && noexcept;
        constexpr flag_const_rvalue_reference has_value() const && noexcept;

        constexpr reference get() & noexcept;
        constexpr const_reference get() const & noexcept;
        constexpr rvalue_reference get() && noexcept;
        constexpr const_rvalue_reference get() const && noexcept;
        
        constexpr reference value() &;
        constexpr const_reference value() const &;
        constexpr rvalue_reference value() &&;
        constexpr const_rvalue_reference value() const &&;

        template <class U>
        constexpr value_type value_or(U&& default_value) const &;

        template <class U>
        constexpr value_type value_or(U&& default_value) &&;

        void swap(self_type& other) noexcept;
        void reset() noexcept;

    private:

        void throw_if_null() const;

        T m_value;
        B m_flag;

        template <class TO, boolean_like BO>
        friend class nullable;
    };

    template <class T, class B>
    constexpr void swap(nullable<T, B>& lhs, nullable<T, B>& rhs) noexcept;

    template <class T, class B>
    constexpr bool operator==(const nullable<T, B>& lhs, nullval_t) noexcept;

    template <class T, boolean_like B>
    constexpr std::strong_ordering operator<=>(const nullable<T, B>& lhs, nullval_t) noexcept;
    
    template <class T, class B, class U>
    constexpr bool operator==(const nullable<T, B>& lhs, const U& rhs) noexcept;

    template <class T, class B, class U>
    requires (!impl::is_nullable_v<U> && std::three_way_comparable_with<U, T>)
    constexpr std::compare_three_way_result_t<T, U>
    operator<=>(const nullable<T, B>& lhs, const U& rhs) noexcept;

    template <class T, class B, class U, class UB>
    constexpr bool operator==(const nullable<T, B>& lhs, const nullable<U, UB>& rhs) noexcept;

    template <class T, class B, std::three_way_comparable_with<T> U, class UB>
    constexpr std::compare_three_way_result_t<T, U>
    operator<=>(const nullable<T, B>& lhs, const nullable<U, UB>& rhs) noexcept;

    template <class T, boolean_like B = bool>
    constexpr nullable<T, B> make_nullable(T&& value, B&& flag = true);

    /***************************
     * nullable implementation *
     ***************************/

    template <class T, boolean_like B>
    constexpr nullable<T, B>::operator bool() const noexcept
    {
        return m_flag;
    }

    template <class T, boolean_like B>
    constexpr auto nullable<T, B>::has_value() & noexcept -> flag_reference
    {
        return m_flag;
    }
    
    template <class T, boolean_like B>
    constexpr auto nullable<T, B>::has_value() const & noexcept -> flag_const_reference
    {
        return m_flag;
    }
    
    template <class T, boolean_like B>
    constexpr auto nullable<T, B>::has_value() && noexcept -> flag_rvalue_reference
    {
        if constexpr (std::is_reference_v<B>)
        {
            return m_flag;
        }
        else
        {
            return flag_rvalue_reference(m_flag);
        }
    }
    
    template <class T, boolean_like B>
    constexpr auto nullable<T, B>::has_value() const && noexcept -> flag_const_rvalue_reference
    {
        if constexpr (std::is_reference_v<B>)
        {
            return m_flag;
        }
        else
        {
            return flag_const_rvalue_reference(m_flag);
        }
    }

    template <class T, boolean_like B>
    constexpr auto nullable<T, B>::get() & noexcept -> reference
    {
        return m_value;
    }

    template <class T, boolean_like B>
    constexpr auto nullable<T, B>::get() const & noexcept -> const_reference
    {
        return m_value;
    }
    
    template <class T, boolean_like B>
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

    template <class T, boolean_like B>
    constexpr auto nullable<T, B>::get() const && noexcept -> const_rvalue_reference
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

    template <class T, boolean_like B>
    constexpr auto nullable<T, B>::value() & -> reference
    {
        throw_if_null();
        return get();
    }

    template <class T, boolean_like B>
    constexpr auto nullable<T, B>::value() const & -> const_reference
    {
        throw_if_null();
        return get();
    }
    
    template <class T, boolean_like B>
    constexpr auto nullable<T, B>::value() && -> rvalue_reference
    {
        throw_if_null();
        return std::move(*this).get();
    }

    template <class T, boolean_like B>
    constexpr auto nullable<T, B>::value() const && -> const_rvalue_reference
    {
        throw_if_null();
        return std::move(*this).get();
    }

    template <class T, boolean_like B>
    template <class U>
    constexpr auto nullable<T, B>::value_or(U&& default_value) const & -> value_type
    {
        return *this ? get() : value_type(std::forward<U>(default_value)); 
    }

    template <class T, boolean_like B>
    template <class U>
    constexpr auto nullable<T, B>::value_or(U&& default_value) && -> value_type
    {
        return *this ? get() : value_type(std::forward<U>(default_value)); 
    }

    template <class T, boolean_like B>
    void nullable<T, B>::swap(self_type& other) noexcept
    {
        using std::swap;
        swap(m_value, other.m_value);
        swap(m_flag, other.m_flag);
    }

    template <class T, boolean_like B>
    void nullable<T, B>::reset() noexcept
    {
        m_flag = false;
    }
    
    template <class T, boolean_like B>
    void nullable<T, B>::throw_if_null() const
    {
        if (!m_flag)
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
    requires (!impl::is_nullable_v<U> && std::three_way_comparable_with<U, T>)
    constexpr std::compare_three_way_result_t<T, U>
    operator<=>(const nullable<T, B>& lhs, const U& rhs) noexcept
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

    template <class T, boolean_like B>
    constexpr nullable<T, B> make_nullable(T&& value, B&& flag)
    {
        return nullable<T, B>(std::forward<T>(value), std::forward<B>(flag));
    }

}

