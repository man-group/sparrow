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
#include <iterator>
#include <stdexcept>

#include "sparrow/contracts.hpp"
#include "sparrow/allocator.hpp"
#include "sparrow/iterator.hpp"
#include "sparrow/mp_utils.hpp"

namespace sparrow
{

    /**
     * Base class for buffer.
     *
     * This class provides memory management for the buffer class.
     * The constructor and destructor perform allocation and deallocation
     * of the internal storage, but do not initialize any element.
     */
    template <class T>
    class buffer_base
    {
    protected:

        using allocator_type = any_allocator<T>;
        using alloc_traits = std::allocator_traits<allocator_type>;
        using pointer = typename alloc_traits::pointer;
        using size_type = typename alloc_traits::size_type;

        struct buffer_data
        {
            pointer p_begin = nullptr;
            pointer p_end = nullptr;
            pointer p_storage_end = nullptr;

            buffer_data() = default;
            constexpr buffer_data(buffer_data&&) noexcept;
            constexpr buffer_data& operator=(buffer_data&&) noexcept;
        };

        buffer_base() = default;

        template <allocator A>
        constexpr buffer_base(const A& a) noexcept;

        template <allocator A = allocator_type>
        constexpr buffer_base(size_type n, const A& a = A());

        template <allocator A = allocator_type>
        constexpr buffer_base(pointer p, size_type n, const A& a = A());

        ~buffer_base();

        buffer_base(buffer_base&&) = default;

        template <allocator A>
        constexpr buffer_base(buffer_base&& rhs, const A& a);

        constexpr allocator_type& get_allocator() noexcept;
        constexpr const allocator_type& get_allocator() const noexcept;
        constexpr buffer_data& get_data() noexcept;
        constexpr const buffer_data& get_data() const noexcept;

        constexpr pointer allocate(size_type n);
        constexpr void deallocate(pointer p, size_type n);
        constexpr void create_storage(size_type n);
        constexpr void assign_storage(pointer p, size_type n, size_type cap);

    private:

        allocator_type m_alloc;
        buffer_data m_data;
    };

    /**
     * Object that owns a piece of contiguous memory.
     *
     * This class provides an API similar to std::vector, with
     * two main differences:
     * - it is not templated by the allocator type, but makes use of
     *   any_allocator which type-erases it.
     * - it can acquire ownership of an already allocated raw buffer.
     */
    template <class T>
    class buffer : private buffer_base<T>
    {
        using base_type = buffer_base<T>;
        using alloc_traits = typename base_type::alloc_traits;

        static_assert(
            std::same_as<std::remove_cvref_t<T>, T>,
            "buffer must have a non-const, non-volatile, non-reference value_type"
        );

    public:

        using allocator_type = typename base_type::allocator_type;
        using value_type = T;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = typename alloc_traits::pointer;
        using const_pointer = typename alloc_traits::const_pointer;
        using size_type = typename alloc_traits::size_type;
        using difference_type = typename alloc_traits::difference_type;
        using iterator = pointer_iterator<pointer>;
        using const_iterator = pointer_iterator<const_pointer>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        buffer() = default;

        template <class A>
            requires(not std::same_as<A, buffer<T>> and allocator<A>)
        constexpr explicit buffer(const A& a)
            : base_type(a)
        {
        }

        template <allocator A = allocator_type>
        constexpr explicit buffer(size_type n, const A& a = A());

        template <allocator A = allocator_type>
        constexpr buffer(size_type n, const value_type& v, const A& a = A());

        template <allocator A = allocator_type>
        constexpr buffer(pointer p, size_type n, const A& a = A());

        template <allocator A = allocator_type>
        constexpr buffer(std::initializer_list<value_type> init, const A& a = A());

        template <class It, allocator A = allocator_type>
        constexpr buffer(It first, It last, const A& a = A());

        ~buffer();

        buffer(const buffer& rhs);

        template <allocator A>
        buffer(const buffer& rhs, const A& a);

        buffer(buffer&& rhs) = default;

        template <allocator A>
        buffer(buffer&& rhs, const A& a);

        buffer& operator=(const buffer& rhs);
        buffer& operator=(buffer&& rhs);
        buffer& operator=(std::initializer_list<value_type> init);

        // Element access

        constexpr reference operator[](size_type i);
        constexpr const_reference operator[](size_type i) const;

        constexpr reference front();
        constexpr const_reference front() const;

        constexpr reference back();
        constexpr const_reference back() const;

        // TODO: make this non template and make a buffer_caster class
        template <class U = T>
        constexpr U* data() noexcept;

        // TODO: make this non template and make a buffer_caster class
        template <class U = T>
        constexpr const U* data() const noexcept;

        // Iterators

        constexpr iterator begin() noexcept;
        constexpr iterator end() noexcept;

        constexpr const_iterator begin() const noexcept;
        constexpr const_iterator end() const noexcept;

        constexpr const_iterator cbegin() const noexcept;
        constexpr const_iterator cend() const noexcept;

        constexpr reverse_iterator rbegin() noexcept;
        constexpr reverse_iterator rend() noexcept;

        constexpr const_reverse_iterator rbegin() const noexcept;
        constexpr const_reverse_iterator rend() const noexcept;

        constexpr const_reverse_iterator crbegin() const noexcept;
        constexpr const_reverse_iterator crend() const noexcept;

        // Capacity

        constexpr bool empty() const noexcept;
        constexpr size_type capacity() const noexcept;
        constexpr size_type size() const noexcept;
        constexpr size_type max_size() const noexcept;
        constexpr void reserve(size_type new_cap);
        constexpr void shrink_to_fit();

        // Modifiers

        constexpr void clear();
        constexpr void resize(size_type new_size);
        constexpr void resize(size_type new_size, const value_type& value);
        constexpr void swap(buffer& rhs);

    private:

        using base_type::get_allocator;
        using base_type::get_data;

        template <class F>
        constexpr void resize_impl(size_type new_size, F&& initializer);

        template <class It>
        constexpr void assign_range_impl(It first, It last, std::forward_iterator_tag);

        constexpr void erase_at_end(pointer p);

        template <class It>
        constexpr pointer allocate_and_copy(size_type n, It first, It last);

        // The following methods are static because:
        // - they accept an allocator argument, and do not depend on
        //   the state of buffer
        // - taking an allocator argument instead of relying on get_allocator
        //   will make it easier to support allocators that propagate
        //   on copy / move, as these methods will be called with allocators
        //   from different instances of buffers.

        static constexpr size_type check_init_length(size_type n, const allocator_type& a);

        static constexpr size_type max_size_impl(const allocator_type& a) noexcept;

        static constexpr pointer default_initialize(pointer begin, size_type n, allocator_type& a);

        static constexpr pointer
        fill_initialize(pointer begin, size_type n, const value_type& v, allocator_type& a);

        template <class It>
        static constexpr pointer copy_initialize(It first, It last, pointer begin, allocator_type& a);

        static constexpr void destroy(pointer first, pointer last, allocator_type& a);
    };

    template <class T>
    constexpr bool operator==(const buffer<T>& lhs, const buffer<T>& rhs) noexcept;

    /******************************
     * buffer_base implementation *
     ******************************/

    template <class T>
    constexpr buffer_base<T>::buffer_data::buffer_data(buffer_data&& rhs) noexcept
        : p_begin(rhs.p_begin)
        , p_end(rhs.p_end)
        , p_storage_end(rhs.p_storage_end)
    {
        rhs.p_begin = nullptr;
        rhs.p_end = nullptr;
        rhs.p_storage_end = nullptr;
    }

    template <class T>
    constexpr auto buffer_base<T>::buffer_data::operator=(buffer_data&& rhs) noexcept -> buffer_data&
    {
        std::swap(p_begin, rhs.p_begin);
        std::swap(p_end, rhs.p_end);
        std::swap(p_storage_end, rhs.p_storage_end);
        return *this;
    }

    template <class T>
    template <allocator A>
    constexpr buffer_base<T>::buffer_base(const A& a) noexcept
        : m_alloc(a)
    {
    }

    template <class T>
    template <allocator A>
    constexpr buffer_base<T>::buffer_base(size_type n, const A& a)
        : m_alloc(a)
    {
        create_storage(n);
    }

    template <class T>
    template <allocator A>
    constexpr buffer_base<T>::buffer_base(pointer p, size_type n, const A& a)
        : m_alloc(a)
    {
        assign_storage(p, n, n);
    }

    template <class T>
    buffer_base<T>::~buffer_base()
    {
        deallocate(m_data.p_begin, (m_data.p_storage_end - m_data.p_begin));
    }

    template <class T>
    template <allocator A>
    constexpr buffer_base<T>::buffer_base(buffer_base&& rhs, const A& a)
        : m_alloc(a)
        , m_data(std::move(rhs.m_data))
    {
    }

    template <class T>
    constexpr auto buffer_base<T>::get_allocator() noexcept -> allocator_type&
    {
        return m_alloc;
    }

    template <class T>
    constexpr auto buffer_base<T>::get_allocator() const noexcept -> const allocator_type&
    {
        return m_alloc;
    }

    template <class T>
    constexpr auto buffer_base<T>::get_data() noexcept -> buffer_data&
    {
        return m_data;
    }

    template <class T>
    constexpr auto buffer_base<T>::get_data() const noexcept -> const buffer_data&
    {
        return m_data;
    }

    template <class T>
    constexpr auto buffer_base<T>::allocate(size_type n) -> pointer
    {
        return alloc_traits::allocate(m_alloc, n);
    }

    template <class T>
    constexpr void buffer_base<T>::deallocate(pointer p, size_type n)
    {
        alloc_traits::deallocate(m_alloc, p, n);
    }

    template <class T>
    constexpr void buffer_base<T>::create_storage(size_type n)
    {
        m_data.p_begin = allocate(n);
        m_data.p_end = m_data.p_begin + n;
        m_data.p_storage_end = m_data.p_begin + n;
    }

    template <class T>
    constexpr void buffer_base<T>::assign_storage(pointer p, size_type n, size_type cap)
    {
        SPARROW_ASSERT_TRUE(n <= cap);
        m_data.p_begin = p;
        m_data.p_end = p + n;
        m_data.p_storage_end = p + cap;
    }

    /*************************
     * buffer implementation *
     *************************/

    template <class T>
    template <allocator A>
    constexpr buffer<T>::buffer(size_type n, const A& a)
        : base_type(check_init_length(n, a), a)
    {
        get_data().p_end = default_initialize(get_data().p_begin, n, get_allocator());
    }

    template <class T>
    template <allocator A>
    constexpr buffer<T>::buffer(size_type n, const value_type& v, const A& a)
        : base_type(check_init_length(n, a), a)
    {
        get_data().p_end = fill_initialize(get_data().p_begin, n, v, get_allocator());
    }

    template <class T>
    template <allocator A>
    constexpr buffer<T>::buffer(pointer p, size_type n, const A& a)
        : base_type(p, check_init_length(n, a), a)
    {
    }

    template <class T>
    template <allocator A>
    constexpr buffer<T>::buffer(std::initializer_list<value_type> init, const A& a)
        : base_type(check_init_length(init.size(), a), a)
    {
        get_data().p_end = copy_initialize(init.begin(), init.end(), get_data().p_begin, get_allocator());
    }

    template <class T>
    template <class It, allocator A>
    constexpr buffer<T>::buffer(It first, It last, const A& a)
        : base_type(check_init_length(std::distance(first, last), a), a)
    {
        get_data().p_end = copy_initialize(first, last, get_data().p_begin, get_allocator());
    }

    template <class T>
    buffer<T>::~buffer()
    {
        destroy(get_data().p_begin, get_data().p_end, get_allocator());
    }

    template <class T>
    buffer<T>::buffer(const buffer& rhs)
        : base_type(rhs.size(), rhs.get_allocator())
    {
        get_data().p_end = copy_initialize(rhs.begin(), rhs.end(), get_data().p_begin, get_allocator());
    }

    template <class T>
    template <allocator A>
    buffer<T>::buffer(const buffer& rhs, const A& a)
        : base_type(rhs.size(), a)
    {
        get_data().p_end = copy_initialize(rhs.begin(), rhs.end(), get_data().p_begin, get_allocator());
    }

    template <class T>
    template <allocator A>
    buffer<T>::buffer(buffer&& rhs, const A& a)
        : base_type(a)
    {
        if (rhs.get_allocator() == get_allocator())
        {
            get_data() = std::move(rhs.m_data);
        }
        else if (!rhs.empty())
        {
            this->create_storage(rhs.size());
            get_data().p_end = copy_initialize(rhs.begin(), rhs.end(), get_data().p_begin, get_allocator());
            rhs.clear();
        }
    }

    template <class T>
    buffer<T>& buffer<T>::operator=(const buffer& rhs)
    {
        if (std::addressof(rhs) != this)
        {
            // We assume that any_allocator never propagates on assign
            assign_range_impl(rhs.get_data().p_begin, rhs.get_data().p_end, std::random_access_iterator_tag());
        }
        return *this;
    }

    template <class T>
    buffer<T>& buffer<T>::operator=(buffer&& rhs)
    {
        if (get_allocator() == rhs.get_allocator())
        {
            get_data() = std::move(rhs.get_data());
        }
        else
        {
            assign_range_impl(
                std::make_move_iterator(rhs.begin()),
                std::make_move_iterator(rhs.end()),
                std::random_access_iterator_tag()
            );
            rhs.clear();
        }
        return *this;
    }

    template <class T>
    buffer<T>& buffer<T>::operator=(std::initializer_list<value_type> init)
    {
        assign_range_impl(
            std::make_move_iterator(init.begin()),
            std::make_move_iterator(init.end()),
            std::random_access_iterator_tag()
        );
        return *this;
    }

    template <class T>
    constexpr auto buffer<T>::operator[](size_type i) -> reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return get_data().p_begin[i];
    }

    template <class T>
    constexpr auto buffer<T>::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return get_data().p_begin[i];
    }

    template <class T>
    constexpr auto buffer<T>::front() -> reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return *(get_data().p_begin);
    }

    template <class T>
    constexpr auto buffer<T>::front() const -> const_reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return *(get_data().p_begin);
    }

    template <class T>
    constexpr auto buffer<T>::back() -> reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return *(get_data().p_end - 1);
    }

    template <class T>
    constexpr auto buffer<T>::back() const -> const_reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return *(get_data().p_end - 1);
    }

    template <class T>
    template <class U>
    constexpr U* buffer<T>::data() noexcept
    {
        return reinterpret_cast<U*>(get_data().p_begin);
    }

    template <class T>
    template <class U>
    constexpr const U* buffer<T>::data() const noexcept
    {
        return reinterpret_cast<U*>(get_data().p_begin);
    }

    template <class T>
    constexpr auto buffer<T>::begin() noexcept -> iterator
    {
        return make_pointer_iterator(get_data().p_begin);
    }

    template <class T>
    constexpr auto buffer<T>::end() noexcept -> iterator
    {
        return make_pointer_iterator(get_data().p_end);
    }

    template <class T>
    constexpr auto buffer<T>::begin() const noexcept -> const_iterator
    {
        return cbegin();
    }

    template <class T>
    constexpr auto buffer<T>::end() const noexcept -> const_iterator
    {
        return cend();
    }

    template <class T>
    constexpr auto buffer<T>::cbegin() const noexcept -> const_iterator
    {
        return make_pointer_iterator(const_pointer(get_data().p_begin));
    }

    template <class T>
    constexpr auto buffer<T>::cend() const noexcept -> const_iterator
    {
        return make_pointer_iterator(const_pointer(get_data().p_end));
    }

    template <class T>
    constexpr auto buffer<T>::rbegin() noexcept -> reverse_iterator
    {
        return reverse_iterator(end());
    }

    template <class T>
    constexpr auto buffer<T>::rend() noexcept -> reverse_iterator
    {
        return reverse_iterator(begin());
    }

    template <class T>
    constexpr auto buffer<T>::rbegin() const noexcept -> const_reverse_iterator
    {
        return crbegin();
    }

    template <class T>
    constexpr auto buffer<T>::rend() const noexcept -> const_reverse_iterator
    {
        return crend();
    }

    template <class T>
    constexpr auto buffer<T>::crbegin() const noexcept -> const_reverse_iterator
    {
        return const_reverse_iterator(end());
    }

    template <class T>
    constexpr auto buffer<T>::crend() const noexcept -> const_reverse_iterator
    {
        return const_reverse_iterator(begin());
    }

    template <class T>
    constexpr bool buffer<T>::empty() const noexcept
    {
        return get_data().p_begin == get_data().p_end;
    }

    template <class T>
    constexpr auto buffer<T>::capacity() const noexcept -> size_type
    {
        return static_cast<size_type>(get_data().p_storage_end - get_data().p_begin);
    }

    template <class T>
    constexpr auto buffer<T>::size() const noexcept -> size_type
    {
        return static_cast<size_type>(get_data().p_end - get_data().p_begin);
    }

    template <class T>
    constexpr auto buffer<T>::max_size() const noexcept -> size_type
    {
        return max_size_impl(get_allocator());
    }

    template <class T>
    constexpr void buffer<T>::reserve(size_type new_cap)
    {
        if (new_cap > max_size())
        {
            throw std::length_error("buffer::reserve called with new_cap > max_size()");
        }
        if (new_cap > capacity())
        {
            const size_type old_size = size();
            pointer tmp = allocate_and_copy(
                new_cap,
                std::make_move_iterator(get_data().p_begin),
                std::make_move_iterator(get_data().p_end)
            );
            destroy(get_data().p_begin, get_data().p_end, get_allocator());
            this->deallocate(get_data().p_begin, get_data().p_storage_end - get_data().p_begin);
            this->assign_storage(tmp, old_size, new_cap);
        }
    }

    template <class T>
    constexpr void buffer<T>::shrink_to_fit()
    {
        if (capacity() != size())
        {
            buffer(std::make_move_iterator(begin()), std::make_move_iterator(end()), get_allocator()).swap(*this);
        }
    }

    template <class T>
    constexpr void buffer<T>::clear()
    {
        erase_at_end(get_data().p_begin);
    }

    template <class T>
    constexpr void buffer<T>::resize(size_type new_size)
    {
        resize_impl(
            new_size,
            [this](size_type nb_init)
            {
                get_data().p_end = default_initialize(get_data().p_end, nb_init, get_allocator());
            }
        );
    }

    template <class T>
    constexpr void buffer<T>::resize(size_type new_size, const value_type& value)
    {
        resize_impl(
            new_size,
            [this, &value](size_type nb_init)
            {
                get_data().p_end = fill_initialize(get_data().p_end, nb_init, value, get_allocator());
            }
        );
    }

    template <class T>
    constexpr void buffer<T>::swap(buffer& rhs)
    {
        std::swap(this->get_data(), rhs.get_data());
    }

    template <class T>
    template <class F>
    constexpr void buffer<T>::resize_impl(size_type new_size, F&& initializer)
    {
        if (new_size > size())
        {
            const std::size_t nb_init = new_size - size();
            if (new_size <= capacity())
            {
                initializer(nb_init);
            }
            else
            {
                reserve(new_size);
                initializer(nb_init);
            }
        }
        else if (new_size < size())
        {
            erase_at_end(get_data().p_begin + new_size);
        }
    }

    template <class T>
    template <class It>
    constexpr void buffer<T>::assign_range_impl(It first, It last, std::forward_iterator_tag)
    {
        const size_type sz = size();
        const size_type len = std::distance(first, last);
        if (len > capacity())
        {
            check_init_length(len, get_allocator());
            pointer p = allocate_and_copy(len, first, last);
            destroy(get_data().p_begin, get_data().p_end, get_allocator());
            this->deallocate(get_data().p_begin, capacity());
            this->assign_storage(p, len, len);
        }
        else if (sz >= len)
        {
            pointer p = std::copy(first, last, get_data().p_begin);
            erase_at_end(p);
        }
        else
        {
            It mid = first;
            std::advance(mid, sz);
            std::copy(first, mid, get_data().p_begin);
            get_data().p_end = copy_initialize(mid, last, get_data().p_end, get_allocator());
        }
    }

    template <class T>
    constexpr void buffer<T>::erase_at_end(pointer p)
    {
        destroy(p, get_data().p_end, get_allocator());
        get_data().p_end = p;
    }

    template <class T>
    template <class It>
    constexpr auto buffer<T>::allocate_and_copy(size_type n, It first, It last) -> pointer
    {
        pointer p = this->allocate(n);
        try
        {
            copy_initialize(first, last, p, get_allocator());
        }
        catch (...)
        {
            this->deallocate(p, n);
            throw;
        }
        return p;
    }

    template <class T>
    constexpr auto buffer<T>::check_init_length(size_type n, const allocator_type& a) -> size_type
    {
        if (n > max_size_impl(a))
        {
            throw std::length_error("cannot create buffer larger than max_size()");
        }
        return n;
    }

    template <class T>
    constexpr auto buffer<T>::max_size_impl(const allocator_type& a) noexcept -> size_type
    {
        const size_type diff_max = std::numeric_limits<difference_type>::max();
        const size_type alloc_max = std::allocator_traits<allocator_type>::max_size(a);
        return (std::min)(diff_max, alloc_max);
    }

    template <class T>
    constexpr auto buffer<T>::default_initialize(pointer begin, size_type n, allocator_type& a) -> pointer
    {
        pointer current = begin;
        for (; n > 0; --n, ++current)
        {
            alloc_traits::construct(a, current);
        }
        return current;
    }

    template <class T>
    constexpr auto
    buffer<T>::fill_initialize(pointer begin, size_type n, const value_type& v, allocator_type& a) -> pointer
    {
        pointer current = begin;
        for (; n > 0; --n, ++current)
        {
            alloc_traits::construct(a, current, v);
        }
        return current;
    }

    template <class T>
    template <class It>
    constexpr auto buffer<T>::copy_initialize(It first, It last, pointer begin, allocator_type& a) -> pointer
    {
        pointer current = begin;
        for (; first != last; ++first, ++current)
        {
            alloc_traits::construct(a, current, *first);
        }
        return current;
    }

    template <class T>
    constexpr void buffer<T>::destroy(pointer first, pointer last, allocator_type& a)
    {
        for (; first != last; ++first)
        {
            alloc_traits::destroy(a, first);
        }
    }

    template <class T>
    constexpr bool operator==(const buffer<T>& lhs, const buffer<T>& rhs) noexcept
    {
        return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
}
