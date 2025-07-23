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

#include <ranges>
#include <string>
#include <vector>

#include "sparrow/types/data_type.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/mp_utils.hpp"

#if defined(__cpp_lib_format)
#    include <format>
#    include <ostream>
#endif

namespace sparrow
{
    /**
     * @brief Reference proxy for fixed-width binary elements in array layouts.
     *
     * This class provides a reference-like interface for accessing and modifying
     * fixed-width binary elements stored in array layouts. It acts as a proxy that
     * forwards operations to the underlying layout while providing an iterator-based
     * interface over the binary data bytes.
     *
     * The reference supports:
     * - Assignment operations that modify the underlying array
     * - Iterator interface for byte-level access to binary data
     * - Comparison operations with other binary sequences
     * - Range-based operations and algorithms
     *
     * Key features:
     * - Provides mutable and const iterators over the binary data
     * - Supports assignment from any compatible sized range
     * - Maintains fixed-width constraint during operations
     * - Efficient byte-level access without copying
     *
     * @tparam L The layout type that stores fixed-width binary values
     *
     * @pre L must provide assign(T, size_type) method for assignment operations
     * @pre L must provide data(size_type) method returning byte pointers
     * @pre L must have m_element_size member indicating fixed width
     * @pre L must define appropriate iterator types for data access
     * @post Reference operations modify the underlying layout storage
     * @post Iterator operations provide byte-level access to binary data
     * @post Thread-safe for read operations, requires external synchronization for writes
     *
     * @example
     * ```cpp
     * fixed_width_binary_array arr = ...;
     * auto ref = arr[0];                         // Get binary reference
     *
     * // Assignment from compatible range
     * std::vector<uint8_t> data = {1, 2, 3, 4};
     * ref = data;                                // Assign new binary data
     *
     * // Iterator-based access
     * for (auto byte : ref) {
     *     // Process each byte
     * }
     *
     * // Comparison with other sequences
     * bool equal = (ref == data);
     * ```
     */
    template <class L>
    class fixed_width_binary_reference
    {
    public:

        using self_type = fixed_width_binary_reference<L>;
        using value_type = typename L::inner_value_type;
        using reference = typename L::inner_reference;
        using const_reference = typename L::inner_const_reference;
        using size_type = typename L::size_type;
        using difference_type = std::ptrdiff_t;
        using iterator = typename L::data_iterator;
        using const_iterator = typename L::const_data_iterator;

        /**
         * @brief Constructs a binary reference for the given layout and index.
         *
         * @param layout Pointer to the layout containing the binary data
         * @param index Index of the binary element in the layout
         *
         * @pre layout must not be nullptr
         * @pre index must be valid within the layout bounds
         * @post Reference is bound to the specified element
         * @post Layout pointer and index are stored for future operations
         */
        constexpr fixed_width_binary_reference(L* layout, size_type index);

        constexpr fixed_width_binary_reference(const fixed_width_binary_reference&) noexcept = default;
        constexpr fixed_width_binary_reference(fixed_width_binary_reference&&) noexcept = default;

        /**
         * @brief Assignment from a sized range of binary data.
         *
         * Assigns new binary data to the referenced element. The source range
         * must have exactly the same size as the fixed width of this binary type.
         *
         * @tparam T Type of the source range
         * @param rhs Source range to assign from
         * @return Reference to this object
         *
         * @pre T must be a sized range convertible to L::inner_value_type
         * @pre rhs.size() must equal the fixed element size
         * @pre Layout must remain valid during assignment
         * @pre Index must be within layout bounds
         * @post Underlying layout element is assigned the data from rhs
         * @post Layout buffers are updated to reflect changes
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(p_layout->m_element_size == std::ranges::size(rhs))
         */
        template <std::ranges::sized_range T>
            requires mpl::convertible_ranges<T, typename L::inner_value_type>
        constexpr self_type& operator=(T&& rhs);

        /**
         * @brief Gets the size of the binary element in bytes.
         *
         * @return Number of bytes in the fixed-width binary element
         *
         * @post Returns the fixed element size for this binary type
         * @post Value is consistent across all elements in the layout
         */
        [[nodiscard]] constexpr size_type size() const;

        /**
         * @brief Gets mutable iterator to the beginning of binary data.
         *
         * @return Iterator pointing to the first byte of the element
         *
         * @post Iterator is valid for reading and writing binary data
         * @post Iterator range spans exactly size() bytes
         */
        [[nodiscard]] constexpr iterator begin();

        /**
         * @brief Gets mutable iterator to the end of binary data.
         *
         * @return Iterator pointing past the last byte of the element
         *
         * @post Iterator marks the end of the binary data range
         * @post Distance from begin() to end() equals size()
         */
        [[nodiscard]] constexpr iterator end();

        /**
         * @brief Gets const iterator to the beginning of binary data.
         *
         * @return Const iterator pointing to the first byte of the element
         *
         * @post Iterator is valid for reading binary data
         * @post Equivalent to cbegin()
         */
        [[nodiscard]] constexpr const_iterator begin() const;

        /**
         * @brief Gets const iterator to the end of binary data.
         *
         * @return Const iterator pointing past the last byte of the element
         *
         * @post Iterator marks the end of the binary data range
         * @post Equivalent to cend()
         */
        [[nodiscard]] constexpr const_iterator end() const;

        /**
         * @brief Gets const iterator to the beginning of binary data.
         *
         * @return Const iterator pointing to the first byte of the element
         *
         * @post Iterator is valid for reading binary data
         * @post Guarantees const access even for mutable reference
         */
        [[nodiscard]] constexpr const_iterator cbegin() const;

        /**
         * @brief Gets const iterator to the end of binary data.
         *
         * @return Const iterator pointing past the last byte of the element
         *
         * @post Iterator marks the end of the binary data range
         * @post Guarantees const access even for mutable reference
         */
        [[nodiscard]] constexpr const_iterator cend() const;

        /**
         * @brief Equality comparison with another range of binary data.
         *
         * Compares this binary element with another range byte-by-byte.
         *
         * @tparam T Type of the range to compare with
         * @param rhs Range to compare with
         * @return true if all bytes are equal, false otherwise
         *
         * @pre T must be an input range convertible to L::inner_value_type
         * @post Comparison is performed element-wise using std::equal
         * @post Returns true iff ranges have same content
         */
        template <std::ranges::input_range T>
            requires mpl::convertible_ranges<T, typename L::inner_value_type>
        constexpr bool operator==(const T& rhs) const;

        /**
         * @brief Three-way comparison with another range of binary data.
         *
         * Performs lexicographical comparison of this binary element with another range.
         *
         * @tparam T Type of the range to compare with
         * @param rhs Range to compare with
         * @return Ordering result of lexicographical comparison
         *
         * @pre T must be an input range convertible to L::inner_value_type
         * @post Comparison is performed lexicographically
         * @post Returns ordering consistent with lexicographical_compare_three_way
         */
        template <std::ranges::input_range T>
            requires mpl::convertible_ranges<T, typename L::inner_value_type>
        constexpr auto operator<=>(const T& rhs) const;

        /**
         * @brief Byte-level access to the binary element (unchecked).
         * @param i Byte index (0 <= i < size())
         * @return Reference to the i-th byte
         */
        [[nodiscard]] constexpr reference operator[](size_type i);

        /**
         * @brief Byte-level access to the binary element (const, unchecked).
         * @param i Byte index (0 <= i < size())
         * @return Const reference to the i-th byte
         */
        [[nodiscard]] constexpr const_reference operator[](size_type i) const;

        /**
         * @brief Checked byte-level access to the binary element.
         * @param i Byte index (0 <= i < size())
         * @return Reference to the i-th byte
         * @throws std::out_of_range if i >= size()
         */
        [[nodiscard]] constexpr reference at(size_type i);

        /**
         * @brief Checked byte-level access to the binary element (const).
         * @param i Byte index (0 <= i < size())
         * @return Const reference to the i-th byte
         * @throws std::out_of_range if i >= size()
         */
        [[nodiscard]] constexpr const_reference at(size_type i) const;

    private:

        /**
         * @brief Calculates byte offset for the given element index.
         *
         * @param index Element index to calculate offset for
         * @return Byte offset for the specified element
         *
         * @post Returns index * element_size
         * @post Used internally for iterator positioning
         */
        [[nodiscard]] constexpr size_type offset(size_type index) const;

        L* p_layout = nullptr;             ///< Pointer to the layout containing the data
        size_type m_index = size_type(0);  ///< Index of the element in the layout
    };
}

namespace std
{
    template <typename Layout, template <typename> typename TQual, template <typename> typename UQual>
    struct basic_common_reference<sparrow::fixed_width_binary_reference<Layout>, std::vector<sparrow::byte_t>, TQual, UQual>
    {
        using type = std::vector<sparrow::byte_t>;
    };

    template <typename Layout, template <typename> typename TQual, template <class> class UQual>
    struct basic_common_reference<std::vector<sparrow::byte_t>, sparrow::fixed_width_binary_reference<Layout>, TQual, UQual>
    {
        using type = std::vector<sparrow::byte_t>;
    };
}

namespace sparrow
{
    /***********************************************
     * fixed_width_binary_reference implementation *
     ***********************************************/

    template <class L>
    constexpr fixed_width_binary_reference<L>::fixed_width_binary_reference(L* layout, size_type index)
        : p_layout(layout)
        , m_index(index)
    {
    }

    template <class L>
    template <std::ranges::sized_range T>
        requires mpl::convertible_ranges<T, typename L::inner_value_type>
    constexpr auto fixed_width_binary_reference<L>::operator=(T&& rhs) -> self_type&
    {
        SPARROW_ASSERT_TRUE(p_layout->m_element_size == std::ranges::size(rhs));
        p_layout->assign(std::forward<T>(rhs), m_index);
        p_layout->get_arrow_proxy().update_buffers();
        return *this;
    }

    template <class L>
    constexpr auto fixed_width_binary_reference<L>::size() const -> size_type
    {
        return p_layout->m_element_size;
    }

    template <class L>
    constexpr auto fixed_width_binary_reference<L>::begin() -> iterator
    {
        return iterator(p_layout->data(offset(m_index)));
    }

    template <class L>
    constexpr auto fixed_width_binary_reference<L>::end() -> iterator
    {
        return iterator(p_layout->data(offset(m_index + 1)));
    }

    template <class L>
    constexpr auto fixed_width_binary_reference<L>::begin() const -> const_iterator
    {
        return cbegin();
    }

    template <class L>
    constexpr auto fixed_width_binary_reference<L>::end() const -> const_iterator
    {
        return cend();
    }

    template <class L>
    constexpr auto fixed_width_binary_reference<L>::cbegin() const -> const_iterator
    {
        return const_iterator(p_layout->data(offset(m_index)));
    }

    template <class L>
    constexpr auto fixed_width_binary_reference<L>::cend() const -> const_iterator
    {
        return const_iterator(p_layout->data(offset(m_index + 1)));
    }

    template <class L>
    template <std::ranges::input_range T>
        requires mpl::convertible_ranges<T, typename L::inner_value_type>
    constexpr bool fixed_width_binary_reference<L>::operator==(const T& rhs) const
    {
        return std::equal(cbegin(), cend(), std::cbegin(rhs), std::cend(rhs));
    }

    template <class L>
    template <std::ranges::input_range T>
        requires mpl::convertible_ranges<T, typename L::inner_value_type>
    constexpr auto fixed_width_binary_reference<L>::operator<=>(const T& rhs) const
    {
        return lexicographical_compare_three_way(*this, rhs);
    }

    template <class L>
    constexpr auto fixed_width_binary_reference<L>::offset(size_type index) const -> size_type
    {
        return p_layout->m_element_size * index;
    }

    template <class L>
    [[nodiscard]] constexpr auto fixed_width_binary_reference<L>::operator[](size_type i) -> reference
    {
        return *(begin() + i);
    }

    template <class L>
    [[nodiscard]] constexpr auto fixed_width_binary_reference<L>::operator[](size_type i) const
        -> const_reference
    {
        return *(cbegin() + i);
    }

    template <class L>
    [[nodiscard]] constexpr auto fixed_width_binary_reference<L>::at(size_type i) -> reference
    {
        if (i >= size())
        {
            throw std::out_of_range("fixed_width_binary_reference::at() index out of range");
        }
        return operator[](i);
    }

    template <class L>
    [[nodiscard]] constexpr auto fixed_width_binary_reference<L>::at(size_type i) const -> const_reference
    {
        if (i >= size())
        {
            throw std::out_of_range("fixed_width_binary_reference::at() index out of range");
        }
        return operator[](i);
    }
}

#if defined(__cpp_lib_format)

template <typename Layout>
struct std::formatter<sparrow::fixed_width_binary_reference<Layout>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::fixed_width_binary_reference<Layout>& ref, std::format_context& ctx) const
    {
        std::for_each(
            ref.cbegin(),
            sparrow::next(ref.cbegin(), ref.size() - 1),
            [&ctx](const auto& value)
            {
                std::format_to(ctx.out(), "{}, ", value);
            }
        );

        return std::format_to(ctx.out(), "{}>", *std::prev(ref.cend()));
    }
};

template <typename Layout>
inline std::ostream& operator<<(std::ostream& os, const sparrow::fixed_width_binary_reference<Layout>& value)
{
    os << std::format("{}", value);
    return os;
}

#endif
