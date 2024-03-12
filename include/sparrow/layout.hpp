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

#include "sparrow/array_data.hpp"
#include "sparrow/iterator.hpp"
#include "sparrow/buffer.hpp"

namespace sparrow
{
    template <class T, bool is_const>
    class primitive_layout_iterator
        : public iterator_base
        <
            primitive_layout_iterator<T, is_const>,
            mpl::constify_t<T, is_const>,
            std::contiguous_iterator_tag
        >
    {
    public:

        using self_type = primitive_layout_iterator<T, is_const>;
        using base_type = iterator_base
        <
            self_type,
            mpl::constify_t<T, is_const>,
            std::contiguous_iterator_tag
        >;
        using pointer = typename base_type::pointer;

        // Required so that std::ranges::end(p) is
        // valid when p is a primitive_layout.
        primitive_layout_iterator() = default;
        explicit primitive_layout_iterator(pointer p);

    private:

        // TODO: use reference proxies with an API similar to `std::optional` instead.
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;
        using size_type = std::size_t;

        reference dereference() const;
        void increment();
        void decrement();
        void advance(difference_type n);
        difference_type distance_to(const self_type& rhs) const;
        bool equal(const self_type& rhs) const;
        bool less_than(const self_type& rhs) const;

        // We only use the first buffer and the bitmap.
        pointer m_pointer = nullptr;
        size_type m_index = 0;

        friend class iterator_access;
    };

    template <class T>
    class primitive_layout
    {
    public:

        using self_type = primitive_layout<T>;
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        explicit primitive_layout(array_data p);

        using iterator = primitive_layout_iterator<T, false>;
        using const_iterator = primitive_layout_iterator<T, true>;

        size_type size() const;
        reference element(size_type i);
        const_reference element(size_type i) const;

        iterator begin();
        iterator end();

        const_iterator begin() const;
        const_iterator end() const;

        const_iterator cbegin() const;
        const_iterator cend() const ;

    private:
        // We only use the first buffer and the bitmap.
        array_data m_data;

        pointer data();
        const_pointer data() const;

    };

    /********************************************
     * primitive_layout_iterator implementation *
     *******************************************/

    template <class T, bool is_const>
    primitive_layout_iterator<T, is_const>::primitive_layout_iterator(pointer pointer)
        : m_pointer(pointer), m_index(0)
    {
    }

    template <class T, bool is_const>
    auto primitive_layout_iterator<T, is_const>::dereference() const -> reference
    {
        return m_pointer[m_index];
    }

    template <class T, bool is_const>
    void primitive_layout_iterator<T, is_const>::increment()
    {
        ++m_index;
    }

    template <class T, bool is_const>
    void primitive_layout_iterator<T, is_const>::decrement()
    {
        --m_index;
    }

    template <class T, bool is_const>
    void primitive_layout_iterator<T, is_const>::advance(difference_type n)
    {
        m_index += n;
    }

    template <class T, bool is_const>
    auto primitive_layout_iterator<T, is_const>::distance_to(const self_type& rhs) const -> difference_type
    {
        return rhs.m_index - m_index;
    }

    template <class T, bool is_const>
    bool primitive_layout_iterator<T, is_const>::equal(const self_type& rhs) const
    {
        return distance_to(rhs) == 0;
    }

    template <class T, bool is_const>
    bool primitive_layout_iterator<T, is_const>::less_than(const self_type& rhs) const
    {
        return distance_to(rhs) > 0;
    }

    /***********************************
     * primitive_layout implementation *
     * ********************************/

    template <class T>
    primitive_layout<T>::primitive_layout(array_data ad)
        : m_data(ad)
    {
        if (m_data.buffers.size() == 0)
        {
            throw std::runtime_error("No buffers are present in array_data");
        }
    }

    template <class T>
    auto primitive_layout<T>::size() const -> size_type
    {
        return m_data.buffers[0].size();
    }

    template <class T>
    auto primitive_layout<T>::element(size_type i) -> reference
    {
        assert(pos < size());
        return i[data()];
    }

    template <class T>
    auto primitive_layout<T>::element(size_type i) const -> const_reference
    {
        assert(pos < size());
        return i[data()];
    }

    template <class T>
    auto primitive_layout<T>::begin() -> iterator
    {
        return iterator(data());
    }

    template <class T>
    auto primitive_layout<T>::end() -> iterator
    {
        return iterator(data()) + self_type::size();
    }

    template <class T>
    auto primitive_layout<T>::begin() const -> const_iterator
    {
        return cbegin();
    }

    template <class T>
    auto primitive_layout<T>::end() const -> const_iterator
    {
        return cend();
    }

    template <class T>
    auto primitive_layout<T>::cbegin() const -> const_iterator
    {
        return const_iterator(data());
    }

    template <class T>
    auto primitive_layout<T>::cend() const -> const_iterator
    {
        return const_iterator(data()) + self_type::size();
    }

    template <class T>
    auto primitive_layout<T>::data() -> pointer
    {
        return m_data.buffers[0].data();
    }

    template <class T>
    auto primitive_layout<T>::data() const -> const_pointer
    {
        return m_data.buffers[0].data();
    }

} // namespace sparrow


