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

#include <climits>
#include <cstdint>
#include <vector>

#include "sparrow/buffer.hpp"
#include "sparrow/data_type.hpp"

namespace sparrow
{
    class bitset_view
    {
    public:

        using block_type = std::uint8_t;
        using value_type = bool;
        using size_type = std::size_t;
        using storage_type = buffer<block_type>;

        explicit bitset_view(storage_type& bitmap);

        size_type size() const;

        bool test(size_type pos) const;
        void set(size_type pos, value_type value);

    private:

        size_type block_index(size_type pos) const noexcept;
        size_type bit_index(size_type pos) const noexcept;
        block_type bit_mask(size_type pos) const noexcept;

        storage_type* p_bitmap;

        static constexpr std::size_t s_bits_per_block = sizeof(block_type) * CHAR_BIT;
    };

    struct array_data
    {
        data_descriptor type;
        std::int64_t length = 0;
        std::int64_t null_count = 0;
        std::int64_t offset = 0;
        std::vector<buffer<std::uint8_t>> buffers;
        std::vector<array_data> child_data;

        bitset_view bitmap();
    };

    struct null_type
    {
    };
    constexpr null_type null;

    template <class T>
    class reference_proxy
    {
    public:

        using self_type = reference_proxy<T>;
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using size_type = std::size_t;

        reference_proxy(reference ref, size_type index, array_data& ad);
        reference_proxy(const self_type&) = default;
        reference_proxy(self_type&&) = default;

        self_type& operator=(const self_type&);
        self_type& operator=(self_type&&);
        self_type& operator=(null_type);

        template <class U>
        self_type& operator=(const U&);

        template <class U>
        self_type& operator+=(const U&);

        template <class U>
        self_type& operator-=(const U&);

        template <class U>
        self_type& operator*=(const U&);

        template <class U>
        self_type& operator/=(const U&);

        operator const_reference() const;

    private:

        void update_value(value_type value);
        void update_value(null_type);

        pointer p_ref;
        size_type m_index;
        array_data* p_array_data;
    };

    /******************************
     * bitset_view implementation *
     ******************************/

    inline bitset_view::bitset_view(buffer<block_type>& bitmap)
        : p_bitmap(&bitmap)
    {
    }

    inline auto bitset_view::size() const -> size_type 
    {
        return p_bitmap->size() * s_bits_per_block;
    }

    inline bool bitset_view::test(size_type pos) const
    {
        return p_bitmap->data()[block_index(pos)] & bit_mask(pos);
    }
    
    inline void bitset_view::set(size_type pos, value_type value)
    {
        block_type& block = p_bitmap->data()[block_index(pos)];
        if (value)
        {
            block |= bit_mask(pos);
        }
        else
        {
            block &= ~bit_mask(pos);
        }
    }

    inline auto bitset_view::block_index(size_type pos) const noexcept -> size_type
    {
        return pos / s_bits_per_block;
    }

    inline auto bitset_view::bit_index(size_type pos) const noexcept -> size_type
    {
        return pos % s_bits_per_block;
    }

    inline auto bitset_view::bit_mask(size_type pos) const noexcept -> block_type 
    {
        return block_type(1) << bit_index(pos);
    }

    /*****************************
     * array_data implementation *
     *****************************/

    bitset_view array_data::bitmap()
    {
        return bitset_view(buffers[0]);
    }

    /**********************************
     * reference_proxy implementation *
     **********************************/

    template <class T>
    reference_proxy<T>::reference_proxy(reference ref, size_type index, array_data& ad)
        : p_ref(&ref)
        , m_index(index)
        , p_array_data(&ad)
    {
    }

    template <class T>
    auto reference_proxy<T>::operator=(const self_type& rhs) -> self_type&
    {
        update_value(*rhs.p_ref);
        return *this;
    }

    template <class T>
    auto reference_proxy<T>::operator=(self_type&& rhs) -> self_type&
    {
        update_value(*rhs.p_ref);
        return *this;
    }

    template <class T>
    auto reference_proxy<T>::operator=(null_type rhs) -> self_type&
    {
        update_value(std::move(rhs));
        return *this;
    }

    template <class T>
    template <class U>
    auto reference_proxy<T>::operator=(const U& u) -> self_type& 
    {
        update_value(u);
        return *this;
    }

    template <class T>
    template <class U>
    auto reference_proxy<T>::operator+=(const U& u) -> self_type& 
    {
        update_value(*p_ref + u);
        return *this;
    }


    template <class T>
    template <class U>
    auto reference_proxy<T>::operator-=(const U& u) -> self_type& 
    {
        update_value(*p_ref - u);
        return *this;
    }

    template <class T>
    template <class U>
    auto reference_proxy<T>::operator*=(const U& u) -> self_type& 
    {
        update_value(*p_ref * u);
        return *this;
    }

    template <class T>
    template <class U>
    auto reference_proxy<T>::operator/=(const U& u) -> self_type& 
    {
        update_value(*p_ref / u);
        return *this;
    }

    template <class T>
    reference_proxy<T>::operator const_reference() const
    {
        return *p_ref;
    }

    template <class T>
    void reference_proxy<T>::update_value(value_type value)
    {
        auto view = p_array_data->bitmap();
        bool has_val = (p_array_data->null_count == 0u) || view.test(m_index);
        if (!has_val)
        {
            // TODO: handle empty bitmap
            view.set(m_index, true);
            --(p_array_data->null_count);
        }
        *p_ref = value;
    }

    template <class T>
    void reference_proxy<T>::update_value(null_type)
    {
        auto view = p_array_data->bitmap();
        bool has_val = (p_array_data->null_count == 0u) || view.test(m_index);
        if (has_val)
        {
            // TODO: handle empty bitmap
            view.set(m_index, false);
            ++(p_array_data->null_count);
        }
    }
}

