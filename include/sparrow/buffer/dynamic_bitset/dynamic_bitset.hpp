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
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset_base.hpp"
#include "sparrow/utils/ranges.hpp"

namespace sparrow
{
    /**
     * @class dynamic_bitset
     *
     * A dynamic size sequence of bits with efficient storage and manipulation operations.
     *
     * This class provides a container for storing and manipulating sequences of boolean values
     * using an underlying integer type for efficient bit-level operations. The bitset can grow
     * and shrink dynamically, similar to std::vector, while providing specialized bit manipulation
     * methods.
     *
     * @tparam T The integer type used to store the bits. Must satisfy std::integral.
     *          Common choices are std::uint8_t, std::uint32_t, or std::uint64_t.
     *
     * @note The class inherits from dynamic_bitset_base which provides the core bit manipulation
     *       functionality. This class adds constructors and manages the underlying buffer storage.
     *
     * Example usage:
     * @code
     * // Create a bitset with 10 bits, all set to false
     * dynamic_bitset<std::uint8_t> bits(10);
     *
     * // Create from a range of boolean values
     * std::vector<bool> values = {true, false, true, false};
     * dynamic_bitset<std::uint8_t> bits_from_range(values);
     *
     * // Set and test individual bits
     * bits.set(5, true);
     * bool bit_value = bits.test(5);
     * @endcode
     */
    template <std::integral T>
    class dynamic_bitset : public dynamic_bitset_base<buffer<T>>
    {
    public:

        using base_type = dynamic_bitset_base<buffer<T>>;       ///< Base class type
        using storage_type = typename base_type::storage_type;  ///< Underlying storage container type
        using default_allocator = typename storage_type::default_allocator;  ///< Type of default allocator
        using block_type = typename base_type::block_type;  ///< Type of each storage block (same as T)
        using value_type = typename base_type::value_type;  ///< Type of individual bit values (bool)
        using size_type = typename base_type::size_type;    ///< Type used for sizes and indices

        /**
         * @brief Constructs a dynamic_bitset from an input range of convertible values.
         *
         * Creates a bitset with the same size as the input range, where each bit is set
         * according to the truthiness of the corresponding range element. Non-zero/true
         * values result in set bits (1), while zero/false values result in unset bits (0).
         *
         * @tparam R Input range type that must satisfy std::ranges::input_range
         * @tparam A Allocator type that must satisfy sparrow::allocator
         * @param r The input range whose elements will be converted to bits
         * @param a The allocator used internally
         *
         * @pre The range elements must be convertible to value_type (bool)
         * @pre The range must have a computable size via std::ranges::size
         *
         * Example:
         * @code
         * std::vector<int> values = {1, 0, 3, 0, 5};  // non-zero = true, zero = false
         * dynamic_bitset<std::uint8_t> bits(values);  // Results in: 10101
         * @endcode
         */
        template <std::ranges::input_range R, allocator A>
            requires std::convertible_to<std::ranges::range_value_t<R>, value_type>
        constexpr explicit dynamic_bitset(const R& r, const A& a)
            : dynamic_bitset(std::ranges::size(r), true, a)
        {
            std::size_t i = 0;
            for (auto value : r)
            {
                if (!value)
                {
                    this->set(i, false);
                }
                i++;
            }
        }

        /**
         * @brief Default constructor. Creates an empty bitset.
         *
         * Constructs a bitset with zero bits. The bitset can later be resized
         * or bits can be added using the provided methods.
         */
        template <typename A>
            requires(not std::same_as<A, dynamic_bitset<T>> and allocator<A>)
        constexpr dynamic_bitset(const A& a);

        /**
         * @brief Constructs a bitset with n bits, all initialized to false.
         *
         * @param n The number of bits in the bitset
         *
         * @post size() == n
         * @post All bits are set to false
         */
        template <allocator A>
        constexpr explicit dynamic_bitset(size_type n, const A& a);

        /**
         * @brief Constructs a bitset with n bits, all initialized to the specified value.
         *
         * @param n The number of bits in the bitset
         * @param v The value to initialize all bits to (true or false)
         *
         * @post size() == n
         * @post All bits are set to v
         */
        template <allocator A>
        constexpr dynamic_bitset(size_type n, value_type v, const A& a);

        /**
         * @brief Constructs a bitset using existing memory.
         *
         * Creates a bitset that uses the provided memory buffer for storage.
         * The bitset takes ownership of the memory.
         *
         * @param p Pointer to the memory buffer containing bit data
         * @param n The number of bits represented in the buffer
         *
         * @pre If p is not nullptr, it must point to valid memory of sufficient size
         * @post size() == n
         * @post offset() == 0
         *
         * @warning The caller must ensure the memory pointed to by p remains valid
         *          and contains properly formatted bit data.
         */
        template <allocator A>
        constexpr dynamic_bitset(block_type* p, size_type n, const A& a);

        /**
         * @brief Constructs a bitset using existing memory with null count tracking.
         *
         * Creates a bitset that uses the provided memory buffer and tracks the number
         * of null (unset) bits for optimization purposes.
         *
         * @param p Pointer to the memory buffer containing bit data
         * @param n The number of bits represented in the buffer
         * @param offset The offset in bits from the start of the buffer
         *
         * @pre If p is not nullptr, it must point to valid memory of sufficient size
         * @pre null_count must accurately reflect the number of unset bits
         * @post size() == n
         * @post offset() == offset
         */
        template <allocator A>
        constexpr dynamic_bitset(block_type* p, size_type n, size_type offset, const A& a);

        /**
         * @brief Constructs a bitset using existing memory with offset and null count tracking.
         *
         * Creates a bitset that uses the provided memory buffer starting at the specified
         * bit offset and tracks the number of null (unset) bits for optimization purposes.
         *
         * @param p Pointer to the memory buffer containing bit data
         * @param n The number of bits represented in the buffer
         * @param offset The offset in bits from the start of the buffer
         * @param null_count The number of bits that are set to false/null
         *
         * @pre If p is not nullptr, it must point to valid memory of sufficient size
         * @pre null_count must accurately reflect the number of unset bits
         * @post size() == n
         * @post offset() == offset
         * @post null_count() == null_count
         */
        template <allocator A>
        constexpr dynamic_bitset(block_type* p, size_type n, size_type offset, size_type null_count, const A& a);

        constexpr ~dynamic_bitset() = default;
        constexpr dynamic_bitset(const dynamic_bitset&) = default;
        constexpr dynamic_bitset(dynamic_bitset&&) noexcept = default;

        template <allocator A>
        constexpr dynamic_bitset(const dynamic_bitset& rhs, const A& a);

        template <allocator A>
        constexpr dynamic_bitset(dynamic_bitset&& rhs, const A& a);

        constexpr dynamic_bitset& operator=(const dynamic_bitset&) = default;
        constexpr dynamic_bitset& operator=(dynamic_bitset&&) noexcept = default;

        /**
         * @brief Creates a copy of a subset of bits.
         * @param start The starting position of the slice
         * @param length The number of bits in the slice
         * @return A new bitset containing a copy of the specified range
         * @pre start + length <= size()
         * @post The returned bitset has size() == length
         * @post The returned bitset contains copies of bits [start, start+length)
         * @throws std::out_of_range if start + length > size()
         * @note The returned bitset owns its own storage (copy operation)
         */
        [[nodiscard]] constexpr dynamic_bitset slice(size_type start, size_type length) const;

        /**
         * @brief Creates a copy of a subset of bits from start to end.
         * @param start The starting position of the slice
         * @return A new bitset containing a copy from start to the end
         * @pre start <= size()
         * @post The returned bitset has size() == size() - start
         * @post The returned bitset contains copies of bits [start, size())
         * @throws std::out_of_range if start > size()
         * @note The returned bitset owns its own storage (copy operation)
         */
        [[nodiscard]] constexpr dynamic_bitset slice(size_type start) const;

        // Inherit container-like operations from base class
        using base_type::clear;      ///< Remove all bits from the bitset
        using base_type::emplace;    ///< Emplace a bit at a specific position
        using base_type::erase;      ///< Remove bits from the bitset
        using base_type::insert;     ///< Insert bits into the bitset
        using base_type::pop_back;   ///< Remove the last bit
        using base_type::push_back;  ///< Add a bit to the end
        using base_type::resize;     ///< Change the size of the bitset
    };

    template <std::integral T>
    template <class A>
        requires(not std::same_as<A, dynamic_bitset<T>> and allocator<A>)
    constexpr dynamic_bitset<T>::dynamic_bitset(const A& a)
        : base_type(storage_type(a), 0u)
    {
        base_type::zero_unused_bits();
    }

    template <std::integral T>
    template <allocator A>
    constexpr dynamic_bitset<T>::dynamic_bitset(size_type n, const A& a)
        : dynamic_bitset(n, false, a)
    {
        base_type::zero_unused_bits();
    }

    template <std::integral T>
    template <allocator A>
    constexpr dynamic_bitset<T>::dynamic_bitset(size_type n, value_type value, const A& a)
        : base_type(
              storage_type(this->compute_block_count(n), value ? block_type(~block_type(0)) : block_type(0), a),
              n,
              0,
              value ? 0u : n
          )
    {
        base_type::zero_unused_bits();
    }

    template <std::integral T>
    template <allocator A>
    constexpr dynamic_bitset<T>::dynamic_bitset(block_type* p, size_type n, const A& a)
        : base_type(storage_type(p, p != nullptr ? this->compute_block_count(n) : 0, a), n)
    {
        base_type::zero_unused_bits();
    }

    template <std::integral T>
    template <allocator A>
    constexpr dynamic_bitset<T>::dynamic_bitset(block_type* p, size_type n, size_type offset, const A& a)
        : base_type(storage_type(p, this->compute_block_count(n + offset), a), n, offset)
    {
        base_type::zero_unused_bits();
    }

    template <std::integral T>
    template <allocator A>
    constexpr dynamic_bitset<
        T>::dynamic_bitset(block_type* p, size_type n, size_type offset, size_type null_count, const A& a)
        : base_type(storage_type(p, this->compute_block_count(n + offset), a), n, offset, null_count)
    {
        base_type::zero_unused_bits();
    }

    template <std::integral T>
    template <allocator A>
    constexpr dynamic_bitset<T>::dynamic_bitset(const dynamic_bitset& rhs, const A& a)
        : base_type(storage_type(rhs, a))
    {
    }

    template <std::integral T>
    template <allocator A>
    constexpr dynamic_bitset<T>::dynamic_bitset(dynamic_bitset&& rhs, const A& a)
        : base_type(storage_type(std::move(rhs), a))
    {
    }

    template <std::integral T>
    constexpr auto dynamic_bitset<T>::slice(size_type start, size_type length) const -> dynamic_bitset
    {
        if (start + length > this->size())
        {
            throw std::out_of_range("slice: start + length exceeds bitset size");
        }

        // Create a new bitset with the specified length
        dynamic_bitset result(length, default_allocator{});

        // Copy the bits
        for (size_type i = 0; i < length; ++i)
        {
            result.set(i, this->test(start + i));
        }

        return result;
    }

    template <std::integral T>
    constexpr auto dynamic_bitset<T>::slice(size_type start) const -> dynamic_bitset
    {
        if (start > this->size())
        {
            throw std::out_of_range("slice: start exceeds bitset size");
        }

        return slice(start, this->size() - start);
    }

    /**
     * @brief Type alias for a validity bitmap using 8-bit storage blocks.
     *
     * A validity bitmap is a specialized dynamic_bitset commonly used in data processing
     * to track which elements in a data array are valid (non-null). Uses std::uint8_t
     * for efficient memory usage and cache performance.
     *
     * Example usage:
     * @code
     * validity_bitmap validity(1000, true);  // 1000 valid elements
     * validity.set(42, false);  // Mark element 42 as invalid/null
     * @endcode
     */
    using validity_bitmap = dynamic_bitset<std::uint8_t>;

    namespace detail
    {
        using validity_bitmap = sparrow::validity_bitmap;

        inline validity_bitmap ensure_validity_bitmap_impl(std::size_t size, const validity_bitmap& bitmap)
        {
            if (bitmap.size() == 0)
            {
                return {size, true, bitmap.buffer().get_allocator()};
            }
            return bitmap;  // copy
        }

        inline validity_bitmap ensure_validity_bitmap_impl(std::size_t size, validity_bitmap&& bitmap)
        {
            if (bitmap.size() == 0)
            {
                bitmap.resize(size, true);
            }
            return std::move(bitmap);
        }

        // range of booleans
        template <std::ranges::input_range R>
            requires(std::same_as<std::ranges::range_value_t<R>, bool>)
        validity_bitmap ensure_validity_bitmap_impl(std::size_t size, R&& range)
        {
            SPARROW_ASSERT_TRUE(size == range_size(range) || range_size(range) == 0);
            validity_bitmap bitmap(size, true, validity_bitmap::default_allocator{});
            std::size_t i = 0;
            for (auto value : range)
            {
                if (!value)
                {
                    bitmap.set(i, false);
                }
                i++;
            }
            return bitmap;
        }

        // range of indices / integers (but not booleans)
        template <std::ranges::input_range R>
            requires(
                std::unsigned_integral<std::ranges::range_value_t<R>>
                && !std::same_as<std::ranges::range_value_t<R>, bool>
                && !std::same_as<std::decay_t<R>, validity_bitmap>
            )
        validity_bitmap ensure_validity_bitmap_impl(std::size_t size, R&& range_of_indices)
        {
            validity_bitmap bitmap(size, true, validity_bitmap::default_allocator{});
            for (auto index : range_of_indices)
            {
                bitmap.set(index, false);
            }
            return bitmap;
        }
    }  // namespace detail

    /**
     * @brief Concept defining valid input types for validity bitmap creation.
     *
     * This concept specifies what types can be used as input when creating or ensuring
     * a validity bitmap. Accepts:
     * - validity_bitmap objects (by value or const reference)
     * - Ranges of boolean values (where each bool indicates validity)
     * - Ranges of unsigned integers (where each integer is an index of invalid elements)
     *
     * Explicitly excludes string types to prevent accidental conversions.
     *
     * @tparam T The type to check for validity bitmap input compatibility
     */
    template <class T>
    concept validity_bitmap_input = (std::same_as<T, validity_bitmap> || std::same_as<T, const validity_bitmap&>
                                     || (std::ranges::input_range<T>
                                         && std::same_as<std::ranges::range_value_t<T>, bool>)
                                     || (std::ranges::input_range<T>
                                         && std::unsigned_integral<std::ranges::range_value_t<T>>) )
                                    && (!std::same_as<std::remove_cvref_t<T>, std::string>
                                        && !std::same_as<std::remove_cvref_t<T>, std::string_view>
                                        && !std::same_as<std::decay_t<T>, const char*>);

    /**
     * @brief Ensures a validity bitmap of the specified size from various input types.
     *
     * This function creates or adapts a validity bitmap to have the specified size,
     * handling different input types appropriately:
     *
     * - If given an existing validity_bitmap with size 0, creates a new one with all bits set to true
     * - If given an existing validity_bitmap with the correct size, returns it (copy or move)
     * - If given a range of booleans, creates a bitmap where true means valid
     * - If given a range of indices, creates a bitmap where those indices are marked invalid
     *
     * @tparam R Type of the validity input, must satisfy validity_bitmap_input concept
     * @param size The desired size of the resulting validity bitmap
     * @param validity_input The input data to create/adapt the validity bitmap from
     * @return A validity_bitmap of the specified size
     *
     * @pre For range inputs: range size must match the specified size or be empty
     * @pre For index ranges: all indices must be less than the specified size
     *
     * Example usage:
     * @code
     * // Create from boolean range
     * std::vector<bool> valid = {true, false, true};
     * auto bitmap1 = ensure_validity_bitmap(3, valid);
     *
     * // Create from invalid indices
     * std::vector<std::size_t> invalid_indices = {1, 5, 9};
     * auto bitmap2 = ensure_validity_bitmap(10, invalid_indices);
     * @endcode
     */
    template <validity_bitmap_input R>
    validity_bitmap ensure_validity_bitmap(std::size_t size, R&& validity_input)
    {
        return detail::ensure_validity_bitmap_impl(size, std::forward<R>(validity_input));
    }

}  // namespace sparrow
