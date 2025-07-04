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
    /**
     * Concept that checks if a type is a buffer reference suitable for adaptation.
     *
     * @tparam FromBufferRef The buffer reference type to check.
     * @tparam T The target element type.
     */
    template <typename FromBufferRef, typename T>
    concept BufferReference = std::ranges::contiguous_range<FromBufferRef> && std::is_reference_v<FromBufferRef>
                              && (sizeof(std::ranges::range_value_t<FromBufferRef>) <= sizeof(T));

    /**
     * Concept that ensures T is const if FromBufferRef is const.
     *
     * @tparam FromBufferRef The buffer reference type.
     * @tparam T The target type.
     */
    template <typename FromBufferRef, typename T>
    concept T_is_const_if_FromBufferRef_is_const = mpl::T_matches_qualifier_if_Y_is<T, FromBufferRef, std::is_const>;

    /**
     * Class which has internally a reference to a contiguous container of a certain type and provides an API
     * to access it as if it was a buffer<T>.
     *
     * @tparam To The type to which the buffer will be adapted. The size of the type To must be equal or
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

        /**
         * Constructs a buffer adaptor with a non-const buffer reference.
         *
         * @param buf The buffer reference to adapt.
         */
        constexpr explicit buffer_adaptor(FromBufferRef buf)
            requires(not is_const);

        /**
         * Constructs a buffer adaptor with a const buffer reference.
         *
         * @param buf The const buffer reference to adapt.
         */
        constexpr explicit buffer_adaptor(const FromBufferRef buf);

        /**
         * Returns a pointer to the underlying data.
         *
         * @return Pointer to the data.
         */
        [[nodiscard]] constexpr pointer data() noexcept
            requires(not is_const);

        /**
         * Returns a constant pointer to the underlying data.
         *
         * @return Constant pointer to the data.
         */
        [[nodiscard]] constexpr const_pointer data() const noexcept;

        // Element access

        /**
         * Returns a reference to the element at the specified index.
         *
         * @param idx The index of the element.
         * @return Reference to the element.
         */
        [[nodiscard]] constexpr reference operator[](size_type idx)
            requires(not is_const);

        /**
         * Returns a constant reference to the element at the specified index.
         *
         * @param idx The index of the element.
         * @return Constant reference to the element.
         */
        [[nodiscard]] constexpr const_reference operator[](size_type idx) const;

        /**
         * Returns a reference to the first element.
         *
         * @return Reference to the first element.
         */
        [[nodiscard]] constexpr reference front()
            requires(not is_const);

        /**
         * Returns a constant reference to the first element.
         *
         * @return Constant reference to the first element.
         */
        [[nodiscard]] constexpr const_reference front() const;

        /**
         * Returns a reference to the last element.
         *
         * @return Reference to the last element.
         */
        [[nodiscard]] constexpr reference back()
            requires(not is_const);

        /**
         * Returns a constant reference to the last element.
         *
         * @return Constant reference to the last element.
         */
        [[nodiscard]] constexpr const_reference back() const;

        // Iterators

        /**
         * Returns an iterator to the beginning.
         *
         * @return Iterator to the beginning.
         */
        [[nodiscard]] constexpr iterator begin() noexcept
            requires(not is_const);

        /**
         * Returns an iterator to the end.
         *
         * @return Iterator to the end.
         */
        [[nodiscard]] constexpr iterator end() noexcept
            requires(not is_const);

        /**
         * Returns a constant iterator to the beginning.
         *
         * @return Constant iterator to the beginning.
         */
        [[nodiscard]] constexpr const_iterator begin() const noexcept;

        /**
         * Returns a constant iterator to the end.
         *
         * @return Constant iterator to the end.
         */
        [[nodiscard]] constexpr const_iterator end() const noexcept;

        /**
         * Returns a constant iterator to the beginning.
         *
         * @return Constant iterator to the beginning.
         */
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept;

        /**
         * Returns a constant iterator to the end.
         *
         * @return Constant iterator to the end.
         */
        [[nodiscard]] constexpr const_iterator cend() const noexcept;

        /**
         * Returns a reverse iterator to the beginning.
         *
         * @return Reverse iterator to the beginning.
         */
        [[nodiscard]] constexpr reverse_iterator rbegin() noexcept
            requires(not is_const);

        /**
         * Returns a reverse iterator to the end.
         *
         * @return Reverse iterator to the end.
         */
        [[nodiscard]] constexpr reverse_iterator rend() noexcept
            requires(not is_const);

        /**
         * Returns a constant reverse iterator to the beginning.
         *
         * @return Constant reverse iterator to the beginning.
         */
        [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept;

        /**
         * Returns a constant reverse iterator to the end.
         *
         * @return Constant reverse iterator to the end.
         */
        [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept;

        /**
         * Returns a constant reverse iterator to the beginning.
         *
         * @return Constant reverse iterator to the beginning.
         */
        [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept;

        /**
         * Returns a constant reverse iterator to the end.
         *
         * @return Constant reverse iterator to the end.
         */
        [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept;

        // Capacity

        /**
         * Returns the number of elements that can be held in currently allocated storage.
         *
         * @return The capacity.
         */
        [[nodiscard]] constexpr size_type size() const noexcept(!SPARROW_CONTRACTS_THROW_ON_FAILURE);
        /**
         * Returns the maximum possible number of elements.
         *
         * @return The maximum possible number of elements.
         */
        [[nodiscard]] constexpr size_type max_size() const noexcept;

        /**
         * Returns the number of elements that can be held in currently allocated storage.
         *
         * @return The capacity.
         */
        [[nodiscard]] constexpr size_type capacity() const noexcept;

        /**
         * Checks whether the container is empty.
         *
         * @return true if the container is empty, false otherwise.
         */
        [[nodiscard]] constexpr bool empty() const noexcept;

        /**
         * Reserves storage for at least the specified number of elements.
         *
         * @param new_cap The new capacity.
         */
        constexpr void reserve(size_type new_cap)
            requires(not is_const);

        /**
         * Requests the removal of unused capacity.
         */
        constexpr void shrink_to_fit()
            requires(not is_const);

        // Modifiers

        /**
         * Clears the contents.
         */
        constexpr void clear() noexcept
            requires(not is_const);

        /**
         * Inserts an element at the specified position.
         *
         * @param pos The position to insert at.
         * @param value The value to insert.
         * @return Iterator pointing to the inserted element.
         */
        constexpr iterator insert(const_iterator pos, const value_type& value)
            requires(not is_const);

        /**
         * Inserts multiple copies of an element at the specified position.
         *
         * @param pos The position to insert at.
         * @param count The number of elements to insert.
         * @param value The value to insert.
         * @return Iterator pointing to the first inserted element.
         */
        constexpr iterator insert(const_iterator pos, size_type count, const value_type& value)
            requires(not is_const);

        /**
         * Inserts elements from a range at the specified position.
         *
         * @tparam InputIt The iterator type.
         * @param pos The position to insert at.
         * @param first The beginning of the range to insert.
         * @param last The end of the range to insert.
         * @return Iterator pointing to the first inserted element.
         */
        template <class InputIt>
            requires std::input_iterator<InputIt>
        constexpr iterator insert(const_iterator pos, InputIt first, InputIt last)
            requires(not is_const);

        /**
         * Inserts elements from an initializer list at the specified position.
         *
         * @param pos The position to insert at.
         * @param ilist The initializer list to insert.
         * @return Iterator pointing to the first inserted element.
         */
        constexpr iterator insert(const_iterator pos, std::initializer_list<value_type> ilist)
            requires(not is_const);

        /**
         * Constructs an element in-place at the specified position.
         *
         * @tparam Args The argument types.
         * @param pos The position to emplace at.
         * @param args Arguments to forward to the constructor.
         * @return Iterator pointing to the emplaced element.
         */
        template <class... Args>
        constexpr iterator emplace(const_iterator pos, Args&&... args)
            requires(not is_const);

        /**
         * Erases an element at the specified position.
         *
         * @param pos The position of the element to erase.
         * @return Iterator following the last element removed.
         */
        constexpr iterator erase(const_iterator pos)
            requires(not is_const);

        /**
         * Erases elements in the specified range.
         *
         * @param first The beginning of the range to erase.
         * @param last The end of the range to erase.
         * @return Iterator following the last element removed.
         */
        constexpr iterator erase(const_iterator first, const_iterator last)
            requires(not is_const);

        /**
         * Adds an element to the end.
         *
         * @param value The value to add.
         */
        constexpr void push_back(const value_type& value)
            requires(not is_const);

        /**
         * Removes the last element.
         */
        constexpr void pop_back()
            requires(not is_const);

        /**
         * Changes the number of elements stored.
         *
         * @param new_size The new size.
         */
        constexpr void resize(size_type new_size)
            requires(not is_const);

        /**
         * Changes the number of elements stored, filling new elements with the specified value.
         *
         * @param new_size The new size.
         * @param value The value to fill new elements with.
         */
        constexpr void resize(size_type new_size, const value_type& value)
            requires(not is_const);

    private:

        /** Size ratio from source buffer to target buffer. */
        static constexpr double m_from_to_size_ratio = static_cast<double>(sizeof(buffer_reference_value_type))
                                                       / static_cast<double>(sizeof(value_type));

        /** Size ratio from target buffer to source buffer. */
        static constexpr size_type m_to_from_size_ratio = static_cast<size_type>(1. / m_from_to_size_ratio);

        /**
         * Converts an index in the adapted buffer to an index in the source buffer.
         *
         * @param idx The index in the adapted buffer.
         * @return The corresponding index in the source buffer.
         */
        [[nodiscard]] constexpr std::remove_cvref_t<buffer_reference>::size_type
        index_for_buffer(size_type idx) const noexcept
        {
            return idx * m_to_from_size_ratio;
        }

        /**
         * Gets a buffer reference iterator from a const_iterator position.
         *
         * @param pos The const_iterator position.
         * @return The corresponding buffer reference iterator.
         */
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
    constexpr buffer_adaptor<To, FromBufferRef>::buffer_adaptor(FromBufferRef buf)
        requires(not is_const)
        : m_buffer(buf)
        , m_max_size(m_buffer.max_size() / m_to_from_size_ratio)
    {
    }

    template <typename To, BufferReference<To> FromBufferRef>
        requires T_is_const_if_FromBufferRef_is_const<FromBufferRef, To>
    constexpr buffer_adaptor<To, FromBufferRef>::buffer_adaptor(const FromBufferRef buf)
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
    buffer_adaptor<To, FromBufferRef>::size() const noexcept(!SPARROW_CONTRACTS_THROW_ON_FAILURE)
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
