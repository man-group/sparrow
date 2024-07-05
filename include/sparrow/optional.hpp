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
#include <optional>
#include <type_traits>

#include "sparrow/mp_utils.hpp"

namespace sparrow
{
    template <class T, class B>
    class optional;

    template <class T>
    struct optional_traits
    {
        using value_type = std::decay_t<T>;
        using reference = value_type&;
        using const_reference = const value_type&;
        using rvalue_reference = value_type&&;
        using const_rvalue_reference = const value_type&&;
    };

    template <class T>
    struct optional_traits<T&>
    {
        using value_type = std::decay_t<T>;
        using unref_type = T;
        using reference = T&;
        using const_reference = std::add_lvalue_reference_t<std::add_const_t<unref_type>>;
        using rvalue_reference = T&&;
        using const_rvalue_reference = std::add_rvalue_reference_t<std::add_const_t<unref_type>>;
    };

    namespace util
    {
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
        struct is_optional : std::false_type
        {
        };

        template <class T, class B>
        struct is_optional<optional<T, B>> : std::true_type
        {
        };

        template <class T>
        static constexpr bool is_optional_v = is_optional<T>::value;
    }

    /**
     * The optional class is similar to std::optional, with two major differences:
     * - it can act as a proxy, meaning its template parameter can be lvalue references
     *   or lvalue const references
     * - the semantic for empty optoinal: resetting an non empty optional does not destruct
     *   the contained value. Allocating an empty optional construct the contained value. This
     *   behavior is temporary and will be changed in the near future.
     */
    template <class T, class B = bool>
    class optional
    {
    public:

        using self_type = optional<T, B>;
        using traits = optional_traits<T>;
        using value_type = typename traits::value_type;
        using reference = typename traits::reference;
        using const_reference = typename traits::const_reference;
        using rvalue_reference = typename traits::rvalue_reference;
        using const_rvalue_reference = typename traits::const_rvalue_reference;
        using flag_type = std::decay_t<B>;
        
        constexpr optional() noexcept
            : m_value()
            , m_flag(false)
        {
        }
        
        constexpr optional(std::nullopt_t) noexcept
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
        constexpr optional(U&& value)
            : m_value(std::forward<U>(value))
            , m_flag(true)
        {
        }

        constexpr optional(const self_type&) = default;

        template <class TO, class BO>
        requires (
            util::both_constructible_from_cref<T, TO, B, BO> and
            not util::initializable_from_refs<T, optional<TO, BO>>
        )
        explicit(not util::both_convertible_from_cref<T, TO, B, BO>)
        constexpr optional(const optional<TO, BO>& rhs)
            : m_value(rhs.m_value)
            , m_flag(rhs.m_flag)
        {
        }

        constexpr optional(self_type&&) noexcept = default;

        template <class TO, class BO>
        requires (
            util::both_constructible_from_cond_ref<T, TO, B, BO> and
            not util::initializable_from_refs<T, optional<TO, BO>>
        )
        explicit(not util::both_convertible_from_cond_ref<T, TO, B, BO>)
        constexpr optional(optional<TO, BO>&& rhs)
            : m_value(std::move(rhs.m_value))
            , m_flag(std::move(rhs.m_flag))
        {
        }

        constexpr optional(value_type&& value, flag_type&& flag)
            : m_value(std::move(value))
            , m_flag(std::move(flag))
        {
        }

        constexpr optional(std::add_lvalue_reference_t<T> value, std::add_lvalue_reference_t<B> flag)
            : m_value(value)
            , m_flag(flag)
        {
        }

        constexpr optional(value_type&& value, std::add_lvalue_reference_t<B> flag)
            : m_value(std::move(value))
            , m_flag(flag)
        {
        }

        constexpr optional(std::add_lvalue_reference_t<T> value, flag_type&& flag)
            : m_value(value)
            , m_flag(std::move(flag))
        {
        }

        constexpr self_type& operator=(std::nullopt_t)
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
            m_value = rhs.m_value;
            m_flag = rhs.m_flag;
            return *this;
        }

        template <class TO, class BO>
        requires(
            util::both_assignable_from_cref<T, TO, B, BO> and
            not util::initializable_from_refs<T, optional<TO, BO>> and
            not util::assignable_from_refs<T, optional<TO, BO>>
        )
        constexpr self_type& operator=(const optional<TO, BO>& rhs)
        {
            m_value = rhs.m_value;
            m_flag = rhs.m_flag;
            return *this;
        }

        constexpr self_type& operator=(self_type&& rhs)
        {
            m_value = std::move(rhs.m_value);
            m_flag = std::move(rhs.m_flag);
            return *this;
        }

        template <class TO, class BO>
        requires(
            util::both_assignable_from_cond_ref<T, TO, B, BO> and
            not util::initializable_from_refs<T, optional<TO, BO>> and
            not util::assignable_from_refs<T, optional<TO, BO>>
        )
        constexpr self_type& operator=(optional<TO, BO>&& rhs)
        {
            m_value = std::move(rhs.m_value);
            m_flag = std::move(rhs.m_flag);
            return *this;
        }

        constexpr bool has_value() const noexcept;
        constexpr explicit operator bool() const noexcept;

        constexpr reference operator*() & noexcept;
        constexpr const_reference operator*() const & noexcept;
        constexpr rvalue_reference operator*() && noexcept;
        constexpr const_rvalue_reference operator*() const && noexcept;
        
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

        void throw_if_empty() const;

        T m_value;
        B m_flag;

        template <class TO, class BO>
        friend class optional;
    };

    template <class T, class B>
    constexpr void swap(optional<T, B>& lhs, optional<T, B>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template <class T, class B>
    constexpr bool operator==(const optional<T, B>& lhs, std::nullopt_t) noexcept;

    template <class T, class B>
    constexpr std::strong_ordering operator<=>(const optional<T, B>& lhs, std::nullopt_t) noexcept;
    
    template <class T, class B, class U>
    constexpr bool operator==(const optional<T, B>& lhs, const U& rhs) noexcept;

    template <class T, class B, class U>
    requires (!util::is_optional_v<U> && std::three_way_comparable_with<U, T>)
    constexpr std::compare_three_way_result_t<T, U>
    operator<=>(const optional<T, B>& lhs, const U& rhs) noexcept;

    template <class T, class B, class U, class UB>
    constexpr bool operator==(const optional<T, B>& lhs, const optional<U, UB>& rhs) noexcept;

    template <class T, class B, std::three_way_comparable_with<T> U, class UB>
    constexpr std::compare_three_way_result_t<T, U>
    operator<=>(const optional<T, B>& lhs, const optional<U, UB>& rhs) noexcept;

    template <class T, class B = bool>
    constexpr optional<T, B> make_optional(T&& value, B&& flag = true);

    /***************************
     * optional implementation *
     ***************************/

    template <class T, class B>
    constexpr bool optional<T, B>::has_value() const noexcept
    {
        return m_flag;
    }

    template <class T, class B>
    constexpr optional<T, B>::operator bool() const noexcept
    {
        return m_flag;
    }

    template <class T, class B>
    constexpr auto optional<T, B>::operator*() & noexcept -> reference
    {
        return m_value;
    }

    template <class T, class B>
    constexpr auto optional<T, B>::operator*() const & noexcept -> const_reference
    {
        return m_value;
    }
    
    template <class T, class B>
    constexpr auto optional<T, B>::operator*() && noexcept -> rvalue_reference
    {
        return std::move(m_value);
    }

    template <class T, class B>
    constexpr auto optional<T, B>::operator*() const && noexcept -> const_rvalue_reference
    {
        return std::move(m_value);
    }

    template <class T, class B>
    constexpr auto optional<T, B>::value() & -> reference
    {
        throw_if_empty();
        return m_value;
    }

    template <class T, class B>
    constexpr auto optional<T, B>::value() const & -> const_reference
    {
        throw_if_empty();
        return m_value;
    }
    
    template <class T, class B>
    constexpr auto optional<T, B>::value() && -> rvalue_reference
    {
        throw_if_empty();
        return std::move(m_value);
    }

    template <class T, class B>
    constexpr auto optional<T, B>::value() const && -> const_rvalue_reference
    {
        throw_if_empty();
        return std::move(m_value);
    }

    template <class T, class B>
    template <class U>
    constexpr auto optional<T, B>::value_or(U&& default_value) const & -> value_type
    {
        return bool(*this) ? **this : static_cast<value_type>(std::forward<U>(default_value)); 
    }

    template <class T, class B>
    template <class U>
    constexpr auto optional<T, B>::value_or(U&& default_value) && -> value_type
    {
        return bool(*this) ? std::move(**this) : static_cast<value_type>(std::forward<U>(default_value)); 
    }

    template <class T, class B>
    void optional<T, B>::swap(self_type& other) noexcept
    {
        using std::swap;
        swap(m_value, other.m_value);
        swap(m_flag, other.m_flag);
    }

    template <class T, class B>
    void optional<T, B>::reset() noexcept
    {
        m_flag = false;
    }
    
    template <class T, class B>
    void optional<T, B>::throw_if_empty() const
    {
        if (!m_flag)
        {
            throw std::bad_optional_access{};
        }
    }

    template <class T, class B>
    constexpr bool operator==(const optional<T, B>& lhs, std::nullopt_t) noexcept
    {
        return !lhs;
    }

    template <class T, class B>
    constexpr std::strong_ordering operator<=>(const optional<T, B>& lhs, std::nullopt_t) noexcept
    {
        return bool(lhs) <=> false;
    }
    
    template <class T, class B, class U>
    constexpr bool operator==(const optional<T, B>& lhs, const U& rhs) noexcept
    {
        return bool(lhs) && *lhs == rhs;
    }

    template <class T, class B, class U>
    requires (!util::is_optional_v<U> && std::three_way_comparable_with<U, T>)
    constexpr std::compare_three_way_result_t<T, U>
    operator<=>(const optional<T, B>& lhs, const U& rhs) noexcept
    {
        return bool(lhs) ? *lhs <=> rhs : std::strong_ordering::less;
    }

    template <class T, class B, class U, class UB>
    constexpr bool operator==(const optional<T, B>& lhs, const optional<U, UB>& rhs) noexcept
    {
        return bool(rhs) ? lhs == *rhs : !bool(lhs);
    }

    template <class T, class B, std::three_way_comparable_with<T> U, class UB>
    constexpr std::compare_three_way_result_t<T, U>
    operator<=>(const optional<T, B>& lhs, const optional<U, UB>& rhs) noexcept
    {
        return (bool(lhs) && bool(rhs)) ? *lhs <=> *rhs : bool(lhs) <=> bool(rhs);
    }

    template <class T, class B>
    constexpr optional<T, B> make_optional(T&& value, B&& flag)
    {
        return optional<T, B>(std::forward<T>(value), std::forward<B>(flag));
    }

}

