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

#include <compare>
#include <concepts>
#include <memory>

namespace sparrow
{
    /**
     * Matches types that provide a `clone` method.
     *
     * This concept checks if a type provides a `clone` method that
     * returns a pointer convertible to a pointer to the type.
     *
     * @tparam T The type to check
     */
    template <class T>
    concept clonable = requires(const T* t)
    {
        { t->clone() } -> std::convertible_to<T*>;
    };

    /**
     * Smart pointer behaving like a copiable std::unique_ptr.
     * 
     * `cloning_ptr` owns and manages another object through a pointer, like
     * `std::unique_ptr`. The difference with `std::unique_ptr` is that 
     * `cloning_ptr`calls the `clone` method of the managed object upon copy.
     * `Therefore, `cloning_ptr` is meant to be used with hierarchies of
     * classes which provide a `clone` method.
     * 
     * @tparam T The type of the object managed by the `cloning_ptr`. It must
     * satisfy the `clonable` concept.
     */
    template <clonable T>
    class cloning_ptr
    {
        using internal_pointer = std::unique_ptr<T>;
    public:

        using self_type = cloning_ptr<T>;
        using pointer = typename internal_pointer::pointer;
        using element_type = typename internal_pointer::element_type;

        // Value semantic

        constexpr cloning_ptr() noexcept = default;
        constexpr cloning_ptr(std::nullptr_t) noexcept;
        explicit constexpr cloning_ptr(pointer p) noexcept;

        constexpr ~cloning_ptr() = default;

        constexpr cloning_ptr(const self_type& rhs) noexcept;
        constexpr cloning_ptr(self_type&& rhs) noexcept = default;

        template <clonable U>
        requires std::convertible_to<U*, T*>
        constexpr cloning_ptr(const cloning_ptr<U>& rhs) noexcept;

        template <clonable U>
        requires std::convertible_to<U*, T*>
        constexpr cloning_ptr(cloning_ptr<U>&& rhs) noexcept;

        constexpr self_type& operator=(const self_type&) noexcept;
        constexpr self_type& operator=(self_type&&) noexcept = default;
        constexpr self_type& operator=(std::nullptr_t) noexcept;

        template <clonable U>
        requires std::convertible_to<U*, T*>
        constexpr self_type& operator=(const cloning_ptr<U>& rhs) noexcept;

        template <clonable U>
        requires std::convertible_to<U*, T*>
        constexpr self_type& operator=(cloning_ptr<U>&& rhs) noexcept;

        // Modifiers

        constexpr pointer release() noexcept;
        constexpr void reset(pointer ptr = pointer()) noexcept;
        void swap(self_type&) noexcept;

        // Observers

        constexpr pointer get() const noexcept;

        constexpr explicit operator bool() const noexcept;

        constexpr std::add_lvalue_reference_t<T>
        operator*() const noexcept(noexcept(*std::declval<pointer>()));
        
        constexpr pointer operator->() const noexcept;

    private:

        constexpr internal_pointer& ptr_impl() noexcept;
        constexpr const internal_pointer& ptr_impl() const noexcept;

        internal_pointer m_data;
    };

    template <class T>
    void swap(cloning_ptr<T>& lhs, cloning_ptr<T>& rhs) noexcept;

    template <class T1, class T2>
    requires std::equality_comparable_with<
        typename cloning_ptr<T1>::pointer,
        typename cloning_ptr<T2>::pointer>
    constexpr
    bool operator==(const cloning_ptr<T1>& lhs, const cloning_ptr<T2>& rhs) noexcept;

    template <class T1, class T2>
    requires std::three_way_comparable_with<
        typename cloning_ptr<T1>::pointer,
        typename cloning_ptr<T2>::pointer>
    constexpr
    std::compare_three_way_result_t<
        typename cloning_ptr<T1>::pointer,
        typename cloning_ptr<T2>::pointer>
    operator<=>(const cloning_ptr<T1>& lhs, const cloning_ptr<T2>& rhs) noexcept;

    template <class T>
    constexpr
    bool operator==(const cloning_ptr<T>& lhs, std::nullptr_t) noexcept;

    template <class T>
    requires std::three_way_comparable<typename cloning_ptr<T>::pointer>
    constexpr
    std::compare_three_way_result_t<typename cloning_ptr<T>::pointer>
    operator<=>(const cloning_ptr<T>& lhs, std::nullptr_t) noexcept;

    /******************************
     * cloning_ptr implementation *
     ******************************/

    template <clonable T>
    constexpr cloning_ptr<T>::cloning_ptr(std::nullptr_t) noexcept
        : m_data(nullptr)
    {
    }

    template <clonable T>
    constexpr cloning_ptr<T>::cloning_ptr(pointer p) noexcept
        : m_data(p)
    {
    }

    template <clonable T>
    constexpr cloning_ptr<T>::cloning_ptr(const cloning_ptr& rhs) noexcept
        : m_data(rhs ? rhs->clone() : nullptr)
    {
    }

    template <clonable T>
    template <clonable U>
    requires std::convertible_to<U*, T*>
    constexpr cloning_ptr<T>::cloning_ptr(const cloning_ptr<U>& rhs) noexcept
        : m_data(rhs ? rhs->clone() : nullptr)
    {
    }
    
    template <clonable T>
    template <clonable U>
    requires std::convertible_to<U*, T*>
    constexpr cloning_ptr<T>::cloning_ptr(cloning_ptr<U>&& rhs) noexcept
        : m_data(rhs.release())
    {
    }

    template <clonable T>
    constexpr auto cloning_ptr<T>::operator=(const self_type& rhs) noexcept -> self_type&
    {
        reset(rhs ? rhs->clone() : nullptr);
        return *this;
    }

    template <clonable T>
    constexpr auto cloning_ptr<T>::operator=(std::nullptr_t) noexcept -> self_type&
    {
        reset(nullptr);
        return *this;
    }

    template <clonable T>
    template <clonable U>
    requires std::convertible_to<U*, T*>
    constexpr auto cloning_ptr<T>::operator=(const cloning_ptr<U>& rhs) noexcept -> self_type&
    {
        reset(rhs ? rhs->clone() : nullptr);
        return *this;
    }

    template <clonable T>
    template <clonable U>
    requires std::convertible_to<U*, T*>
    constexpr auto cloning_ptr<T>::operator=(cloning_ptr<U>&& rhs) noexcept -> self_type&
    {
        reset(rhs.release());
        return *this;
    }

    template <clonable T>
    constexpr auto cloning_ptr<T>::release() noexcept -> pointer
    {
        return ptr_impl().release();
    }

    template <clonable T>
    constexpr void cloning_ptr<T>::reset(pointer ptr) noexcept
    {
        ptr_impl().reset(ptr);
    }

    template <clonable T>
    void cloning_ptr<T>::swap(self_type& other) noexcept
    {
        using std::swap;
        swap(ptr_impl(), other.ptr_impl());
    }

    template <clonable T>
    constexpr auto cloning_ptr<T>::get() const noexcept -> pointer
    {
        return ptr_impl().get();
    }

    template <clonable T>
    constexpr cloning_ptr<T>::operator bool() const noexcept
    {
        return bool(ptr_impl());
    }

    template <clonable T>
    constexpr std::add_lvalue_reference_t<T>
    cloning_ptr<T>::operator*() const noexcept(noexcept(*std::declval<pointer>()))
    {
        return *get();
    }
    
    template <clonable T>
    constexpr auto cloning_ptr<T>::operator->() const noexcept -> pointer
    {
        return get();
    }
    
    template <clonable T>
    constexpr auto cloning_ptr<T>::ptr_impl() noexcept -> internal_pointer&
    {
        return m_data;
    }

    template <clonable T>
    constexpr auto cloning_ptr<T>::ptr_impl() const noexcept -> const internal_pointer&
    {
        return m_data;
    }

    /*********************************
     * Free functions implementation *
     *********************************/

    template <class T>
    void swap(cloning_ptr<T>& lhs, cloning_ptr<T>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template <class T1, class T2>
    requires std::equality_comparable_with<
        typename cloning_ptr<T1>::pointer,
        typename cloning_ptr<T2>::pointer>
    constexpr
    bool operator==(const cloning_ptr<T1>& lhs, const cloning_ptr<T2>& rhs) noexcept
    {
        return lhs.get() == rhs.get();
    }

    template <class T1, class T2>
    requires std::three_way_comparable_with<
        typename cloning_ptr<T1>::pointer,
        typename cloning_ptr<T2>::pointer>
    constexpr
    std::compare_three_way_result_t<
        typename cloning_ptr<T1>::pointer,
        typename cloning_ptr<T2>::pointer>
    operator<=>(const cloning_ptr<T1>& lhs, const cloning_ptr<T2>& rhs) noexcept
    {
        return lhs.get() <=> rhs.get();
    }

    template <class T>
    constexpr
    bool operator==(const cloning_ptr<T>& lhs, std::nullptr_t) noexcept
    {
        return !lhs;
    }

    template <class T>
    requires std::three_way_comparable<typename cloning_ptr<T>::pointer>
    constexpr
    std::compare_three_way_result_t<typename cloning_ptr<T>::pointer>
    operator<=>(const cloning_ptr<T>& lhs, std::nullptr_t) noexcept
    {
        using pointer = typename cloning_ptr<T>::pointer;
        return lhs.get() <=> static_cast<pointer>(nullptr);
    }
}
