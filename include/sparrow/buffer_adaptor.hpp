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

#include <cmath>
#include <limits>
#include <utility>

#include "sparrow/buffer.hpp"
#include "sparrow/contracts.hpp"

namespace sparrow
{
    /**
     * Class which have internally a reference to a buffer<From> and provides the API of a buffer<To>
     */
    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    class buffer_adaptor
    {
    public:

        using value_type = To;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;

        using buffer_reference_value_type = From;
        using buffer_reference = buffer<buffer_reference_value_type>;

        using size_type = buffer_reference::size_type;
        using difference_type = buffer_reference::difference_type;
        using iterator = pointer_iterator<pointer>;
        using const_iterator = pointer_iterator<const_pointer>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        buffer_adaptor(buffer_reference& buf);

        constexpr pointer data() noexcept;
        constexpr const_pointer data() const noexcept;


        // Element access()

        constexpr reference operator[](size_type idx);
        constexpr const_reference operator[](size_type idx) const;

        constexpr reference front();
        constexpr const_reference front() const;

        constexpr reference back();
        constexpr const_reference back() const;

        // Iterators

        constexpr iterator begin() noexcept;
        constexpr iterator end() noexcept;

        constexpr const_iterator begin() const noexcept;
        constexpr const_iterator end() const noexcept;

        constexpr const_iterator cbegin() const noexcept;
        constexpr const_iterator cend() const noexcept;

        constexpr reverse_iterator rbegin() noexcept;
        constexpr reverse_iterator rend() noexcept;

        constexpr const_reverse_iterator rbegin() const noexcept;
        constexpr const_reverse_iterator rend() const noexcept;

        constexpr const_reverse_iterator crbegin() const noexcept;
        constexpr const_reverse_iterator crend() const noexcept;

        // Capacity

        [[nodiscard]] constexpr size_type size() const noexcept;
        [[nodiscard]] constexpr size_type max_size() const noexcept;
        [[nodiscard]] constexpr size_type capacity() const noexcept;
        [[nodiscard]] constexpr bool empty() const noexcept;
        constexpr void reserve(size_type new_cap);
        constexpr void shrink_to_fit();

        // Modifiers

        constexpr void clear() noexcept;

        constexpr iterator insert(const_iterator pos, const value_type& value);
        constexpr iterator insert(const_iterator pos, size_type count, const value_type& value);
        template <class InputIt>
            requires std::input_iterator<InputIt>
        constexpr iterator insert(const_iterator pos, InputIt first, InputIt last);
        constexpr iterator insert(const_iterator pos, std::initializer_list<value_type> ilist);

        template <class... Args>
        constexpr iterator emplace(const_iterator pos, Args&&... args);

        constexpr iterator erase(const_iterator pos);
        constexpr iterator erase(const_iterator first, const_iterator last);

        constexpr void push_back(const value_type& value);

        constexpr void pop_back();

        constexpr void resize(size_type new_size);
        constexpr void resize(size_type new_size, const value_type& value);

    private:

        static constexpr double m_from_to_size_ratio = static_cast<double>(sizeof(buffer_reference_value_type))
                                                       / static_cast<double>(sizeof(value_type));

        static constexpr size_type m_to_from_size_ratio = static_cast<size_type>(1. / m_from_to_size_ratio);

        [[nodiscard]] constexpr buffer_reference::size_type index_for_buffer(size_type idx) const noexcept;

        [[nodiscard]] buffer_reference::const_iterator get_buffer_reference_iterator(const_iterator pos);

        buffer_reference& m_buffer;
        size_type m_max_size;
    };

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    buffer_adaptor<To, From>::buffer_adaptor(buffer_adaptor<To, From>::buffer_reference& buf)
        : m_buffer(buf)
        , m_max_size(m_buffer.max_size() / m_to_from_size_ratio)
    {
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::buffer_reference::size_type
    buffer_adaptor<To, From>::index_for_buffer(size_type idx) const noexcept
    {
        return idx * m_to_from_size_ratio;
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::pointer buffer_adaptor<To, From>::data() noexcept
    {
        return m_buffer.template data<value_type>();
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::const_pointer buffer_adaptor<To, From>::data() const noexcept
    {
        return m_buffer.template data<value_type>();
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::reference buffer_adaptor<To, From>::operator[](size_type idx)
    {
        SPARROW_ASSERT_TRUE(idx < size());
        return data()[idx];
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::const_reference
    buffer_adaptor<To, From>::operator[](size_type idx) const
    {
        SPARROW_ASSERT_TRUE(idx < size());
        return data()[idx];
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::reference buffer_adaptor<To, From>::front()
    {
        SPARROW_ASSERT_TRUE(!empty());
        return operator[](0);
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::const_reference buffer_adaptor<To, From>::front() const
    {
        SPARROW_ASSERT_TRUE(!empty());
        return operator[](0);
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::reference buffer_adaptor<To, From>::back()
    {
        SPARROW_ASSERT_TRUE(!empty());
        return operator[](size() - 1);
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::const_reference buffer_adaptor<To, From>::back() const
    {
        SPARROW_ASSERT_TRUE(!empty());
        return operator[](size() - 1);
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::iterator buffer_adaptor<To, From>::begin() noexcept
    {
        return iterator(data());
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::iterator buffer_adaptor<To, From>::end() noexcept
    {
        return iterator(data() + size());
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::const_iterator buffer_adaptor<To, From>::begin() const noexcept
    {
        return const_iterator(data());
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::const_iterator buffer_adaptor<To, From>::end() const noexcept
    {
        return const_iterator(data() + size());
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::const_iterator buffer_adaptor<To, From>::cbegin() const noexcept
    {
        return begin();
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::const_iterator buffer_adaptor<To, From>::cend() const noexcept
    {
        return end();
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::reverse_iterator buffer_adaptor<To, From>::rbegin() noexcept
    {
        return reverse_iterator(end());
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::reverse_iterator buffer_adaptor<To, From>::rend() noexcept
    {
        return reverse_iterator(begin());
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::const_reverse_iterator
    buffer_adaptor<To, From>::rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::const_reverse_iterator
    buffer_adaptor<To, From>::rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::const_reverse_iterator
    buffer_adaptor<To, From>::crbegin() const noexcept
    {
        return rbegin();
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::const_reverse_iterator
    buffer_adaptor<To, From>::crend() const noexcept
    {
        return rend();
    }

    // Capacity

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr buffer_adaptor<To, From>::size_type buffer_adaptor<To, From>::size() const noexcept
    {
        const double new_size = static_cast<double>(m_buffer.size()) * m_from_to_size_ratio;
        SPARROW_ASSERT(
            std::trunc(new_size) == new_size,
            "The size of the buffer is not a multiple of the size of the new type"
        );
        return static_cast<size_type>(new_size);
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::size_type buffer_adaptor<To, From>::max_size() const noexcept
    {
        return m_max_size;
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::size_type buffer_adaptor<To, From>::capacity() const noexcept
    {
        return m_buffer.capacity() / m_to_from_size_ratio;
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr bool buffer_adaptor<To, From>::empty() const noexcept
    {
        return m_buffer.empty();
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr void buffer_adaptor<To, From>::reserve(size_type new_cap)
    {
        m_buffer.reserve(new_cap * m_to_from_size_ratio);
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr void buffer_adaptor<To, From>::shrink_to_fit()
    {
        m_buffer.shrink_to_fit();
    }

    // Modifiers

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr void buffer_adaptor<To, From>::clear() noexcept
    {
        m_buffer.clear();
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::iterator
    buffer_adaptor<To, From>::insert(const_iterator pos, const value_type& value)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const difference_type index = std::distance(cbegin(), pos);
        const auto buffer_pos = get_buffer_reference_iterator(pos);
        m_buffer.insert(buffer_pos, m_to_from_size_ratio, 0);
        operator[](static_cast<size_type>(index)) = value;
        return std::next(begin(), index);
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::iterator
    buffer_adaptor<To, From>::insert(const_iterator pos, size_type count, const value_type& value)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const difference_type index = std::distance(cbegin(), pos);
        const auto buffer_pos = get_buffer_reference_iterator(pos);
        m_buffer.insert(buffer_pos, count * m_to_from_size_ratio, 0);
        for (size_type i = 0; i < count; ++i)
        {
            data()[static_cast<size_t>(index) + i] = value;
        }
        return std::next(begin(), index);
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    template <class InputIt>
        requires std::input_iterator<InputIt>
    constexpr typename buffer_adaptor<To, From>::iterator
    buffer_adaptor<To, From>::insert(const_iterator pos, InputIt first, InputIt last)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const difference_type index = std::distance(cbegin(), pos);
        const difference_type count = std::distance(first, last);
        const auto buffer_pos = get_buffer_reference_iterator(pos);
        m_buffer.insert(buffer_pos, static_cast<size_type>(count) * m_to_from_size_ratio, 0);
        std::copy(first, last, data() + index);
        return std::next(begin(), index);
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::iterator
    buffer_adaptor<To, From>::insert(const_iterator pos, std::initializer_list<value_type> ilist)
    {
        return insert(pos, ilist.begin(), ilist.end());
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    template <class... Args>
    constexpr typename buffer_adaptor<To, From>::iterator
    buffer_adaptor<To, From>::emplace(const_iterator pos, Args&&... args)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const difference_type index = std::distance(cbegin(), pos);
        const auto buffer_pos = get_buffer_reference_iterator(pos);
        m_buffer.insert(buffer_pos, m_to_from_size_ratio, 0);
        data()[index] = value_type(std::forward<Args>(args)...);
        return std::next(begin(), index);
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::iterator buffer_adaptor<To, From>::erase(const_iterator pos)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        if (empty())
        {
            return begin();
        }
        const difference_type index = std::distance(cbegin(), pos);
        const auto idx_for_buffer = index_for_buffer(static_cast<size_type>(index));
        SPARROW_ASSERT_TRUE(idx_for_buffer < m_buffer.size());

        m_buffer.erase(
            std::next(m_buffer.cbegin(), static_cast<difference_type>(idx_for_buffer)),
            std::next(m_buffer.cbegin(), static_cast<difference_type>(idx_for_buffer + m_to_from_size_ratio))
        );
        return std::next(begin(), index);
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr typename buffer_adaptor<To, From>::iterator
    buffer_adaptor<To, From>::erase(const_iterator first, const_iterator last)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= first);
        SPARROW_ASSERT_TRUE(last <= cend());
        if (empty())
        {
            return begin();
        }
        const difference_type index_first = std::distance(cbegin(), first);
        const difference_type index_last = std::distance(cbegin(), last);
        const auto idx_for_buffer_first = index_for_buffer(static_cast<size_type>(index_first));
        SPARROW_ASSERT_TRUE(idx_for_buffer_first < m_buffer.size());
        const auto idx_for_buffer_last = index_for_buffer(static_cast<size_type>(index_last));
        SPARROW_ASSERT_TRUE(idx_for_buffer_last <= m_buffer.size());
        m_buffer.erase(
            std::next(m_buffer.cbegin(), static_cast<difference_type>(idx_for_buffer_first)),
            std::next(m_buffer.cbegin(), static_cast<difference_type>(idx_for_buffer_last))
        );
        return std::next(begin(), index_first);
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr void buffer_adaptor<To, From>::push_back(const value_type& value)
    {
        insert(cend(), value);
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr void buffer_adaptor<To, From>::pop_back()
    {
        erase(std::prev(cend()));
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr void buffer_adaptor<To, From>::resize(size_type new_size)
    {
        const auto new_size_for_buffer = static_cast<size_type>(
            static_cast<double>(new_size) / m_from_to_size_ratio
        );
        m_buffer.resize(new_size_for_buffer);
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    constexpr void buffer_adaptor<To, From>::resize(size_type new_size, const value_type& value)
    {
        const size_type original_size = size();
        const auto new_size_for_buffer = static_cast<size_type>(
            static_cast<double>(new_size) / m_from_to_size_ratio
        );
        m_buffer.resize(new_size_for_buffer, 0);
        for (size_type i = original_size; i < new_size; ++i)
        {
            operator[](i) = value;
        }
    }

    template <typename To, typename From>
        requires(sizeof(From) < sizeof(To))
    buffer_adaptor<To, From>::buffer_reference::const_iterator
    buffer_adaptor<To, From>::get_buffer_reference_iterator(const_iterator pos)
    {
        const difference_type index = std::distance(cbegin(), pos);
        const auto idx_for_buffer = index_for_buffer(static_cast<size_type>(index));
        SPARROW_ASSERT_TRUE(idx_for_buffer <= m_buffer.size());
        return std::next(m_buffer.cbegin(), static_cast<difference_type>(idx_for_buffer));
    }
}
