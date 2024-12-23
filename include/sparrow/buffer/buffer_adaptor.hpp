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
#include <ranges>
#include <type_traits>

#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    template <typename FromBufferRef, typename T>
    concept BufferReference = std::ranges::contiguous_range<FromBufferRef> && std::is_reference_v<FromBufferRef>
                              && (sizeof(std::ranges::range_value_t<FromBufferRef>) <= sizeof(T));

    template <typename FromBufferRef, typename T>
    concept T_is_const_if_FromBufferRef_is_const = mpl::T_matches_qualifier_if_Y_is<T, FromBufferRef, std::is_const>;

    /**
     * Class which have internally a reference to a contiguous container of a certain type and provides an API
     * to access it as if it was a buffer<T>.
     * @tparam To The type to which the buffer will be adapted. The size of the type To must be equal of
     * bigger than the element type of the container.
     * @tparam FromBufferRef The type of the container to adapt. If it's a const reference, all non-const
     * methods are disabled.
     */
    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    class buffer_adaptor
    {
    public:

        using value_type = To;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        static constexpr bool is_const = std::is_const_v<To>;

        using buffer_reference_value_type = std::remove_cvref_t<FromBufferRef>::value_type;
        using buffer_reference = std::conditional_t<is_const, const FromBufferRef, FromBufferRef>;

        using size_type = std::remove_cvref_t<buffer_reference>::size_type;
        using difference_type = std::remove_cvref_t<buffer_reference>::difference_type;
        using iterator = pointer_iterator<pointer>;
        using const_iterator = pointer_iterator<const_pointer>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        explicit buffer_adaptor(FromBufferRef buf)
            requires(not is_const);
        explicit buffer_adaptor(const FromBufferRef buf);

        constexpr pointer data() noexcept
            requires(not is_const);
        constexpr const_pointer data() const noexcept;

        // Element access

        constexpr reference operator[](size_type idx)
            requires(not is_const);
        constexpr const_reference operator[](size_type idx) const;

        constexpr reference front()
            requires(not is_const);
        constexpr const_reference front() const;

        constexpr reference back()
            requires(not is_const);
        constexpr const_reference back() const;

        // Iterators

        constexpr iterator begin() noexcept
            requires(not is_const);
        constexpr iterator end() noexcept
            requires(not is_const);

        constexpr const_iterator begin() const noexcept;
        constexpr const_iterator end() const noexcept;

        constexpr const_iterator cbegin() const noexcept;
        constexpr const_iterator cend() const noexcept;

        constexpr reverse_iterator rbegin() noexcept
            requires(not is_const);
        constexpr reverse_iterator rend() noexcept
            requires(not is_const);

        constexpr const_reverse_iterator rbegin() const noexcept;
        constexpr const_reverse_iterator rend() const noexcept;

        constexpr const_reverse_iterator crbegin() const noexcept;
        constexpr const_reverse_iterator crend() const noexcept;

        // Capacity

        [[nodiscard]] constexpr size_type size() const noexcept;
        [[nodiscard]] constexpr size_type max_size() const noexcept;
        [[nodiscard]] constexpr size_type capacity() const noexcept;
        [[nodiscard]] constexpr bool empty() const noexcept;
        constexpr void reserve(size_type new_cap)
            requires(not is_const);
        constexpr void shrink_to_fit()
            requires(not is_const);

        // Modifiers

        constexpr void clear() noexcept
            requires(not is_const);

        constexpr iterator insert(const_iterator pos, const value_type& value)
            requires(not is_const);
        constexpr iterator insert(const_iterator pos, size_type count, const value_type& value)
            requires(not is_const);

        template <class InputIt>
            requires std::input_iterator<InputIt>
        constexpr iterator insert(const_iterator pos, InputIt first, InputIt last)
            requires(not is_const);
        constexpr iterator insert(const_iterator pos, std::initializer_list<value_type> ilist)
            requires(not is_const);

        template <class... Args>
        constexpr iterator emplace(const_iterator pos, Args&&... args)
            requires(not is_const);

        constexpr iterator erase(const_iterator pos)
            requires(not is_const);
        constexpr iterator erase(const_iterator first, const_iterator last)
            requires(not is_const);

        constexpr void push_back(const value_type& value)
            requires(not is_const);

        constexpr void pop_back()
            requires(not is_const);

        constexpr void resize(size_type new_size)
            requires(not is_const);

        constexpr void resize(size_type new_size, const value_type& value)
            requires(not is_const);

    private:

        static constexpr double m_from_to_size_ratio = static_cast<double>(sizeof(buffer_reference_value_type))
                                                       / static_cast<double>(sizeof(value_type));

        static constexpr size_type m_to_from_size_ratio = static_cast<size_type>(1. / m_from_to_size_ratio);

        [[nodiscard]] constexpr std::remove_cvref_t<buffer_reference>::size_type
        index_for_buffer(size_type idx) const noexcept
        {
            return idx * m_to_from_size_ratio;
        }

        [[nodiscard]] std::remove_cvref_t<buffer_reference>::const_iterator
        get_buffer_reference_iterator(const_iterator pos)
        {
            const difference_type index = std::distance(cbegin(), pos);
            const auto idx_for_buffer = index_for_buffer(static_cast<size_type>(index));
            SPARROW_ASSERT_TRUE(idx_for_buffer <= m_buffer.size());
            return std::next(m_buffer.cbegin(), static_cast<difference_type>(idx_for_buffer));
        }

        buffer_reference m_buffer;
        size_type m_max_size;
    };

    template <class T>
    class holder
    {
    public:

        template <class... Args>
        holder(Args&&... args)
            : value(std::forward<Args>(args)...)
        {
        }

        T value;
    };

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    buffer_adaptor<To, FromBufferRef>::buffer_adaptor(FromBufferRef buf)
        requires(not is_const)
        : m_buffer(buf)
        , m_max_size(m_buffer.max_size() / m_to_from_size_ratio)
    {
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    buffer_adaptor<To, FromBufferRef>::buffer_adaptor(const FromBufferRef buf)
        : m_buffer(buf)
        , m_max_size(m_buffer.max_size() / m_to_from_size_ratio)
    {
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::pointer
    buffer_adaptor<To, FromBufferRef>::data() noexcept
        requires(not is_const)
    {
        return m_buffer.template data<value_type>();
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::const_pointer
    buffer_adaptor<To, FromBufferRef>::data() const noexcept
    {
        return m_buffer.template data<value_type>();
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::reference
    buffer_adaptor<To, FromBufferRef>::operator[](size_type idx)
        requires(not is_const)
    {
        SPARROW_ASSERT_TRUE(idx < size());
        return data()[idx];
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::const_reference
    buffer_adaptor<To, FromBufferRef>::operator[](size_type idx) const
    {
        SPARROW_ASSERT_TRUE(idx < size());
        return data()[idx];
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::reference buffer_adaptor<To, FromBufferRef>::front()
        requires(not is_const)
    {
        SPARROW_ASSERT_TRUE(!empty());
        return operator[](0);
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::const_reference
    buffer_adaptor<To, FromBufferRef>::front() const
    {
        SPARROW_ASSERT_TRUE(!empty());
        return operator[](0);
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::reference buffer_adaptor<To, FromBufferRef>::back()
        requires(not is_const)
    {
        SPARROW_ASSERT_TRUE(!empty());
        return operator[](size() - 1);
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::const_reference
    buffer_adaptor<To, FromBufferRef>::back() const
    {
        SPARROW_ASSERT_TRUE(!empty());
        return operator[](size() - 1);
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::iterator
    buffer_adaptor<To, FromBufferRef>::begin() noexcept
        requires(not is_const)
    {
        return iterator(data());
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::iterator
    buffer_adaptor<To, FromBufferRef>::end() noexcept
        requires(not is_const)
    {
        return iterator(data() + size());
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::const_iterator
    buffer_adaptor<To, FromBufferRef>::begin() const noexcept
    {
        return const_iterator(data());
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::const_iterator
    buffer_adaptor<To, FromBufferRef>::end() const noexcept
    {
        return const_iterator(data() + size());
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::const_iterator
    buffer_adaptor<To, FromBufferRef>::cbegin() const noexcept
    {
        return begin();
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::const_iterator
    buffer_adaptor<To, FromBufferRef>::cend() const noexcept
    {
        return end();
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::reverse_iterator
    buffer_adaptor<To, FromBufferRef>::rbegin() noexcept
        requires(not is_const)
    {
        return reverse_iterator(end());
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::reverse_iterator
    buffer_adaptor<To, FromBufferRef>::rend() noexcept
        requires(not is_const)
    {
        return reverse_iterator(begin());
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::const_reverse_iterator
    buffer_adaptor<To, FromBufferRef>::rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::const_reverse_iterator
    buffer_adaptor<To, FromBufferRef>::rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::const_reverse_iterator
    buffer_adaptor<To, FromBufferRef>::crbegin() const noexcept
    {
        return rbegin();
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::const_reverse_iterator
    buffer_adaptor<To, FromBufferRef>::crend() const noexcept
    {
        return rend();
    }

    // Capacity

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr buffer_adaptor<To, FromBufferRef>::size_type
    buffer_adaptor<To, FromBufferRef>::size() const noexcept
    {
        const double new_size = static_cast<double>(m_buffer.size()) * m_from_to_size_ratio;
        SPARROW_ASSERT(
            std::trunc(new_size) == new_size,
            "The size of the buffer is not a multiple of the size of the new type"
        );
        return static_cast<size_type>(new_size);
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::size_type
    buffer_adaptor<To, FromBufferRef>::max_size() const noexcept
    {
        return m_max_size;
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::size_type
    buffer_adaptor<To, FromBufferRef>::capacity() const noexcept
    {
        return m_buffer.capacity() / m_to_from_size_ratio;
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr bool buffer_adaptor<To, FromBufferRef>::empty() const noexcept
    {
        return m_buffer.empty();
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr void buffer_adaptor<To, FromBufferRef>::reserve(size_type new_cap)
        requires(not is_const)
    {
        m_buffer.reserve(new_cap * m_to_from_size_ratio);
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr void buffer_adaptor<To, FromBufferRef>::shrink_to_fit()
        requires(not is_const)
    {
        m_buffer.shrink_to_fit();
    }

    // Modifiers

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr void buffer_adaptor<To, FromBufferRef>::clear() noexcept
        requires(not is_const)
    {
        m_buffer.clear();
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::iterator
    buffer_adaptor<To, FromBufferRef>::insert(const_iterator pos, const value_type& value)
        requires(not is_const)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const difference_type index = std::distance(cbegin(), pos);
        const auto buffer_pos = get_buffer_reference_iterator(pos);
        m_buffer.insert(buffer_pos, m_to_from_size_ratio, 0);
        operator[](static_cast<size_type>(index)) = value;
        return std::next(begin(), index);
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::iterator
    buffer_adaptor<To, FromBufferRef>::insert(const_iterator pos, size_type count, const value_type& value)
        requires(not is_const)
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

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    template <class InputIt>
        requires std::input_iterator<InputIt>
    constexpr typename buffer_adaptor<To, FromBufferRef>::iterator
    buffer_adaptor<To, FromBufferRef>::insert(const_iterator pos, InputIt first, InputIt last)
        requires(not is_const)
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

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::iterator
    buffer_adaptor<To, FromBufferRef>::insert(const_iterator pos, std::initializer_list<value_type> ilist)
        requires(not is_const)
    {
        return insert(pos, ilist.begin(), ilist.end());
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    template <class... Args>
    constexpr typename buffer_adaptor<To, FromBufferRef>::iterator
    buffer_adaptor<To, FromBufferRef>::emplace(const_iterator pos, Args&&... args)
        requires(not is_const)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const difference_type index = std::distance(cbegin(), pos);
        const auto buffer_pos = get_buffer_reference_iterator(pos);
        m_buffer.insert(buffer_pos, m_to_from_size_ratio, 0);
        data()[index] = value_type(std::forward<Args>(args)...);
        return std::next(begin(), index);
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::iterator
    buffer_adaptor<To, FromBufferRef>::erase(const_iterator pos)
        requires(not is_const)
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

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr typename buffer_adaptor<To, FromBufferRef>::iterator
    buffer_adaptor<To, FromBufferRef>::erase(const_iterator first, const_iterator last)
        requires(not is_const)
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

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr void buffer_adaptor<To, FromBufferRef>::push_back(const value_type& value)
        requires(not is_const)
    {
        insert(cend(), value);
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr void buffer_adaptor<To, FromBufferRef>::pop_back()
        requires(not is_const)
    {
        erase(std::prev(cend()));
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr void buffer_adaptor<To, FromBufferRef>::resize(size_type new_size)
        requires(not is_const)
    {
        const auto new_size_for_buffer = static_cast<size_type>(
            static_cast<double>(new_size) / m_from_to_size_ratio
        );
        m_buffer.resize(new_size_for_buffer);
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr void buffer_adaptor<To, FromBufferRef>::resize(size_type new_size, const value_type& value)
        requires(not is_const)
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

    template <typename To, class FromBufferRef>
    auto make_buffer_adaptor(FromBufferRef& buf)
    {
        constexpr bool is_const = std::is_const_v<FromBufferRef>;
        using RealToType = std::conditional_t<is_const, const To, To>;
        return buffer_adaptor<RealToType, decltype(buf)&>(buf);
    }
}
