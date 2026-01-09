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

#include <type_traits>

#include "sparrow/buffer/buffer.hpp"
#include "sparrow/buffer/bit_vector/bit_vector_base.hpp"
#include "sparrow/utils/ranges.hpp"

namespace sparrow
{
    /**
     * @class bit_vector
     *
     * A dynamic size bit-packed boolean container.
     *
     * This class provides efficient storage for sequences of boolean values using bit-packing.
     * Unlike validity_bitmap, this class has NO null-counting semantics - it's a pure
     * data structure for storing packed bits.
     *
     * Use this for storing actual boolean data values. For validity/null tracking, use
     * validity_bitmap instead.
     *
     * @tparam T The integer type used to store the bits. Must satisfy std::integral.
     *          Common choices are std::uint8_t, std::uint32_t, or std::uint64_t.
     */
    template <std::integral T>
    class bit_vector : public bit_vector_base<buffer<T>>
    {
    public:

        using base_type = bit_vector_base<buffer<T>>;
        using storage_type = typename base_type::storage_type;
        using default_allocator = typename storage_type::default_allocator;
        using block_type = typename base_type::block_type;
        using value_type = typename base_type::value_type;
        using size_type = typename base_type::size_type;

        template <std::ranges::input_range R, allocator A>
            requires std::convertible_to<std::ranges::range_value_t<R>, value_type>
        constexpr bit_vector(R&& r, const A& a = A{})
            : base_type(
                  storage_type(
                      static_cast<size_type>(this->compute_block_count(std::ranges::size(r))),
                      block_type(0),
                      a
                  ),
                  static_cast<size_type>(std::ranges::size(r))
              )
        {
            size_type i = 0;
            for (auto&& val : r)
            {
                this->set(i++, static_cast<value_type>(val));
            }
        }

        template <typename A>
            requires(not std::same_as<A, bit_vector<T>> and allocator<A>)
        constexpr explicit bit_vector(const A& a = A{})
            : base_type(storage_type(a), 0u)
        {
        }

        template <allocator A>
        constexpr bit_vector(size_type n, const A& a = A{})
            : bit_vector(n, false, a)
        {
        }

        template <allocator A>
        constexpr bit_vector(size_type n, value_type v, const A& a = A{});

        template <allocator A>
        constexpr bit_vector(block_type* p, size_type n, const A& a = A{});

        constexpr ~bit_vector() = default;
        constexpr bit_vector(const bit_vector&) = default;
        constexpr bit_vector(bit_vector&&) noexcept = default;

        template <allocator A>
        constexpr bit_vector(const bit_vector& rhs, const A& a);

        template <allocator A>
        constexpr bit_vector(bit_vector&& rhs, const A& a);

        constexpr bit_vector& operator=(const bit_vector&) = default;
        constexpr bit_vector& operator=(bit_vector&&) noexcept = default;

        using base_type::clear;
        using base_type::emplace;
        using base_type::erase;
        using base_type::insert;
        using base_type::pop_back;
        using base_type::push_back;
        using base_type::resize;
    };

    template <std::integral T>
    template <allocator A>
    constexpr bit_vector<T>::bit_vector(size_type n, value_type v, const A& a)
        : base_type(storage_type(compute_block_count(n), block_type(0), a), n)
    {
        if (v)
        {
            std::fill(this->buffer().begin(), this->buffer().end(), block_type(~block_type(0)));
            this->zero_unused_bits();
        }
    }

    template <std::integral T>
    template <allocator A>
    constexpr bit_vector<T>::bit_vector(block_type* p, size_type n, const A& a)
        : base_type(storage_type(p, p != nullptr ? this->compute_block_count(n) : 0, a), n)
    {
    }

    template <std::integral T>
    template <allocator A>
    constexpr bit_vector<T>::bit_vector(const bit_vector& rhs, const A& a)
        : base_type(storage_type(rhs.buffer(), a), rhs.size())
    {
    }

    template <std::integral T>
    template <allocator A>
    constexpr bit_vector<T>::bit_vector(bit_vector&& rhs, const A& a)
        : base_type(storage_type(std::move(rhs.buffer()), a), rhs.size())
    {
    }
}
