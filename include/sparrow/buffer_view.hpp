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

#include "sparrow/buffer.hpp"

namespace sparrow
{
    /*
     * Non-owning view of a contiguous sequence of objects of type T.
     *
     * Although this class looks very similar to std::span, it provides
     * methods that are missing in C++20 std::span (like cbegin / cend),
     * and additional std::vector-like APIs. 
     */
    template <class T>
    class buffer_view
    {
    public:

        using self_type = buffer_view<T>;
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        using iterator = pointer_iterator<pointer>;
        using const_iterator = pointer_iterator<const_pointer>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        explicit buffer_view(buffer<T>& buffer);
        buffer_view(pointer p, size_type n);
        // TODO: To be complete we also need a subrange(...) function
        // and a constructor that takes a pair of iterators (they should
        // be template parameters constrained by concept, not buffer_view::iterator).
        // These are often used with view types (see std::span, which also provides
        // first and last subview functions).

        bool empty() const noexcept;
        size_type size() const noexcept;

        reference operator[](size_type);
        const_reference operator[](size_type) const;

        reference front();
        const_reference front() const;

        reference back();
        const_reference back() const;

        template <class U = T>
        U* data() noexcept;

        template <class U = T>
        const U* data() const noexcept;

        iterator begin();
        iterator end();

        const_iterator begin() const;
        const_iterator end() const;
        const_iterator cbegin() const;
        const_iterator cend() const;

        reverse_iterator rbegin();
        reverse_iterator rend();

        const_reverse_iterator rbegin() const;
        const_reverse_iterator rend() const;
        const_reverse_iterator crbegin() const;
        const_reverse_iterator crend() const;

        void swap(buffer_view& rhs) noexcept;

    private:

        pointer p_data = nullptr;
        size_type m_size = 0u;
    };

    template <class T>
    bool operator==(const buffer_view<T>& lhs, const buffer_view<T>& rhs);

    /******************************
     * buffer_view implementation *
     ******************************/

    template <class T>
    buffer_view<T>::buffer_view(buffer<T>& buffer)
        : p_data(buffer.data())
        , m_size(buffer.size())
    {
    }

    template <class T>
    buffer_view<T>::buffer_view(pointer p, size_type n)
        : p_data(p)
        , m_size(n)
    {
    }

    template <class T>
    bool buffer_view<T>::empty() const noexcept
    {
        return size() == size_type(0);
    }

    template <class T>
    auto buffer_view<T>::size() const noexcept -> size_type
    {
        return m_size;
    }

    template <class T>
    auto buffer_view<T>::operator[](size_type pos) -> reference
    {
        assert(pos < size());
        return data()[pos];
    }

    template <class T>
    auto buffer_view<T>::operator[](size_type pos) const -> const_reference
    {
        assert(pos < size());
        return data()[pos];
    }

    template <class T>
    auto buffer_view<T>::front() -> reference
    {
        assert(!empty());
        return data()[0];
    }

    template <class T>
    auto buffer_view<T>::front() const -> const_reference
    {
        assert(!empty());
        return data()[0];
    }

    template <class T>
    auto buffer_view<T>::back() -> reference
    {
        assert(!empty());
        return data()[m_size - 1];
    }

    template <class T>
    auto buffer_view<T>::back() const -> const_reference
    {
        assert(!empty());
        return data()[m_size - 1];
    }

    template <class T>
    template <class U>
    U* buffer_view<T>::data() noexcept
    {
        return reinterpret_cast<U*>(p_data);
    }

    template <class T>
    template <class U>
    const U* buffer_view<T>::data() const noexcept
    {
        return reinterpret_cast<const U*>(p_data);
    }

    template <class T>
    auto buffer_view<T>::begin() -> iterator
    {
        return iterator(p_data);
    }

    template <class T>
    auto buffer_view<T>::end() -> iterator
    {
        return iterator(p_data + m_size);
    }

    template <class T>
    auto buffer_view<T>::begin() const -> const_iterator
    {
        return cbegin();
    }

    template <class T>
    auto buffer_view<T>::end() const -> const_iterator
    {
        return cend();
    }

    template <class T>
    auto buffer_view<T>::cbegin() const -> const_iterator
    {
        return const_iterator(p_data);
    }

    template <class T>
    auto buffer_view<T>::cend() const -> const_iterator
    {
        return const_iterator(p_data + m_size);
    }

    template <class T>
    auto buffer_view<T>::rbegin() -> reverse_iterator
    {
        return reverse_iterator(end());
    }

    template <class T>
    auto buffer_view<T>::rend() -> reverse_iterator
    {
        return reverse_iterator(begin());
    }

    template <class T>
    auto buffer_view<T>::rbegin() const -> const_reverse_iterator
    {
        return crbegin();
    }

    template <class T>
    auto buffer_view<T>::rend() const -> const_reverse_iterator
    {
        return crend();
    }

    template <class T>
    auto buffer_view<T>::crbegin() const -> const_reverse_iterator
    {
        return const_reverse_iterator(cend());
    }

    template <class T>
    auto buffer_view<T>::crend() const -> const_reverse_iterator
    {
        return const_reverse_iterator(cbegin());
    }

    template <class T>
    void buffer_view<T>::swap(buffer_view<T>& rhs) noexcept
    {
        std::swap(p_data, rhs.p_data);
        std::swap(m_size, rhs.m_size);
    }

    template <class T>
    bool operator==(const buffer_view<T>& lhs, const buffer_view<T>& rhs)
    {
        return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
}
