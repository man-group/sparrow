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

#include <ranges>
#include <type_traits>

namespace sparrow
{
    template <class B, bool is_const>
    class bitset_iterator;

    template <typename B>
        requires std::ranges::random_access_range<std::remove_pointer_t<B>>
    class bit_vector_base;

    /**
     * @class bitset_reference
     *
     * A proxy reference class that provides mutable access to individual bits in a bitset.
     *
     * This class acts as a smart reference that allows individual bits in a bitset to be
     * treated as if they were regular boolean references, despite being stored as packed
     * bits within integer blocks. It provides a complete set of boolean operations and
     * assignment operators to make bit manipulation transparent to the user.
     *
     * The reference maintains a pointer to the owning bitset and the index of the bit
     * it represents. All operations are forwarded to the appropriate bitset methods,
     * ensuring that null count tracking and other internal state remain consistent.
     *
     * @tparam B The bitset type that contains the referenced bit. Must be a type that
     *          provides test() and set() methods with appropriate signatures.
     *
     * @note This class is typically not instantiated directly by users, but rather
     *       returned by bitset indexing operations and iterators.
     *
     * @note The reference remains valid only as long as the referenced bitset exists
     *       and the bit index remains valid (i.e., no resize operations that would
     *       invalidate the index).
     *
     * Example usage:
     * @code
     * dynamic_bitset<uint8_t> bits(10);
     * auto ref = bits[5];  // Returns bitset_reference
     * ref = true;          // Sets bit 5 to true
     * bool value = ref;    // Reads bit 5
     * ref ^= true;         // Flips bit 5
     * @endcode
     */
    template <class B>
    class bitset_reference
    {
    public:

        using self_type = bitset_reference<B>;  ///< This class type for convenience

        constexpr bitset_reference(const bitset_reference&) noexcept = default;
        constexpr bitset_reference(bitset_reference&&) noexcept = default;

        /**
         * @brief Copy assignment from another bitset_reference.
         * @param rhs The reference to copy the value from
         * @return Reference to this object
         * @post The referenced bit has the same value as the bit referenced by rhs
         * @note This copies the bit value, not the reference itself
         */
        constexpr self_type& operator=(const self_type& rhs) noexcept;

        /**
         * @brief Move assignment from another bitset_reference.
         * @param rhs The reference to move the value from
         * @return Reference to this object
         * @post The referenced bit has the same value as the bit referenced by rhs
         * @note This copies the bit value, not the reference itself (move semantics don't apply to bit
         * values)
         */
        constexpr self_type& operator=(self_type&& rhs) noexcept;

        /**
         * @brief Assignment from a boolean value.
         * @param value The boolean value to assign to the referenced bit
         * @return Reference to this object
         * @post The referenced bit equals value
         * @post If the bit value changed, the bitset's null count is updated accordingly
         */
        constexpr self_type& operator=(bool value) noexcept;

        /**
         * @brief Implicit conversion to bool.
         * @return The current value of the referenced bit
         * @post Return value represents the current state of the bit
         */
        constexpr operator bool() const noexcept;

        /**
         * @brief Bitwise NOT operator.
         * @return The logical negation of the referenced bit
         * @post Return value is true if the bit is false, false if the bit is true
         * @note This does not modify the referenced bit
         */
        constexpr bool operator~() const noexcept;

        /**
         * @brief Bitwise AND assignment.
         * @param rhs The boolean value to AND with the referenced bit
         * @return Reference to this object
         * @post The referenced bit equals (old_value AND rhs)
         * @post If rhs is false, the bit is set to false regardless of its previous value
         * @post If rhs is true, the bit retains its previous value
         */
        constexpr self_type& operator&=(bool rhs) noexcept;

        /**
         * @brief Bitwise OR assignment.
         * @param rhs The boolean value to OR with the referenced bit
         * @return Reference to this object
         * @post The referenced bit equals (old_value OR rhs)
         * @post If rhs is true, the bit is set to true regardless of its previous value
         * @post If rhs is false, the bit retains its previous value
         */
        constexpr self_type& operator|=(bool rhs) noexcept;

        /**
         * @brief Bitwise XOR assignment.
         * @param rhs The boolean value to XOR with the referenced bit
         * @return Reference to this object
         * @post The referenced bit equals (old_value XOR rhs)
         * @post If rhs is true, the bit is flipped
         * @post If rhs is false, the bit retains its previous value
         */
        constexpr self_type& operator^=(bool rhs) noexcept;

    private:

        using block_type = typename B::block_type;  ///< Type of storage blocks in the bitset
        using bitset_type = B;                      ///< The bitset type being referenced
        using size_type = typename B::size_type;    ///< Type used for indices and sizes

        /**
         * @brief Private constructor for creating bit references.
         * @param bitset Reference to the bitset containing the bit
         * @param index The index of the bit within the bitset
         * @pre index < bitset.size()
         * @post p_bitset points to the provided bitset
         * @post m_index equals the provided index
         */
        bitset_reference(bitset_type& bitset, size_type index);

        /**
         * @brief Internal assignment implementation.
         * @param value The boolean value to assign
         * @post The referenced bit equals value
         */
        constexpr void assign(bool value) noexcept;

        /**
         * @brief Sets the referenced bit to true.
         * @post The referenced bit is true
         */
        constexpr void set() noexcept;

        /**
         * @brief Sets the referenced bit to false.
         * @post The referenced bit is false
         */
        constexpr void reset() noexcept;

        bitset_type* p_bitset;  ///< Pointer to the bitset containing the referenced bit
        size_type m_index;      ///< Index of the referenced bit within the bitset

        friend class bitset_iterator<B, false>;  ///< Mutable iterator needs access to create references
        template <typename RAR>
            requires std::ranges::random_access_range<std::remove_pointer_t<RAR>>
        friend class dynamic_bitset_base;  ///< Bitset base class needs access to create references
        template <typename RAR>
            requires std::ranges::random_access_range<std::remove_pointer_t<RAR>>
        friend class bit_vector_base;  ///< Bit vector base class needs access to create references
    };

    /**
     * @brief Equality comparison between two bitset references.
     * @tparam B1 Type of the first bitset reference
     * @tparam B2 Type of the second bitset reference
     * @param lhs The first reference to compare
     * @param rhs The second reference to compare
     * @return true if both references point to bits with the same value
     * @post Return value is true iff bool(lhs) == bool(rhs)
     */
    template <class B1, class B2>
    bool operator==(const bitset_reference<B1>& lhs, const bitset_reference<B2>& rhs);

    /**
     * @brief Equality comparison between a bitset reference and a boolean value.
     * @tparam B Type of the bitset reference
     * @param lhs The reference to compare
     * @param rhs The boolean value to compare against
     * @return true if the referenced bit equals the boolean value
     * @post Return value is true iff bool(lhs) == rhs
     */
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
        return p_bitset->test(m_index);
    }

    template <class B>
    constexpr bool bitset_reference<B>::operator~() const noexcept
    {
        return !p_bitset->test(m_index);
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
            p_bitset->set(m_index, !p_bitset->test(m_index));
        }
        return *this;
    }

    template <class B>
    bitset_reference<B>::bitset_reference(bitset_type& bitset, size_type index)
        : p_bitset(&bitset)
        , m_index(index)
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
        p_bitset->set(m_index, true);
    }

    template <class B>
    constexpr void bitset_reference<B>::reset() noexcept
    {
        p_bitset->set(m_index, false);
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
