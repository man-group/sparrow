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
     * @class dynamic_bitset_base
     *
     * Base class for dynamic_bitset and dynamic_bitset_view.
     * Both represent a dynamic size sequence of bits. The only
     * difference between dynamic_bitset and dynamic_bitset_view
     * is that the former holds and manages its memory while
     * the second does not.
     *
     * @tparam B the underlying storage
     */
    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    class dynamic_bitset_base
    {
    public:

        using self_type = dynamic_bitset_base<B>;
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
        [[nodiscard]] constexpr size_type null_count() const noexcept;

        [[nodiscard]] constexpr bool test(size_type pos) const;
        constexpr void set(size_type pos, value_type value);

        [[nodiscard]] constexpr const_reference at(size_type pos) const;
        [[nodiscard]] constexpr reference at(size_type pos);

        [[nodiscard]] constexpr reference operator[](size_type i);
        [[nodiscard]] constexpr const_reference operator[](size_type i) const;

        [[nodiscard]] constexpr block_type* data() noexcept;
        [[nodiscard]] constexpr const block_type* data() const noexcept;
        [[nodiscard]] constexpr size_type block_count() const noexcept;

        constexpr void swap(self_type&) noexcept;

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

        // storage_type is a value_type
        [[nodiscard]] storage_type extract_storage() noexcept
            requires std::same_as<storage_type, storage_type_without_cvrefpointer>
        {
            return std::move(m_buffer);
        }

    protected:

        constexpr dynamic_bitset_base(storage_type buffer, size_type size);
        constexpr dynamic_bitset_base(storage_type buffer, size_type size, size_type null_count);
        constexpr ~dynamic_bitset_base() = default;

        constexpr dynamic_bitset_base(const dynamic_bitset_base&) = default;
        constexpr dynamic_bitset_base(dynamic_bitset_base&&) noexcept = default;

        constexpr dynamic_bitset_base& operator=(const dynamic_bitset_base&) = default;
        constexpr dynamic_bitset_base& operator=(dynamic_bitset_base&&) noexcept = default;

        constexpr void resize(size_type n, value_type b = false);
        constexpr void clear() noexcept;

        constexpr iterator insert(const_iterator pos, value_type value);
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
        [[nodiscard]] size_type count_non_null() const noexcept;

    private:

        static constexpr std::size_t s_bits_per_block = sizeof(block_type) * CHAR_BIT;
        [[nodiscard]] static constexpr size_type block_index(size_type pos) noexcept;
        [[nodiscard]] static constexpr size_type bit_index(size_type pos) noexcept;
        [[nodiscard]] static constexpr block_type bit_mask(size_type pos) noexcept;

        [[nodiscard]] constexpr size_type count_extra_bits() const noexcept;
        constexpr void update_null_count(bool old_value, bool new_value);

        storage_type m_buffer;
        size_type m_size;
        size_type m_null_count;

        friend class bitset_iterator<self_type, true>;
        friend class bitset_iterator<self_type, false>;
        friend class bitset_reference<self_type>;
    };

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::size() const noexcept -> size_type
    {
        return m_size;
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr bool dynamic_bitset_base<B>::empty() const noexcept
    {
        return m_size == 0;
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
        SPARROW_ASSERT_TRUE(data() != nullptr);
        return reference(*this, buffer().data()[block_index(pos)], bit_mask(pos));
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
        SPARROW_ASSERT_TRUE(pos < size());
        if (data() == nullptr)
        {
            return true;
        }
        return !m_null_count || buffer().data()[block_index(pos)] & bit_mask(pos);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B>::set(size_type pos, value_type value)
    {
        SPARROW_ASSERT_TRUE(pos < size());
        SPARROW_ASSERT_TRUE(data() != nullptr);
        block_type& block = buffer().data()[block_index(pos)];
        const bool old_value = block & bit_mask(pos);
        if (value)
        {
            block |= bit_mask(pos);
        }
        else
        {
            block &= block_type(~bit_mask(pos));
        }
        update_null_count(old_value, value);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::data() noexcept -> block_type*
    {
        return buffer().data();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::data() const noexcept -> const block_type*
    {
        return buffer().data();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::block_count() const noexcept -> size_type
    {
        return buffer().size();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B>::swap(self_type& rhs) noexcept
    {
        using std::swap;
        swap(m_buffer, rhs.m_buffer);
        swap(m_size, rhs.m_size);
        swap(m_null_count, rhs.m_null_count);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::begin() -> iterator
    {
        return iterator(this, data(), 0u);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::end() -> iterator
    {
        block_type* block = buffer().size() ? data() + buffer().size() - 1 : data();
        return iterator(this, block, size() % s_bits_per_block);
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
        return const_iterator(this, data(), 0u);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::cend() const -> const_iterator
    {
        const block_type* block = buffer().size() ? data() + buffer().size() - 1 : data();
        return const_iterator(this, block, size() % s_bits_per_block);
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
        : m_buffer(std::move(buf))
        , m_size(size)
        , m_null_count(m_size - count_non_null())
    {
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr dynamic_bitset_base<B>::dynamic_bitset_base(storage_type buf, size_type size, size_type null_count)
        : m_buffer(std::move(buf))
        , m_size(size)
        , m_null_count(null_count)
    {
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::compute_block_count(size_type bits_count) noexcept -> size_type
    {
        return bits_count / s_bits_per_block + static_cast<size_type>(bits_count % s_bits_per_block != 0);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::block_index(size_type pos) noexcept -> size_type
    {
        return pos / s_bits_per_block;
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::bit_index(size_type pos) noexcept -> size_type
    {
        return pos % s_bits_per_block;
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::bit_mask(size_type pos) noexcept -> block_type
    {
        const size_type bit = bit_index(pos);
        return static_cast<block_type>(block_type(1) << bit);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    auto dynamic_bitset_base<B>::count_non_null() const noexcept -> size_type
    {
        if (data() == nullptr)
        {
            return m_size;
        }
        if (buffer().empty())
        {
            return 0u;
        }

        int res = 0;
        size_t full_blocks = m_size / s_bits_per_block;
        for (size_t i = 0; i < full_blocks; ++i)
        {
            res += std::popcount(buffer().data()[i]);
        }
        if (full_blocks != buffer().size())
        {
            const size_t bits_count = m_size % s_bits_per_block;
            const block_type mask = ~block_type(~block_type(0) << bits_count);
            const block_type block = buffer().data()[full_blocks] & mask;
            res += std::popcount(block);
        }

        return static_cast<size_t>(res);
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr auto dynamic_bitset_base<B>::count_extra_bits() const noexcept -> size_type
    {
        return bit_index(size());
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B>::zero_unused_bits()
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
        const size_type old_block_count = buffer().size();
        const size_type new_block_count = compute_block_count(n);
        const block_type value = b ? block_type(~block_type(0)) : block_type(0);

        if (new_block_count != old_block_count)
        {
            buffer().resize(new_block_count, value);
        }

        if (b && n > m_size)
        {
            const size_type extra_bits = count_extra_bits();
            if (extra_bits > 0)
            {
                buffer().data()[old_block_count - 1] |= static_cast<block_type>(value << extra_bits);
            }
        }

        m_size = n;
        m_null_count = m_size - count_non_null();
        zero_unused_bits();
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    constexpr void dynamic_bitset_base<B>::clear() noexcept
    {
        buffer().clear();
        m_size = 0;
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
        SPARROW_ASSERT_TRUE(data() != nullptr);
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const auto index = static_cast<size_type>(std::distance(cbegin(), pos));
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

        auto block = data() + block_index(index);
        return iterator(this, block, bit_index(index));
    }

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    template <std::input_iterator InputIt>
    constexpr dynamic_bitset_base<B>::iterator
    dynamic_bitset_base<B>::insert(const_iterator pos, InputIt first, InputIt last)
    {
        SPARROW_ASSERT_TRUE(data() != nullptr);
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const auto index = static_cast<size_type>(std::distance(cbegin(), pos));
        const size_type old_size = size();
        const size_type count = static_cast<size_type>(std::distance(first, last));
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

        auto block = data() + block_index(index);
        return iterator(this, block, bit_index(index));
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
        SPARROW_ASSERT_TRUE(data() != nullptr);
        SPARROW_ASSERT_TRUE(cbegin() <= first);
        SPARROW_ASSERT_TRUE(first <= last);
        SPARROW_ASSERT_TRUE(last <= cend());

        const auto first_index = static_cast<size_type>(std::distance(cbegin(), first));

        if (last == cend())
        {
            resize(first_index);
            return end();
        }

        // TODO: The current implementation is not efficient. It can be improved.

        const auto last_index = static_cast<size_type>(std::distance(cbegin(), last));
        const size_type count = last_index - first_index;

        const size_type bit_to_move = size() - last_index;
        for (size_type i = 0; i < bit_to_move; ++i)
        {
            set(first_index + i, test(last_index + i));
        }

        resize(size() - count);

        auto block = data() + block_index(first_index);
        return iterator(this, block, bit_index(first_index));
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
        resize(size() - 1);
    }
}
