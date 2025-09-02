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

#include <ranges>
#include <type_traits>

#include "sparrow/buffer/allocator.hpp"
#include "sparrow/buffer/buffer.hpp"
#include "sparrow/buffer/buffer_adaptor.hpp"
#include "sparrow/utils/ranges.hpp"

namespace sparrow
{

    namespace detail
    {
        /**
         * A holder class that wraps a value and provides storage management.
         *
         * @tparam T The type of the value to hold.
         */
        template <class T>
        class holder
        {
        public:

            using inner_type = T;

            /**
             * Constructs a holder with the given arguments forwarded to the wrapped value.
             *
             * @tparam Args The argument types.
             * @param args Arguments to forward to the value constructor.
             */
            template <class... Args>
            constexpr holder(Args&&... args) noexcept
                : value(std::forward<Args>(args)...)
            {
            }

            T value;

            /**
             * Extracts the storage by moving the wrapped value.
             *
             * @return The moved storage value.
             */
            [[nodiscard]] constexpr T extract_storage() && noexcept
            {
                return std::move(value);
            }

            /**
             * Gets a constant reference to the storage.
             *
             * @return Constant reference to the storage.
             */
            [[nodiscard]] constexpr const T& storage() const noexcept
            {
                return value;
            }

            /**
             * Gets a reference to the storage.
             *
             * @return Reference to the storage.
             */
            [[nodiscard]] constexpr T& storage() noexcept
            {
                return value;
            }

            /**
             * Assigns a new value to the storage.
             *
             * @param other The value to assign.
             */
            constexpr void assign(T&& other)
            {
                value = std::move(other);
            }
        };
    }

    /**
     * This buffer class is used as storage buffer for all sparrow arrays.
     * Its internal storage can be extracted.
     *
     * @tparam T The element type stored in the buffer.
     */
    template <class T>
    class u8_buffer : private detail::holder<buffer<std::uint8_t>>,
                      public buffer_adaptor<T, buffer<std::uint8_t>&>
    {
    public:

        using buffer_type = buffer<std::uint8_t>;
        using holder_type = detail::holder<buffer_type>;
        using buffer_adaptor_type = buffer_adaptor<T, buffer_type&>;
        using holder_type::extract_storage;
        using default_allocator = buffer_type::default_allocator;

        /**
         * Move constructor.
         *
         * @param other The buffer to move from.
         */
        constexpr u8_buffer(u8_buffer&& other) noexcept;

        /**
         * Copy constructor.
         *
         * @param other The buffer to copy from.
         */
        constexpr u8_buffer(const u8_buffer& other);

        /**
         * Move assignment operator (deleted).
         */
        u8_buffer& operator=(u8_buffer&& other) = delete;

        /**
         * Copy assignment operator (deleted).
         */
        u8_buffer& operator=(u8_buffer& other) = delete;

        /**
         * Destructor.
         */
        ~u8_buffer() = default;
        /**
         * Constructs a buffer with \c n uninitialized elements.
         *
         * @param n Number of elements.
         */
        constexpr explicit u8_buffer(std::size_t n);

        /**
         * Constructs a buffer with \c n elements, each initialized to \c val.
         *
         * @param n Number of elements.
         * @param val Value to initialize the elements with.
         */
        constexpr u8_buffer(std::size_t n, const T& val);

        /**
         * Constructs a buffer with the elements of the range \c range.
         * The range elements must be convertible to \c T.
         * This constructor performs a copy of the elements in the range into the buffer.
         *
         * @tparam R The range type.
         * @param range The range to copy elements from.
         */
        template <std::ranges::input_range R>
            requires(
                !std::same_as<u8_buffer<T>, std::decay_t<R>>
                && std::convertible_to<std::ranges::range_value_t<R>, T>
            )
        constexpr explicit u8_buffer(R&& range);

        /**
         * Constructs a buffer with the elements of the initializer list \c ilist.
         *
         * @param ilist The initializer list.
         */
        constexpr u8_buffer(std::initializer_list<T> ilist);

        /**
         * Constructs a buffer by taking ownership of the storage pointed to by \c data_ptr.
         * `data_ptr` must have been allocated with the same allocator used by u8_buffer.
         * Especially, one should not mixed operator new[] and std::allocator, as this
         * later is not guaranteed to free the memory with a call to operator delete[] only.
         *
         * The recommended way to allocate data_ptr is to use the default allocator
         * provided by u8_buffer:
         * \code{.cpp}
         * using allocator = u8_buffer::default_allocator;
         * auto* data_ptr = reinterpret_cast<T*>(default_allocator().allocate(sizeof(uint8_t) * count));
         * u8_buffer buf(data_ptr, count);
         * \endcode
         *
         * However, if you prefer to provide your own allocator, you must ensure that:
         * - either it allocates and returns a buffer or std::uint8_t
         * - or it accepts a std::uint8_t* as its deallocate argument
         * The first option is safer and more conformant to the standard allocator,
         * but requires and extract cast in the client code.
         *
         * @tparam A The allocator type.
         * @param data_ptr Pointer to the storage.
         * @param count Number of elements in the storage.
         * @param a The allocator to use.
         */
        template <allocator A = default_allocator>
        constexpr u8_buffer(T* data_ptr, std::size_t count, const A& a = A());
    };

    template <class T>
    constexpr u8_buffer<T>::u8_buffer(u8_buffer&& other) noexcept
        : holder_type(std::move(other).extract_storage())
        , buffer_adaptor_type(holder_type::value)
    {
    }

    template <class T>
    constexpr u8_buffer<T>::u8_buffer(const u8_buffer& other)
        : holder_type(other.storage())
        , buffer_adaptor_type(holder_type::value)
    {
    }

    template <class T>
    constexpr u8_buffer<T>::u8_buffer(std::size_t n)
        : holder_type{n * sizeof(T)}
        , buffer_adaptor_type(holder_type::value)
    {
    }

    template <class T>
    constexpr u8_buffer<T>::u8_buffer(std::size_t n, const T& val)
        : u8_buffer(n)
    {
        std::fill(this->begin(), this->end(), val);
    }

    template <class T>
    template <std::ranges::input_range R>
        requires(
            !std::same_as<u8_buffer<T>, std::decay_t<R>> && std::convertible_to<std::ranges::range_value_t<R>, T>
        )
    constexpr u8_buffer<T>::u8_buffer(R&& range)
        : u8_buffer(range_size(range))
    {
        sparrow::ranges::copy(range, this->begin());
    }

    template <class T>
    constexpr u8_buffer<T>::u8_buffer(std::initializer_list<T> ilist)
        : u8_buffer(ilist.size())
    {
        std::copy(ilist.begin(), ilist.end(), this->begin());
    }

    template <class T>
    template <allocator A>
    constexpr u8_buffer<T>::u8_buffer(T* data_ptr, std::size_t count, const A& a)
        : holder_type{reinterpret_cast<uint8_t*>(data_ptr), count * sizeof(T), a}
        , buffer_adaptor_type(holder_type::value)
    {
    }
}
