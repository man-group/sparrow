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
#include <cassert>

#include "sparrow/array_data.hpp"
#include "sparrow/iterator.hpp"
#include "sparrow/buffer.hpp"

namespace sparrow
{
    /**
     * An iterator for `primitive_layout` operating on contiguous data only.
     *
     * @tparam T The type of the elements in the layout's data buffer.
     * @tparam is_const A boolean indicating whether the iterator is const.
     *
     * @note This class is not thread-safe, exception-safe, copyable, movable, equality comparable.
     */
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

        pointer m_pointer = nullptr;

        friend class iterator_access;
    };

    /**
     * A contiguous layout for primitive types.
     *
     * This class provides a contiguous layout for primitive types, such as `uint8_t`, `int32_t`, etc.
     * It iterates over the first buffer in the array_data, and uses the bitmap to skip over null.
     * The bitmap is assumed to be present in the array_data.
     *
     * @tparam T The type of the elements in the layout's data buffer.
     *           A primitive type or a fixed size type.
     *
     * @note This class is not thread-safe, exception-safe, copyable, movable, equality comparable.
     */
    template <class T>
    class primitive_layout
    {
    public:

        using self_type = primitive_layout<T>;
        using inner_value_type = T;
        using value_type = std::optional<inner_value_type>;
        using inner_reference = inner_value_type&;
        using inner_const_reference = const inner_reference;
        using reference = reference_proxy<self_type>;
        using const_reference = const_reference_proxy<self_type>;
        using pointer = inner_value_type*;
        using const_pointer = const inner_value_type*;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        explicit primitive_layout(array_data p);

        using iterator = primitive_layout_iterator<T, false>;
        using const_iterator = primitive_layout_iterator<T, true>;

        size_type size() const;
        inner_reference value(size_type i);
        inner_const_reference value(size_type i) const;

        reference operator[](size_type i);
        const_reference operator[](size_type i) const;

        bool has_value(size_type i) const;

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
        : m_pointer(pointer)
    {
    }

    template <class T, bool is_const>
    auto primitive_layout_iterator<T, is_const>::dereference() const -> reference
    {
        return *m_pointer;
    }

    template <class T, bool is_const>
    void primitive_layout_iterator<T, is_const>::increment()
    {
        ++m_pointer;
    }

    template <class T, bool is_const>
    void primitive_layout_iterator<T, is_const>::decrement()
    {
        --m_pointer;
    }

    template <class T, bool is_const>
    void primitive_layout_iterator<T, is_const>::advance(difference_type n)
    {
        m_pointer += n;
    }

    template <class T, bool is_const>
    auto primitive_layout_iterator<T, is_const>::distance_to(const self_type& rhs) const -> difference_type
    {
        return rhs.m_pointer - m_pointer;
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
        // We only require the presence of the bitmap and the first buffer.
        assert(m_data.buffers.size() > 0);
        assert(m_data.length == m_data.buffers[0].size());
        assert(m_data.length == m_data.bitmap.size());
    }

    template <class T>
    auto primitive_layout<T>::size() const -> size_type
    {
        assert(m_data.buffers.size() > 0);
        return m_data.buffers[0].size();
    }

    template <class T>
    auto primitive_layout<T>::value(size_type i) -> inner_reference
    {
        assert(i < size());
        return data()[i];
    }

    template <class T>
    auto primitive_layout<T>::value(size_type i) const -> inner_const_reference
    {
        assert(i < size());
        return data()[i];
    }

    template <class T>
    auto primitive_layout<T>::operator[](size_type i) -> reference
    {
        assert(i < size());
        return reference(*this, i);
    }

    template <class T>
    auto primitive_layout<T>::operator[](size_type i) const -> const_reference
    {
        assert(i < size());
        return const_reference(*this, i);
    }

    template <class T>
    auto primitive_layout<T>::has_value(size_type i) const -> bool
    {
        assert(i < size());
        return m_data.bitmap.test(i);
    }

    template <class T>
    auto primitive_layout<T>::begin() -> iterator
    {
        return iterator{data()};
    }

    template <class T>
    auto primitive_layout<T>::end() -> iterator
    {
        return iterator{data() + self_type::size()};
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
        return const_iterator{data()};
    }

    template <class T>
    auto primitive_layout<T>::cend() const -> const_iterator
    {
        return const_iterator{data() + self_type::size()};
    }

    template <class T>
    auto primitive_layout<T>::data() -> pointer
    {
        assert(m_data.buffers.size() > 0);
        return m_data.buffers[0].data();
    }

    template <class T>
    auto primitive_layout<T>::data() const -> const_pointer
    {
        assert(m_data.buffers.size() > 0);
        return m_data.buffers[0].data();
    }

} // namespace sparrow


