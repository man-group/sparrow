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

#include <ostream>

#include "sparrow/config/config.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/utils/iterator.hpp"

namespace sparrow
{

    class list_value;

    /**
     * @brief Iterator for traversing elements within a list_value.
     *
     * This iterator provides random access traversal over the elements contained
     * within a list_value object. It implements the full random access iterator
     * interface, allowing efficient element access, arithmetic operations, and
     * comparison operations.
     *
     * The iterator maintains a pointer to the parent list_value and an index
     * into the flattened array data, providing O(1) access to any element
     * within the list bounds.
     *
     * @pre Parent list_value must remain valid for iterator lifetime
     * @post Iterator provides random access to list elements
     * @post Dereferencing yields const_reference to array elements
     * @post Supports all random access iterator operations
     */
    class SPARROW_API list_value_iterator : public iterator_base<
                                                list_value_iterator,
                                                array_traits::const_reference,
                                                std::random_access_iterator_tag,
                                                array_traits::const_reference>
    {
    public:

        using self_type = list_value_iterator;
        using base_type = iterator_base<list_value_iterator, list_value, std::random_access_iterator_tag>;
        using size_type = size_t;

        /**
         * @brief Default constructor creating an invalid iterator.
         *
         * @post Iterator is in default-constructed state
         * @post Iterator must be assigned before use
         */
        list_value_iterator() noexcept = default;

        /**
         * @brief Constructs iterator for the given list and index.
         *
         * @param layout Pointer to the parent list_value
         * @param index Index within the list (relative to list start)
         *
         * @pre layout must be a valid pointer to list_value
         * @pre index must be <= layout->size() (end iterator allowed)
         * @post Iterator is positioned at the specified index
         * @post Iterator is valid for dereferencing if index < layout->size()
         */
        list_value_iterator(const list_value* layout, size_type index);

    private:

        /**
         * @brief Dereferences the iterator to get the current element.
         *
         * @return Const reference to the element at current position
         *
         * @pre Iterator must be valid and not at end position
         * @pre Parent list_value must still be valid
         * @post Returns valid reference to array element
         */
        [[nodiscard]] reference dereference() const;

        /**
         * @brief Advances iterator to next position.
         *
         * @pre Iterator must not be at end position
         * @post Iterator is advanced by one position
         */
        void increment();

        /**
         * @brief Moves iterator to previous position.
         *
         * @pre Iterator must not be at begin position
         * @post Iterator is moved back by one position
         */
        void decrement();

        /**
         * @brief Advances iterator by specified offset.
         *
         * @param n Number of positions to advance (can be negative)
         *
         * @pre Final position must be within valid range [begin, end]
         * @post Iterator is advanced by n positions
         */
        void advance(difference_type n);

        /**
         * @brief Calculates distance to another iterator.
         *
         * @param rhs Target iterator to measure distance to
         * @return Number of positions between this and rhs
         *
         * @pre Both iterators must refer to the same list_value
         * @post Returns rhs.index - this.index
         */
        [[nodiscard]] difference_type distance_to(const self_type& rhs) const;

        /**
         * @brief Checks equality with another iterator.
         *
         * @param rhs Iterator to compare with
         * @return true if iterators point to same position
         *
         * @post Returns true iff both iterators have same parent and index
         */
        [[nodiscard]] bool equal(const self_type& rhs) const;

        /**
         * @brief Checks if this iterator is less than another.
         *
         * @param rhs Iterator to compare with
         * @return true if this iterator comes before rhs
         *
         * @pre Both iterators must refer to the same list_value
         * @post Returns true iff this.index < rhs.index
         */
        [[nodiscard]] bool less_than(const self_type& rhs) const;

        const list_value* m_list_value = nullptr;  ///< Pointer to parent list_value
        difference_type m_index;                   ///< Current index within the list

        friend class iterator_access;
    };

    /**
     * @brief Value type representing a list/array slice within a flattened array.
     *
     * This class provides a view over a contiguous range of elements within
     * a flattened array, representing a single list value in list array layouts.
     * It offers a container-like interface with random access iterators while
     * maintaining a lightweight view semantic.
     *
     * The list_value does not own the underlying data but provides efficient
     * access to a slice of elements defined by begin and end indices. It supports
     * all standard container operations including iteration, element access,
     * and size queries.
     *
     * Key features:
     * - Lightweight view over array slice
     * - Random access iterator support
     * - STL-compatible container interface
     * - Efficient O(1) element access
     * - Range-based loop support
     *
     * @pre Underlying array must remain valid for list_value lifetime
     * @post Provides container-like interface over array slice
     * @post Supports bidirectional and random access iteration
     * @post Thread-safe for read operations when underlying array is immutable
     *
     * @example
     * ```cpp
     * // Created by list array implementations
     * list_value list = ...;
     *
     * // Container-like access
     * if (!list.empty()) {
     *     auto first = list.front();
     *     auto last = list.back();
     *     auto third = list[2];
     * }
     *
     * // Iteration
     * for (const auto& element : list) {
     *     // Process each element
     * }
     * ```
     */
    class SPARROW_API list_value
    {
    public:

        using value_type = array_traits::value_type;
        using const_reference = array_traits::const_reference;
        using size_type = std::size_t;
        using list_value_reverse_iterator = std::reverse_iterator<list_value_iterator>;

        /**
         * @brief Default constructor creating an empty list view.
         *
         * @post size() returns 0
         * @post empty() returns true
         * @post All iterators are equal (begin() == end())
         */
        list_value() = default;

        /**
         * @brief Constructs list view over specified array range.
         *
         * @param flat_array Pointer to the flattened array containing list data
         * @param index_begin Starting index of the list (inclusive)
         * @param index_end Ending index of the list (exclusive)
         *
         * @pre flat_array must be a valid pointer to array_wrapper
         * @pre index_begin must be <= index_end
         * @pre index_end must be <= flat_array->size()
         * @post size() returns (index_end - index_begin)
         * @post List provides view over elements [index_begin, index_end)
         * @post Iterators are valid for traversing the specified range
         */
        list_value(const array_wrapper* flat_array, size_type index_begin, size_type index_end);

        /**
         * @brief Gets the number of elements in the list.
         *
         * @return Number of elements in the list view
         *
         * @post Returns non-negative count
         * @post Equals (index_end - index_begin)
         */
        [[nodiscard]] size_type size() const;

        /**
         * @brief Checks if the list is empty.
         *
         * @return true if list contains no elements, false otherwise
         *
         * @post Return value equals (size() == 0)
         * @post Equivalent to (begin() == end())
         */
        [[nodiscard]] bool empty() const;

        /**
         * @brief Gets element at specified position without bounds checking.
         *
         * @param i Index of element to access
         * @return Const reference to element at position i
         *
         * @pre i must be < size()
         * @post Returns valid reference to element
         * @post Reference remains valid while underlying array exists
         */
        const_reference operator[](size_type i) const;

        /**
         * @brief Gets reference to the first element.
         *
         * @return Const reference to the first element
         *
         * @pre List must not be empty (!empty())
         * @post Returns valid reference to first element
         * @post Equivalent to (*this)[0]
         */
        [[nodiscard]] const_reference front() const;

        /**
         * @brief Gets reference to the last element.
         *
         * @return Const reference to the last element
         *
         * @pre List must not be empty (!empty())
         * @post Returns valid reference to last element
         * @post Equivalent to (*this)[size() - 1]
         */
        [[nodiscard]] const_reference back() const;

        /**
         * @brief Gets iterator to the beginning of the list.
         *
         * @return Iterator pointing to the first element
         *
         * @post Iterator is valid for list traversal
         * @post For empty list, equals end()
         */
        [[nodiscard]] list_value_iterator begin();

        /**
         * @brief Gets const iterator to the beginning of the list.
         *
         * @return Const iterator pointing to the first element
         *
         * @post Iterator is valid for list traversal
         * @post For empty list, equals end()
         */
        [[nodiscard]] list_value_iterator begin() const;

        /**
         * @brief Gets const iterator to the beginning of the list.
         *
         * @return Const iterator pointing to the first element
         *
         * @post Iterator is valid for list traversal
         * @post Guarantees const iterator even for non-const list
         */
        [[nodiscard]] list_value_iterator cbegin() const;

        /**
         * @brief Gets iterator to the end of the list.
         *
         * @return Iterator pointing past the last element
         *
         * @post Iterator marks the end of the list range
         * @post Not dereferenceable
         */
        [[nodiscard]] list_value_iterator end();

        /**
         * @brief Gets const iterator to the end of the list.
         *
         * @return Const iterator pointing past the last element
         *
         * @post Iterator marks the end of the list range
         * @post Not dereferenceable
         */
        [[nodiscard]] list_value_iterator end() const;

        /**
         * @brief Gets const iterator to the end of the list.
         *
         * @return Const iterator pointing past the last element
         *
         * @post Iterator marks the end of the list range
         * @post Guarantees const iterator even for non-const list
         */
        [[nodiscard]] list_value_iterator cend() const;

        /**
         * @brief Gets reverse iterator to the beginning of reversed list.
         *
         * @return Reverse iterator pointing to the last element
         *
         * @post Iterator is valid for reverse traversal
         * @post For empty list, equals rend()
         */
        [[nodiscard]] list_value_reverse_iterator rbegin();

        /**
         * @brief Gets const reverse iterator to the beginning of reversed list.
         *
         * @return Const reverse iterator pointing to the last element
         *
         * @post Iterator is valid for reverse traversal
         * @post For empty list, equals rend()
         */
        [[nodiscard]] list_value_reverse_iterator rbegin() const;

        /**
         * @brief Gets const reverse iterator to the beginning of reversed list.
         *
         * @return Const reverse iterator pointing to the last element
         *
         * @post Iterator is valid for reverse traversal
         * @post Guarantees const iterator even for non-const list
         */
        [[nodiscard]] list_value_reverse_iterator crbegin() const;

        /**
         * @brief Gets reverse iterator to the end of reversed list.
         *
         * @return Reverse iterator pointing before the first element
         *
         * @post Iterator marks the end of reverse traversal
         * @post Not dereferenceable
         */
        [[nodiscard]] list_value_reverse_iterator rend();

        /**
         * @brief Gets const reverse iterator to the end of reversed list.
         *
         * @return Const reverse iterator pointing before the first element
         *
         * @post Iterator marks the end of reverse traversal
         * @post Not dereferenceable
         */
        [[nodiscard]] list_value_reverse_iterator rend() const;

        /**
         * @brief Gets const reverse iterator to the end of reversed list.
         *
         * @return Const reverse iterator pointing before the first element
         *
         * @post Iterator marks the end of reverse traversal
         * @post Guarantees const iterator even for non-const list
         */
        [[nodiscard]] list_value_reverse_iterator crend() const;

    private:

        const array_wrapper* p_flat_array = nullptr;  ///< Pointer to underlying flattened array
        size_type m_index_begin = 0u;                 ///< Starting index in flattened array
        size_type m_index_end = 0u;                   ///< Ending index in flattened array (exclusive)
    };

    /**
     * @brief Equality comparison operator for list_value objects.
     *
     * Compares two list_value objects for element-wise equality. Two lists are
     * considered equal if they have the same size and all corresponding elements
     * compare equal.
     *
     * @param lhs First list to compare
     * @param rhs Second list to compare
     * @return true if lists are element-wise equal, false otherwise
     *
     * @post Returns true iff both lists have same size and all elements compare equal
     * @post Comparison is performed element by element using array element equality
     */
    SPARROW_API
    bool operator==(const list_value& lhs, const list_value& rhs);
}

#if defined(__cpp_lib_format)

template <>
struct std::formatter<sparrow::list_value>
{
    constexpr auto parse(std::format_parse_context& ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();  // Simple implementation
    }

    SPARROW_API auto format(const sparrow::list_value& list_value, std::format_context& ctx) const
        -> decltype(ctx.out());
};

namespace sparrow
{
    SPARROW_API std::ostream& operator<<(std::ostream& os, const sparrow::list_value& value);
}

#endif
