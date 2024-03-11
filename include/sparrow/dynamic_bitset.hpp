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

#include <cassert>
#include <climits>

#include "sparrow/buffer.hpp"

namespace sparrow
{
    template <class B>
    class dynamic_bitset_base
    {
    public:

        using self_type = dynamic_bitset_base<B>;
        using storage_type = B;
        using block_type = typename storage_type::value_type;
        using value_type = bool;
        using size_type = typename storage_type::size_type;

        size_type size() const noexcept;
        size_type null_count() const noexcept;

        bool test(size_type pos) const;
        void set(size_type pos, value_type value);

        block_type* data() noexcept;
        const block_type* data() const noexcept;
        size_type block_count() const noexcept;

        void swap(self_type&) noexcept;

    protected:

        dynamic_bitset_base(storage_type&& buffer, size_type size);
        dynamic_bitset_base(storage_type&& buffer, size_type size, size_type null_count);
        ~dynamic_bitset_base() = default;

        dynamic_bitset_base(const dynamic_bitset_base&) = default;
        dynamic_bitset_base(dynamic_bitset_base&&) = default;

        dynamic_bitset_base& operator=(const dynamic_bitset_base&) = default;
        dynamic_bitset_base& operator=(dynamic_bitset_base&&) = default;

        void resize(size_type n, value_type = false);

        size_type compute_block_count(size_type bits_count) const noexcept;

    private:

        size_type count_non_null() const noexcept;
        size_type block_index(size_type pos) const noexcept;
        size_type bit_index(size_type pos) const noexcept;
        block_type bit_mask(size_type pos) const noexcept;
        size_type count_extra_bits() const noexcept;
        void zero_unused_bits();

        static constexpr std::size_t s_bits_per_block = sizeof(block_type) * CHAR_BIT;

        storage_type m_buffer;
        size_type m_size;
        size_type m_null_count;
    };

    template <class T>
    class dynamic_bitset : public dynamic_bitset_base<buffer<T>>
    {
    public:

        using base_type = dynamic_bitset_base<buffer<T>>;
        using storage_type = typename base_type::storage_type;
        using block_type = typename base_type::block_type;
        using value_type = typename base_type::size_type;
        using size_type = typename base_type::size_type;

        dynamic_bitset();
        explicit dynamic_bitset(size_type n, value_type v = false);
        dynamic_bitset(block_type* p, size_type n);
        dynamic_bitset(block_type* p, size_type n, size_type null_count);

        ~dynamic_bitset() = default;
        dynamic_bitset(const dynamic_bitset&) = default;
        dynamic_bitset(dynamic_bitset&&) = default;

        dynamic_bitset& operator=(const dynamic_bitset&) = default;
        dynamic_bitset& operator=(dynamic_bitset&&) = default;

        using base_type::resize;
    };

    template <class T>
    class dynamic_bitset_view : public dynamic_bitset_base<buffer_view<T>>
    {
    public:

        using base_type = dynamic_bitset_base<buffer_view<T>>;
        using storage_type = typename base_type::storage_type;
        using block_type = typename base_type::block_type;
        using size_type = typename base_type::size_type;

        dynamic_bitset_view(block_type* p, size_type n);
        dynamic_bitset_view(block_type* p, size_type n, size_type null_count);
        ~dynamic_bitset_view() = default;

        dynamic_bitset_view(const dynamic_bitset_view&) = default;
        dynamic_bitset_view(dynamic_bitset_view&&) = default;

        dynamic_bitset_view& operator=(const dynamic_bitset_view&) = default;
        dynamic_bitset_view& operator=(dynamic_bitset_view&&) = default;
    };

    /**************************************
     * dynamic_bitset_base implementation *
     **************************************/

    template <class B>
    auto dynamic_bitset_base<B>::size() const noexcept -> size_type 
    {
        return m_size;
    }

    template <class B>
    auto dynamic_bitset_base<B>::null_count() const noexcept -> size_type
    {
        return m_null_count;
    }

    template <class B>
    bool dynamic_bitset_base<B>::test(size_type pos) const
    {
        return !m_null_count || m_buffer.data()[block_index(pos)] & bit_mask(pos);
    }
    
    template <class B>
    void dynamic_bitset_base<B>::set(size_type pos, value_type value)
    {
        block_type& block = m_buffer.data()[block_index(pos)];
        if (value)
        {
            block |= bit_mask(pos);
            if (m_null_count)
            {
                --m_null_count;
            }
        }
        else
        {
            block &= ~bit_mask(pos);
            ++m_null_count;
        }
    }

    template <class B>
    auto dynamic_bitset_base<B>::data() noexcept -> block_type*
    {
        return m_buffer.data();
    }

    template <class B>
    auto dynamic_bitset_base<B>::data() const noexcept -> const block_type*
    {
        return m_buffer.data();
    }

    template <class B>
    auto dynamic_bitset_base<B>::block_count() const noexcept -> size_type
    {
        return m_buffer.size();
    }

    template <class B>
    void dynamic_bitset_base<B>::swap(self_type& rhs) noexcept
    {
        using std::swap;
        swap(m_buffer, rhs.m_buffer);
        swap(m_size, rhs.m_size);
        swap(m_null_count, rhs.m_null_count);
    }

    template <class B>
    dynamic_bitset_base<B>::dynamic_bitset_base(storage_type&& buf, size_type size)
        : m_buffer(std::move(buf))
        , m_size(size)
        , m_null_count(m_size - count_non_null())
    {
        zero_unused_bits();
    }

    template <class B>
    dynamic_bitset_base<B>::dynamic_bitset_base(storage_type&& buf, size_type size, size_type null_count)
        : m_buffer(std::move(buf))
        , m_size(size)
        , m_null_count(null_count)
    {
        zero_unused_bits();
        assert(m_null_count == m_size - count_non_null());
    }

    template <class B>
    auto dynamic_bitset_base<B>::compute_block_count(size_type bits_count) const noexcept -> size_type
    {
        return bits_count / s_bits_per_block
            + static_cast<size_type>(bits_count % s_bits_per_block != 0);
    }

    template <class B>
    auto dynamic_bitset_base<B>::count_non_null() const noexcept -> size_type
    {
        // Number of bits set to 1 in i for i from 0 to 255
        static constexpr unsigned char table[] =
        {
            0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
            1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
            1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
            2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
            1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
            2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
            2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
            3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
        };
        size_type res = 0;
        const unsigned char* p = static_cast<const unsigned char*>(static_cast<const void*>(&m_buffer[0]));
        size_type length = m_buffer.size() * sizeof(block_type);
        for (size_type i = 0; i < length; ++i, ++p)
        {
            res += table[*p];
        }
        return res;
    }

    template <class B>
    auto dynamic_bitset_base<B>::block_index(size_type pos) const noexcept -> size_type
    {
        return pos / s_bits_per_block;
    }

    template <class B>
    auto dynamic_bitset_base<B>::bit_index(size_type pos) const noexcept -> size_type
    {
        return pos % s_bits_per_block;
    }

    template <class B>
    auto dynamic_bitset_base<B>::bit_mask(size_type pos) const noexcept -> block_type 
    {
        return block_type(1) << bit_index(pos);
    }

    template <class B>
    auto dynamic_bitset_base<B>::count_extra_bits() const noexcept -> size_type
    {
        return bit_index(size());
    }

    template <class B>
    void dynamic_bitset_base<B>::zero_unused_bits()
    {
        size_type extra_bits = count_extra_bits();
        if (extra_bits != 0)
        {
            m_buffer.back() &= ~(~block_type(0) << extra_bits);
        }
    }

    template <class B>
    void dynamic_bitset_base<B>::resize(size_type n, value_type b)
    {
        size_type old_block_count = m_buffer.size();
        size_type new_block_count = compute_block_count(n);
        block_type value = b ? ~block_type(0) : block_type(0);

        if (new_block_count != old_block_count)
        {
            m_buffer.resize(new_block_count, value);
        }

        if (b && n > m_size)
        {
            size_type extra_bits = count_extra_bits();
            if (extra_bits > 0)
            {
                m_buffer.data()[old_block_count - 1] |= (value << extra_bits);
            }
        }

        m_size = n;
        m_null_count = m_size - count_non_null();
        zero_unused_bits();
    }

    /*********************************
     * dynamic_bitset implementation *
     *********************************/

    template <class T>
    dynamic_bitset<T>::dynamic_bitset()
        : base_type(storage_type(), 0u)
    {
    }

    template <class T>
    dynamic_bitset<T>::dynamic_bitset(size_type n, value_type value)
        : base_type(
            storage_type(this->compute_block_count(n), value ? ~block_type(0) : 0),
            n,
            value ? 0u : n)
    {
    }

    template <class T>
    dynamic_bitset<T>::dynamic_bitset(block_type* p, size_type n)
        : base_type(storage_type(p, this->compute_block_count(n)), n)
    {
    }

    template <class T>
    dynamic_bitset<T>::dynamic_bitset(block_type* p, size_type n, size_type null_count)
        : base_type(storage_type(p, this->compute_block_count(n)), n, null_count)
    {
    }
}
