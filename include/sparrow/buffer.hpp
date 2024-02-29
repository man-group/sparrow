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
#include <concepts>
#include <cstdint>

namespace sparrow
{
    namespace impl
    {
        template <class T>
        struct buffer_data
        {
            using value_type = T;
            using pointer = T*;
            using size_type = std::size_t;
            
            bool empty() const noexcept;
            size_type size() const noexcept;

            template <class U = T>
            U* data() noexcept;

            template <class U = T>
            const U* data() const noexcept;

            void swap(buffer_data& rhs) noexcept;
            bool equal(const buffer_data& rhs) const;

            pointer p_data = nullptr;
            size_type m_size = 0;
        };
    }

    /**
     * @class buffer
     * @brief Object that owns a piece of contiguous memory
     */
    template <class T>
    class buffer : private impl::buffer_data<T>
    {
    public:

        using base_type = impl::buffer_data<T>;
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

        using base_type::empty;
        using base_type::size;
        using base_type::data;

        void resize(size_type new_size);

        void swap(buffer&) noexcept;
        bool equal(const buffer& rhs) const;

    private:

        pointer allocate(size_type size) const;
        void deallocate(pointer mem) const;
    };

    template <class T>
    bool operator==(const buffer<T>& lhs, const buffer<T>& rhs);

    /******************************
     * buffer_data implementation *
     ******************************/

    namespace impl
    {
        template <class T>
        bool buffer_data<T>::empty() const noexcept
        {
            return size() == size_type(0);
        }

        template <class T>
        auto buffer_data<T>::size() const noexcept -> size_type
        {
            return m_size;
        }

        template <class T>
        template <class U>
        U* buffer_data<T>::data() noexcept
        {
            return reinterpret_cast<U*>(p_data);
        }

        template <class T>
        template <class U>
        const U* buffer_data<T>::data() const noexcept
        {
            return reinterpret_cast<const U*>(p_data);
        }

        template <class T>
        void buffer_data<T>::swap(buffer_data<T>& rhs) noexcept
        {
            std::swap(p_data, rhs.p_data);
            std::swap(m_size, rhs.m_size);
        }

        template <class T>
        bool buffer_data<T>::equal(const buffer_data<T>& rhs) const
        {
            return m_size == rhs.m_size && std::equal(p_data, p_data + m_size, rhs.p_data);
        }
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
        deallocate(this->p_data);
    }
        
    template <class T>
    buffer<T>::buffer(const buffer<T>& rhs)
        : base_type{allocate(rhs.m_size), rhs.size()}
    {
        std::copy(rhs.data(), rhs.data() + rhs.size(), data());
    }

    template <class T>
    buffer<T>& buffer<T>::operator=(const buffer<T>& rhs)
    {
        if (this != &rhs)
        {
            buffer<T> tmp(rhs);
            swap(tmp);
        }
        return *this;
    }

    template <class T>
    buffer<T>::buffer(buffer&& rhs)
        : base_type{rhs.data(), rhs.size()}
    {
        rhs.p_data = nullptr;
        rhs.m_size = 0u;
    }

    template <class T>
    buffer<T>& buffer<T>::operator=(buffer<T>&& rhs)
    {
        swap(rhs);
        return *this;
    }

    template <class T>
    void buffer<T>::resize(size_type n)
    {
        // TODO: add capacity, resize if growing only and define a shrink_to_fit method
        if (n != size())
        {
            buffer<T> tmp(n);
            std::copy(data(), data() + size(), tmp.data());
            swap(tmp);
        }
    }

    template <class T>
    void buffer<T>::swap(buffer<T>& rhs) noexcept
    {
        base_type::swap(rhs);
    }

    template <class T>
    bool buffer<T>::equal(const buffer<T>& rhs) const
    {
        return base_type::equal(rhs);
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

    template <class T>
    bool operator==(const buffer<T>& lhs, const buffer<T>& rhs)
    {
        return lhs.equal(rhs);
    }
}

