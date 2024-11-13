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

#include "sparrow/utils/iterator.hpp"

namespace sparrow
{
    /**
     * Concept for iterator types
     */
    template <class T>
    concept iterator_types = requires {
        typename T::value_type;
        typename T::reference;
        typename T::value_iterator;
        typename T::bitmap_iterator;
        typename T::iterator_tag;
    };

    /**
     * Layout iterator class
     *
     * Relies on a layout's couple of value iterator and bitmap iterator to
     * return reference proxies when it is dereferenced.
     */
    template <iterator_types Iterator_types>
    class layout_iterator : public iterator_base<
                                layout_iterator<Iterator_types>,
                                typename Iterator_types::value_type,
                                typename Iterator_types::iterator_tag,
                                typename Iterator_types::reference>
    {
    public:

        using self_type = layout_iterator<Iterator_types>;
        using base_type = iterator_base<
            self_type,
            typename Iterator_types::value_type,
            typename Iterator_types::iterator_tag,
            typename Iterator_types::reference>;

        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;

        using value_iterator = Iterator_types::value_iterator;

        using bitmap_iterator = Iterator_types::bitmap_iterator;

        layout_iterator() noexcept = default;
        layout_iterator(value_iterator value_iter, bitmap_iterator bitmap_iter);

    private:

        reference dereference() const;
        reference dereference();
        void increment();
        void decrement();
        void advance(difference_type n);
        difference_type distance_to(const self_type& rhs) const;
        bool equal(const self_type& rhs) const;
        bool less_than(const self_type& rhs) const;

        value_iterator m_value_iter;
        bitmap_iterator m_bitmap_iter;

        friend class iterator_access;
    };

    /**********************************
     * layout_iterator implementation *
     **********************************/

    template <iterator_types Iterator_types>
    layout_iterator<Iterator_types>::layout_iterator(value_iterator value_iter, bitmap_iterator bitmap_iter)
        : m_value_iter(value_iter)
        , m_bitmap_iter(bitmap_iter)
    {
    }

    template <iterator_types Iterator_types>
    auto layout_iterator<Iterator_types>::dereference() const -> reference
    {
        return reference(*m_value_iter, *m_bitmap_iter);
    }

    template <iterator_types Iterator_types>
    auto layout_iterator<Iterator_types>::dereference() -> reference
    {
        return reference(*m_value_iter, *m_bitmap_iter);
    }

    template <iterator_types Iterator_types>
    void layout_iterator<Iterator_types>::increment()
    {
        ++m_value_iter;
        ++m_bitmap_iter;
    }

    template <iterator_types Iterator_types>
    void layout_iterator<Iterator_types>::decrement()
    {
        --m_value_iter;
        --m_bitmap_iter;
    }

    template <iterator_types Iterator_types>
    void layout_iterator<Iterator_types>::advance(difference_type n)
    {
        m_value_iter += n;
        m_bitmap_iter += n;
    }

    template <iterator_types Iterator_types>
    auto layout_iterator<Iterator_types>::distance_to(const self_type& rhs) const -> difference_type
    {
        return rhs.m_value_iter - m_value_iter;
    }

    template <iterator_types Iterator_types>
    bool layout_iterator<Iterator_types>::equal(const self_type& rhs) const
    {
        return m_value_iter == rhs.m_value_iter && m_bitmap_iter == rhs.m_bitmap_iter;
    }

    template <iterator_types Iterator_types>
    bool layout_iterator<Iterator_types>::less_than(const self_type& rhs) const
    {
        return m_value_iter < rhs.m_value_iter && m_bitmap_iter < rhs.m_bitmap_iter;
    }
}
