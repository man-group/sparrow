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
#include <cstddef>
#include <iterator>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <type_traits>

#include "sparrow/buffer/allocator.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/memory_alignment.hpp"
#include "sparrow/utils/mp_utils.hpp"


#if not defined(SPARROW_BUFFER_GROWTH_FACTOR)
#    define SPARROW_BUFFER_GROWTH_FACTOR 2
#endif

namespace sparrow
{
    template <typename T>
    concept is_buffer_view = requires(T t) { typename T::is_buffer_view; };

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

            constexpr buffer_data() noexcept = default;
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

        constexpr buffer_base(buffer_base&&) noexcept = default;

        template <allocator A>
        constexpr buffer_base(buffer_base&& rhs, const A& a);

        [[nodiscard]] constexpr allocator_type& get_allocator() noexcept;
        [[nodiscard]] constexpr const allocator_type& get_allocator() const noexcept;
        [[nodiscard]] constexpr buffer_data& get_data() noexcept;
        [[nodiscard]] constexpr const buffer_data& get_data() const noexcept;

        constexpr pointer allocate(size_type n);
        constexpr void deallocate(pointer p, size_type n);
        constexpr void create_storage(size_type n);
        constexpr void assign_storage(pointer p, size_type n, size_type cap);

    private:

        constexpr pointer allocate_aligned(size_type n);

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

        template <std::ranges::input_range Range, allocator A = allocator_type>
            requires(std::same_as<std::ranges::range_value_t<Range>, T> && !is_buffer_view<Range>)
        constexpr buffer(const Range& range, const A& a = A());

        ~buffer();

        constexpr buffer(const buffer& rhs);

        template <allocator A>
        constexpr buffer(const buffer& rhs, const A& a);

        constexpr buffer(buffer&& rhs) noexcept = default;

        template <allocator A>
        constexpr buffer(buffer&& rhs, const A& a);

        constexpr buffer& operator=(const buffer& rhs);
        constexpr buffer& operator=(buffer&& rhs);
        constexpr buffer& operator=(std::initializer_list<value_type> init);

        // Element access

        [[nodiscard]] constexpr reference operator[](size_type i);
        [[nodiscard]] constexpr const_reference operator[](size_type i) const;

        [[nodiscard]] constexpr reference front();
        [[nodiscard]] constexpr const_reference front() const;

        [[nodiscard]] constexpr reference back();
        [[nodiscard]] constexpr const_reference back() const;

        // TODO: make this non template and make a buffer_caster class
        template <class U = T>
        [[nodiscard]] constexpr U* data() noexcept;

        // TODO: make this non template and make a buffer_caster class
        template <class U = T>
        [[nodiscard]] constexpr const U* data() const noexcept;

        // Iterators

        [[nodiscard]] constexpr iterator begin() noexcept;
        [[nodiscard]] constexpr iterator end() noexcept;

        [[nodiscard]] constexpr const_iterator begin() const noexcept;
        [[nodiscard]] constexpr const_iterator end() const noexcept;

        [[nodiscard]] constexpr const_iterator cbegin() const noexcept;
        [[nodiscard]] constexpr const_iterator cend() const noexcept;

        [[nodiscard]] constexpr reverse_iterator rbegin() noexcept;
        [[nodiscard]] constexpr reverse_iterator rend() noexcept;

        [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept;
        [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept;

        [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept;
        [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept;
        [[nodiscard]]
        // Capacity

        [[nodiscard]] constexpr bool
        empty() const noexcept;
        [[nodiscard]] constexpr size_type capacity() const noexcept;
        [[nodiscard]] constexpr size_type size() const noexcept;
        [[nodiscard]] constexpr size_type max_size() const noexcept;
        constexpr void reserve(size_type new_cap);
        constexpr void shrink_to_fit();

        // Modifiers

        constexpr void clear();

        constexpr iterator insert(const_iterator pos, const T& value);
        constexpr iterator insert(const_iterator pos, T&& value);
        constexpr iterator insert(const_iterator pos, size_type count, const T& value);
        template <mpl::iterator_of_type<T> InputIt>
        constexpr iterator insert(const_iterator pos, InputIt first, InputIt last);
        template <std::ranges::input_range R>
            requires std::same_as<std::ranges::range_value_t<R>, T>
        constexpr iterator insert(const_iterator pos, R&& range);
        constexpr iterator insert(const_iterator pos, std::initializer_list<T> ilist);

        template <class... Args>
        constexpr iterator emplace(const_iterator pos, Args&&... args);

        constexpr iterator erase(const_iterator pos);
        constexpr iterator erase(const_iterator first, const_iterator last);

        constexpr void push_back(const T& value);
        constexpr void push_back(T&& value);

        constexpr void pop_back();

        constexpr void resize(size_type new_size);
        constexpr void resize(size_type new_size, const value_type& value);
        constexpr void swap(buffer& rhs) noexcept;

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

        constexpr void reserve_with_growth_factor(size_type new_cap);

        // The following methods are static because:
        // - they accept an allocator argument, and do not depend on
        //   the state of buffer
        // - taking an allocator argument instead of relying on get_allocator
        //   will make it easier to support allocators that propagate
        //   on copy / move, as these methods will be called with allocators
        //   from different instances of buffers.

        static constexpr size_type check_init_length(size_type n, const allocator_type& a);

        [[nodiscard]] static constexpr size_type max_size_impl(const allocator_type& a) noexcept;

        [[nodiscard]] static constexpr pointer
        default_initialize(pointer begin, size_type n, allocator_type& a);

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
        SPARROW_ASSERT_TRUE((p != nullptr) || (p == nullptr && n == 0));
        assign_storage(p, n, n);
    }

    template <class T>
    buffer_base<T>::~buffer_base()
    {
        deallocate(m_data.p_begin, static_cast<size_type>(m_data.p_storage_end - m_data.p_begin));
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
        return allocate_aligned(n);
    }

    template <class T>
    constexpr void buffer_base<T>::deallocate(pointer p, size_type n)
    {
        alloc_traits::deallocate(m_alloc, p, n);
    }

    template <class T>
    constexpr void buffer_base<T>::create_storage(size_type n)
    {
        m_data.p_begin = allocate_aligned(n);
        m_data.p_end = m_data.p_begin + n;
        m_data.p_storage_end = m_data.p_begin + calculate_aligned_size<T>(n) / sizeof(T);
    }

    template <class T>
    constexpr void buffer_base<T>::assign_storage(pointer p, size_type n, size_type cap)
    {
        SPARROW_ASSERT_TRUE(n <= cap);
        m_data.p_begin = p;
        m_data.p_end = p + n;
        m_data.p_storage_end = p + cap;
    }

    template <class T>
    constexpr auto buffer_base<T>::allocate_aligned(size_type n) -> pointer
    {
        if (n == 0)
        {
            return nullptr;
        }

        // Calculate the aligned size in elements
        const size_type byte_size = n * sizeof(T);
        const size_type aligned_byte_size = align_to_64_bytes(byte_size);
        const size_type aligned_element_count = aligned_byte_size / sizeof(T);

        // Simply allocate the aligned size
        // Modern allocators typically return aligned memory for larger allocations
        return alloc_traits::allocate(m_alloc, aligned_element_count);
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
        : base_type(check_init_length(static_cast<size_type>(std::distance(first, last)), a), a)
    {
        get_data().p_end = copy_initialize(first, last, get_data().p_begin, get_allocator());
    }

    template <class T>
    template <std::ranges::input_range Range, allocator A>
        requires(std::same_as<std::ranges::range_value_t<Range>, T> && !is_buffer_view<Range>)
    constexpr buffer<T>::buffer(const Range& range, const A& a)
        : base_type(check_init_length(static_cast<size_type>(std::ranges::size(range)), a), a)
    {
        get_data().p_end = copy_initialize(
            std::ranges::begin(range),
            std::ranges::end(range),
            get_data().p_begin,
            get_allocator()
        );
    }

    template <class T>
    buffer<T>::~buffer()
    {
        destroy(get_data().p_begin, get_data().p_end, get_allocator());
    }

    template <class T>
    constexpr buffer<T>::buffer(const buffer& rhs)
        : base_type(rhs.get_allocator())
    {
        if (rhs.get_data().p_begin != nullptr)
        {
            this->create_storage(rhs.size());
            get_data().p_end = copy_initialize(rhs.begin(), rhs.end(), get_data().p_begin, get_allocator());
        }
    }

    template <class T>
    template <allocator A>
    constexpr buffer<T>::buffer(const buffer& rhs, const A& a)
        : base_type(a)
    {
        if (rhs.get_data().p_begin != nullptr)
        {
            this->create_storage(rhs.size());
            get_data().p_end = copy_initialize(rhs.begin(), rhs.end(), get_data().p_begin, get_allocator());
        }
    }

    template <class T>
    template <allocator A>
    constexpr buffer<T>::buffer(buffer&& rhs, const A& a)
        : base_type(a)
    {
        if (rhs.get_allocator() == get_allocator())
        {
            get_data() = std::move(rhs.m_data);
        }
        else if (!rhs.empty())
        {
            if (rhs.get_data().p_begin != nullptr)
            {
                this->create_storage(rhs.size());
                get_data().p_end = copy_initialize(rhs.begin(), rhs.end(), get_data().p_begin, get_allocator());
            }
            rhs.clear();
        }
    }

    template <class T>
    constexpr buffer<T>& buffer<T>::operator=(const buffer& rhs)
    {
        if (std::addressof(rhs) != this)
        {
            if (rhs.get_data().p_begin != nullptr)
            {
                // We assume that any_allocator never propagates on assign
                assign_range_impl(rhs.get_data().p_begin, rhs.get_data().p_end, std::random_access_iterator_tag());
            }
            else
            {
                clear();
                this->deallocate(
                    get_data().p_begin,
                    static_cast<size_type>(get_data().p_storage_end - get_data().p_begin)
                );
                this->assign_storage(nullptr, 0, 0);
            }
        }
        return *this;
    }

    template <class T>
    constexpr buffer<T>& buffer<T>::operator=(buffer&& rhs)
    {
        if (get_allocator() == rhs.get_allocator())
        {
            get_data() = std::move(rhs.get_data());
        }
        else
        {
            if (rhs.get_data().p_begin != nullptr)
            {
                assign_range_impl(
                    std::make_move_iterator(rhs.begin()),
                    std::make_move_iterator(rhs.end()),
                    std::random_access_iterator_tag()
                );
            }
            else
            {
                clear();
            }

            rhs.clear();
        }
        return *this;
    }

    template <class T>
    constexpr buffer<T>& buffer<T>::operator=(std::initializer_list<value_type> init)
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
#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-align"
#endif
        return reinterpret_cast<U*>(get_data().p_begin);
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif
    }

    template <class T>
    template <class U>
    constexpr const U* buffer<T>::data() const noexcept
    {
#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-align"
#endif
        return reinterpret_cast<U*>(get_data().p_begin);
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif
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
            if (data() == nullptr)
            {
                this->create_storage(new_cap);
                get_data().p_end = get_data().p_begin;
            }
            else
            {
                const size_type old_size = size();
                pointer tmp = allocate_and_copy(
                    new_cap,
                    std::make_move_iterator(get_data().p_begin),
                    std::make_move_iterator(get_data().p_end)
                );
                destroy(get_data().p_begin, get_data().p_end, get_allocator());
                this->deallocate(
                    get_data().p_begin,
                    static_cast<size_type>(get_data().p_storage_end - get_data().p_begin)
                );
                this->assign_storage(tmp, old_size, new_cap);
            }
        }
    }

    template <class T>
    constexpr void buffer<T>::reserve_with_growth_factor(size_type new_cap)
    {
        if (new_cap > capacity())
        {
            reserve(new_cap * SPARROW_BUFFER_GROWTH_FACTOR);
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
        if (get_data().p_begin != nullptr)
        {
            erase_at_end(get_data().p_begin);
        }
    }

    template <class T>
    constexpr auto buffer<T>::insert(const_iterator pos, const T& value) -> iterator
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        return emplace(pos, value);
    }

    template <class T>
    constexpr auto buffer<T>::insert(const_iterator pos, T&& value) -> iterator
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        return emplace(pos, std::move(value));
    }

    template <class T>
    constexpr auto buffer<T>::insert(const_iterator pos, size_type count, const T& value) -> iterator
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());

        const difference_type offset = std::distance(cbegin(), pos);
        if (count != 0)
        {
            reserve_with_growth_factor(size() + count);
            const iterator it = std::next(begin(), offset);
            std::move_backward(it, end(), std::next(end(), static_cast<difference_type>(count)));
            std::fill_n(it, count, value);
            get_data().p_end += count;
        }
        return std::next(begin(), offset);
    }

    template <typename T>
    struct is_move_iterator : std::false_type
    {
    };

    template <typename Iterator>
    struct is_move_iterator<std::move_iterator<Iterator>> : std::true_type
    {
    };

    template <typename T>
    constexpr bool is_move_iterator_v = is_move_iterator<T>::value;

    template <class T>
    template <mpl::iterator_of_type<T> InputIt>
    constexpr auto buffer<T>::insert(const_iterator pos, InputIt first, InputIt last) -> iterator
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos && pos <= cend());
        const difference_type num_elements = std::distance(first, last);
        const size_type new_size = size() + static_cast<size_type>(num_elements);
        const difference_type offset = std::distance(cbegin(), pos);
        const size_type old_size = size();
        reserve_with_growth_factor(new_size);
        resize(new_size);
        const iterator new_pos = std::next(begin(), offset);
        const iterator end_it = std::next(begin(), static_cast<difference_type>(old_size));
        std::move_backward(new_pos, end_it, end());
        if constexpr (is_move_iterator_v<InputIt>)
        {
            std::uninitialized_move(first, last, new_pos);
        }
        else
        {
            std::uninitialized_copy(first, last, new_pos);
        }
        return new_pos;
    }

    template <class T>
    template <std::ranges::input_range R>
        requires std::same_as<std::ranges::range_value_t<R>, T>
    constexpr auto buffer<T>::insert(const_iterator pos, R&& range) -> iterator
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        return insert(pos, std::ranges::begin(range), std::ranges::end(range));
    }

    template <class T>
    constexpr auto buffer<T>::insert(const_iterator pos, std::initializer_list<T> ilist) -> iterator
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        return insert(pos, ilist.begin(), ilist.end());
    }

    template <class T>
    template <class... Args>
    constexpr buffer<T>::iterator buffer<T>::emplace(const_iterator pos, Args&&... args)
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= cend());
        const difference_type offset = std::distance(cbegin(), pos);
        reserve_with_growth_factor(size() + 1);
        pointer p = get_data().p_begin + offset;
        if (p != get_data().p_end)
        {
            alloc_traits::construct(get_allocator(), get_data().p_end, std::move(*(get_data().p_end - 1)));
            std::move_backward(p, get_data().p_end - 1, get_data().p_end);
            alloc_traits::construct(get_allocator(), p, std::forward<Args>(args)...);
        }
        else
        {
            alloc_traits::construct(get_allocator(), get_data().p_end, std::forward<Args>(args)...);
        }
        ++get_data().p_end;
        return iterator(p);
    }

    template <class T>
    constexpr auto buffer<T>::erase(const_iterator pos) -> iterator
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos < cend());
        return erase(pos, pos + 1);
    }

    template <class T>
    constexpr auto buffer<T>::erase(const_iterator first, const_iterator last) -> iterator
    {
        SPARROW_ASSERT_TRUE(first < last);
        SPARROW_ASSERT_TRUE(cbegin() <= first);
        SPARROW_ASSERT_TRUE(last <= cend());
        const difference_type offset = std::distance(cbegin(), first);
        const difference_type len = std::distance(first, last);
        pointer p = get_data().p_begin + offset;
        erase_at_end(std::move(p + len, get_data().p_end, p));
        return iterator(p);
    }

    template <class T>
    constexpr void buffer<T>::push_back(const T& value)
    {
        emplace(cend(), value);
    }

    template <class T>
    constexpr void buffer<T>::push_back(T&& value)
    {
        emplace(cend(), std::move(value));
    }

    template <class T>
    constexpr void buffer<T>::pop_back()
    {
        SPARROW_ASSERT_FALSE(empty());
        destroy(get_allocator(), get_data().p_end - 1);
        --get_data().p_end;
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
    constexpr void buffer<T>::swap(buffer& rhs) noexcept
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
        const size_type len = static_cast<size_type>(std::distance(first, last));
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
        const size_type diff_max = static_cast<size_type>(std::numeric_limits<difference_type>::max());
        const size_type alloc_max = std::allocator_traits<allocator_type>::max_size(a);
        return (std::min) (diff_max, alloc_max);
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
        SPARROW_ASSERT_TRUE(first <= last);
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
