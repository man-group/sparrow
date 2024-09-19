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
#include <ranges>

#include "sparrow_v01/layout/array_base.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/nullable.hpp"

// For the definition of empty_iterator
#include "sparrow/layout/null_layout.hpp"

namespace sparrow
{
    // TODO: uncomment when null_layout.hpp is removed
    /*
     * @class empty_iterator
     *
     * @brief Iterator used by the null_layout class.
     *
     * @tparam T the value_type of the iterator
     */
    /*template <class T>
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
    };*/

    class null_array final : public array_base
    {
    public:

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

        explicit null_array(arrow_proxy);
        virtual ~null_array() = default;

        size_type size() const;

        reference operator[](size_type i);
        const_reference operator[](size_type i) const;

        iterator begin();
        iterator end();

        const_iterator begin() const;
        const_iterator end() const;

        const_iterator cbegin() const;
        const_iterator cend() const;

        const_value_range values() const;
        const_bitmap_range bitmap() const;
    
    private:

        difference_type ssize() const;

        null_array(const null_array&) = default;
        null_array* clone_impl() const override;

        arrow_proxy m_proxy;
    };

    // TODO: uncomment when null_layout.hpp is removed
    /*********************************
     * empty_iterator implementation *
     *********************************/

    /*template <class T>
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
    }*/

    /*****************************
     * null_array implementation *
     *****************************/

    inline null_array::null_array(arrow_proxy proxy)
        : array_base(proxy.data_type())
        , m_proxy(std::move(proxy))
    {
        SPARROW_ASSERT_TRUE(m_proxy.data_type() == data_type::NA);
    }

    inline auto null_array::size() const -> size_type
    {
        return m_proxy.length();
    }

    inline auto null_array::operator[](size_type i) -> reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return *(begin());
    }

    inline auto null_array::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return *(cbegin());
    }

    inline auto null_array::begin() -> iterator
    {
        return iterator(0);
    }

    inline auto null_array::end() -> iterator
    {
        return iterator(ssize());
    }

    inline auto null_array::begin() const -> const_iterator
    {
        return cbegin();
    }

    inline auto null_array::end() const -> const_iterator
    {
        return cend();
    }

    inline auto null_array::cbegin() const -> const_iterator
    {
        return const_iterator(0);
    }

    inline auto null_array::cend() const -> const_iterator
    {
        return const_iterator(ssize());
    }

    inline auto null_array::values() const -> const_value_range
    {
        return std::ranges::subrange(const_value_iterator(0), const_value_iterator(ssize()));
    }

    inline auto null_array::bitmap() const -> const_bitmap_range
    {
        return std::ranges::subrange(const_bitmap_iterator(0), const_bitmap_iterator(ssize()));
    }

    inline auto null_array::ssize() const -> difference_type
    {
        return static_cast<difference_type>(size());
    }

    inline null_array* null_array::clone_impl() const
    {
        return new null_array(*this);
    }
}

