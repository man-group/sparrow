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

#include "sparrow/buffer/buffer_view.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset_base.hpp"

namespace sparrow
{
    /**
     * @class dynamic_bitset_view
     *
     * A non-owning view to a dynamic size sequence of bits stored in external memory.
     *
     * This class provides a lightweight, non-owning interface to manipulate sequences of
     * boolean values stored in external memory buffers. Unlike dynamic_bitset, this class
     * does not manage memory allocation or deallocation - it only provides a view into
     * existing bit data.
     *
     * The view is designed for scenarios where you need to work with bit sequences that
     * are managed elsewhere (e.g., memory-mapped files, shared memory, or buffers owned
     * by other objects) while still benefiting from the rich bit manipulation API.
     *
     * @tparam T The integer type used to store the bits. Must satisfy std::integral.
     *          Common choices are std::uint8_t, std::uint32_t, or std::uint64_t.
     *          The choice affects memory layout and performance characteristics.
     *
     * @note This class inherits from dynamic_bitset_base which provides the core bit
     *       manipulation functionality. The view does not support operations that would
     *       change the size of the underlying storage (no resize, push_back, etc.).
     *
     * @warning The user is responsible for ensuring that the viewed memory remains valid
     *          for the lifetime of the view object. Accessing a view after the underlying
     *          memory has been deallocated results in undefined behavior.
     *
     * Example usage:
     * @code
     * // External buffer containing bit data
     * std::vector<std::uint8_t> external_buffer = {0b10101010, 0b11110000};
     *
     * // Create a view of the first 12 bits
     * dynamic_bitset_view<std::uint8_t> view(external_buffer.data(), 12);
     *
     * // Read and modify bits through the view
     * bool bit_value = view.test(5);
     * view.set(7, true);
     *
     * // The changes are reflected in the original buffer
     * assert(external_buffer[0] == view.block(0));
     * @endcode
     */
    template <std::integral T>
    class dynamic_bitset_view : public dynamic_bitset_base<buffer_view<T>>
    {
    public:

        using base_type = dynamic_bitset_base<buffer_view<T>>;  ///< Base class type providing bit operations
        using storage_type = typename base_type::storage_type;  ///< Underlying buffer view type (non-owning)
        using block_type = typename base_type::block_type;      ///< Type of each storage block (same as T)
        using size_type = typename base_type::size_type;        ///< Type used for sizes and indices

        /**
         * @brief Constructs a bitset view from external memory.
         *
         * Creates a non-owning view over the provided memory buffer. The view will
         * interpret the memory as a sequence of bits using the specified block type.
         * All bits are initially assumed to be valid (non-null).
         *
         * @param p Pointer to the external memory buffer containing bit data
         * @param n The number of bits represented in the buffer
         *
         * @pre p must point to a valid memory buffer of at least
         *      `compute_block_count(n) * sizeof(block_type)` bytes, or be nullptr if n is 0
         * @pre n must accurately represent the number of bits available in the buffer
         * @pre The memory pointed to by p must remain valid for the lifetime of this view
         *
         * @post size() == n
         * @post null_count() == 0 (all bits assumed valid initially)
         * @post The view provides access to n bits starting from memory location p
         *
         * @note If p is nullptr, n should be 0, and the view will be empty
         * @note No memory allocation occurs - this is a pure view operation
         *
         * Example:
         * @code
         * std::array<std::uint32_t, 2> buffer = {0xFFFFFFFF, 0x0000FFFF};
         * dynamic_bitset_view<std::uint32_t> view(buffer.data(), 48);
         * assert(view.size() == 48);
         * assert(view.test(0) == true);  // First bit is set
         * @endcode
         */
        constexpr dynamic_bitset_view(block_type* p, size_type n, size_type offset);

        /**
         * @brief Constructs a bitset view from external memory with null count tracking.
         *
         * Creates a non-owning view over the provided memory buffer with explicit tracking
         * of how many bits are null (invalid/unset). This constructor is useful when you
         * already know the null count and want to avoid recomputing it.
         *
         * @param p Pointer to the external memory buffer containing bit data
         * @param n The number of bits represented in the buffer
         * @param null_count The number of bits that are set to false/null in the buffer
         *
         * @pre p must point to a valid memory buffer of at least
         *      `compute_block_count(n) * sizeof(block_type)` bytes, or be nullptr if n is 0
         * @pre n must accurately represent the number of bits available in the buffer
         * @pre null_count <= n (cannot have more null bits than total bits)
         * @pre null_count must accurately reflect the actual number of unset bits in the buffer
         * @pre The memory pointed to by p must remain valid for the lifetime of this view
         *
         * @post size() == n
         * @post null_count() == null_count
         * @post count_non_null() == n - null_count
         * @post The view provides access to n bits starting from memory location p
         *
         * @note This constructor is more efficient when the null count is already known,
         *       as it avoids scanning the buffer to count null bits
         * @note An internal assertion verifies that the provided null_count matches
         *       the actual count of null bits in debug builds
         *
         * Example:
         * @code
         * std::array<std::uint8_t, 2> buffer = {0b10101010, 0b11110000};
         * // We know there are 6 unset bits in the pattern above
         * dynamic_bitset_view<std::uint8_t> view(buffer.data(), 16, 6);
         * assert(view.size() == 16);
         * assert(view.null_count() == 6);
         * assert(view.count_non_null() == 10);
         * @endcode
         */
        constexpr dynamic_bitset_view(block_type* p, size_type n, size_type null_count, size_type offset);

        constexpr ~dynamic_bitset_view() = default;

        constexpr dynamic_bitset_view(const dynamic_bitset_view&) = default;
        constexpr dynamic_bitset_view(dynamic_bitset_view&&) noexcept = default;

        constexpr dynamic_bitset_view& operator=(const dynamic_bitset_view&) = default;
        constexpr dynamic_bitset_view& operator=(dynamic_bitset_view&&) noexcept = default;
    };

    template <std::integral T>
    constexpr dynamic_bitset_view<T>::dynamic_bitset_view(block_type* p, size_type n, size_type offset)
        : base_type(storage_type(p, p != nullptr ? this->compute_block_count(n) : 0), n, offset)
    {
    }

    template <std::integral T>
    constexpr dynamic_bitset_view<T>::dynamic_bitset_view(
        block_type* p,
        size_type n,
        size_type null_count,
        size_type offset
    )
        : base_type(storage_type(p, p != nullptr ? this->compute_block_count(n) : 0), n, null_count, offset)
    {
    }
}
