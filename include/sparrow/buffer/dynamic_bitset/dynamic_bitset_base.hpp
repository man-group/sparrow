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


#include <algorithm>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "sparrow/buffer/dynamic_bitset/bitset_iterator.hpp"
#include "sparrow/buffer/dynamic_bitset/bitset_reference.hpp"
#include "sparrow/buffer/dynamic_bitset/null_count_policy.hpp"
#include "sparrow/utils/contracts.hpp"

namespace sparrow
{
    /**
     * @class dynamic_bitset_base
     *
     * Base class providing core functionality for dynamic bitset implementations.
     *
     * This template class serves as the foundation for both dynamic_bitset and dynamic_bitset_view,
     * providing a comprehensive API for manipulating sequences of bits stored in memory blocks.
     * The key difference between derived classes is memory ownership: dynamic_bitset owns and
     * manages its storage, while dynamic_bitset_view provides a non-owning view.
     *
     * The class efficiently stores bits using blocks of integral types, with specialized algorithms
     * for bit manipulation, counting, and iteration. It supports all standard container operations
     * including insertion, deletion, resizing, and element access.
     *
     * @tparam B The underlying storage type, which must be a random access range.
     *          Typically buffer<T> for owning storage or buffer_view<T> for non-owning views.
     *          The value_type of B must be an integral type used as storage blocks.
     *
     * @note This class tracks both the total size and null count for efficient validity checking
     *       in data processing scenarios where null/invalid values are common.
     *
     * @note The class uses bit-level operations and assumes little-endian bit ordering within blocks.
     *
     * Example usage through derived classes:
     * @code
     * // Through dynamic_bitset (owning)
     * dynamic_bitset<std::uint8_t> bits(100, false);
     * bits.set(50, true);
     * bool value = bits.test(50);
     *
     * // Through dynamic_bitset_view (non-owning)
     * std::vector<std::uint8_t> buffer(16, 0);
     * dynamic_bitset_view<std::uint8_t> view(buffer.data(), 128);
     * view.set(64, true);
     * @endcode
     */
    template <typename B, null_count_policy NCP = tracking_null_count<>>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    class dynamic_bitset_base : private NCP
    {
    public:

        using self_type = dynamic_bitset_base<B, NCP>;  ///< This class type
        using null_count_policy_type = NCP;             ///< Null count tracking policy type
        using storage_type = B;                         ///< Underlying storage container type
        using storage_type_without_cvrefpointer = std::remove_pointer_t<
            std::remove_cvref_t<storage_type>>;  ///< Storage type without CV/ref/pointer qualifiers
        using block_type = typename storage_type_without_cvrefpointer::value_type;  ///< Type of each storage
                                                                                    ///< block (integral type)
        using value_type = bool;                        ///< Type of individual bit values
        using reference = bitset_reference<self_type>;  ///< Mutable reference to a bit
        using const_reference = bool;                   ///< Immutable reference to a bit (plain bool)
        using size_type = typename storage_type_without_cvrefpointer::size_type;  ///< Type for sizes and
                                                                                  ///< indices
        using difference_type = typename storage_type_without_cvrefpointer::difference_type;  ///< Type for
                                                                                              ///< iterator
                                                                                              ///< differences
        using iterator = bitset_iterator<self_type, false>;       ///< Mutable iterator type
        using const_iterator = bitset_iterator<self_type, true>;  ///< Immutable iterator type

        /**
         * @brief Returns the number of bits in the bitset.
         * @return The total number of bits stored
         * @post Return value is non-negative
         */
        [[nodiscard]] constexpr size_type size() const noexcept;

        /**
         * @brief Returns the bit offset within the buffer.
         * @return The offset in bits from the start of the buffer
         * @post Return value is non-negative
         */
        [[nodiscard]] constexpr size_type offset() const noexcept;

        /**
         * @brief Sets the bit offset within the buffer.
         * @param offset The new offset in bits from the start of the buffer
         * @post offset() == offset
         */
        constexpr void set_offset(size_type offset) noexcept;

        /**
         * @brief Checks if the bitset contains no bits.
         * @return true if size() == 0, false otherwise
         * @post Return value is equivalent to (size() == 0)
         */
        [[nodiscard]] constexpr bool empty() const noexcept;

        /**
         * @brief Returns the number of bits set to false (null/invalid).
         * @return The count of unset bits
         * @post Return value <= size()
         * @note Only available when using tracking_null_count policy
         */
        [[nodiscard]] constexpr size_type null_count() const noexcept
            requires(NCP::track_null_count);

        /**
         * @brief Tests the value of a bit at the specified position.
         * @param pos The position of the bit to test
         * @return true if the bit is set, false otherwise
         * @pre pos < size()
         * @note Asserts that pos is within bounds in debug builds
         * @note Returns true for null buffers (all bits assumed set)
         */
        [[nodiscard]] constexpr bool test(size_type pos) const;

        /**
         * @brief Sets the value of a bit at the specified position.
         * @param pos The position of the bit to set
         * @param value The value to set (true or false)
         * @pre pos < size()
         * @post test(pos) == value
         * @note Asserts that pos is within bounds in debug builds
         * @note May allocate storage if setting false on a null buffer and storage supports resize
         * @throws std::runtime_error if trying to set false on a non-resizable null buffer
         */
        constexpr void set(size_type pos, value_type value);

        /**
         * @brief Accesses a bit with bounds checking.
         * @param pos The position of the bit to access
         * @return Immutable reference to the bit value
         * @pre pos < size()
         * @post Return value represents the bit at position pos
         * @throws std::out_of_range if pos >= size()
         */
        [[nodiscard]] constexpr const_reference at(size_type pos) const;

        /**
         * @brief Accesses a bit with bounds checking.
         * @param pos The position of the bit to access
         * @return Mutable reference to the bit
         * @pre pos < size()
         * @post Return value allows modification of the bit at position pos
         * @throws std::out_of_range if pos >= size()
         */
        [[nodiscard]] constexpr reference at(size_type pos);

        /**
         * @brief Accesses a bit without bounds checking.
         * @param i The position of the bit to access
         * @return Mutable reference to the bit
         * @pre i < size()
         * @post Return value allows modification of the bit at position i
         * @note Asserts that i is within bounds in debug builds
         */
        [[nodiscard]] constexpr reference operator[](size_type i);

        /**
         * @brief Accesses a bit without bounds checking.
         * @param i The position of the bit to access
         * @return The value of the bit
         * @pre i < size()
         * @post Return value represents the bit at position i
         * @note Asserts that i is within bounds in debug builds
         */
        [[nodiscard]] constexpr const_reference operator[](size_type i) const;

        /**
         * @brief Returns a pointer to the underlying block storage.
         * @return Mutable pointer to the first block, or nullptr if storage is null
         * @post Return value points to valid memory or is nullptr
         */
        [[nodiscard]] constexpr block_type* data() noexcept;

        /**
         * @brief Returns a pointer to the underlying block storage.
         * @return Immutable pointer to the first block, or nullptr if storage is null
         * @post Return value points to valid memory or is nullptr
         */
        [[nodiscard]] constexpr const block_type* data() const noexcept;

        /**
         * @brief Returns the number of storage blocks.
         * @return The number of blocks used to store the bits
         * @post Return value >= compute_block_count(size())
         */
        [[nodiscard]] constexpr size_type block_count() const noexcept;

        /**
         * @brief Swaps the contents with another bitset.
         * @param rhs The other bitset to swap with
         * @post This bitset contains the former contents of rhs
         * @post rhs contains the former contents of this bitset
         */
        constexpr void swap(self_type& rhs) noexcept;

        /**
         * @brief Returns a mutable iterator to the first bit.
         * @return Iterator pointing to the first bit
         * @post Return value points to position 0 or equals end() if empty
         */
        [[nodiscard]] constexpr iterator begin();

        /**
         * @brief Returns a mutable iterator past the last bit.
         * @return Iterator pointing past the last bit
         * @post Return value points to position size()
         */
        [[nodiscard]] constexpr iterator end();

        /**
         * @brief Returns an immutable iterator to the first bit.
         * @return Const iterator pointing to the first bit
         * @post Return value points to position 0 or equals end() if empty
         */
        [[nodiscard]] constexpr const_iterator begin() const;

        /**
         * @brief Returns an immutable iterator past the last bit.
         * @return Const iterator pointing past the last bit
         * @post Return value points to position size()
         */
        [[nodiscard]] constexpr const_iterator end() const;

        /**
         * @brief Returns an immutable iterator to the first bit.
         * @return Const iterator pointing to the first bit
         * @post Return value points to position 0 or equals cend() if empty
         */
        [[nodiscard]] constexpr const_iterator cbegin() const;

        /**
         * @brief Returns an immutable iterator past the last bit.
         * @return Const iterator pointing past the last bit
         * @post Return value points to position size()
         */
        [[nodiscard]] constexpr const_iterator cend() const;

        /**
         * @brief Accesses the first bit.
         * @return Mutable reference to the first bit
         * @pre size() >= 1
         * @post Return value allows modification of the bit at position 0
         * @note Asserts that the bitset is not empty in debug builds
         */
        [[nodiscard]] constexpr reference front();

        /**
         * @brief Accesses the first bit.
         * @return The value of the first bit
         * @pre size() >= 1
         * @post Return value represents the bit at position 0
         * @note Asserts that the bitset is not empty in debug builds
         * @note Returns true for null buffers
         */
        [[nodiscard]] constexpr const_reference front() const;

        /**
         * @brief Accesses the last bit.
         * @return Mutable reference to the last bit
         * @pre size() >= 1
         * @post Return value allows modification of the bit at position size()-1
         * @note Asserts that the bitset is not empty in debug builds
         */
        [[nodiscard]] constexpr reference back();

        /**
         * @brief Accesses the last bit.
         * @return The value of the last bit
         * @pre size() >= 1
         * @post Return value represents the bit at position size()-1
         * @note Asserts that the bitset is not empty in debug builds
         * @note Returns true for null buffers
         */
        [[nodiscard]] constexpr const_reference back() const;

        /**
         * @brief Returns an immutable reference to the underlying buffer.
         * @return Reference to the storage buffer
         * @post Return value provides access to the underlying storage
         */
        [[nodiscard]] constexpr const storage_type_without_cvrefpointer& buffer() const noexcept
        {
            if constexpr (std::is_pointer_v<storage_type>)
            {
                return *m_buffer;
            }
            else
            {
                return m_buffer;
            }
        }

        /**
         * @brief Returns a mutable reference to the underlying buffer.
         * @return Reference to the storage buffer
         * @post Return value provides mutable access to the underlying storage
         */
        [[nodiscard]] constexpr storage_type_without_cvrefpointer& buffer() noexcept
        {
            if constexpr (std::is_pointer_v<storage_type>)
            {
                return *m_buffer;
            }
            else
            {
                return m_buffer;
            }
        }

        /**
         * @brief Computes the number of blocks needed to store the specified number of bits.
         * @param bits_count The number of bits to store
         * @return The minimum number of blocks required
         * @post Return value >= (bits_count + bits_per_block - 1) / bits_per_block
         * @post Return value is 0 if bits_count is 0
         */
        [[nodiscard]] static constexpr size_type compute_block_count(size_type bits_count) noexcept;

        /**
         * @brief Extracts the underlying storage (move operation).
         * @return The moved storage object
         * @pre Storage type must be a value type (not a pointer or reference)
         * @post The bitset becomes invalid and should not be used after this call
         * @note Only available when storage_type is the same as storage_type_without_cvrefpointer
         */
        [[nodiscard]] storage_type extract_storage() noexcept
            requires std::same_as<storage_type, storage_type_without_cvrefpointer>
        {
            return std::move(m_buffer);
        }

    protected:

        /**
         * @brief Constructs a bitset with the given storage and size.
         * @param buffer The storage buffer to use
         * @param size The number of bits in the bitset
         * @post size() == size
         * @post offset() == 0
         * @post null_count() is computed by counting unset bits
         */
        constexpr dynamic_bitset_base(storage_type buffer, size_type size);

        /**
         * @brief Constructs a bitset with the given storage, size, and null count.
         * @param buffer The storage buffer to use
         * @param size The number of bits in the bitset
         * @param null_count The number of unset bits
         * @pre null_count <= size
         * @post size() == size
         * @post offset() == 0
         * @post null_count() == null_count
         * @note Only available when using tracking_null_count policy
         */
        constexpr dynamic_bitset_base(storage_type buffer, size_type size, size_type null_count)
            requires(NCP::track_null_count);

        /**
         * @brief Constructs a bitset with the given storage, size, offset, and null count.
         * @param buffer The storage buffer to use
         * @param size The number of bits in the bitset
         * @param offset The offset in bits from the start of the buffer
         * @param null_count The number of unset bits
         * @pre null_count <= size
         * @post size() == size
         * @post offset() == offset
         * @post null_count() == null_count
         * @note Only available when using tracking_null_count policy
         */
        constexpr dynamic_bitset_base(storage_type buffer, size_type size, size_type offset, size_type null_count)
            requires(NCP::track_null_count);

        constexpr ~dynamic_bitset_base() = default;

        constexpr dynamic_bitset_base(const dynamic_bitset_base&) = default;
        constexpr dynamic_bitset_base(dynamic_bitset_base&&) noexcept = default;

        constexpr dynamic_bitset_base& operator=(const dynamic_bitset_base&) = default;
        constexpr dynamic_bitset_base& operator=(dynamic_bitset_base&&) noexcept = default;

        /**
         * @brief Resizes the bitset to contain n bits.
         * @param n The new size in bits
         * @param b The value to initialize new bits with (default false)
         * @post size() == n
         * @post New bits (if any) are set to value b
         * @note May allocate additional storage blocks if needed
         * @note Preserves existing bits when growing
         */
        constexpr void resize(size_type n, value_type b = false);

        /**
         * @brief Removes all bits from the bitset.
         * @post size() == 0
         * @post empty() == true
         * @post null_count() == 0
         */
        constexpr void clear() noexcept;

        /**
         * @brief Inserts a single bit at the specified position.
         * @param pos Iterator pointing to the insertion position
         * @param value The value of the bit to insert
         * @return Iterator pointing to the inserted bit
         * @pre cbegin() <= pos <= cend()
         * @post size() increases by 1
         * @post The bit at the returned iterator position has value 'value'
         * @note Asserts that pos is within valid range in debug builds
         */
        constexpr iterator insert(const_iterator pos, value_type value);

        /**
         * @brief Inserts multiple bits with the same value at the specified position.
         * @param pos Iterator pointing to the insertion position
         * @param count The number of bits to insert
         * @param value The value of the bits to insert
         * @return Iterator pointing to the first inserted bit
         * @pre cbegin() <= pos <= cend()
         * @post size() increases by count
         * @post All inserted bits have value 'value'
         * @note Asserts that pos is within valid range in debug builds
         */
        constexpr iterator insert(const_iterator pos, size_type count, value_type value);

        /**
         * @brief Inserts bits from an iterator range at the specified position.
         * @tparam InputIt Input iterator type
         * @param pos Iterator pointing to the insertion position
         * @param first Iterator pointing to the first bit to insert
         * @param last Iterator pointing past the last bit to insert
         * @return Iterator pointing to the first inserted bit
         * @pre cbegin() <= pos <= cend()
         * @pre first and last form a valid iterator range
         * @post size() increases by distance(first, last)
         * @note Asserts that pos is within valid range in debug builds
         */
        template <std::input_iterator InputIt>
        constexpr iterator insert(const_iterator pos, InputIt first, InputIt last);

        /**
         * @brief Inserts bits from an initializer list at the specified position.
         * @param pos Iterator pointing to the insertion position
         * @param ilist Initializer list containing the bits to insert
         * @return Iterator pointing to the first inserted bit
         * @pre cbegin() <= pos <= cend()
         * @post size() increases by ilist.size()
         */
        constexpr iterator insert(const_iterator pos, std::initializer_list<value_type> ilist);

        /**
         * @brief Constructs a bit in-place at the specified position.
         * @param pos Iterator pointing to the insertion position
         * @param value The value of the bit to emplace
         * @return Iterator pointing to the emplaced bit
         * @pre cbegin() <= pos <= cend()
         * @post size() increases by 1
         * @note Equivalent to insert(pos, value) for bool values
         */
        constexpr iterator emplace(const_iterator pos, value_type value);

        /**
         * @brief Removes a single bit at the specified position.
         * @param pos Iterator pointing to the bit to remove
         * @return Iterator pointing to the bit following the removed bit
         * @pre cbegin() <= pos < cend()
         * @post size() decreases by 1
         * @note Asserts that pos is within valid range in debug builds
         */
        constexpr iterator erase(const_iterator pos);

        /**
         * @brief Removes bits in the specified range.
         * @param first Iterator pointing to the first bit to remove
         * @param last Iterator pointing past the last bit to remove
         * @return Iterator pointing to the bit following the removed range
         * @pre cbegin() <= first <= last <= cend()
         * @post size() decreases by distance(first, last)
         * @note Asserts that iterators form a valid range in debug builds
         */
        constexpr iterator erase(const_iterator first, const_iterator last);

        /**
         * @brief Adds a bit to the end of the bitset.
         * @param value The value of the bit to add
         * @post size() increases by 1
         * @post back() == value
         */
        constexpr void push_back(value_type value);

        /**
         * @brief Removes the last bit from the bitset.
         * @pre !empty()
         * @post size() decreases by 1 (if not empty)
         * @note Does nothing if the bitset is empty
         */
        constexpr void pop_back();

        /**
         * @brief Clears any unused bits in the last storage block.
         * @post Unused bits in the last block are set to 0
         * @note This ensures consistent behavior and correct bit counting
         */
        constexpr void zero_unused_bits();

    private:

        static constexpr std::size_t s_bits_per_block = sizeof(block_type) * CHAR_BIT;  ///< Number of bits
                                                                                        ///< per storage block

        /**
         * @brief Computes the block index for a given bit position.
         * @param pos The bit position
         * @return The index of the block containing the bit
         */
        [[nodiscard]] static constexpr size_type block_index(size_type pos) noexcept;

        /**
         * @brief Computes the bit index within a block for a given bit position.
         * @param pos The bit position
         * @return The index of the bit within its block
         */
        [[nodiscard]] static constexpr size_type bit_index(size_type pos) noexcept;

        /**
         * @brief Creates a bit mask for a given bit position.
         * @param pos The bit position
         * @return A mask with only the corresponding bit set
         */
        [[nodiscard]] static constexpr block_type bit_mask(size_type pos) noexcept;

        /**
         * @brief Counts the number of extra bits in the last block.
         * @return The number of bits used in the last block
         */
        [[nodiscard]] constexpr size_type count_extra_bits() const noexcept;

        storage_type m_buffer;  ///< The underlying storage for bit data
        size_type m_size;       ///< The number of bits in the bitset
        size_type m_offset;     ///< The offset in bits from the start of the buffer

        friend class bitset_iterator<self_type, true>;   ///< Const iterator needs access to internals
        friend class bitset_iterator<self_type, false>;  ///< Mutable iterator needs access to internals
        friend class bitset_reference<self_type>;        ///< Bit reference needs access to internals
    };

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::size() const noexcept -> size_type
    {
        return m_size;
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr bool dynamic_bitset_base<B, NCP>::empty() const noexcept
    {
        return m_size == 0;
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::offset() const noexcept -> size_type
    {
        return m_offset;
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B, NCP>::set_offset(size_type offset) noexcept
    {
        m_offset = offset;
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::null_count() const noexcept -> size_type
        requires(NCP::track_null_count)
    {
        return NCP::null_count();
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::operator[](size_type pos) -> reference
    {
        SPARROW_ASSERT_TRUE(pos < size());
        return reference(*this, pos);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr bool dynamic_bitset_base<B, NCP>::operator[](size_type pos) const
    {
        return test(pos);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr bool dynamic_bitset_base<B, NCP>::test(size_type pos) const
    {
        SPARROW_ASSERT_TRUE(pos < size());
        if constexpr (std::is_pointer_v<storage_type>)
        {
            if (m_buffer == nullptr)
            {
                return true;
            }
        }
        if (data() == nullptr)
        {
            return true;
        }
        const size_type actual_pos = m_offset + pos;
        return buffer().data()[block_index(actual_pos)] & bit_mask(actual_pos);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B, NCP>::set(size_type pos, value_type value)
    {
        SPARROW_ASSERT_TRUE(pos < size());
        if (data() == nullptr)
        {
            if (value == true)  // In this case,  we don't need to set the bit
            {
                return;
            }
            else
            {
                if constexpr (requires(storage_type_without_cvrefpointer s) { s.resize(0); })
                {
                    constexpr block_type true_value = block_type(~block_type(0));
                    const auto block_count = compute_block_count(size() + m_offset);
                    buffer().resize(block_count, true_value);
                    zero_unused_bits();
                }
                else
                {
                    throw std::runtime_error("Cannot set a bit in a null buffer.");
                }
            }
        }
        const size_type actual_pos = m_offset + pos;
        block_type& block = buffer().data()[block_index(actual_pos)];
        const bool old_value = block & bit_mask(actual_pos);
        if (value)
        {
            block |= bit_mask(actual_pos);
        }
        else
        {
            block &= block_type(~bit_mask(actual_pos));
        }
        this->update_null_count(old_value, value);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::data() noexcept -> block_type*
    {
        if constexpr (std::is_pointer_v<storage_type>)
        {
            if (m_buffer == nullptr)
            {
                return nullptr;
            }
        }
        return buffer().data();
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::data() const noexcept -> const block_type*
    {
        return buffer().data();
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::block_count() const noexcept -> size_type
    {
        return buffer().size();
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B, NCP>::swap(self_type& rhs) noexcept
    {
        using std::swap;
        swap(m_buffer, rhs.m_buffer);
        swap(m_size, rhs.m_size);
        swap(m_offset, rhs.m_offset);
        this->swap_null_count(rhs);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::begin() -> iterator
    {
        return iterator(this, 0u);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::end() -> iterator
    {
        return iterator(this, size());
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::begin() const -> const_iterator
    {
        return cbegin();
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::end() const -> const_iterator
    {
        return cend();
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::cbegin() const -> const_iterator
    {
        return const_iterator(this, 0);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::cend() const -> const_iterator
    {
        return const_iterator(this, size());
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::at(size_type pos) -> reference
    {
        if (pos >= size())
        {
            throw std::out_of_range(
                "dynamic_bitset_base::at: index out of range for dynamic_bitset_base of size"
                + std::to_string(size()) + " at index " + std::to_string(pos)
            );
        }
        return (*this)[pos];
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::at(size_type pos) const -> const_reference
    {
        if (pos >= size())
        {
            throw std::out_of_range(
                "dynamic_bitset_base::at: index out of range for dynamic_bitset_base of size"
                + std::to_string(size()) + " at index " + std::to_string(pos)
            );
        }
        return test(pos);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::front() -> reference
    {
        SPARROW_ASSERT_TRUE(size() >= 1);
        return (*this)[0];
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::front() const -> const_reference
    {
        SPARROW_ASSERT_TRUE(size() >= 1);
        if (data() == nullptr)
        {
            return true;
        }
        return (*this)[0];
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::back() -> reference
    {
        SPARROW_ASSERT_TRUE(size() >= 1);
        return (*this)[size() - 1];
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::back() const -> const_reference
    {
        SPARROW_ASSERT_TRUE(size() >= 1);
        if (data() == nullptr)
        {
            return true;
        }
        return (*this)[size() - 1];
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr dynamic_bitset_base<B, NCP>::dynamic_bitset_base(storage_type buf, size_type size)
        : m_buffer(std::move(buf))
        , m_size(size)
        , m_offset(0)
    {
        this->initialize_null_count(data(), m_size, buffer().size(), m_offset);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr dynamic_bitset_base<B, NCP>::dynamic_bitset_base(storage_type buf, size_type size, size_type null_count)
        requires(NCP::track_null_count)
        : m_buffer(std::move(buf))
        , m_size(size)
        , m_offset(0)
    {
        this->set_null_count(null_count);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr dynamic_bitset_base<B, NCP>::dynamic_bitset_base(storage_type buf, size_type size, size_type offset, size_type null_count)
        requires(NCP::track_null_count)
        : m_buffer(std::move(buf))
        , m_size(size)
        , m_offset(offset)
    {
        this->set_null_count(null_count);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::compute_block_count(size_type bits_count) noexcept -> size_type
    {
        return bits_count / s_bits_per_block + static_cast<size_type>(bits_count % s_bits_per_block != 0);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::block_index(size_type pos) noexcept -> size_type
    {
        return pos / s_bits_per_block;
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::bit_index(size_type pos) noexcept -> size_type
    {
        return pos % s_bits_per_block;
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::bit_mask(size_type pos) noexcept -> block_type
    {
        const size_type bit = bit_index(pos);
        return static_cast<block_type>(block_type(1) << bit);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B, NCP>::count_extra_bits() const noexcept -> size_type
    {
        return bit_index(size() + m_offset);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B, NCP>::zero_unused_bits()
    {
        if (data() == nullptr)
        {
            return;
        }
        const size_type extra_bits = count_extra_bits();
        if (extra_bits != 0)
        {
            buffer().back() &= block_type(~(~block_type(0) << extra_bits));
        }
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B, NCP>::resize(size_type n, value_type b)
    {
        if ((data() == nullptr) && b)
        {
            m_size = n;
            return;
        }
        size_type old_block_count = buffer().size();
        const size_type new_block_count = compute_block_count(n + m_offset);
        const block_type value = b ? block_type(~block_type(0)) : block_type(0);

        if (new_block_count != old_block_count)
        {
            if (data() == nullptr)
            {
                constexpr block_type true_value = block_type(~block_type(0));
                old_block_count = compute_block_count(size() + m_offset);
                buffer().resize(old_block_count, true_value);
                zero_unused_bits();
            }
            buffer().resize(new_block_count, value);
        }

        if (b && (n > m_size))
        {
            const size_type extra_bits = count_extra_bits();
            if (extra_bits > 0)
            {
                buffer().data()[old_block_count - 1] |= static_cast<block_type>(value << extra_bits);
            }
        }

        m_size = n;
        this->recompute_null_count(data(), m_size, buffer().size(), m_offset);
        zero_unused_bits();
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B, NCP>::clear() noexcept
    {
        buffer().clear();
        m_size = 0;
        this->clear_null_count();
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr dynamic_bitset_base<B, NCP>::iterator
    dynamic_bitset_base<B, NCP>::insert(const_iterator pos, value_type value)
    {
        return insert(pos, 1, value);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr dynamic_bitset_base<B, NCP>::iterator
    dynamic_bitset_base<B, NCP>::insert(const_iterator pos, size_type count, value_type value)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const auto index = static_cast<size_type>(std::distance(cbegin(), pos));
        if (data() == nullptr && value)
        {
            m_size += count;
        }
        else
        {
            const size_type old_size = size();
            const size_type new_size = old_size + count;

            // TODO: The current implementation is not efficient. It can be improved.

            resize(new_size);

            for (size_type i = old_size + count - 1; i >= index + count; --i)
            {
                set(i, test(i - count));
            }

            for (size_type i = 0; i < count; ++i)
            {
                set(index + i, value);
            }
        }

        return iterator(this, index);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    template <std::input_iterator InputIt>
    constexpr dynamic_bitset_base<B, NCP>::iterator
    dynamic_bitset_base<B, NCP>::insert(const_iterator pos, InputIt first, InputIt last)
    {
        const auto index = static_cast<size_type>(std::distance(cbegin(), pos));
        const auto count = static_cast<size_type>(std::distance(first, last));
        if (data() == nullptr)
        {
            if (std::all_of(
                    first,
                    last,
                    [](auto v)
                    {
                        return bool(v);
                    }
                ))
            {
                m_size += count;
            }
            return iterator(this, index);
        }
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());

        const size_type old_size = size();
        const size_type new_size = old_size + count;

        resize(new_size);

        // TODO: The current implementation is not efficient. It can be improved.

        for (size_type i = old_size + count - 1; i >= index + count; --i)
        {
            set(i, test(i - count));
        }

        for (size_type i = 0; i < count; ++i)
        {
            set(index + i, *first++);
        }

        return iterator(this, index);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr dynamic_bitset_base<B, NCP>::iterator
    dynamic_bitset_base<B, NCP>::emplace(const_iterator pos, value_type value)
    {
        return insert(pos, value);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr dynamic_bitset_base<B, NCP>::iterator dynamic_bitset_base<B, NCP>::erase(const_iterator pos)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos < cend());

        return erase(pos, pos + 1);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr dynamic_bitset_base<B, NCP>::iterator
    dynamic_bitset_base<B, NCP>::erase(const_iterator first, const_iterator last)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= first);
        SPARROW_ASSERT_TRUE(first <= last);
        SPARROW_ASSERT_TRUE(last <= cend());

        const auto first_index = static_cast<size_type>(std::distance(cbegin(), first));
        const auto last_index = static_cast<size_type>(std::distance(cbegin(), last));
        const size_type count = last_index - first_index;

        if (data() == nullptr)
        {
            m_size -= count;
        }
        else
        {
            if (last == cend())
            {
                resize(first_index);
                return end();
            }

            // TODO: The current implementation is not efficient. It can be improved.

            const size_type bit_to_move = size() - last_index;
            for (size_type i = 0; i < bit_to_move; ++i)
            {
                set(first_index + i, test(last_index + i));
            }

            resize(size() - count);
        }
        return iterator(this, first_index);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B, NCP>::push_back(value_type value)
    {
        resize(size() + 1, value);
    }

    template <typename B, null_count_policy NCP>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B, NCP>::pop_back()
    {
        if (empty())
        {
            return;
        }
        resize(size() - 1);
    }
}
