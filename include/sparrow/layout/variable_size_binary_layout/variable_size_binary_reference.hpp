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

#include <algorithm>
#include <concepts>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>

#if defined(__cpp_lib_format)
#    include <format>
#    include <ostream>
#endif

#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    /**
     * @brief Reference proxy for variable-size binary elements in array layouts.
     *
     * This class provides a reference-like interface for accessing and modifying
     * variable-size binary elements (such as strings or byte arrays) stored in
     * array layouts. It acts as a proxy that forwards operations to the underlying
     * layout while providing an iterator-based interface over the binary data.
     *
     * The reference supports:
     * - Assignment operations that can resize the underlying binary data
     * - Iterator interface for byte-level or character-level access
     * - Comparison operations with other binary sequences and C-strings
     * - Range-based operations and algorithms
     * - Automatic handling of offset-based storage
     *
     * Key features:
     * - Supports variable-length binary data with automatic offset management
     * - Provides mutable and const iterators over the data
     * - Special handling for C-string assignments to avoid null terminator issues
     * - Efficient comparison and assignment operations
     * - Compatible with standard algorithms and range operations
     *
     * @tparam L The layout type that stores variable-size binary values
     *
     * @pre L must provide assign(T, size_type) method for assignment operations
     * @pre L must provide data(size_type) method returning byte/char pointers
     * @pre L must provide offset(size_type) method for offset-based access
     * @pre L must define appropriate iterator types for data access
     * @pre L must define offset_type for offset calculations
     * @post Reference operations may modify underlying layout storage and offsets
     * @post Iterator operations provide access to variable-length binary data
     * @post Thread-safe for read operations, requires external synchronization for writes
     *
     * @example
     * ```cpp
     * variable_size_binary_array<std::string> arr = ...;
     * auto ref = arr[0];                     // Get binary reference
     *
     * // Assignment from various sources
     * ref = std::string("Hello, World!");   // Assign from string
     * ref = "C-style string";               // Assign from C-string
     * std::vector<char> data = {'A', 'B'};
     * ref = data;                           // Assign from range
     *
     * // Iterator-based access
     * for (auto ch : ref) {
     *     // Process each character/byte
     * }
     *
     * // Comparison operations
     * bool equal = (ref == "Hello");
     * auto ordering = ref <=> std::string("World");
     * ```
     */
    template <class L>
    class variable_size_binary_reference
    {
    public:

        using self_type = variable_size_binary_reference<L>;
        using value_type = typename L::inner_value_type;
        using reference = typename L::inner_reference;
        using const_reference = typename L::inner_const_reference;
        using size_type = typename L::size_type;
        using difference_type = std::ptrdiff_t;
        using iterator = typename L::data_iterator;
        using const_iterator = typename L::const_data_iterator;
        using offset_type = typename L::offset_type;

        /**
         * @brief Constructs a variable-size binary reference for the given layout and index.
         *
         * @param layout Pointer to the layout containing the binary data
         * @param index Index of the binary element in the layout
         *
         * @pre layout must not be nullptr
         * @pre index must be valid within the layout bounds
         * @post Reference is bound to the specified element
         * @post Layout pointer and index are stored for future operations
         */
        constexpr variable_size_binary_reference(L* layout, size_type index);

        constexpr variable_size_binary_reference(const variable_size_binary_reference&) = default;
        constexpr variable_size_binary_reference(variable_size_binary_reference&&) = default;

        /**
         * @brief Assignment from a sized range of binary data.
         *
         * Assigns new binary data to the referenced element. The operation may
         * resize the underlying storage and update offset tables to accommodate
         * the new data size.
         *
         * @tparam T Type of the source range
         * @param rhs Source range to assign from
         * @return Reference to this object
         *
         * @pre T must be a sized range convertible to L::inner_value_type
         * @pre Layout must remain valid during assignment
         * @pre Index must be within layout bounds
         * @post Underlying layout element is assigned the data from rhs
         * @post Layout buffers and offsets are updated to reflect changes
         * @post Element size may change to match rhs.size()
         */
        template <std::ranges::sized_range T>
            requires mpl::convertible_ranges<T, typename L::inner_value_type>
        constexpr self_type& operator=(T&& rhs);

        /**
         * @brief Assignment from a C-string.
         *
         * Special overload to handle C-string assignments correctly, avoiding
         * issues with the null terminator being included in the binary data.
         * The C-string is converted to a string_view before assignment.
         *
         * @tparam U Value type (deduced, must be assignable from const char*)
         * @param rhs C-string to assign from
         * @return Reference to this object
         *
         * @pre U must be assignable from const char*
         * @pre rhs must be a valid null-terminated C-string
         * @pre Layout must remain valid during assignment
         * @post Underlying element is assigned the string content (without null terminator)
         * @post Layout buffers are updated appropriately
         *
         * @note This overload prevents const char* from being treated as a range
         *       which would include the null terminator
         */
        template <class U = typename L::inner_value_type>
            requires std::assignable_from<U&, const char*>
        constexpr self_type& operator=(const char* rhs);

        /**
         * @brief Gets the size of the binary element in bytes/characters.
         *
         * @return Number of bytes/characters in the variable-size element
         *
         * @post Returns non-negative size
         * @post Size is calculated from offset differences
         * @post May be 0 for empty elements
         */
        [[nodiscard]] constexpr size_type size() const;

        /**
         * @brief Checks if the binary element is empty.
         *
         * @return true if element contains no data, false otherwise
         *
         * @post Return value equals (size() == 0)
         */
        [[nodiscard]] constexpr bool empty() const;

        /**
         * @brief Gets mutable iterator to the beginning of binary data.
         *
         * @return Iterator pointing to the first byte/character of the element
         *
         * @post Iterator is valid for reading and writing binary data
         * @post Iterator range spans exactly size() elements
         */
        [[nodiscard]] constexpr iterator begin();

        /**
         * @brief Gets mutable iterator to the end of binary data.
         *
         * @return Iterator pointing past the last byte/character of the element
         *
         * @post Iterator marks the end of the binary data range
         * @post Distance from begin() to end() equals size()
         */
        [[nodiscard]] constexpr iterator end();

        /**
         * @brief Gets const iterator to the beginning of binary data.
         *
         * @return Const iterator pointing to the first byte/character of the element
         *
         * @post Iterator is valid for reading binary data
         * @post Equivalent to cbegin()
         */
        [[nodiscard]] constexpr const_iterator begin() const;

        /**
         * @brief Gets const iterator to the end of binary data.
         *
         * @return Const iterator pointing past the last byte/character of the element
         *
         * @post Iterator marks the end of the binary data range
         * @post Equivalent to cend()
         */
        [[nodiscard]] constexpr const_iterator end() const;

        /**
         * @brief Gets const iterator to the beginning of binary data.
         *
         * @return Const iterator pointing to the first byte/character of the element
         *
         * @post Iterator is valid for reading binary data
         * @post Guarantees const access even for mutable reference
         */
        [[nodiscard]] constexpr const_iterator cbegin() const;

        /**
         * @brief Gets const iterator to the end of binary data.
         *
         * @return Const iterator pointing past the last byte/character of the element
         *
         * @post Iterator marks the end of the binary data range
         * @post Guarantees const access even for mutable reference
         */
        [[nodiscard]] constexpr const_iterator cend() const;

        /**
         * @brief Equality comparison with another range of binary data.
         *
         * Compares this binary element with another range element-by-element.
         *
         * @tparam T Type of the range to compare with
         * @param rhs Range to compare with
         * @return true if all elements are equal, false otherwise
         *
         * @pre T must be an input range convertible to L::inner_value_type
         * @post Comparison is performed element-wise using std::equal
         * @post Returns true iff ranges have same content
         */
        template <std::ranges::input_range T>
            requires mpl::convertible_ranges<T, typename L::inner_value_type>
        constexpr bool operator==(const T& rhs) const;

        /**
         * @brief Equality comparison with a C-string.
         *
         * Special overload for comparing with C-strings, converting the C-string
         * to a string_view for proper comparison without null terminator issues.
         *
         * @tparam U Value type (deduced, must be assignable from const char*)
         * @param rhs C-string to compare with
         * @return true if content matches the C-string, false otherwise
         *
         * @pre U must be assignable from const char*
         * @pre rhs must be a valid null-terminated C-string
         * @post Comparison excludes the null terminator
         * @post Equivalent to comparing with std::string_view(rhs)
         */
        template <class U = typename L::inner_value_type>
            requires std::assignable_from<U&, const char*>
        constexpr bool operator==(const char* rhs) const;

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
         * @brief Three-way comparison with a C-string.
         *
         * Special overload for comparing with C-strings, converting the C-string
         * to a string_view for proper lexicographical comparison.
         *
         * @tparam U Value type (deduced, must be assignable from const char*)
         * @param rhs C-string to compare with
         * @return Ordering result of lexicographical comparison
         *
         * @pre U must be assignable from const char*
         * @pre rhs must be a valid null-terminated C-string
         * @post Comparison excludes the null terminator
         * @post Equivalent to comparing with std::string_view(rhs)
         */
        template <class U = typename L::inner_value_type>
            requires std::assignable_from<U&, const char*>
        constexpr auto operator<=>(const char* rhs) const;

    private:

        /**
         * @brief Gets the offset value for the given element index.
         *
         * @param index Element index to get offset for
         * @return Offset value for the specified element
         *
         * @post Returns the actual offset stored in the layout
         * @post Used for calculating element boundaries
         */
        [[nodiscard]] constexpr offset_type offset(size_type index) const;

        /**
         * @brief Gets the offset as an unsigned size_type.
         *
         * @param index Element index to get offset for
         * @return Offset value cast to size_type
         *
         * @post Returns offset(index) cast to size_type
         * @post Used for iterator positioning and size calculations
         */
        [[nodiscard]] constexpr size_type uoffset(size_type index) const;

        L* p_layout = nullptr;             ///< Pointer to the layout containing the data
        size_type m_index = size_type(0);  ///< Index of the element in the layout
    };
}

namespace std
{
    template <typename Layout, template <typename> typename TQual, template <typename> typename UQual>
    struct basic_common_reference<sparrow::variable_size_binary_reference<Layout>, std::string, TQual, UQual>
    {
        using type = std::string;
    };

    template <typename Layout, template <typename> typename TQual, template <class> class UQual>
    struct basic_common_reference<std::string, sparrow::variable_size_binary_reference<Layout>, TQual, UQual>
    {
        using type = std::string;
    };

    template <typename Layout, template <typename> typename TQual, template <typename> typename UQual>
    struct basic_common_reference<sparrow::variable_size_binary_reference<Layout>, std::vector<std::byte>, TQual, UQual>
    {
        using type = std::vector<std::byte>;
    };

    template <typename Layout, template <typename> typename TQual, template <class> class UQual>
    struct basic_common_reference<std::vector<std::byte>, sparrow::variable_size_binary_reference<Layout>, TQual, UQual>
    {
        using type = std::vector<std::byte>;
    };
}

namespace sparrow
{
    /*************************************************
     * variable_size_binary_reference implementation *
     *************************************************/

    template <class L>
    constexpr variable_size_binary_reference<L>::variable_size_binary_reference(L* layout, size_type index)
        : p_layout(layout)
        , m_index(index)
    {
    }

    template <class L>
    template <std::ranges::sized_range T>
        requires mpl::convertible_ranges<T, typename L::inner_value_type>
    constexpr auto variable_size_binary_reference<L>::operator=(T&& rhs) -> self_type&
    {
        p_layout->assign(std::forward<T>(rhs), m_index);
        p_layout->get_arrow_proxy().update_buffers();
        return *this;
    }

    template <class L>
    template <class U>
        requires std::assignable_from<U&, const char*>
    constexpr auto variable_size_binary_reference<L>::operator=(const char* rhs) -> self_type&
    {
        return *this = std::string_view(rhs);
    }

    template <class L>
    constexpr auto variable_size_binary_reference<L>::size() const -> size_type
    {
        return static_cast<size_type>(offset(m_index + 1) - offset(m_index));
    }

    template <class L>
    constexpr auto variable_size_binary_reference<L>::empty() const -> bool
    {
        return size() == 0;
    }

    template <class L>
    constexpr auto variable_size_binary_reference<L>::begin() -> iterator
    {
        return iterator(p_layout->data(uoffset(m_index)));
    }

    template <class L>
    constexpr auto variable_size_binary_reference<L>::end() -> iterator
    {
        return iterator(p_layout->data(uoffset(m_index + 1)));
    }

    template <class L>
    constexpr auto variable_size_binary_reference<L>::begin() const -> const_iterator
    {
        return cbegin();
    }

    template <class L>
    constexpr auto variable_size_binary_reference<L>::end() const -> const_iterator
    {
        return cend();
    }

    template <class L>
    constexpr auto variable_size_binary_reference<L>::cbegin() const -> const_iterator
    {
        return const_iterator(p_layout->data(uoffset(m_index)));
    }

    template <class L>
    constexpr auto variable_size_binary_reference<L>::cend() const -> const_iterator
    {
        return const_iterator(p_layout->data(uoffset(m_index + 1)));
    }

    template <class L>
    template <std::ranges::input_range T>
        requires mpl::convertible_ranges<T, typename L::inner_value_type>
    constexpr bool variable_size_binary_reference<L>::operator==(const T& rhs) const
    {
        return std::equal(cbegin(), cend(), std::cbegin(rhs), std::cend(rhs));
    }

    template <class L>
    template <class U>
        requires std::assignable_from<U&, const char*>
    constexpr bool variable_size_binary_reference<L>::operator==(const char* rhs) const
    {
        return operator==(std::string_view(rhs));
    }

    template <class L>
    template <std::ranges::input_range T>
        requires mpl::convertible_ranges<T, typename L::inner_value_type>
    constexpr auto variable_size_binary_reference<L>::operator<=>(const T& rhs) const
    {
        return lexicographical_compare_three_way(*this, rhs);
    }

    template <class L>
    template <class U>
        requires std::assignable_from<U&, const char*>
    constexpr auto variable_size_binary_reference<L>::operator<=>(const char* rhs) const
    {
        return operator<=>(std::string_view(rhs));
    }

    template <class L>
    constexpr auto variable_size_binary_reference<L>::offset(size_type index) const -> offset_type
    {
        return *(p_layout->offset(index));
    }

    template <class L>
    constexpr auto variable_size_binary_reference<L>::uoffset(size_type index) const -> size_type
    {
        return static_cast<size_type>(offset(index));
    }
}

#if defined(__cpp_lib_format)

template <typename Layout>
struct std::formatter<sparrow::variable_size_binary_reference<Layout>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::variable_size_binary_reference<Layout>& ref, std::format_context& ctx) const
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
inline std::ostream& operator<<(std::ostream& os, const sparrow::variable_size_binary_reference<Layout>& value)
{
    os << std::format("{}", value);
    return os;
}

#endif
