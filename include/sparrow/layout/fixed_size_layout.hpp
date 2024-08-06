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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or mplied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <functional>
#include <type_traits>

#include "sparrow/array/array_data.hpp"
#include "sparrow/array/array_data_concepts.hpp"
#include "sparrow/buffer/buffer_adaptor.hpp"
#include "sparrow/layout/layout_iterator.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{
    template <typename DS, typename T>
    concept Pouet = mutable_data_storage<DS> || (immutable_data_storage<DS> && std::is_const_v<T>);

    /**
     * A contiguous layout for fixed size types.
     *
     * This class provides a contiguous layout for fixed size types, such as `uint8_t`, `int32_t`, etc.
     * It iterates over the first buffer in the data storage, and uses the bitmap to skip over null.
     * The bitmap is assumed to be present in the data storage.
     *
     * @tparam T The type of the elements in the layout's data buffer.
     *           A fixed size type, such as a primitive type.
     * @tparam DS The type for the structure holding the data. Default to array_data.
     */
    template <class T, Pouet<T> DS = array_data>
    class fixed_size_layout
    {
    public:

        using self_type = fixed_size_layout<T, DS>;
        using data_storage_type = std::remove_reference_t<DS>;
        using inner_value_type = T;
        using inner_reference = inner_value_type&;
        using inner_const_reference = const inner_value_type&;
        using bitmap_type = typename data_storage_type::bitmap_type;
        using bitmap_reference = typename bitmap_type::reference;
        using bitmap_const_reference = typename bitmap_type::const_reference;
        using value_type = nullable<inner_value_type>;
        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using pointer = inner_value_type*;
        using const_pointer = const inner_value_type*;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using iterator_tag = std::contiguous_iterator_tag;

        using const_bitmap_iterator = bitmap_type::const_iterator;
        using const_value_iterator = pointer_iterator<const_pointer>;

        using bitmap_iterator = bitmap_type::iterator;
        using value_iterator = pointer_iterator<pointer>;

        using const_bitmap_range = std::ranges::subrange<const_bitmap_iterator>;
        using const_value_range = std::ranges::subrange<const_value_iterator>;

        using iterator = layout_iterator<self_type, false>;
        using const_iterator = layout_iterator<self_type, true>;

        static constexpr bool is_const = std::is_const_v<T>;

        explicit fixed_size_layout(const data_storage_type& data)
            requires(is_const);

        explicit fixed_size_layout(DS& data);

        size_type size() const;
        bool empty() const;

        reference operator[](size_type i)
            requires(not is_const);
        const_reference operator[](size_type i) const;

        iterator begin()
            requires(not is_const);
        iterator end()
            requires(not is_const);

        const_iterator cbegin() const;
        const_iterator cend() const;

        const_bitmap_range bitmap() const;
        const_value_range values() const;

        // Modifiers
        void clear()
            requires(not is_const);

        constexpr iterator insert(const_iterator pos, const value_type& value)
            requires(not is_const);

        constexpr iterator insert(const_iterator pos, value_type&& value)
            requires(not is_const);

        constexpr iterator insert(const_iterator pos, size_type count, const value_type& value)
            requires(not is_const);

        constexpr iterator emplace(const_iterator pos, const value_type& value)
            requires(not is_const);

        constexpr iterator erase(const_iterator first, const_iterator last)
            requires(not is_const);

        constexpr iterator erase(const_iterator pos)
            requires(not is_const);

        constexpr void push_back(const value_type& value)
            requires(not is_const);

        constexpr void push_back(value_type&& value)
            requires(not is_const);

        constexpr void pop_back()
            requires(not is_const);

        constexpr void resize(size_type count, const value_type& value)
            requires(not is_const);

        constexpr void resize(size_type count)
            requires(not is_const);

    private:

        pointer data()
            requires(not is_const);
        const_pointer data() const;

        bitmap_reference has_value(size_type i)
            requires(not is_const);
        bitmap_const_reference has_value(size_type i) const;
        inner_reference value(size_type i)
            requires(not is_const);
        inner_const_reference value(size_type i) const;

        value_iterator value_begin()
            requires(not is_const);
        value_iterator value_end()
            requires(not is_const);

        const_value_iterator value_cbegin() const;
        const_value_iterator value_cend() const;

        bitmap_iterator bitmap_begin()
            requires(not is_const);
        bitmap_iterator bitmap_end()
            requires(not is_const);

        const_bitmap_iterator bitmap_cbegin() const;
        const_bitmap_iterator bitmap_cend() const;

        data_storage_type& storage()
            requires(not is_const);
        const data_storage_type& storage() const;

        std::reference_wrapper<data_storage_type> m_data;
        using buffer_at_type = decltype(buffer_at(std::declval<data_storage_type&>(), 0u));
        using buffer_member_type = std::conditional_t<
            std::is_reference_v<buffer_at_type>,
            std::reference_wrapper<std::remove_reference_t<buffer_at_type>>,
            buffer_at_type>;
        buffer_member_type m_buffer;
        using buffer_adaptor_type = buffer_adaptor<inner_value_type, buffer_at_type&>;
        buffer_adaptor_type m_buffer_adaptor;
    };

    /************************************
     * fixed_size_layout implementation *
     ***********************************/

    template <class T, Pouet<T> DS>
    fixed_size_layout<T, DS>::fixed_size_layout(const data_storage_type& data)
        requires(is_const)
        : m_data(data)
        , m_buffer(buffer_at(storage(), 0u))
        , m_buffer_adaptor(m_buffer)
    {
        // We only require the presence of the bitmap and the first buffer.
        SPARROW_ASSERT_TRUE(buffers_size(storage()) > 0);
        SPARROW_ASSERT_TRUE(static_cast<size_type>(length(storage())) == sparrow::bitmap(storage()).size())
    }

    template <class T, Pouet<T> DS>
    fixed_size_layout<T, DS>::fixed_size_layout(DS& data)
        : m_data(data)
        , m_buffer(buffer_at(storage(), 0u))
        , m_buffer_adaptor(m_buffer)
    {
        // We only require the presence of the bitmap and the first buffer.
        SPARROW_ASSERT_TRUE(buffers_size(storage()) > 0);
        SPARROW_ASSERT_TRUE(static_cast<size_type>(length(storage())) == sparrow::bitmap(storage()).size())
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::size() const -> size_type
    {
        SPARROW_ASSERT_TRUE(offset(storage()) <= length(storage()));
        return static_cast<size_type>(length(storage()) - offset(storage()));
    }

    template <class T, Pouet<T> DS>
    bool fixed_size_layout<T, DS>::empty() const
    {
        return size() == 0;
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::value(size_type i) -> inner_reference
        requires(not is_const)
    {
        SPARROW_ASSERT_TRUE(i < size());
        return data()[i + static_cast<size_type>(offset(storage()))];
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::value(size_type i) const -> inner_const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return data()[i + static_cast<size_type>(offset(storage()))];
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::operator[](size_type i) -> reference
        requires(not is_const)
    {
        SPARROW_ASSERT_TRUE(i < size());
        return reference(value(i), has_value(i));
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return const_reference(value(i), has_value(i));
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::begin() -> iterator
        requires(not is_const)
    {
        return iterator(value_begin(), bitmap_begin());
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::end() -> iterator
        requires(not is_const)
    {
        return iterator(value_end(), bitmap_end());
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::cbegin() const -> const_iterator
    {
        return const_iterator(value_cbegin(), bitmap_cbegin());
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::cend() const -> const_iterator
    {
        return const_iterator(value_cend(), bitmap_cend());
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::bitmap() const -> const_bitmap_range
    {
        return std::ranges::subrange(bitmap_cbegin(), bitmap_cend());
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::values() const -> const_value_range
    {
        return std::ranges::subrange(value_cbegin(), value_cend());
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::has_value(size_type i) -> bitmap_reference
        requires(not is_const)
    {
        SPARROW_ASSERT_TRUE(i < size());
        return sparrow::bitmap(storage())[i + static_cast<size_type>(offset(storage()))];
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::has_value(size_type i) const -> bitmap_const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return sparrow::bitmap(storage())[i + static_cast<size_type>(offset(storage()))];
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::value_begin() -> value_iterator
        requires(not is_const)
    {
        return value_iterator{data() + offset(storage())};
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::value_end() -> value_iterator
        requires(not is_const)
    {
        value_iterator it = value_begin();
        std::advance(it, size());
        return it;
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator{data() + offset(storage())};
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::value_cend() const -> const_value_iterator
    {
        auto it = value_cbegin();
        std::advance(it, size());
        return it;
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::bitmap_begin() -> bitmap_iterator
        requires(not is_const)
    {
        return sparrow::bitmap(storage()).begin() + offset(storage());
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::bitmap_end() -> bitmap_iterator
        requires(not is_const)
    {
        bitmap_iterator it = bitmap_begin();
        std::advance(it, size());
        return it;
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::bitmap_cbegin() const -> const_bitmap_iterator
    {
        return sparrow::bitmap(storage()).cbegin() + offset(storage());
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::bitmap_cend() const -> const_bitmap_iterator
    {
        const_bitmap_iterator it = bitmap_cbegin();
        std::advance(it, size());
        return it;
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::data() -> pointer
        requires(not is_const)
    {
        return m_buffer_adaptor.data();
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::data() const -> const_pointer
    {
        return m_buffer_adaptor.data();
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::storage() -> data_storage_type&
        requires(not is_const)
    {
        return m_data.get();
    }

    template <class T, Pouet<T> DS>
    auto fixed_size_layout<T, DS>::storage() const -> const data_storage_type&
    {
        return m_data.get();
    }

    // Modifiers
    template <class T, Pouet<T> DS>
    void fixed_size_layout<T, DS>::clear()
        requires(not is_const)
    {
        sparrow::buffers_clear(storage());
        sparrow::bitmap(storage()).clear();
    }

    template <class T, Pouet<T> DS>
    constexpr fixed_size_layout<T, DS>::iterator
    fixed_size_layout<T, DS>::insert(const_iterator pos, const value_type& value)
        requires(not is_const)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        return emplace(pos, value);
    }

    template <class T, Pouet<T> DS>
    constexpr fixed_size_layout<T, DS>::iterator
    fixed_size_layout<T, DS>::insert(const_iterator pos, value_type&& value)
        requires(not is_const)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        return emplace(pos, std::move(value));
    }

    template <class T, Pouet<T> DS>
    constexpr fixed_size_layout<T, DS>::iterator
    fixed_size_layout<T, DS>::insert(const_iterator pos, size_type count, const value_type& value)
        requires(not is_const)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());

        const difference_type offset = std::distance(cbegin(), pos);
        if (count != 0)
        {
            m_buffer_adaptor.insert(
                std::next(m_buffer_adaptor.begin(), offset),
                count,
                value.value_or(inner_value_type{})
            );
            sparrow::bitmap(storage())
                .insert(sparrow::bitmap(storage()).begin() + offset, count, value.has_value());
        }
        return std::next(begin(), offset);
    }

    template <class T, Pouet<T> DS>
    constexpr fixed_size_layout<T, DS>::iterator
    fixed_size_layout<T, DS>::emplace(const_iterator pos, const value_type& value)
        requires(not is_const)
    {
        SPARROW_ASSERT_TRUE(pos >= cbegin());
        SPARROW_ASSERT_TRUE(pos <= cend());
        const difference_type offset = std::distance(cbegin(), pos);
        auto& bitmap = sparrow::bitmap(storage());
        bitmap.insert(bitmap.cbegin() + offset, value.has_value());
        const auto it = m_buffer_adaptor.cbegin() + offset;
        m_buffer_adaptor.emplace(it, value.value_or(inner_value_type{}));
        return iterator(value_end(), bitmap_end());
    }

    template <class T, Pouet<T> DS>
    constexpr fixed_size_layout<T, DS>::iterator
    fixed_size_layout<T, DS>::erase(const_iterator first, const_iterator last)
        requires(not is_const)
    {
        SPARROW_ASSERT_TRUE(first < last);
        SPARROW_ASSERT_TRUE(cbegin() <= first);
        SPARROW_ASSERT_TRUE(last <= cend());
        const difference_type offset = std::distance(cbegin(), first);
        const difference_type len = std::distance(first, last);
        auto& bitmap = sparrow::bitmap(storage());
        bitmap.erase(bitmap.begin() + offset, bitmap.begin() + offset + len);
        m_buffer_adaptor.erase(m_buffer_adaptor.begin() + offset, m_buffer_adaptor.begin() + offset + len);
        return iterator(begin() + offset);
    }

    template <class T, Pouet<T> DS>
    constexpr fixed_size_layout<T, DS>::iterator fixed_size_layout<T, DS>::erase(const_iterator pos)
        requires(not is_const)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos < cend());
        return erase(pos, pos + 1);
    }

    template <class T, Pouet<T> DS>
    constexpr void fixed_size_layout<T, DS>::push_back(const value_type& value)
        requires(not is_const)
    {
        m_buffer_adaptor.push_back(value.value_or(inner_value_type{}));
        sparrow::bitmap(storage()).push_back(value.has_value());
    }

    template <class T, Pouet<T> DS>
    constexpr void fixed_size_layout<T, DS>::push_back(value_type&& value)
        requires(not is_const)
    {
        m_buffer_adaptor.push_back(std::move(value.value_or(inner_value_type{})));
        sparrow::bitmap(storage()).push_back(value.has_value());
    }

    template <class T, Pouet<T> DS>
    constexpr void fixed_size_layout<T, DS>::pop_back()
        requires(not is_const)
    {
        m_buffer_adaptor.pop_back();
        sparrow::bitmap(storage()).pop_back();
    }

    template <class T, Pouet<T> DS>
    constexpr void fixed_size_layout<T, DS>::resize(size_type count, const value_type& value)
        requires(not is_const)
    {
        const size_type current_size = size();
        if (count < current_size)
        {
            erase(begin() + count, end());
        }
        else if (count > current_size)
        {
            insert(end(), count - current_size, value);
        }
    }

    template <class T, Pouet<T> DS>
    constexpr void fixed_size_layout<T, DS>::resize(size_type count)
        requires(not is_const)
    {
        resize(count, value_type{});
    }

}  // namespace sparrow
