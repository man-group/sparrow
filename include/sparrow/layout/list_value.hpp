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
#include "sparrow/layout/array_helper.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{
    class array;

    class list_value;

    template <class L>
    class list_reference;

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
    class SPARROW_API list_value_iterator : public pointer_index_iterator_base<
                                                list_value_iterator,
                                                const array,
                                                array_traits::const_reference,
                                                array_traits::const_reference,
                                                std::random_access_iterator_tag>
    {
    public:

        using self_type = list_value_iterator;
        using base_type = pointer_index_iterator_base<
            list_value_iterator,
            const array,
            array_traits::const_reference,
            array_traits::const_reference,
            std::random_access_iterator_tag>;
        using size_type = size_t;

        /**
         * @brief Default constructor creating an invalid iterator.
         *
         * @post Iterator is in default-constructed state
         * @post Iterator must be assigned before use
         */
        list_value_iterator() noexcept = default;

        /**
         * @brief Constructs iterator from the raw flat array and absolute position.
         *
         * @param flat_array Pointer to the flat child array
         * @param index Absolute position within the flat array
         *
         * @pre flat_array must be a valid non-null pointer
         * @post Iterator is positioned at index in the flat array
         */
        list_value_iterator(const array* flat_array, size_type index);
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
         * @pre flat_array must be a valid pointer to the flat child handle
         * @pre index_begin must be <= index_end
         * @pre index_end must be <= flat_array->size()
         * @post size() returns (index_end - index_begin)
         * @post List provides view over elements [index_begin, index_end)
         * @post Iterators are valid for traversing the specified range
         */
        list_value(const array* flat_array, size_type index_begin, size_type index_end);

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

        /// @brief Returns a pointer to the flat array backing this list view.
        [[nodiscard]] const array* flat_array() const noexcept
        {
            return p_flat_array;
        }

        /**
         * @brief Returns the inclusive start index of this list slice in the flat array.
         *
         * @return Inclusive start index in the flattened array
         */
        [[nodiscard]] size_type begin_index() const noexcept
        {
            return m_index_begin;
        }

        /**
         * @brief Returns the exclusive end index of this list slice in the flat array.
         *
         * @return Exclusive end index in the flattened array
         */
        [[nodiscard]] size_type end_index() const noexcept
        {
            return m_index_end;
        }

        const array* p_flat_array = nullptr;  ///< Pointer to underlying flattened array
        size_type m_index_begin = 0u;         ///< Starting index in flattened array
        size_type m_index_end = 0u;           ///< Ending index in flattened array (exclusive)


        template <bool BIG>
        friend class list_array_impl;

        template <bool BIG>
        friend class list_view_array_impl;

        friend class fixed_sized_list_array;

        template <class DERIVED>
        friend class list_array_crtp_base;
    };

    SPARROW_API bool operator==(const list_value& lhs, const list_value& rhs);

    template <class L>
    class list_reference
    {
    public:

        using self_type = list_reference<L>;
        using value_type = list_value;
        using size_type = list_value::size_type;
        using iterator = list_value_iterator;
        using const_iterator = list_value_iterator;

        constexpr list_reference(L* layout, size_type index)
            : p_layout(layout)
            , m_index(index)
        {
        }

        constexpr list_reference(const self_type&) noexcept = default;
        constexpr list_reference(self_type&&) noexcept = default;
        ~list_reference() = default;

        self_type& operator=(const list_value& rhs)
        {
            p_layout->replace_value(m_index, rhs);
            return *this;
        }

        self_type& operator=(const self_type& rhs)
        {
            return operator=(static_cast<list_value>(rhs));
        }

        self_type& operator=(self_type&& rhs)
        {
            return operator=(static_cast<list_value>(rhs));
        }

        operator list_value() const noexcept
        {
            const auto [b, e] = p_layout->offset_range(m_index);
            return list_value(p_layout->raw_flat_array(), static_cast<size_type>(b), static_cast<size_type>(e));
        }

        [[nodiscard]] size_type size() const noexcept
        {
            return view().size();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return view().empty();
        }

        [[nodiscard]] auto operator[](size_type i) const
        {
            return view()[i];
        }

        [[nodiscard]] auto front() const
        {
            return view().front();
        }

        [[nodiscard]] auto back() const
        {
            return view().back();
        }

        [[nodiscard]] iterator begin() const
        {
            return view().begin();
        }

        [[nodiscard]] iterator end() const
        {
            return view().end();
        }

        [[nodiscard]] iterator cbegin() const
        {
            return view().cbegin();
        }

        [[nodiscard]] iterator cend() const
        {
            return view().cend();
        }

        [[nodiscard]] bool operator==(const list_value& rhs) const
        {
            return view() == rhs;
        }

        [[nodiscard]] bool operator==(const self_type& rhs) const
        {
            return view() == rhs.view();
        }

    private:

        [[nodiscard]] list_value view() const noexcept
        {
            return static_cast<list_value>(*this);
        }

        L* p_layout = nullptr;
        size_type m_index = 0;
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
    SPARROW_API std::ostream& operator<<(std::ostream& os, const list_value& value);
}

#endif
