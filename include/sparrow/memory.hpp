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

#include <cstddef>
#include <memory>

#include "sparrow/contracts.hpp"
#include "sparrow/mp_utils.hpp"

namespace sparrow
{

    /**
     * @brief A value_ptr is a smart pointer that behaves like a value.
     * It manages the lifetime of an object of type T which is not stored in the `value_ptr` but a pointer,
     * similar to `unique_ptr`. When copied, it copies the managed object.
     *
     * @tparam T The type of the object managed by the `value_ptr`.
     * @tparam D The deleter type used by the `unique_ptr` that manages the object.
     * @todo Make it constexpr.
     */
    template <class T, class D = std::default_delete<T>>
    class value_ptr
    {
    public:

        using element_type = T;

        constexpr value_ptr() noexcept = default;

        constexpr value_ptr(std::nullptr_t) noexcept
        {
        }

        explicit value_ptr(T value)
            : m_value(std::unique_ptr<T, D>(new T(std::move(value)), D()))
        {
        }

        explicit value_ptr(T* value)
            : m_value(value != nullptr ? std::unique_ptr<T, D>(*value) : std::unique_ptr<T, D>())
        {
        }

        explicit value_ptr(std::unique_ptr<T, D> unique_ptr)
            : m_value(std::move(unique_ptr))
        {
        }

        value_ptr(const value_ptr& other)
            : m_value(
                  other.m_value ? std::unique_ptr<T, D>(new T(*other.m_value), other.m_value.get_deleter())
                                : std::unique_ptr<T, D>()
              )
        {
        }

        value_ptr(value_ptr&& other) noexcept = default;

        ~value_ptr() = default;

        value_ptr& operator=(const value_ptr& other)
        {
            if (other.has_value())
            {
                if (m_value)
                {
                    *m_value = *other.m_value;
                }
                else
                {
                    m_value = std::unique_ptr<T, D>(new T(*other.m_value), other.m_value.get_deleter());
                }
            }
            else
            {
                m_value.reset();
            }
            return *this;
        }

        value_ptr& operator=(value_ptr&& other) noexcept = default;

        value_ptr& operator=(std::nullptr_t) noexcept
        {
            reset();
            return *this;
        }

        T& operator*()
        {
            SPARROW_ASSERT_TRUE(m_value);
            return *m_value;
        }

        const T& operator*() const
        {
            SPARROW_ASSERT_TRUE(m_value);
            return *m_value;
        }

        T* operator->()
        {
            SPARROW_ASSERT_TRUE(m_value);
            return &*m_value;
        }

        T* get()
        {
            return m_value.get();
        }

        const T* operator->() const
        {
            SPARROW_ASSERT_TRUE(m_value);
            return &*m_value;
        }

        explicit operator bool() const noexcept
        {
            return has_value();
        }

        [[nodiscard]] bool has_value() const noexcept
        {
            return bool(m_value);
        }

        void reset() noexcept
        {
            m_value.reset();
        }

    private:

        std::unique_ptr<T, D> m_value;
    };

    /// This type is `value_ptr` if `T` is a `unique_ptr` instance, `T` otherwise.
    template <class T>
    using replace_unique_ptr_by_value_ptr = std::conditional_t<
        mpl::unique_ptr_or_derived<T>,
        sparrow::value_ptr<mpl::get_element_type_t<T>, mpl::get_deleter_type_t<T>>,
        T>;

    /// Given a typelist, it replaces all unique_ptrs with value_ptrs.
    template <class Typelist>
    using replace_unique_ptrs_by_value_ptrs_t = mpl::transform<replace_unique_ptr_by_value_ptr, Typelist>;
}
