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

#include "sparrow/array/data_type.hpp"
#include "sparrow/buffer/buffer.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/memory.hpp"

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

    data_descriptor type_descriptor(const array_data& data);
    array_data::length_type length(const array_data& data);
    std::int64_t offset(const array_data& data);

    array_data::bitmap_type& bitmap(array_data& data);
    const array_data::bitmap_type& bitmap(const array_data& data);

    std::size_t buffers_size(const array_data& data);
    array_data::buffer_type& buffer_at(array_data& data, std::size_t i);
    const array_data::buffer_type& buffer_at(const array_data& data, std::size_t i);

    std::size_t child_data_size(const array_data& data);
    array_data& child_data_at(array_data& data, std::size_t i);
    const array_data& child_data_at(const array_data& data, std::size_t i);

    value_ptr<array_data>& dictionary(array_data& data);
    const value_ptr<array_data>& dictionary(const array_data& data);

    /**
     * Concept for a structure that can be used as a data storage in the layout and the
     * typed_array class.
     */
    template <class T>
    concept data_storage = requires(const T t, std::size_t i)
    {
        { type_descriptor(t) } -> std::same_as<data_descriptor>;
        { length(t) } -> std::same_as<std::int64_t>;
        { offset(t) } -> std::same_as<std::int64_t>;
        bitmap(t);
        { buffers_size(t) } -> std::same_as<std::size_t>;
        buffer_at(t, i);
        { child_data_size(t) } -> std::same_as<std::size_t>;
        child_data_at(t, i);
        dictionary(t);
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

    /***********************************
     * getter functions for array_data *
     ***********************************/

    inline data_descriptor type_descriptor(const array_data& data)
    {
        return data.type;
    }

    inline array_data::length_type length(const array_data& data)
    {
        return data.length;
    }

    inline std::int64_t offset(const array_data& data)
    {
        return data.offset;
    }

    inline array_data::bitmap_type& bitmap(array_data& data)
    {
        return data.bitmap;
    }

    inline const array_data::bitmap_type& bitmap(const array_data& data)
    {
        return data.bitmap;
    }

    inline std::size_t buffers_size(const array_data& data)
    {
        return data.buffers.size();
    }

    inline array_data::buffer_type& buffer_at(array_data& data, std::size_t i)
    {
        SPARROW_ASSERT_TRUE(i < buffers_size(data));
        return data.buffers[i];
    }

    inline const array_data::buffer_type& buffer_at(const array_data& data, std::size_t i)
    {
        SPARROW_ASSERT_TRUE(i < buffers_size(data));
        return data.buffers[i];
    }

    inline std::size_t child_data_size(const array_data& data)
    {
        return data.child_data.size();
    }

    inline array_data& child_data_at(array_data& data, std::size_t i)
    {
        SPARROW_ASSERT_TRUE(i < child_data_size(data));
        return data.child_data[i];
    }

    inline const array_data& child_data(const array_data& data, std::size_t i)
    {
        SPARROW_ASSERT_TRUE(i < child_data_size(data));
        return data.child_data[i];
    }

    inline value_ptr<array_data>& dictionary(array_data& data)
    {
        return data.dictionary;
    }

    inline const value_ptr<array_data>& dictionary(const array_data& data)
    {
        return data.dictionary;
    }
    
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
