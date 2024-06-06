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
     * @brief Class which have internally a reference to a buffer<T> and provides the API of a buffer<U>
     */
    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    class buffer_adaptor
    {
    public:

        using value_type = T;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;

        using buffer_reference_value_type = U;
        using buffer_reference = buffer<buffer_reference_value_type>;

        using size_type = buffer_reference::size_type;
        using difference_type = buffer_reference::difference_type;
        using iterator = pointer_iterator<pointer>;
        using const_iterator = pointer_iterator<const_pointer>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        buffer_adaptor(buffer_reference& buf)
            : _buffer(buf)
        {
        }

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
        // constexpr void swap(buffer& rhs) noexcept;

    private:

        static constexpr double sizeof_ratio = static_cast<double>(sizeof(buffer_reference_value_type))
                                               / static_cast<double>(sizeof(value_type));

        static constexpr size_type _extra_buffervalue_type_size = sizeof(buffer_reference_value_type)
                                                                  % sizeof(value_type);

        [[nodiscard]] constexpr buffer_reference::size_type index_for_buffer(size_type idx) const noexcept;

        buffer_reference& _buffer;
    };

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::buffer_reference::size_type
    buffer_adaptor<T, U>::index_for_buffer(size_type idx) const noexcept
    {
        auto idx_for_T = static_cast<buffer_reference::size_type>(static_cast<double>(idx) / sizeof_ratio);
        return idx_for_T;
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::pointer buffer_adaptor<T, U>::data() noexcept
    {
        return _buffer.template data<value_type>();
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::const_pointer buffer_adaptor<T, U>::data() const noexcept
    {
        return _buffer.template data<value_type>();
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::reference buffer_adaptor<T, U>::operator[](size_type idx)
    {
        SPARROW_ASSERT_TRUE(idx < size());
        return data()[idx];
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::const_reference
    buffer_adaptor<T, U>::operator[](size_type idx) const
    {
        SPARROW_ASSERT_TRUE(idx < size());
        return data()[idx];
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::reference buffer_adaptor<T, U>::front()
    {
        return operator[](0);
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::const_reference buffer_adaptor<T, U>::front() const
    {
        return operator[](0);
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::reference buffer_adaptor<T, U>::back()
    {
        return operator[](size() - 1);
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::const_reference buffer_adaptor<T, U>::back() const
    {
        return operator[](size() - 1);
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::iterator buffer_adaptor<T, U>::begin() noexcept
    {
        return iterator(data());
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::iterator buffer_adaptor<T, U>::end() noexcept
    {
        return iterator(data() + size());
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::const_iterator buffer_adaptor<T, U>::begin() const noexcept
    {
        return const_iterator(data());
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::const_iterator buffer_adaptor<T, U>::end() const noexcept
    {
        return const_iterator(data() + size());
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::const_iterator buffer_adaptor<T, U>::cbegin() const noexcept
    {
        return begin();
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::const_iterator buffer_adaptor<T, U>::cend() const noexcept
    {
        return end();
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::reverse_iterator buffer_adaptor<T, U>::rbegin() noexcept
    {
        return reverse_iterator(end());
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::reverse_iterator buffer_adaptor<T, U>::rend() noexcept
    {
        return reverse_iterator(begin());
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::const_reverse_iterator buffer_adaptor<T, U>::rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::const_reverse_iterator buffer_adaptor<T, U>::rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::const_reverse_iterator
    buffer_adaptor<T, U>::crbegin() const noexcept
    {
        return rbegin();
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::const_reverse_iterator buffer_adaptor<T, U>::crend() const noexcept
    {
        return rend();
    }

    // Capacity

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr buffer_adaptor<T, U>::size_type buffer_adaptor<T, U>::size() const noexcept
    {
        const double new_size = static_cast<double>(_buffer.size()) * sizeof_ratio;
        SPARROW_ASSERT(
            std::trunc(new_size) == new_size,
            "The size of the buffer is not a multiple of the size of the new type"
        );
        return static_cast<size_type>(new_size);
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::size_type buffer_adaptor<T, U>::max_size() const noexcept
    {
        return static_cast<size_type>(static_cast<double>(_buffer.max_size()) * sizeof_ratio);
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::size_type buffer_adaptor<T, U>::capacity() const noexcept
    {
        return static_cast<size_type>(static_cast<double>(_buffer.capacity()) * sizeof_ratio);
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr bool buffer_adaptor<T, U>::empty() const noexcept
    {
        return _buffer.empty();
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr void buffer_adaptor<T, U>::reserve(size_type new_cap)
    {
        _buffer.reserve(static_cast<size_type>(static_cast<double>(new_cap) / sizeof_ratio));
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr void buffer_adaptor<T, U>::shrink_to_fit()
    {
        _buffer.shrink_to_fit();
    }

    // Modifiers

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr void buffer_adaptor<T, U>::clear() noexcept
    {
        _buffer.clear();
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::iterator
    buffer_adaptor<T, U>::insert(const_iterator pos, const value_type& value)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const difference_type index = std::distance(cbegin(), pos);
        const auto idx_for_buffer = index_for_buffer(static_cast<size_type>(index));
        const auto type_size = static_cast<difference_type>(1. / sizeof_ratio);
        _buffer.insert(std::next(_buffer.cbegin(), static_cast<difference_type>(idx_for_buffer)), type_size, 0);
        operator[](static_cast<size_type>(index)) = value;
        return std::next(begin(), index);
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::iterator
    buffer_adaptor<T, U>::insert(const_iterator pos, size_type count, const value_type& value)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const difference_type index = std::distance(cbegin(), pos);
        const auto idx_for_buffer = index_for_buffer(static_cast<size_type>(index));
        constexpr auto type_size = static_cast<size_type>(1. / sizeof_ratio);
        const auto buffer_pos = std::next(_buffer.cbegin(), static_cast<difference_type>(idx_for_buffer));
        _buffer.insert(buffer_pos, count * type_size, 0);
        for (size_type i = 0; i < count; ++i)
        {
            data()[static_cast<size_t>(index) + i] = value;
        }
        return std::next(begin(), index);
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    template <class InputIt>
        requires std::input_iterator<InputIt>
    constexpr typename buffer_adaptor<T, U>::iterator
    buffer_adaptor<T, U>::insert(const_iterator pos, InputIt first, InputIt last)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const difference_type index = std::distance(cbegin(), pos);
        const auto idx_for_buffer = index_for_buffer(static_cast<size_t>(index));
        constexpr auto type_size = static_cast<size_type>(1. / sizeof_ratio);
        const difference_type count = std::distance(first, last);
        const auto buffer_pos = std::next(_buffer.cbegin(), static_cast<difference_type>(idx_for_buffer));
        _buffer.insert(buffer_pos, static_cast<size_type>(count) * type_size, 0);
        std::copy(first, last, data() + index);
        return std::next(begin(), index);
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::iterator
    buffer_adaptor<T, U>::insert(const_iterator pos, std::initializer_list<value_type> ilist)
    {
        return insert(pos, ilist.begin(), ilist.end());
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    template <class... Args>
    constexpr typename buffer_adaptor<T, U>::iterator
    buffer_adaptor<T, U>::emplace(const_iterator pos, Args&&... args)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const difference_type index = std::distance(cbegin(), pos);
        const auto idx_for_buffer = index_for_buffer(static_cast<size_type>(index));
        constexpr auto type_size = static_cast<size_type>(1. / sizeof_ratio);
        const auto buffer_pos = std::next(_buffer.cbegin(), static_cast<difference_type>(idx_for_buffer));
        _buffer.insert(buffer_pos, type_size, 0);
        data()[index] = value_type(std::forward<Args>(args)...);
        return std::next(begin(), index);
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::iterator buffer_adaptor<T, U>::erase(const_iterator pos)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const difference_type index = std::distance(cbegin(), pos);
        const auto idx_for_buffer = index_for_buffer(static_cast<size_type>(index));
        constexpr auto type_size = static_cast<buffer_reference::difference_type>(1. / sizeof_ratio);
        _buffer.erase(
            std::next(_buffer.cbegin(), static_cast<difference_type>(idx_for_buffer)),
            std::next(_buffer.cbegin(), static_cast<difference_type>(idx_for_buffer) + type_size)
        );
        return std::next(begin(), index);
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr typename buffer_adaptor<T, U>::iterator
    buffer_adaptor<T, U>::erase(const_iterator first, const_iterator last)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= first);
        SPARROW_ASSERT_TRUE(last <= cend());
        const difference_type index_first = std::distance(cbegin(), first);
        const difference_type index_last = std::distance(cbegin(), last);
        const auto idx_for_buffer_first = index_for_buffer(static_cast<size_type>(index_first));
        const auto idx_for_buffer_last = index_for_buffer(static_cast<size_type>(index_last));
        _buffer.erase(
            std::next(_buffer.cbegin(), static_cast<difference_type>(idx_for_buffer_first)),
            std::next(_buffer.cbegin(), static_cast<difference_type>(idx_for_buffer_last))
        );
        return std::next(begin(), index_first);
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr void buffer_adaptor<T, U>::push_back(const value_type& value)
    {
        insert(cend(), value);
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr void buffer_adaptor<T, U>::pop_back()
    {
        erase(std::prev(cend()));
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr void buffer_adaptor<T, U>::resize(size_type new_size)
    {
        const auto new_size_for_buffer = static_cast<size_type>(static_cast<double>(new_size) / sizeof_ratio);
        _buffer.resize(new_size_for_buffer);
    }

    template <typename T, typename U>
        requires(sizeof(U) < sizeof(T))
    constexpr void buffer_adaptor<T, U>::resize(size_type new_size, const value_type& value)
    {
        const size_type current_size = size();
        const auto new_size_for_buffer = static_cast<size_type>(static_cast<double>(new_size) / sizeof_ratio);
        _buffer.resize(new_size_for_buffer, 0);

        for (size_type i = current_size; i < new_size; ++i)
        {
            operator[](i) = value;
        }
    }
}
