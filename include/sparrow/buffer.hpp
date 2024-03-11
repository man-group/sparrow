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

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstdint>

namespace sparrow
{
    /**
     * @class buffer_base
     * @brief Base class for buffer and buffer_view
     *
     * This class implements the common API for buffer and buffer_view.
     * The only difference between these two inheriting classes is
     * that buffer owns its memory while buffer_view only references it.
     *
     **/
    template <class T>
    class buffer_base
    {
    public:

        using self_type = buffer_base<T>;
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

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

        void swap(buffer_base& rhs) noexcept;
        bool equal(const buffer_base& rhs) const;
    
    protected:

        buffer_base() = default;
        buffer_base(pointer p, size_type n);
        ~buffer_base() = default;

        buffer_base(const self_type&) = default;
        self_type& operator=(const self_type&) = default;

        buffer_base(self_type&&) = default;
        self_type& operator=(self_type&&) = default;

        void reset(pointer p, size_type n);

    private:

        pointer p_data = nullptr;
        size_type m_size = 0u;
    };

    template <class T>
    bool operator==(const buffer_base<T>& lhs, const buffer_base<T>& rhs);
    
    /**
     * @class buffer
     * @brief Object that owns a piece of contiguous memory
     */
    template <class T>
    class buffer : public buffer_base<T>
    {
    public:

        using base_type = buffer_base<T>;
        using value_type = typename base_type::value_type;
        using pointer = typename base_type::pointer;
        using size_type = typename base_type::size_type;

        buffer() = default;
        explicit buffer(size_type size);
        buffer(pointer data, size_type size);
        
        ~buffer();
        
        buffer(const buffer&);
        buffer& operator=(const buffer&);

        buffer(buffer&&);
        buffer& operator=(buffer&&);

        void resize(size_type new_size);
        void resize(size_type new_size, value_type value);
        void clear();

    private:

        pointer allocate(size_type size) const;
        void deallocate(pointer mem) const;
    };

    /*
     * @class buffer_view
     * @brief Object that references but does not own a piece of contiguous memory
     */
    template <class T>
    class buffer_view : public buffer_base<T>
    {
    public:

        using base_type = buffer_base<T>;
        using pointer = typename base_type::pointer;
        using size_type = typename base_type::size_type;

        explicit buffer_view(buffer<T>& buffer);
        buffer_view(pointer data, size_type size);
        ~buffer_view() = default;

        buffer_view(const buffer_view&) = default;
        buffer_view& operator=(const buffer_view&) = default;

        buffer_view(buffer_view&&) = default;
        buffer_view& operator=(buffer_view&&) = default;
    };

    /******************************
     * buffer_base implementation *
     ******************************/

    template <class T>
    buffer_base<T>::buffer_base(pointer p, size_type n)
        : p_data(p)
        , m_size(n)
    {
    }

    template <class T>
    void buffer_base<T>::reset(pointer p, size_type n)
    {
        p_data = p;
        m_size = n;
    }

    template <class T>
    bool buffer_base<T>::empty() const noexcept
    {
        return size() == size_type(0);
    }

    template <class T>
    auto buffer_base<T>::size() const noexcept -> size_type
    {
        return m_size;
    }

    template <class T>
    auto buffer_base<T>::operator[](size_type pos) -> reference
    {
        assert(pos < size());
        return data()[pos];
    }

    template <class T>
    auto buffer_base<T>::operator[](size_type pos) const -> const_reference
    {
        assert(pos < size());
        return data()[pos];
    }

    template <class T>
    auto buffer_base<T>::front() -> reference
    {
        assert(!empty());
        return data()[0];
    }

    template <class T>
    auto buffer_base<T>::front() const -> const_reference
    {
        assert(!empty());
        return data()[0];
    }

    template <class T>
    auto buffer_base<T>::back() -> reference
    {
        assert(!empty());
        return data()[m_size - 1];
    }

    template <class T>
    auto buffer_base<T>::back() const -> const_reference
    {
        assert(!empty());
        return data()[m_size - 1];
    }

    template <class T>
    template <class U>
    U* buffer_base<T>::data() noexcept
    {
        return reinterpret_cast<U*>(p_data);
    }

    template <class T>
    template <class U>
    const U* buffer_base<T>::data() const noexcept
    {
        return reinterpret_cast<const U*>(p_data);
    }

    template <class T>
    void buffer_base<T>::swap(buffer_base<T>& rhs) noexcept
    {
        std::swap(p_data, rhs.p_data);
        std::swap(m_size, rhs.m_size);
    }

    template <class T>
    bool buffer_base<T>::equal(const buffer_base<T>& rhs) const
    {
        return m_size == rhs.m_size && std::equal(p_data, p_data + m_size, rhs.p_data);
    }

    template <class T>
    bool operator==(const buffer_base<T>& lhs, const buffer_base<T>& rhs)
    {
        return lhs.equal(rhs);
    }

    /*************************
     * buffer implementation *
     *************************/

    template <class T>
    buffer<T>::buffer(size_type size)
        : base_type{allocate(size), size}
    {
    }

    template <class T>
    buffer<T>::buffer(pointer data, size_type size)
        : base_type{data, size}
    {
    }
        
    template <class T>
    buffer<T>::~buffer()
    {
        deallocate(base_type::data());
    }
        
    template <class T>
    buffer<T>::buffer(const buffer<T>& rhs)
        : base_type{allocate(rhs.size()), rhs.size()}
    {
        std::copy(rhs.data(), rhs.data() + rhs.size(), base_type::data());
    }

    template <class T>
    buffer<T>& buffer<T>::operator=(const buffer<T>& rhs)
    {
        if (this != &rhs)
        {
            buffer<T> tmp(rhs);
            base_type::swap(tmp);
        }
        return *this;
    }

    template <class T>
    buffer<T>::buffer(buffer&& rhs)
        : base_type{rhs.data(), rhs.size()}
    {
        rhs.reset(nullptr, 0u);
    }

    template <class T>
    buffer<T>& buffer<T>::operator=(buffer<T>&& rhs)
    {
        base_type::swap(rhs);
        return *this;
    }

    template <class T>
    void buffer<T>::resize(size_type n)
    {
        // TODO: add capacity, resize if growing only and define a shrink_to_fit method
        if (n != base_type::size())
        {
            buffer<T> tmp(n);
            const size_type copy_size = std::min(base_type::size(), n);
            std::copy(base_type::data(), base_type::data() + copy_size, tmp.data());
            base_type::swap(tmp);
        }
    }

    template <class T>
    void buffer<T>::resize(size_type n, value_type value)
    {
        const size_type old_size = base_type::size();
        resize(n);
        if (old_size < n)
        {
            std::fill(base_type::data() + old_size, base_type::data() + n, value);
        }
    }

    template <class T>
    void buffer<T>::clear()
    {
        resize(size_type(0));
    }
    
    template <class T>
    auto buffer<T>::allocate(size_type size) const -> pointer
    {
        return new T[size];
    }

    template <class T>
    void buffer<T>::deallocate(pointer mem) const
    {
        delete[] mem;
    }

    /******************************
     * buffer_view implementation *
     ******************************/

    template <class T>
    buffer_view<T>::buffer_view(buffer<T>& buffer)
        : base_type{buffer.data(), buffer.size()}
    {
    }

    template <class T>
    buffer_view<T>::buffer_view(pointer data, size_type size)
        : base_type{data, size}
    {
    }
}

