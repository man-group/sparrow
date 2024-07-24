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
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    /**
     * Layout iterator class
     *
     * Relies on a layout's couple of value iterator and bitmap iterator to
     * return reference proxies when it is dereferenced.
     */
    template <class Layout, bool is_const>
    class layout_iterator : public iterator_base<
                                layout_iterator<Layout, is_const>,
                                mpl::constify_t<typename Layout::value_type, is_const>,
                                typename Layout::iterator_tag,
                                std::conditional_t<is_const, typename Layout::const_reference, typename Layout::reference>>
    {
    public:

        using self_type = layout_iterator<Layout, is_const>;
        using base_type = iterator_base<
            self_type,
            mpl::constify_t<typename Layout::value_type, is_const>,
            typename Layout::iterator_tag,
            std::conditional_t<is_const, typename Layout::const_reference, typename Layout::reference>>;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;

        using value_iterator = std::conditional_t<is_const, typename Layout::const_value_iterator, typename Layout::value_iterator>;

        using bitmap_iterator = std::conditional_t<is_const, typename Layout::const_bitmap_iterator, typename Layout::bitmap_iterator>;

        layout_iterator() noexcept = default;
        layout_iterator(value_iterator value_iter, bitmap_iterator bitmap_iter);

    private:

        reference dereference() const;
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

    template <class Layout, bool is_const>
    layout_iterator<Layout, is_const>::layout_iterator(value_iterator value_iter, bitmap_iterator bitmap_iter)
        : m_value_iter(value_iter)
        , m_bitmap_iter(bitmap_iter)
    {
    }

    template <class Layout, bool is_const>
    auto layout_iterator<Layout, is_const>::dereference() const -> reference
    {
        return reference(*m_value_iter, *m_bitmap_iter);
    }

    template <class Layout, bool is_const>
    void layout_iterator<Layout, is_const>::increment()
    {
        ++m_value_iter;
        ++m_bitmap_iter;
    }

    template <class Layout, bool is_const>
    void layout_iterator<Layout, is_const>::decrement()
    {
        --m_value_iter;
        --m_bitmap_iter;
    }

    template <class Layout, bool is_const>
    void layout_iterator<Layout, is_const>::advance(difference_type n)
    {
        m_value_iter += n;
        m_bitmap_iter += n;
    }

    template <class Layout, bool is_const>
    auto layout_iterator<Layout, is_const>::distance_to(const self_type& rhs) const -> difference_type
    {
        return rhs.m_value_iter - m_value_iter;
    }

    template <class Layout, bool is_const>
    bool layout_iterator<Layout, is_const>::equal(const self_type& rhs) const
    {
        return m_value_iter == rhs.m_value_iter && m_bitmap_iter == rhs.m_bitmap_iter;
    }

    template <class Layout, bool is_const>
    bool layout_iterator<Layout, is_const>::less_than(const self_type& rhs) const
    {
        return m_value_iter < rhs.m_value_iter && m_bitmap_iter < rhs.m_bitmap_iter;
    }
}

