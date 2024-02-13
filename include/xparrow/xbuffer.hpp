/***************************************************************************
* Copyright (c) QuantStack                                                 *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
***************************************************************************/

#pragma once

#include <algorithm>
#include <concepts>
#include <cstdint>

namespace xparrow
{
    namespace impl
    {
        template <class T>
        struct xbuffer_data
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

            void swap(xbuffer_data& rhs) noexcept;
            bool equal(const xbuffer_data& rhs) const;

            pointer p_data = nullptr;
            size_type m_size = 0;
        };
    }

    /**
     * @class xbuffer
     * @brief Object that owns a piece of contiguous memory
     */
    template <class T>
    class xbuffer : private impl::xbuffer_data<T>
    {
    public:

        using base_type = impl::xbuffer_data<T>;
        using value_type = typename base_type::value_type;
        using pointer = typename base_type::pointer;
        using size_type = typename base_type::size_type;

        xbuffer() = default;
        explicit xbuffer(size_type size);
        xbuffer(pointer data, size_type size);
        
        ~xbuffer();
        
        xbuffer(const xbuffer&);
        xbuffer& operator=(const xbuffer&);

        xbuffer(xbuffer&&);
        xbuffer& operator=(xbuffer&&);

        using base_type::empty;
        using base_type::size;
        using base_type::data;

        void resize(size_type new_size);

        void swap(xbuffer&) noexcept;
        bool equal(const xbuffer& rhs) const;

    private:

        pointer allocate(size_type size) const;
        void deallocate(pointer mem) const;
    };

    template <class T>
    bool operator==(const xbuffer<T>& lhs, const xbuffer<T>& rhs);

    /******************************
     * xbuffer_data implementation *
     ******************************/

    namespace impl
    {
        template <class T>
        bool xbuffer_data<T>::empty() const noexcept
        {
            return size() == size_type(0);
        }

        template <class T>
        auto xbuffer_data<T>::size() const noexcept -> size_type
        {
            return m_size;
        }

        template <class T>
        template <class U>
        U* xbuffer_data<T>::data() noexcept
        {
            return reinterpret_cast<U*>(p_data);
        }

        template <class T>
        template <class U>
        const U* xbuffer_data<T>::data() const noexcept
        {
            return reinterpret_cast<const U*>(p_data);
        }

        template <class T>
        void xbuffer_data<T>::swap(xbuffer_data<T>& rhs) noexcept
        {
            std::swap(p_data, rhs.p_data);
            std::swap(m_size, rhs.m_size);
        }

        template <class T>
        bool xbuffer_data<T>::equal(const xbuffer_data<T>& rhs) const
        {
            return m_size == rhs.m_size && std::equal(p_data, p_data + m_size, rhs.p_data);
        }
    }

    /*************************
     * xbuffer implementation *
     *************************/

    template <class T>
    xbuffer<T>::xbuffer(size_type size)
        : base_type{allocate(size), size}
    {
    }

    template <class T>
    xbuffer<T>::xbuffer(pointer data, size_type size)
        : base_type{data, size}
    {
    }
        
    template <class T>
    xbuffer<T>::~xbuffer()
    {
        deallocate(this->p_data);
    }
        
    template <class T>
    xbuffer<T>::xbuffer(const xbuffer<T>& rhs)
        : base_type{allocate(rhs.m_size), rhs.size()}
    {
        std::copy(rhs.data(), rhs.data() + rhs.size(), data());
    }

    template <class T>
    xbuffer<T>& xbuffer<T>::operator=(const xbuffer<T>& rhs)
    {
        if (this != &rhs)
        {
            xbuffer<T> tmp(rhs);
            swap(tmp);
        }
        return *this;
    }

    template <class T>
    xbuffer<T>::xbuffer(xbuffer&& rhs)
        : base_type{rhs.data(), rhs.size()}
    {
        rhs.p_data = nullptr;
        rhs.m_size = 0u;
    }

    template <class T>
    xbuffer<T>& xbuffer<T>::operator=(xbuffer<T>&& rhs)
    {
        swap(rhs);
        return *this;
    }

    template <class T>
    void xbuffer<T>::resize(size_type n)
    {
        // TODO: add capacity, resize if growing only and define a shrink_to_fit method
        if (n != size())
        {
            xbuffer<T> tmp(n);
            std::copy(data(), data() + size(), tmp.data());
            swap(tmp);
        }
    }

    template <class T>
    void xbuffer<T>::swap(xbuffer<T>& rhs) noexcept
    {
        base_type::swap(rhs);
    }

    template <class T>
    bool xbuffer<T>::equal(const xbuffer<T>& rhs) const
    {
        return base_type::equal(rhs);
    }

    template <class T>
    auto xbuffer<T>::allocate(size_type size) const -> pointer
    {
        return new T[size];
    }

    template <class T>
    void xbuffer<T>::deallocate(pointer mem) const
    {
        delete[] mem;
    }

    template <class T>
    bool operator==(const xbuffer<T>& lhs, const xbuffer<T>& rhs)
    {
        return lhs.equal(rhs);
    }
}

