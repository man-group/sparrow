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

#include <compare>
#include <optional>
#include <vector>

#include "sparrow/buffer.hpp"
#include "sparrow/contracts.hpp"
#include "sparrow/data_type.hpp"
#include "sparrow/dynamic_bitset.hpp"
#include "sparrow/memory.hpp"

namespace sparrow
{
    /**
     * Structure holding the raw data.
     *
     * array_data is meant to be used by the
     * different layout classes to implement
     * the array API, based on the type
     * specified in the type attribute.
     */
    struct array_data
    {
        using block_type = std::uint8_t;
        using bitmap_type = dynamic_bitset<block_type>;
        using buffer_type = buffer<block_type>;
        using length_type = std::int64_t;

        data_descriptor type;
        length_type length = 0;
        std::int64_t offset = 0;
        // bitmap buffer and null_count
        bitmap_type bitmap;
        // Other buffers
        std::vector<buffer_type> buffers;
        std::vector<array_data> child_data;
        value_ptr<array_data> dictionary;
    };

    /**
     * Layout iterator class
     *
     * Relies on a layout's couple of value iterator and bitmap iterator to
     * return reference proxies when it is dereferenced.
     */
    template <class L, bool is_const>
    class layout_iterator : public iterator_base<
                                layout_iterator<L, is_const>,
                                mpl::constify_t<typename L::value_type, is_const>,
                                typename L::iterator_tag,
                                std::conditional_t<is_const, typename L::const_reference, typename L::reference>>
    {
    public:

        using self_type = layout_iterator<L, is_const>;
        using base_type = iterator_base<
            self_type,
            mpl::constify_t<typename L::value_type, is_const>,
            typename L::iterator_tag,
            std::conditional_t<is_const, typename L::const_reference, typename L::reference>>;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;

        using value_iterator = std::conditional_t<is_const, typename L::const_value_iterator, typename L::value_iterator>;

        using bitmap_iterator = std::conditional_t<is_const, typename L::const_bitmap_iterator, typename L::bitmap_iterator>;

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

    template <class L, bool is_const>
    layout_iterator<L, is_const>::layout_iterator(value_iterator value_iter, bitmap_iterator bitmap_iter)
        : m_value_iter(value_iter)
        , m_bitmap_iter(bitmap_iter)
    {
    }

    template <class L, bool is_const>
    auto layout_iterator<L, is_const>::dereference() const -> reference
    {
        return reference(*m_value_iter, *m_bitmap_iter);
    }

    template <class L, bool is_const>
    void layout_iterator<L, is_const>::increment()
    {
        ++m_value_iter;
        ++m_bitmap_iter;
    }

    template <class L, bool is_const>
    void layout_iterator<L, is_const>::decrement()
    {
        --m_value_iter;
        --m_bitmap_iter;
    }

    template <class L, bool is_const>
    void layout_iterator<L, is_const>::advance(difference_type n)
    {
        m_value_iter += n;
        m_bitmap_iter += n;
    }

    template <class L, bool is_const>
    auto layout_iterator<L, is_const>::distance_to(const self_type& rhs) const -> difference_type
    {
        return rhs.m_value_iter - m_value_iter;
    }

    template <class L, bool is_const>
    bool layout_iterator<L, is_const>::equal(const self_type& rhs) const
    {
        return m_value_iter == rhs.m_value_iter && m_bitmap_iter == rhs.m_bitmap_iter;
    }

    template <class L, bool is_const>
    bool layout_iterator<L, is_const>::less_than(const self_type& rhs) const
    {
        return m_value_iter < rhs.m_value_iter && m_bitmap_iter < rhs.m_bitmap_iter;
    }
}
