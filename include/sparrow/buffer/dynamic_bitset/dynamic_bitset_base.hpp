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
#include <bit>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "sparrow/buffer/bit_vector/bit_vector_base.hpp"
#include "sparrow/buffer/dynamic_bitset/bitset_iterator.hpp"
#include "sparrow/buffer/dynamic_bitset/bitset_reference.hpp"
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
     * @tparam B The underlying storage type, which must be a random access range.
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
     * @see bit_vector_base For pure bit manipulation without null tracking
     */
    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    class dynamic_bitset_base
    {
    public:

        using self_type = dynamic_bitset_base<B>;  ///< This class type
        using storage_type = B;                    ///< Underlying storage container type
        using storage_type_without_cvrefpointer = std::remove_pointer_t<
            std::remove_cvref_t<storage_type>>;  ///< Storage type without CV/ref/pointer qualifiers
        using bit_vector_type = bit_vector_base<B>;  ///< Underlying bit vector type
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
         * @brief Checks if the bitset contains no bits.
         * @return true if size() == 0, false otherwise
         * @post Return value is equivalent to (size() == 0)
         */
        [[nodiscard]] constexpr bool empty() const noexcept;

        /**
         * @brief Returns the number of bits set to false (null/invalid).
         * @return The count of unset bits
         * @post Return value <= size()
         */
        [[nodiscard]] constexpr size_type null_count() const noexcept;

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
            return m_bit_vector.buffer();
        }

        /**
         * @brief Returns a mutable reference to the underlying buffer.
         * @return Reference to the storage buffer
         * @post Return value provides mutable access to the underlying storage
         */
        [[nodiscard]] constexpr storage_type_without_cvrefpointer& buffer() noexcept
        {
            return m_bit_vector.buffer();
        }

        /**
         * @brief Computes the number of blocks needed to store the specified number of bits.
         * @param bits_count The number of bits to store
         * @return The minimum number of blocks required
         * @post Return value >= (bits_count + bits_per_block - 1) / bits_per_block
         * @post Return value is 0 if bits_count is 0
         */
        [[nodiscard]] static constexpr size_type compute_block_count(size_type bits_count) noexcept
        {
            return bit_vector_type::compute_block_count(bits_count);
        }

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
            return m_bit_vector.extract_storage();
        }

    protected:

        /**
         * @brief Constructs a bitset with the given storage and size.
         * @param buffer The storage buffer to use
         * @param size The number of bits in the bitset
         * @post size() == size
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
         * @post null_count() == null_count
         */
        constexpr dynamic_bitset_base(storage_type buffer, size_type size, size_type null_count);

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
        constexpr void zero_unused_bits()
        {
            m_bit_vector.zero_unused_bits();
        }

        /**
         * @brief Counts the number of bits set to true.
         * @return The number of set bits
         * @post Return value <= size()
         * @post Return value == size() - null_count()
         * @note Returns size() for null buffers (all bits assumed set)
         */
        [[nodiscard]] size_type count_non_null() const noexcept;

    private:

        /**
         * @brief Updates the null count when a bit value changes.
         * @param old_value The previous bit value
         * @param new_value The new bit value
         * @post null_count() is adjusted based on the value change
         */
        constexpr void update_null_count(bool old_value, bool new_value);

        bit_vector_type m_bit_vector;  ///< The underlying bit vector for data storage and manipulation
        size_type m_null_count;        ///< The number of bits set to false

        friend class bitset_iterator<self_type, true>;   ///< Const iterator needs access to internals
        friend class bitset_iterator<self_type, false>;  ///< Mutable iterator needs access to internals
        friend class bitset_reference<self_type>;        ///< Bit reference needs access to internals
    };

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::size() const noexcept -> size_type
    {
        return m_bit_vector.size();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr bool dynamic_bitset_base<B>::empty() const noexcept
    {
        return m_bit_vector.empty();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::null_count() const noexcept -> size_type
    {
        return m_null_count;
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::operator[](size_type pos) -> reference
    {
        SPARROW_ASSERT_TRUE(pos < size());
        return reference(*this, pos);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr bool dynamic_bitset_base<B>::operator[](size_type pos) const
    {
        return test(pos);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr bool dynamic_bitset_base<B>::test(size_type pos) const
    {
        // Validity semantics: null buffer means all bits are valid (true)
        if (m_bit_vector.data() == nullptr)
        {
            SPARROW_ASSERT_TRUE(pos < size());
            return true;
        }
        return m_bit_vector.test(pos);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B>::set(size_type pos, value_type value)
    {
        SPARROW_ASSERT_TRUE(pos < size());
        
        // Handle null buffer transition for validity semantics
        if (m_bit_vector.data() == nullptr)
        {
            // Null buffer means all bits are valid (true)
            // If setting to true, nothing changes
            if (value == true)
            {
                return;
            }
            
            // If setting to false, we need to materialize the buffer
            // Initialize to all 1s (all valid) for validity semantics
            if constexpr (requires { m_bit_vector.buffer().resize(0); })
            {
                const auto block_count = m_bit_vector.compute_block_count(size());
                m_bit_vector.buffer().resize(block_count, block_type(~block_type(0)));  // All 1s
                m_bit_vector.zero_unused_bits();
                
                // Now all bits are materialized as 1 (valid)
                // Set the specific bit and update null count
                m_bit_vector.set(pos, value);
                // Changed from true to false, so increment null count
                ++m_null_count;
            }
            else
            {
                throw std::runtime_error("Cannot set a bit in a null buffer.");
            }
            return;
        }
        
        // Normal case: buffer exists
        const bool old_value = m_bit_vector.test(pos);
        m_bit_vector.set(pos, value);
        update_null_count(old_value, value);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::data() noexcept -> block_type*
    {
        return m_bit_vector.data();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::data() const noexcept -> const block_type*
    {
        return m_bit_vector.data();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::block_count() const noexcept -> size_type
    {
        return m_bit_vector.block_count();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B>::swap(self_type& rhs) noexcept
    {
        using std::swap;
        swap(m_bit_vector, rhs.m_bit_vector);
        swap(m_null_count, rhs.m_null_count);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::begin() -> iterator
    {
        return iterator(this, 0u);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::end() -> iterator
    {
        return iterator(this, size());
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::begin() const -> const_iterator
    {
        return cbegin();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::end() const -> const_iterator
    {
        return cend();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::cbegin() const -> const_iterator
    {
        return const_iterator(this, 0);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::cend() const -> const_iterator
    {
        return const_iterator(this, size());
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::at(size_type pos) -> reference
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

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::at(size_type pos) const -> const_reference
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

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::front() -> reference
    {
        SPARROW_ASSERT_TRUE(size() >= 1);
        return (*this)[0];
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::front() const -> const_reference
    {
        SPARROW_ASSERT_TRUE(size() >= 1);
        if (data() == nullptr)
        {
            return true;
        }
        return (*this)[0];
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::back() -> reference
    {
        SPARROW_ASSERT_TRUE(size() >= 1);
        return (*this)[size() - 1];
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::back() const -> const_reference
    {
        SPARROW_ASSERT_TRUE(size() >= 1);
        if (data() == nullptr)
        {
            return true;
        }
        return (*this)[size() - 1];
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr dynamic_bitset_base<B>::dynamic_bitset_base(storage_type buf, size_type size)
        : m_bit_vector(std::move(buf), size)
        , m_null_count(this->size() - count_non_null())
    {
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr dynamic_bitset_base<B>::dynamic_bitset_base(storage_type buf, size_type size, size_type null_count)
        : m_bit_vector(std::move(buf), size)
        , m_null_count(null_count)
    {
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    auto dynamic_bitset_base<B>::count_non_null() const noexcept -> size_type
    {
        // Validity semantics: null buffer means all bits are valid (counted as set)
        if (m_bit_vector.data() == nullptr)
        {
            return size();
        }
        return m_bit_vector.count();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B>::update_null_count(bool old_value, bool new_value)
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

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B>::resize(size_type n, value_type b)
    {
        const bool was_null = (m_bit_vector.data() == nullptr);
        const size_type old_size = size();
        
        // Validity semantics: null buffer means all bits are true (valid)
        // If resizing from null and only adding true bits, stay null
        if (was_null && b == true)
        {
            // Just update size directly, DON'T call m_bit_vector.resize which would allocate
            m_bit_vector.m_size = n;
            // null_count stays 0 since all bits (old and new) are valid
            m_null_count = 0;
            return;
        }
        
        // If transitioning from null buffer and adding false bits,
        // we need to materialize the buffer with all 1s (all valid) for existing bits
        if (was_null && b == false && n > old_size)
        {
            if constexpr (requires { m_bit_vector.buffer().resize(0); })
            {
                // Allocate buffer and initialize old bits to 1 (all valid for validity semantics)
                const auto old_block_count = m_bit_vector.compute_block_count(old_size);
                if (old_block_count > 0)
                {
                    m_bit_vector.buffer().resize(old_block_count, block_type(~block_type(0)));  // All 1s
                    m_bit_vector.zero_unused_bits();
                }
            }
        }
        
        m_bit_vector.resize(n, b);
        m_null_count = size() - count_non_null();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B>::clear() noexcept
    {
        m_bit_vector.clear();
        m_null_count = 0;
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr dynamic_bitset_base<B>::iterator
    dynamic_bitset_base<B>::insert(const_iterator pos, value_type value)
    {
        return insert(pos, 1, value);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr dynamic_bitset_base<B>::iterator
    dynamic_bitset_base<B>::insert(const_iterator pos, size_type count, value_type value)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const auto index = static_cast<size_type>(std::distance(cbegin(), pos));
        
        const size_type old_size = size();
        const size_type new_size = old_size + count;

        // TODO: The current implementation is not efficient. It can be improved.

        // Resize with the value being inserted to maintain null buffer optimization
        resize(new_size, value);

        for (size_type i = old_size + count - 1; i >= index + count; --i)
        {
            set(i, test(i - count));
        }

        for (size_type i = 0; i < count; ++i)
        {
            set(index + i, value);
        }

        return iterator(this, index);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    template <std::input_iterator InputIt>
    constexpr dynamic_bitset_base<B>::iterator
    dynamic_bitset_base<B>::insert(const_iterator pos, InputIt first, InputIt last)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        
        const auto index = static_cast<size_type>(std::distance(cbegin(), pos));
        const auto count = static_cast<size_type>(std::distance(first, last));

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

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr dynamic_bitset_base<B>::iterator
    dynamic_bitset_base<B>::emplace(const_iterator pos, value_type value)
    {
        return insert(pos, value);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr dynamic_bitset_base<B>::iterator dynamic_bitset_base<B>::erase(const_iterator pos)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos < cend());

        return erase(pos, pos + 1);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr dynamic_bitset_base<B>::iterator
    dynamic_bitset_base<B>::erase(const_iterator first, const_iterator last)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= first);
        SPARROW_ASSERT_TRUE(first <= last);
        SPARROW_ASSERT_TRUE(last <= cend());

        const auto first_index = static_cast<size_type>(std::distance(cbegin(), first));
        const auto last_index = static_cast<size_type>(std::distance(cbegin(), last));
        const size_type count = last_index - first_index;

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
        return iterator(this, first_index);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B>::push_back(value_type value)
    {
        resize(size() + 1, value);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B>::pop_back()
    {
        if (empty())
        {
            return;
        }
        resize(size() - 1);
    }
}
