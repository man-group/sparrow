
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

namespace sparrow
{
    template <class B, bool is_const>
    class bitset_iterator;

    /**
     * @class bitset_reference
     *
     * Reference proxy used by the bitset_iterator class
     * to make it possible to assign a bit of a bitset as a regular reference.
     *
     * @tparam B the dynamic_bitset containing the bit this class refers to.
     */
    template <class B>
    class bitset_reference
    {
    public:

        using self_type = bitset_reference<B>;

        constexpr bitset_reference(const bitset_reference&) noexcept = default;
        constexpr bitset_reference(bitset_reference&&) noexcept = default;

        constexpr self_type& operator=(const self_type&) noexcept;
        constexpr self_type& operator=(self_type&&) noexcept;
        constexpr self_type& operator=(bool) noexcept;

        constexpr operator bool() const noexcept;

        constexpr bool operator~() const noexcept;

        constexpr self_type& operator&=(bool) noexcept;
        constexpr self_type& operator|=(bool) noexcept;
        constexpr self_type& operator^=(bool) noexcept;

    private:

        using block_type = typename B::block_type;
        using bitset_type = B;

        bitset_reference(bitset_type& bitset, block_type& block, block_type mask);

        constexpr void assign(bool) noexcept;
        constexpr void set() noexcept;
        constexpr void reset() noexcept;

        bitset_type& m_bitset;
        block_type& m_block;
        block_type m_mask;

        friend class bitset_iterator<B, false>;
        template <typename RAR>
            requires std::ranges::random_access_range<std::remove_pointer_t<RAR>>
        friend class dynamic_bitset_base;
    };

    template <class B1, class B2>
    bool operator==(const bitset_reference<B1>& lhs, const bitset_reference<B2>& rhs);

    template <class B>
    bool operator==(const bitset_reference<B>& lhs, bool rhs);

    template <class B>
    constexpr auto bitset_reference<B>::operator=(const self_type& rhs) noexcept -> self_type&
    {
        assign(rhs);
        return *this;
    }

    template <class B>
    constexpr auto bitset_reference<B>::operator=(self_type&& rhs) noexcept -> self_type&
    {
        assign(rhs);
        return *this;
    }

    template <class B>
    constexpr auto bitset_reference<B>::operator=(bool rhs) noexcept -> self_type&
    {
        assign(rhs);
        return *this;
    }

    template <class B>
    constexpr bitset_reference<B>::operator bool() const noexcept
    {
        if (m_bitset.data() == nullptr)
        {
            return true;
        }
        return (m_block & m_mask) != 0;
    }

    template <class B>
    constexpr bool bitset_reference<B>::operator~() const noexcept
    {
        return (m_block & m_mask) == 0;
    }

    template <class B>
    constexpr auto bitset_reference<B>::operator&=(bool rhs) noexcept -> self_type&
    {
        if (!rhs)
        {
            reset();
        }
        return *this;
    }

    template <class B>
    constexpr auto bitset_reference<B>::operator|=(bool rhs) noexcept -> self_type&
    {
        if (rhs)
        {
            set();
        }
        return *this;
    }

    template <class B>
    constexpr auto bitset_reference<B>::operator^=(bool rhs) noexcept -> self_type&
    {
        if (rhs)
        {
            bool old_value = m_block & m_mask;
            m_block ^= m_mask;
            m_bitset.update_null_count(old_value, !old_value);
        }
        return *this;
    }

    template <class B>
    bitset_reference<B>::bitset_reference(bitset_type& bitset, block_type& block, block_type mask)
        : m_bitset(bitset)
        , m_block(block)
        , m_mask(mask)
    {
    }

    template <class B>
    constexpr void bitset_reference<B>::assign(bool rhs) noexcept
    {
        rhs ? set() : reset();
    }

    template <class B>
    constexpr void bitset_reference<B>::set() noexcept
    {
        bool old_value = m_block & m_mask;
        m_block |= m_mask;
        m_bitset.update_null_count(old_value, m_block & m_mask);
    }

    template <class B>
    constexpr void bitset_reference<B>::reset() noexcept
    {
        bool old_value = m_block & m_mask;
        m_block &= ~m_mask;
        m_bitset.update_null_count(old_value, m_block & m_mask);
    }

    template <class B1, class B2>
    bool operator==(const bitset_reference<B1>& lhs, const bitset_reference<B2>& rhs)
    {
        return bool(lhs) == bool(rhs);
    }

    template <class B>
    bool operator==(const bitset_reference<B>& lhs, bool rhs)
    {
        return bool(lhs) == rhs;
    }
}

#if defined(__cpp_lib_format)
#    include <format>

template <class B>
struct std::formatter<sparrow::bitset_reference<B>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const sparrow::bitset_reference<B>& b, std::format_context& ctx) const
    {
        bool val = b;
        return std::format_to(ctx.out(), "{}", val);
    }
};

#endif
