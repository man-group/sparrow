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

#include <cstddef>
#include <functional>
#include <ranges>

#include "sparrow/array/array_data.hpp"
#include "sparrow/array/data_type.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{

    /*
     * @class empty_iterator
     *
     * @brief Iterator used by the null_layout class.
     *
     * @tparam T the value_type of the iterator
     */
    template <class T>
    class empty_iterator : public iterator_base<empty_iterator<T>, T, std::contiguous_iterator_tag, T>
    {
    public:

        using self_type = empty_iterator<T>;
        using base_type = iterator_base<self_type, T, std::contiguous_iterator_tag, T>;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;

        explicit empty_iterator(difference_type index = difference_type()) noexcept;

    private:

        reference dereference() const;
        void increment();
        void decrement();
        void advance(difference_type n);
        difference_type distance_to(const self_type& rhs) const;
        bool equal(const self_type& rhs) const;
        bool less_than(const self_type& rhs) const;

        difference_type m_index;

        friend class iterator_access;
    };

    /*
     * @class null_layout
     *
     * @brief Memory-efficient layout for the Null data type.
     *
     * This layout is a memory-efficient layout for the Null data type where
     * all values are null. In this case, no memory buffers are allocated.
     *
     * @tparam DS The type for the structure holding the data. Default to
     *            array_data.
     */
    template <data_storage DS = array_data>
    class null_layout
    {
    public:

        using data_storage_type = DS;
        using inner_value_type = null_type;
        using value_type = nullable<inner_value_type>;
        using iterator = empty_iterator<value_type>;
        using const_iterator = empty_iterator<value_type>;
        using reference = iterator::reference;
        using const_reference = const_iterator::reference;
        using size_type = std::size_t;
        using difference_type = iterator::difference_type;
        using iterator_tag = std::contiguous_iterator_tag;


        using const_value_iterator = empty_iterator<int>;
        using const_bitmap_iterator = empty_iterator<bool>;

        using const_value_range = std::ranges::subrange<const_value_iterator>;
        using const_bitmap_range = std::ranges::subrange<const_bitmap_iterator>;

        explicit null_layout(data_storage_type& data);
        void rebind_data(data_storage_type& data);

        size_type size() const;

        reference operator[](size_type i);
        const_reference operator[](size_type i) const;

        iterator begin();
        iterator end();

        const_iterator cbegin() const;
        const_iterator cend() const;

        const_value_range values() const;
        const_bitmap_range bitmap() const;

    private:

        difference_type ssize() const;

        data_storage_type& storage();
        const data_storage_type& storage() const;

        std::reference_wrapper<data_storage_type> m_data;
    };

    /*********************************
     * empty_iterator implementation *
     *********************************/

    template <class T>
    empty_iterator<T>::empty_iterator(difference_type index) noexcept
        : m_index(index)
    {
    }

    template <class T>
    auto empty_iterator<T>::dereference() const -> reference
    {
        return T();
    }

    template <class T>
    void empty_iterator<T>::increment()
    {
        ++m_index;
    }

    template <class T>
    void empty_iterator<T>::decrement()
    {
        --m_index;
    }

    template <class T>
    void empty_iterator<T>::advance(difference_type n)
    {
        m_index += n;
    }

    template <class T>
    auto empty_iterator<T>::distance_to(const self_type& rhs) const -> difference_type
    {
        return rhs.m_index - m_index;
    }

    template <class T>
    bool empty_iterator<T>::equal(const self_type& rhs) const
    {
        return m_index == rhs.m_index;
    }

    template <class T>
    bool empty_iterator<T>::less_than(const self_type& rhs) const
    {
        return m_index < rhs.m_index;
    }

    /******************************
     * null_layout implementation *
     ******************************/

    template <data_storage DS>
    null_layout<DS>::null_layout(data_storage_type& data)
        : m_data(data)
    {
        SPARROW_ASSERT_TRUE(buffers_size(storage()) == 0u);
    }

    template <data_storage DS>
    void null_layout<DS>::rebind_data(data_storage_type& data)
    {
        SPARROW_ASSERT_TRUE(buffers_size(storage()) == 0u);
        m_data = data;
    }

    template <data_storage DS>
    auto null_layout<DS>::size() const -> size_type
    {
        return static_cast<size_type>(length(storage()));
    }

    template <data_storage DS>
    auto null_layout<DS>::operator[](size_type i) -> reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return *(begin());
    }

    template <data_storage DS>
    auto null_layout<DS>::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return *(cbegin());
    }

    template <data_storage DS>
    auto null_layout<DS>::begin() -> iterator
    {
        return iterator(0);
    }

    template <data_storage DS>
    auto null_layout<DS>::end() -> iterator
    {
        return iterator(ssize());
    }

    template <data_storage DS>
    auto null_layout<DS>::cbegin() const -> const_iterator
    {
        return const_iterator(0);
    }

    template <data_storage DS>
    auto null_layout<DS>::cend() const -> const_iterator
    {
        return const_iterator(ssize());
    }

    template <data_storage DS>
    auto null_layout<DS>::values() const -> const_value_range
    {
        return std::ranges::subrange(const_value_iterator(0), const_value_iterator(ssize()));
    }

    template <data_storage DS>
    auto null_layout<DS>::bitmap() const -> const_bitmap_range
    {
        return std::ranges::subrange(const_bitmap_iterator(0), const_bitmap_iterator(ssize()));
    }

    template <data_storage DS>
    auto null_layout<DS>::ssize() const -> difference_type
    {
        return static_cast<difference_type>(size());
    }

    template <data_storage DS>
    auto null_layout<DS>::storage() -> data_storage_type&
    {
        return m_data.get();
    }

    template <data_storage DS>
    auto null_layout<DS>::storage() const -> const data_storage_type&
    {
        return m_data.get();
    }
}
