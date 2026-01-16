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

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "sparrow/config/config.hpp"

namespace sparrow
{
    /**
     * @brief Counts the number of bits set to true in a buffer.
     * @param data Pointer to the byte data (may be nullptr)
     * @param bit_size The total number of bits to count
     * @param byte_size The number of bytes in the buffer
     * @param offset The bit offset from which to start counting (default: 0)
     * @return The number of bits set to true
     */
    [[nodiscard]] SPARROW_API std::size_t
    count_non_null(const std::uint8_t* data, std::size_t bit_size, std::size_t byte_size, std::size_t offset = 0) noexcept;

    /**
     * @class tracking_null_count
     *
     * Policy class that enables null count tracking in dynamic_bitset_base.
     *
     * When this policy is used, the bitset maintains an internal counter of
     * null (unset) bits, which is updated on every bit modification. This
     * enables O(1) null_count() queries but adds slight overhead to
     * modification operations.
     *
     * Use this policy when you frequently need to query the number of null bits.
     *
     * @tparam SizeType The size type used for counting (typically std::size_t)
     */
    template <typename SizeType = std::size_t>
    class tracking_null_count
    {
    public:

        static constexpr bool track_null_count = true;
        using size_type = SizeType;

        constexpr tracking_null_count() noexcept = default;

        constexpr explicit tracking_null_count(size_type count) noexcept
            : m_null_count(count)
        {
        }

        /**
         * @brief Initializes the null count by counting bits in the buffer.
         * @tparam BlockType The integral type used for storage blocks
         * @param data Pointer to the block data
         * @param bit_size The total number of bits
         * @param block_count The number of blocks in the buffer
         * @param offset The bit offset from which to start counting (default: 0)
         */
        template <std::integral BlockType>
        void initialize_null_count(const BlockType* data, size_type bit_size, size_type block_count, size_type offset = 0) noexcept
        {
            recompute_null_count(data, bit_size, block_count, offset);
        }

        [[nodiscard]] constexpr size_type null_count() const noexcept
        {
            return m_null_count;
        }

        constexpr void set_null_count(size_type count) noexcept
        {
            m_null_count = count;
        }

        /**
         * @brief Recomputes the null count from the buffer.
         * @tparam BlockType The integral type used for storage blocks
         * @param data Pointer to the block data
         * @param bit_size The total number of bits to count (logical size, not including offset)
         * @param block_count The number of blocks in the buffer
         * @param offset The bit offset from which to start counting (default: 0)
         */
        template <std::integral BlockType>
        void recompute_null_count(const BlockType* data, size_type bit_size, size_type block_count, size_type offset = 0) noexcept
        {
            const auto* byte_data = reinterpret_cast<const std::uint8_t*>(data);
            const std::size_t byte_size = block_count * sizeof(BlockType);
            m_null_count = static_cast<size_type>(bit_size)
                           - static_cast<size_type>(
                               count_non_null(byte_data, static_cast<std::size_t>(bit_size), byte_size, static_cast<std::size_t>(offset))
                           );
        }

        constexpr void update_null_count(bool old_value, bool new_value) noexcept
        {
            if (new_value && !old_value)
            {
                --m_null_count;
            }
            else if (!new_value && old_value)
            {
                ++m_null_count;
            }
        }

        constexpr void swap_null_count(tracking_null_count& other) noexcept
        {
            std::swap(m_null_count, other.m_null_count);
        }

        constexpr void clear_null_count() noexcept
        {
            m_null_count = 0;
        }

    private:

        size_type m_null_count = 0;
    };

    /**
     * @class non_tracking_null_count
     *
     * Policy class that disables null count tracking in dynamic_bitset_base.
     *
     * When this policy is used, the bitset does not maintain a null count.
     * The null_count() method becomes unavailable (via requires clause).
     * This provides zero-overhead bit operations when null count queries
     * are not needed.
     *
     * Use this policy for pure bitset operations where null count is irrelevant.
     *
     * @tparam SizeType The size type (unused, but kept for interface consistency)
     */
    template <typename SizeType = std::size_t>
    class non_tracking_null_count
    {
    public:

        static constexpr bool track_null_count = false;
        using size_type = SizeType;

        constexpr non_tracking_null_count() noexcept = default;

        // Accepts and ignores a count for interface compatibility
        constexpr explicit non_tracking_null_count(size_type /*count*/) noexcept
        {
        }

        // null_count() is intentionally not provided

        // No-op: non-tracking policy doesn't need to count bits
        template <std::integral BlockType>
        constexpr void
        initialize_null_count(const BlockType* /*data*/, size_type /*bit_size*/, size_type /*block_count*/, size_type /*offset*/ = 0) noexcept
        {
        }

        constexpr void set_null_count(size_type /*count*/) noexcept
        {
            // No-op
        }

        // No-op: non-tracking policy doesn't need to recompute
        template <std::integral BlockType>
        constexpr void
        recompute_null_count(const BlockType* /*data*/, size_type /*bit_size*/, size_type /*block_count*/, size_type /*offset*/ = 0) noexcept
        {
        }

        constexpr void update_null_count(bool /*old_value*/, bool /*new_value*/) noexcept
        {
            // No-op
        }

        constexpr void swap_null_count(non_tracking_null_count& /*other*/) noexcept
        {
            // No-op
        }

        constexpr void clear_null_count() noexcept
        {
            // No-op
        }
    };

    /**
     * @concept null_count_policy
     *
     * Concept that checks if a type is a valid null count policy.
     * A valid policy must:
     * - Have a static constexpr bool member 'track_null_count'
     * - Provide update_null_count(bool, bool), swap(), clear(), set_null_count(),
     *   initialize(), and recompute() methods
     */
    template <typename P>
    concept null_count_policy = requires(P p, P other, bool b, typename P::size_type s, const std::uint8_t* data) {
        { P::track_null_count } -> std::convertible_to<bool>;
        { p.update_null_count(b, b) } -> std::same_as<void>;
        { p.swap_null_count(other) } -> std::same_as<void>;
        { p.clear_null_count() } -> std::same_as<void>;
        { p.set_null_count(s) } -> std::same_as<void>;
        { p.initialize_null_count(data, s, s) } -> std::same_as<void>;
        { p.recompute_null_count(data, s, s) } -> std::same_as<void>;
    };

}  // namespace sparrow
