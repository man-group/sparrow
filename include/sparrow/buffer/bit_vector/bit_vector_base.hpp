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

#include "sparrow/buffer/dynamic_bitset/bitset_iterator.hpp"
#include "sparrow/buffer/dynamic_bitset/bitset_reference.hpp"
#include "sparrow/utils/contracts.hpp"

namespace sparrow
{
    /**
     * @class bit_vector_base
     *
     * Base class providing core bit-packed storage functionality without domain semantics.
     *
     * This template class provides a pure bit-packing container for storing and manipulating
     * sequences of boolean values efficiently in memory blocks. Unlike dynamic_bitset_base,
     * this class has NO knowledge of null/validity concepts - it is a pure data structure
     * for bit manipulation.
     *
     * Use this class when you need bit-packed storage for actual data values (not validity masks).
     * For validity/null tracking semantics, use validity_bitmap which composes this class
     * with null-counting functionality.
     *
     * Design rationale: Separates core bit manipulation (this class) from domain-specific
     * Arrow/analytics concepts (null counting), following Single Responsibility Principle.
     *
     * @tparam B The underlying storage type, which must be a random access range.
     *          Typically buffer<T> for owning storage or buffer_view<T> for non-owning views.
     *          The value_type of B must be an integral type used as storage blocks.
     *
     * @note This class does NOT track null counts or have any validity semantics.
     *       It is purely a bit-packing container.
     *
     * Example usage through derived classes:
     * @code
     * // Through bit_vector (owning)
     * bit_vector<std::uint8_t> bits(100, false);
     * bits.set(50, true);
     * bool value = bits.test(50);
     *
     * // Through bit_vector_view (non-owning)
     * std::vector<std::uint8_t> buffer(16, 0);
     * bit_vector_view<std::uint8_t> view(buffer.data(), 128);
     * view.set(64, true);
     * @endcode
     */
    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    class bit_vector_base
    {
    public:

        using self_type = bit_vector_base<B>;
        using storage_type = B;
        using storage_type_without_cvrefpointer = std::remove_pointer_t<std::remove_cvref_t<storage_type>>;
        using block_type = typename storage_type_without_cvrefpointer::value_type;
        using value_type = bool;
        using reference = bitset_reference<self_type>;
        using const_reference = bool;
        using size_type = typename storage_type_without_cvrefpointer::size_type;
        using difference_type = typename storage_type_without_cvrefpointer::difference_type;
        using iterator = bitset_iterator<self_type, false>;
        using const_iterator = bitset_iterator<self_type, true>;

        [[nodiscard]] constexpr size_type size() const noexcept;
        [[nodiscard]] constexpr bool empty() const noexcept;
        [[nodiscard]] constexpr bool test(size_type pos) const;
        constexpr void set(size_type pos, value_type value);
        [[nodiscard]] constexpr const_reference at(size_type pos) const;
        [[nodiscard]] constexpr reference at(size_type pos);
        [[nodiscard]] constexpr reference operator[](size_type i);
        [[nodiscard]] constexpr const_reference operator[](size_type i) const;
        [[nodiscard]] constexpr block_type* data() noexcept;
        [[nodiscard]] constexpr const block_type* data() const noexcept;
        [[nodiscard]] constexpr size_type block_count() const noexcept;
        constexpr void swap(self_type& rhs) noexcept;

        [[nodiscard]] constexpr iterator begin();
        [[nodiscard]] constexpr iterator end();
        [[nodiscard]] constexpr const_iterator begin() const;
        [[nodiscard]] constexpr const_iterator end() const;
        [[nodiscard]] constexpr const_iterator cbegin() const;
        [[nodiscard]] constexpr const_iterator cend() const;

        [[nodiscard]] constexpr reference front();
        [[nodiscard]] constexpr const_reference front() const;
        [[nodiscard]] constexpr reference back();
        [[nodiscard]] constexpr const_reference back() const;

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

        [[nodiscard]] static constexpr size_type compute_block_count(size_type bits_count) noexcept;

        [[nodiscard]] storage_type extract_storage() noexcept
            requires std::same_as<storage_type, storage_type_without_cvrefpointer>
        {
            return std::move(m_buffer);
        }

        /**
         * @brief Counts the number of bits set to true.
         * @return The number of set bits
         * @post Return value <= size()
         */
        [[nodiscard]] size_type count() const noexcept;

    protected:

        constexpr bit_vector_base(storage_type buffer, size_type size);

        constexpr ~bit_vector_base() = default;
        constexpr bit_vector_base(const bit_vector_base&) = default;
        constexpr bit_vector_base(bit_vector_base&&) noexcept = default;
        constexpr bit_vector_base& operator=(const bit_vector_base&) = default;
        constexpr bit_vector_base& operator=(bit_vector_base&&) noexcept = default;

        constexpr void resize(size_type n, value_type b = false);
        constexpr void clear() noexcept;
        constexpr iterator insert(const_iterator pos, value_type value);

        // Friend declarations for composition
        template <typename U>
            requires std::ranges::random_access_range<std::remove_pointer_t<U>>
        friend class dynamic_bitset_base;
        constexpr iterator insert(const_iterator pos, size_type count, value_type value);

        template <std::input_iterator InputIt>
        constexpr iterator insert(const_iterator pos, InputIt first, InputIt last);

        constexpr iterator insert(const_iterator pos, std::initializer_list<value_type> ilist);
        constexpr iterator emplace(const_iterator pos, value_type value);
        constexpr iterator erase(const_iterator pos);
        constexpr iterator erase(const_iterator first, const_iterator last);
        constexpr void push_back(value_type value);
        constexpr void pop_back();
        constexpr void zero_unused_bits();

    private:

        static constexpr std::size_t s_bits_per_block = sizeof(block_type) * CHAR_BIT;

        [[nodiscard]] static constexpr size_type block_index(size_type pos) noexcept;
        [[nodiscard]] static constexpr size_type bit_index(size_type pos) noexcept;
        [[nodiscard]] static constexpr block_type bit_mask(size_type pos) noexcept;
        [[nodiscard]] constexpr size_type count_extra_bits() const noexcept;

        storage_type m_buffer;
        size_type m_size;

        friend class bitset_iterator<self_type, true>;
        friend class bitset_iterator<self_type, false>;
        friend class bitset_reference<self_type>;
    };

    /*****************************************
     * bit_vector_base implementation
     *****************************************/

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::size() const noexcept -> size_type
    {
        return m_size;
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr bool bit_vector_base<B>::empty() const noexcept
    {
        return m_size == 0;
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::operator[](size_type pos) -> reference
    {
        SPARROW_ASSERT_TRUE(pos < size());
        return reference(*this, pos);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr bool bit_vector_base<B>::operator[](size_type pos) const
    {
        return test(pos);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr bool bit_vector_base<B>::test(size_type pos) const
    {
        SPARROW_ASSERT_TRUE(pos < size());
        if constexpr (std::is_pointer_v<storage_type>)
        {
            if (m_buffer == nullptr)
            {
                return false;
            }
        }
        const auto* block_data = data();
        if (block_data == nullptr)
        {
            return false;
        }
        return block_data[block_index(pos)] & bit_mask(pos);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void bit_vector_base<B>::set(size_type pos, value_type value)
    {
        SPARROW_ASSERT_TRUE(pos < size());
        if (data() == nullptr)
        {
            if (value == false)
            {
                return;
            }
            else
            {
                if constexpr (requires(storage_type_without_cvrefpointer s) { s.resize(0); })
                {
                    const auto block_count = compute_block_count(size());
                    buffer().resize(block_count, block_type(0));
                }
                else
                {
                    throw std::runtime_error("Cannot set a bit in a null buffer.");
                }
            }
        }
        block_type& block = buffer().data()[block_index(pos)];
        if (value)
        {
            block |= bit_mask(pos);
        }
        else
        {
            block &= block_type(~bit_mask(pos));
        }
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::data() noexcept -> block_type*
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

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::data() const noexcept -> const block_type*
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

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::block_count() const noexcept -> size_type
    {
        return buffer().size();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void bit_vector_base<B>::swap(self_type& rhs) noexcept
    {
        using std::swap;
        swap(m_buffer, rhs.m_buffer);
        swap(m_size, rhs.m_size);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::begin() -> iterator
    {
        return iterator(this, 0u);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::end() -> iterator
    {
        return iterator(this, size());
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::begin() const -> const_iterator
    {
        return cbegin();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::end() const -> const_iterator
    {
        return cend();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::cbegin() const -> const_iterator
    {
        return const_iterator(this, 0);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::cend() const -> const_iterator
    {
        return const_iterator(this, size());
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::at(size_type pos) -> reference
    {
        if (pos >= size())
        {
            throw std::out_of_range(
                "bit_vector_base::at: index out of range for bit_vector_base of size "
                + std::to_string(size()) + " at index " + std::to_string(pos)
            );
        }
        return (*this)[pos];
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::at(size_type pos) const -> const_reference
    {
        if (pos >= size())
        {
            throw std::out_of_range(
                "bit_vector_base::at: index out of range for bit_vector_base of size "
                + std::to_string(size()) + " at index " + std::to_string(pos)
            );
        }
        return test(pos);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::front() -> reference
    {
        SPARROW_ASSERT_TRUE(size() >= 1);
        return (*this)[0];
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::front() const -> const_reference
    {
        SPARROW_ASSERT_TRUE(size() >= 1);
        if (data() == nullptr)
        {
            return false;
        }
        return (*this)[0];
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::back() -> reference
    {
        SPARROW_ASSERT_TRUE(size() >= 1);
        return (*this)[size() - 1];
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::back() const -> const_reference
    {
        SPARROW_ASSERT_TRUE(size() >= 1);
        if (data() == nullptr)
        {
            return false;
        }
        return (*this)[size() - 1];
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr bit_vector_base<B>::bit_vector_base(storage_type buf, size_type size)
        : m_buffer(std::move(buf))
        , m_size(size)
    {
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::compute_block_count(size_type bits_count) noexcept -> size_type
    {
        return bits_count / s_bits_per_block + static_cast<size_type>(bits_count % s_bits_per_block != 0);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::block_index(size_type pos) noexcept -> size_type
    {
        return pos / s_bits_per_block;
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::bit_index(size_type pos) noexcept -> size_type
    {
        return pos % s_bits_per_block;
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::bit_mask(size_type pos) noexcept -> block_type
    {
        return static_cast<block_type>(block_type(1) << bit_index(pos));
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    auto bit_vector_base<B>::count() const noexcept -> size_type
    {
        if constexpr (std::is_pointer_v<storage_type>)
        {
            if (m_buffer == nullptr)
            {
                return 0;
            }
        }
        if (data() == nullptr || buffer().empty())
        {
            return 0;
        }

        int res = 0;
        const auto* block_data = buffer().data();
        const size_type full_blocks = m_size / s_bits_per_block;
        
        for (size_type i = 0; i < full_blocks; ++i)
        {
            res += std::popcount(block_data[i]);
        }
        
        // Handle tail block
        const size_type has_tail = full_blocks != buffer().size();
        const size_type bits_count = m_size % s_bits_per_block;
        const block_type mask = static_cast<block_type>((block_type(1) << bits_count) - 1);
        const block_type tail_block = block_data[full_blocks] & mask;
        const int tail_count = std::popcount(tail_block);
        res += tail_count & -static_cast<int>(has_tail);

        return static_cast<size_type>(res);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto bit_vector_base<B>::count_extra_bits() const noexcept -> size_type
    {
        return bit_index(size());
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void bit_vector_base<B>::zero_unused_bits()
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

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void bit_vector_base<B>::resize(size_type n, value_type b)
    {
        if ((data() == nullptr) && !b)
        {
            m_size = n;
            return;
        }
        size_type old_block_count = buffer().size();
        const size_type new_block_count = compute_block_count(n);
        const block_type value = b ? block_type(~block_type(0)) : block_type(0);

        if (new_block_count != old_block_count)
        {
            if (data() == nullptr)
            {
                old_block_count = compute_block_count(size());
                buffer().resize(old_block_count, block_type(0));
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
        zero_unused_bits();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void bit_vector_base<B>::clear() noexcept
    {
        buffer().clear();
        m_size = 0;
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr bit_vector_base<B>::iterator
    bit_vector_base<B>::insert(const_iterator pos, value_type value)
    {
        return insert(pos, 1, value);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr bit_vector_base<B>::iterator
    bit_vector_base<B>::insert(const_iterator pos, size_type count, value_type value)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const auto index = static_cast<size_type>(std::distance(cbegin(), pos));
        if (data() == nullptr && !value)
        {
            m_size += count;
        }
        else
        {
            const size_type old_size = size();
            const size_type new_size = old_size + count;

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

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    template <std::input_iterator InputIt>
    constexpr bit_vector_base<B>::iterator
    bit_vector_base<B>::insert(const_iterator pos, InputIt first, InputIt last)
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
                        return !bool(v);
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
    constexpr bit_vector_base<B>::iterator
    bit_vector_base<B>::insert(const_iterator pos, std::initializer_list<value_type> ilist)
    {
        return insert(pos, ilist.begin(), ilist.end());
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr bit_vector_base<B>::iterator
    bit_vector_base<B>::emplace(const_iterator pos, value_type value)
    {
        return insert(pos, value);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr bit_vector_base<B>::iterator bit_vector_base<B>::erase(const_iterator pos)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos < cend());

        return erase(pos, pos + 1);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr bit_vector_base<B>::iterator
    bit_vector_base<B>::erase(const_iterator first, const_iterator last)
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

            const size_type bit_to_move = size() - last_index;
            for (size_type i = 0; i < bit_to_move; ++i)
            {
                set(first_index + i, test(last_index + i));
            }

            resize(size() - count);
        }
        return iterator(this, first_index);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void bit_vector_base<B>::push_back(value_type value)
    {
        resize(size() + 1, value);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void bit_vector_base<B>::pop_back()
    {
        if (empty())
        {
            return;
        }
        resize(size() - 1);
    }
}
