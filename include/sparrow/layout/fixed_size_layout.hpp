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

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <limits>
#include <utility>

#include "sparrow/array/array_data.hpp"
#include "sparrow/array/array_data_concepts.hpp"
#include "sparrow/buffer/buffer.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/layout/layout_iterator.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{
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
    template <class T, data_storage DS = array_data>
    class fixed_size_layout
    {
    public:

        using self_type = fixed_size_layout<T, DS>;
        using data_storage_type = DS;
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

        explicit fixed_size_layout(data_storage_type& data);
        void rebind_data(data_storage_type& data);

        fixed_size_layout(const self_type&) = delete;
        self_type& operator=(const self_type&) = delete;
        fixed_size_layout(self_type&&) = delete;
        self_type& operator=(self_type&&) = delete;

        size_type size() const;

        reference operator[](size_type i);
        const_reference operator[](size_type i) const;

        iterator begin();
        iterator end();

        const_iterator cbegin() const;
        const_iterator cend() const;

        const_bitmap_range bitmap() const;
        const_value_range values() const;

    private:

        pointer data();
        const_pointer data() const;

        bitmap_reference has_value(size_type i);
        bitmap_const_reference has_value(size_type i) const;
        inner_reference value(size_type i);
        inner_const_reference value(size_type i) const;

        value_iterator value_begin();
        value_iterator value_end();

        const_value_iterator value_cbegin() const;
        const_value_iterator value_cend() const;

        bitmap_iterator bitmap_begin();
        bitmap_iterator bitmap_end();

        const_bitmap_iterator bitmap_cbegin() const;
        const_bitmap_iterator bitmap_cend() const;

        data_storage_type& storage();
        const data_storage_type& storage() const;

        std::reference_wrapper<data_storage_type> m_data;
    };

    /************************************
     * fixed_size_layout implementation *
     ***********************************/

    template <class T, data_storage DS>
    fixed_size_layout<T, DS>::fixed_size_layout(data_storage_type& data)
        : m_data(data)
    {
        // We only require the presence of the bitmap and the first buffer.
        SPARROW_ASSERT_TRUE(buffers_size(storage()) > 0);
        SPARROW_ASSERT_TRUE(static_cast<size_type>(length(storage())) == sparrow::bitmap(storage()).size())
    }

    template <class T, data_storage DS>
    void fixed_size_layout<T, DS>::rebind_data(data_storage_type& data)
    {
        m_data = data;
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::size() const -> size_type
    {
        SPARROW_ASSERT_TRUE(offset(storage()) <= length(storage()));
        return static_cast<size_type>(length(storage()) - offset(storage()));
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::value(size_type i) -> inner_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return data()[i + static_cast<size_type>(offset(storage()))];
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::value(size_type i) const -> inner_const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return data()[i + static_cast<size_type>(offset(storage()))];
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::operator[](size_type i) -> reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return reference(value(i), has_value(i));
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return const_reference(value(i), has_value(i));
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::begin() -> iterator
    {
        return iterator(value_begin(), bitmap_begin());
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::end() -> iterator
    {
        return iterator(value_end(), bitmap_end());
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::cbegin() const -> const_iterator
    {
        return const_iterator(value_cbegin(), bitmap_cbegin());
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::cend() const -> const_iterator
    {
        return const_iterator(value_cend(), bitmap_cend());
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::bitmap() const -> const_bitmap_range
    {
        return std::ranges::subrange(bitmap_cbegin(), bitmap_cend());
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::values() const -> const_value_range
    {
        return std::ranges::subrange(value_cbegin(), value_cend());
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::has_value(size_type i) -> bitmap_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return sparrow::bitmap(storage())[i + static_cast<size_type>(offset(storage()))];
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::has_value(size_type i) const -> bitmap_const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return sparrow::bitmap(storage())[i + static_cast<size_type>(offset(storage()))];
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::value_begin() -> value_iterator
    {
        return value_iterator{data() + offset(storage())};
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::value_end() -> value_iterator
    {
        value_iterator it = value_begin();
        std::advance(it, size());
        return it;
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator{data() + offset(storage())};
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::value_cend() const -> const_value_iterator
    {
        auto it = value_cbegin();
        std::advance(it, size());
        return it;
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::bitmap_begin() -> bitmap_iterator
    {
        return sparrow::bitmap(storage()).begin() + offset(storage());
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::bitmap_end() -> bitmap_iterator
    {
        bitmap_iterator it = bitmap_begin();
        std::advance(it, size());
        return it;
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::bitmap_cbegin() const -> const_bitmap_iterator
    {
        return sparrow::bitmap(storage()).cbegin() + offset(storage());
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::bitmap_cend() const -> const_bitmap_iterator
    {
        const_bitmap_iterator it = bitmap_cbegin();
        std::advance(it, size());
        return it;
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::data() -> pointer
    {
        SPARROW_ASSERT_TRUE(buffers_size(storage()) > 0);
        return buffer_at(storage(), 0u).template data<inner_value_type>();
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::data() const -> const_pointer
    {
        SPARROW_ASSERT_TRUE(buffers_size(storage()) > 0);
        return buffer_at(storage(), 0u).template data<const inner_value_type>();
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::storage() -> data_storage_type&
    {
        return m_data.get();
    }

    template <class T, data_storage DS>
    auto fixed_size_layout<T, DS>::storage() const -> const data_storage_type&
    {
        return m_data.get();
    }

}  // namespace sparrow
