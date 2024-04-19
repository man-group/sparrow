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
#include <compare>

#include "sparrow/algorithm.hpp"
#include "sparrow/array_data.hpp"
#include "sparrow/contracts.hpp"
#include "sparrow/data_traits.hpp"
#include "sparrow/data_type.hpp"
namespace sparrow
{
    template <class T, class L>
    requires is_arrow_base_type<T>
    class typed_array;

    template <class U, class M>
    std::partial_ordering operator<=>(const typed_array<U, M>& ta1, const typed_array<U, M>& ta2);

    template <class U, class M>
    bool operator==(const typed_array<U, M>& ta1, const typed_array<U, M>& ta2);

    /**
     * A class template representing a typed array.
     *
     * The `typed_array` class template provides an container interface over `array_data` for elements of a specific type `T`.
     * The access to the elements are executed according to the layout `L` of the array.
     *
     * @tparam T The type of elements stored in the array.
     * @tparam L The layout type of the array. Defaults to the default layout defined by the `arrow_traits` of `T`.
     */
    template <class T, class L = typename arrow_traits<T>::default_layout>
        requires is_arrow_base_type<T>
    class typed_array
    {
    public:

        using layout_type = L;

        using reference = typename layout_type::reference;
        using const_reference = typename layout_type::const_reference;

        using iterator = typename layout_type::iterator;
        using const_iterator = typename layout_type::const_iterator;

        using size_type = typename layout_type::size_type;
        using const_bitmap_range = typename layout_type::const_bitmap_range;
        using const_value_range = typename layout_type::const_value_range;

        explicit typed_array(array_data data);

        // Element access

        ///@{
        /*
         * Access specified element with bounds checking.
         *
         * Returns a reference to the element at the specified index \p i, with bounds checking.
         * If \p i is not within the range of the container, an exception of type std::out_of_range is thrown.
         *
         * @param i The index of the element to access.
         * @return A reference to the element at the specified index.
         * @throws std::out_of_range if i is out of range.
         */
        reference at(size_type i);
        const_reference at(size_type i) const;
        ///@}

        ///@{
        /*
         * Access specified element.
         *
         * Returns a reference to the element at the specified index \p i. No bounds checking is performed.
         *
         * @param i The index of the element to access.
         * @pre @p i must be lower than the size of the container.
         * @return A reference to the element at the specified index.
         */
        reference operator[](size_type);
        const_reference operator[](size_type) const;
        ///@}

        ///@{
        /*
         * Access the first element.
         *
         * Returns a reference to the first element.
         * @pre The container must not be empty (\see empty()).
         *
         * @return A reference to the first element.
         */
        reference front();
        const_reference front() const;
        ///@}

        ///@{
        /*
         * Access the last element.
         *
         * Returns a reference to the last element.
         *
         * @pre The container must not be empty (\see empty()).
         *
         * @return A reference to the last element.
         */
        reference back();
        const_reference back() const;
        ///@}

        // Iterators

        ///@{
        /* Returns an iterator to the first element.
         * If the vector is empty, the returned iterator will be equal to end().
         *
         * @return An iterator to the first element.
         */
        iterator begin();
        const_iterator begin() const;
        const_iterator cbegin() const;
        ///@}

        ///@{
        /**
         * This element acts as a placeholder; attempting to access it results in undefined behavior.
         *
         * @return An iterator to the element following the last element of the vector.
         */
        iterator end();
        const_iterator end() const;
        const_iterator cend() const;
        ///@}

        /*
         * @return A range of the bitmap. For each index position in this range, if `true` then there is a value at the same index position in the `values()` range, `false` means the value there is null.
         */
        const_bitmap_range bitmap() const;

        /*
         * @return A range of the values.
         */
        const_value_range values() const;

        // Capacity

        /*
         * @return true if the container is empty, false otherwise.
         */
        bool empty() const;

        /*
         * @return The number of elements in the container.
         */
        size_type size() const;

        // TODO: Add reserve, capacity, shrink_to_fit

        // Modifiers

        // TODO: Implement insert, erase, push_back, pop_back, clear, resize, swap

        friend std::partial_ordering operator<=><T, L>(const typed_array& ta1, const typed_array& ta2);

        friend bool operator==<T, L>(const typed_array& ta1, const typed_array& ta2);

    private:

        array_data m_data;
        layout_type m_layout;
    };

    // Constructors
    template <class T, class L>
        requires is_arrow_base_type<T>
    typed_array<T, L>::typed_array(array_data data)
        : m_data(std::move(data))
        , m_layout(m_data)
    {
    }

    // Element access

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::at(size_type i) -> reference
    {
        if (i >= size())
        {
            // TODO: Use our own format function
            throw std::out_of_range(
                "typed_array::at: index out of range for array of size " + std::to_string(size())
                + " at index " + std::to_string(i)
            );
        }
        return m_layout[i];
    }

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::at(size_type i) const -> const_reference
    {
        if (i >= size())
        {
            // TODO: Use our own format function
            throw std::out_of_range(
                "typed_array::at: index out of range for array of size " + std::to_string(size())
                + " at index " + std::to_string(i)
            );
        }
        return m_layout[i];
    }

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::operator[](size_type i) -> reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return m_layout[i];
    }

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return m_layout[i];
    }

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::front() -> reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return m_layout[0];
    }

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::front() const -> const_reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return m_layout[0];
    }

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::back() -> reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return m_layout[size() - 1];
    }

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::back() const -> const_reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return m_layout[size() - 1];
    }

    // Iterators

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::begin() -> iterator
    {
        return m_layout.begin();
    }

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::begin() const -> const_iterator
    {
        return m_layout.cbegin();
    }

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::end() -> iterator
    {
        return m_layout.end();
    }

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::end() const -> const_iterator
    {
        return m_layout.cend();
    }

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::cbegin() const -> const_iterator
    {
        return begin();
    }

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::cend() const -> const_iterator
    {
        return end();
    }

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::bitmap() const -> const_bitmap_range
    {
        return m_layout.bitmap();
    }

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::values() const -> const_value_range
    {
        return m_layout.values();
    }

    // Capacity

    template <class T, class L>
        requires is_arrow_base_type<T>
    bool typed_array<T, L>::empty() const
    {
        return m_layout.size() == 0;
    }

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto typed_array<T, L>::size() const -> size_type
    {
        return m_layout.size();
    }

    // Comparators

    template <class T, class L>
        requires is_arrow_base_type<T>
    auto operator<=>(const typed_array<T, L>& ta1, const typed_array<T, L>& ta2) -> std::partial_ordering
    {
        return lexicographical_compare_three_way(ta1, ta2);
    }

    template <class T, class L>
        requires is_arrow_base_type<T>
    bool operator==(const typed_array<T, L>& ta1, const typed_array<T, L>& ta2)
    {
        return std::equal(ta1.cbegin(), ta1.cend(), ta2.cbegin(), ta2.cend());
    }

}  // namespace sparrow
