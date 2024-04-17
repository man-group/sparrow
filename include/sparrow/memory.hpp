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

#include <memory>

#include "sparrow/contracts.hpp"

namespace sparrow
{

    /**
     * @brief A value_ptr is a smart pointer that behaves like a value.
     * It manages the lifetime of an object of type T which is not stored in the `value_ptr` but a pointer,
     * similar to `unique_ptr`. When copied, it copies the managed object.
     *
     * @tparam T The type of the object managed by the `value_ptr`.
     * @todo Make it constexpr.
     */
    template <class T>
    class value_ptr
    {
    public:

        value_ptr() = default;

        explicit value_ptr(T value)
            : value_(std::make_unique<T>(std::move(value)))
        {
        }

        explicit value_ptr(T* value)
            : value_(value != nullptr ? std::make_unique<T>(*value) : std::unique_ptr<T>())
        {
        }

        value_ptr(const value_ptr& other)
            : value_(other.value_ ? std::make_unique<T>(*other.value_) : std::unique_ptr<T>())
        {
        }

        value_ptr(value_ptr&& other) noexcept = default;

        ~value_ptr() = default;

        value_ptr& operator=(const value_ptr& other)
        {
            if (other.has_value())
            {
                if (value_)
                {
                    *value_ = *other.value_;
                }
                else
                {
                    value_ = std::make_unique<T>(*other.value_);
                }
            }
            else
            {
                value_.reset();
            }
            return *this;
        }

        value_ptr& operator=(value_ptr&& other) noexcept = default;

        T& operator*()
        {
            SPARROW_ASSERT_TRUE(value_);
            return *value_;
        }

        const T& operator*() const
        {
            SPARROW_ASSERT_TRUE(value_);
            return *value_;
        }

        T* operator->()
        {
            SPARROW_ASSERT_TRUE(value_);
            return &*value_;
        }

        const T* operator->() const
        {
            SPARROW_ASSERT_TRUE(value_);
            return &*value_;
        }

        explicit operator bool() const noexcept
        {
            return has_value();
        }

        bool has_value() const noexcept
        {
            return bool(value_);
        }

        void reset() noexcept
        {
            value_.reset();
        }

    private:

        std::unique_ptr<T> value_;
    };

}
