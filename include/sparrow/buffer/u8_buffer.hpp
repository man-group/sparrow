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

        template <class T>
        class holder
        {
        public:

            using inner_type = T;

            template <class... Args>
            holder(Args&&... args)
                : value(std::forward<Args>(args)...)
            {
            }

            T value;

            [[nodiscard]] T extract_storage() &&
            {
                return std::move(value);
            }

            [[nodiscard]] const T& storage() const
            {
                return value;
            }

            [[nodiscard]] T& storage()
            {
                return value;
            }

            void assign(T&& other)
            {
                value = std::move(other);
            }
        };
    }

    /**
     * This buffer class is use as storage buffer for all sparrow arrays.
     * Its internal storage can be extracted.
     */
    template <class T>
    class u8_buffer : private detail::holder<buffer<std::uint8_t>>,
                      public buffer_adaptor<T, buffer<std::uint8_t>&>
    {
    public:

        using holder_type = detail::holder<buffer<std::uint8_t>>;
        using buffer_adaptor_type = buffer_adaptor<T, buffer<std::uint8_t>&>;
        using holder_type::extract_storage;
        using buffer_adaptor_type::operator[];

        u8_buffer(u8_buffer&& other);
        u8_buffer(const u8_buffer& other);
        u8_buffer& operator=(u8_buffer&& other) = delete;
        u8_buffer& operator=(u8_buffer& other) = delete;
        ~u8_buffer() = default;

        /**
         * Constructs a buffer with \c n elements, each initialized to \c val.
         * @param n Number of elements.
         * @param val Value to initialize the elements with.
         */
        u8_buffer(std::size_t n, const T& val = T{});

        /**
         * Constructs a buffer with the elements of the range \c range.
         * The range elements  must be convertible to \c T.
         * @param range The range.
         */
        template <std::ranges::input_range R>
            requires(!std::same_as<u8_buffer<T>, std::decay_t<R>> && std::convertible_to<std::ranges::range_value_t<R>, T>)
        u8_buffer(R&& range);

        /**
         * Constructs a buffer with the elements of the initializer list \c ilist.
         * @param ilist The initializer list.
         */
        u8_buffer(std::initializer_list<T> ilist);

        /**
         * Constructs a buffer by taking ownership of the storage pointed to by \c data_ptr.
         * @param data_ptr Pointer to the storage.
         * @param count Number of elements in the storage.
         */
        template <allocator A = any_allocator<T>>
        u8_buffer(T* data_ptr, std::size_t count, const A& a = A());
    };

    template <class T>
    u8_buffer<T>::u8_buffer(u8_buffer&& other)
        : holder_type(std::move(other).extract_storage())
        , buffer_adaptor_type(holder_type::value)
    {
    }

    template <class T>
    u8_buffer<T>::u8_buffer(const u8_buffer& other)
        : holder_type(other.storage())
        , buffer_adaptor_type(holder_type::value)
    {
    }

    template <class T>
    u8_buffer<T>::u8_buffer(std::size_t n, const T& val)
        : holder_type{n * sizeof(T)}
        , buffer_adaptor_type(holder_type::value)
    {
        std::fill(this->begin(), this->end(), val);
    }

    template <class T>
    template <std::ranges::input_range R>
        requires(!std::same_as<u8_buffer<T>, std::decay_t<R>>
                 && std::convertible_to<std::ranges::range_value_t<R>, T>)
    u8_buffer<T>::u8_buffer(R&& range)
        : holder_type{range_size(range) * sizeof(T)}
        , buffer_adaptor_type(holder_type::value)
    {
        std::ranges::copy(range, this->begin());
    }

    template <class T>
    u8_buffer<T>::u8_buffer(std::initializer_list<T> ilist)
        : holder_type{ilist.size() * sizeof(T)}
        , buffer_adaptor_type(holder_type::value)
    {
        std::copy(ilist.begin(), ilist.end(), this->begin());
    }

    template <class T>
    template <allocator A>
    u8_buffer<T>::u8_buffer(T* data_ptr, std::size_t count, const A& a)
        : holder_type{reinterpret_cast<uint8_t*>(data_ptr), count * sizeof(T), a}
        , buffer_adaptor_type(holder_type::value)
    {
    }
}
