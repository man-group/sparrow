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
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <utility>

#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset_view.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/layout/layout_iterator.hpp"
#include "sparrow/utils/crtp_base.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{
    /**
     * @brief Base class for array_inner_types specializations.
     *
     * Defines common types used across all array implementation classes.
     * Provides the foundation for array type traits and ensures consistency
     * across different array implementations.
     *
     * @post Provides bitmap_type definition for all array implementations
     */
    struct array_inner_types_base
    {
        using bitmap_type = dynamic_bitset_view<std::uint8_t>;
    };

    /**
     * @brief Traits class that must be specialized by array implementations.
     *
     * This traits class defines the types and interfaces that array classes
     * must provide. Each array implementation must specialize this template
     * to define their specific type requirements.
     *
     * @tparam D The derived array class type
     *
     * @pre D must be a complete array implementation type
     * @post Specialization must provide all required type definitions
     * @post Must inherit from array_inner_types_base
     *
     * Required specialization members:
     * - inner_value_type: The type of values stored in the array
     * - inner_reference: Reference type for array elements
     * - inner_const_reference: Const reference type for array elements
     * - value_iterator: Iterator type for values
     * - const_value_iterator: Const iterator type for values
     * - iterator_tag: Iterator category tag
     */
    template <class D>
    struct array_inner_types;

    /**
     * @brief CRTP base class providing common immutable interface for arrays with bitmaps.
     *
     * This class defines and implements the standard interface for arrays that hold
     * nullable elements using a validity bitmap. It provides efficient iteration,
     * element access, and range-based operations while maintaining Arrow format
     * compatibility.
     *
     * Key features:
     * - Const-correct element access with bounds checking
     * - STL-compatible iterator interface
     * - Range-based operations for values and validity bitmap
     * - Efficient slicing operations
     * - Arrow metadata access
     *
     * @tparam D The derived array implementation type (CRTP pattern)
     *
     * @pre D must specialize array_inner_types<D>
     * @pre D must implement required virtual methods (value, value_cbegin, value_cend, get_bitmap)
     * @post Provides complete const array interface
     * @post Maintains Arrow format compatibility
     * @post Thread-safe for read operations
     *
     * @example
     * ```cpp
     * class my_array : public array_crtp_base<my_array> {
     *     // Implement required methods
     * };
     *
     * my_array arr = ...;
     * auto nullable_elem = arr[0];  // Access with null checking
     * for (const auto& elem : arr) { ... }  // Range-based iteration
     * ```
     */
    template <class D>
    class array_crtp_base : public crtp_base<D>
    {
    public:

        using self_type = array_crtp_base<D>;
        using derived_type = D;

        using inner_types = array_inner_types<derived_type>;

        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        using bitmap_type = typename inner_types::bitmap_type;
        using bitmap_const_reference = bitmap_type::const_reference;
        using bitmap_iterator = bitmap_type::iterator;
        using const_bitmap_iterator = bitmap_type::const_iterator;
        using const_bitmap_range = std::ranges::subrange<const_bitmap_iterator>;

        using inner_value_type = typename inner_types::inner_value_type;
        using value_type = nullable<inner_value_type>;

        using inner_const_reference = typename inner_types::inner_const_reference;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;

        using const_value_iterator = typename inner_types::const_value_iterator;
        using const_value_range = std::ranges::subrange<const_value_iterator>;

        using iterator_tag = typename inner_types::iterator_tag;

        struct iterator_types
        {
            using value_type = self_type::value_type;
            using reference = self_type::const_reference;
            using value_iterator = self_type::const_value_iterator;
            using bitmap_iterator = self_type::const_bitmap_iterator;
            using iterator_tag = self_type::iterator_tag;
        };

        using const_iterator = layout_iterator<iterator_types>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        /**
         * @brief Gets the optional name of the array.
         *
         * @return Optional string view of the array name from Arrow schema
         *
         * @post Returns nullopt if no name is set
         * @post Returned string view remains valid while array exists
         */
        [[nodiscard]] constexpr std::optional<std::string_view> name() const;

        /**
         * @brief Gets the metadata associated with the array.
         *
         * @return Optional view of key-value metadata pairs from Arrow schema
         *
         * @post Returns nullopt if no metadata is set
         * @post Returned view remains valid while array exists
         */
        [[nodiscard]] std::optional<key_value_view> metadata() const;

        /**
         * @brief Checks if the array is empty.
         *
         * @return true if the array has no elements, false otherwise
         *
         * @post Return value equals (size() == 0)
         * @post Equivalent to begin() == end()
         */
        [[nodiscard]] constexpr bool empty() const;

        /**
         * @brief Gets the number of elements in the array.
         *
         * @return Number of elements in the array
         *
         * @post Returns non-negative value
         * @post Value corresponds to Arrow array length
         */
        [[nodiscard]] constexpr size_type size() const;

        /**
         * @brief Gets element at specified position with bounds checking.
         *
         * @param i Index of the element to access
         * @return Const reference to nullable element at position i
         *
         * @pre i must be < size()
         * @post Returns valid const_reference to element
         * @post Element includes both value and validity information
         *
         * @throws std::out_of_range if i >= size()
         */
        [[nodiscard]] constexpr const_reference at(size_type i) const;

        /**
         * @brief Gets element at specified position without bounds checking.
         *
         * @param i Index of the element to access
         * @return Const reference to nullable element at position i
         *
         * @pre i must be < size()
         * @post Returns valid const_reference to element
         * @post Element includes both value and validity information
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(i < derived_cast.size())
         */
        [[nodiscard]] constexpr const_reference operator[](size_type i) const;

        /**
         * @brief Gets reference to the first element.
         *
         * @return Const reference to the first element
         *
         * @pre Array must not be empty (!empty())
         * @post Returns valid reference to first element
         * @post Equivalent to (*this)[0]
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(!empty())
         * @note Calling front() on empty array causes undefined behavior
         */
        [[nodiscard]] constexpr const_reference front() const;

        /**
         * @brief Gets reference to the last element.
         *
         * @return Const reference to the last element
         *
         * @pre Array must not be empty (!empty())
         * @post Returns valid reference to last element
         * @post Equivalent to (*this)[size() - 1]
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(!empty())
         * @note Calling back() on empty array causes undefined behavior
         */
        [[nodiscard]] constexpr const_reference back() const;

        /**
         * @brief Gets iterator to the beginning of the array.
         *
         * @return Const iterator pointing to the first element
         *
         * @post Iterator is valid for array traversal
         * @post Equivalent to cbegin()
         */
        [[nodiscard]] constexpr const_iterator begin() const;

        /**
         * @brief Gets iterator to the end of the array.
         *
         * @return Const iterator pointing past the last element
         *
         * @post Iterator marks the end of the array range
         * @post Equivalent to cend()
         */
        [[nodiscard]] constexpr const_iterator end() const;

        /**
         * @brief Gets const iterator to the beginning of the array.
         *
         * @return Const iterator pointing to the first element
         *
         * @post Iterator is valid for array traversal
         * @post Guarantees const iterator even for non-const arrays
         */
        [[nodiscard]] constexpr const_iterator cbegin() const;

        /**
         * @brief Gets const iterator to the end of the array.
         *
         * @return Const iterator pointing past the last element
         *
         * @post Iterator marks the end of the array range
         * @post Guarantees const iterator even for non-const arrays
         */
        [[nodiscard]] constexpr const_iterator cend() const;

        /**
         * @brief Gets reverse iterator to the beginning of reversed array.
         *
         * @return Const reverse iterator pointing to the last element
         *
         * @post Iterator is valid for reverse traversal
         * @post Equivalent to crbegin()
         */
        [[nodiscard]] constexpr const_reverse_iterator rbegin() const;

        /**
         * @brief Gets reverse iterator to the end of reversed array.
         *
         * @return Const reverse iterator pointing before the first element
         *
         * @post Iterator marks the end of reverse traversal
         * @post Equivalent to crend()
         */
        [[nodiscard]] constexpr const_reverse_iterator rend() const;

        /**
         * @brief Gets const reverse iterator to the beginning of reversed array.
         *
         * @return Const reverse iterator pointing to the last element
         *
         * @post Iterator is valid for reverse traversal
         * @post Guarantees const iterator even for non-const arrays
         */
        [[nodiscard]] constexpr const_reverse_iterator crbegin() const;

        /**
         * @brief Gets const reverse iterator to the end of reversed array.
         *
         * @return Const reverse iterator pointing before the first element
         *
         * @post Iterator marks the end of reverse traversal
         * @post Guarantees const iterator even for non-const arrays
         */
        [[nodiscard]] constexpr const_reverse_iterator crend() const;

        /**
         * @brief Gets the validity bitmap as a range.
         *
         * @return Range over the validity bitmap
         *
         * @post Range size equals array size
         * @post Range provides access to validity flags for each element
         * @post true indicates valid element, false indicates null
         */
        [[nodiscard]] constexpr const_bitmap_range bitmap() const;

        /**
         * @brief Gets the raw values as a range.
         *
         * @return Range over the raw values (without validity information)
         *
         * @post Range size equals array size
         * @post Range provides access to stored values regardless of validity
         * @post Values at null positions are still accessible but semantically invalid
         */
        [[nodiscard]] constexpr const_value_range values() const;

        /**
         * @brief Creates a sliced copy of the array.
         *
         * Creates a new array containing only elements between start and end indices.
         * The underlying data is not copied; only the Arrow offset and length are modified.
         *
         * @param start Index of the first element to keep (inclusive)
         * @param end Index of the first element to exclude (exclusive)
         * @return New array containing the sliced range
         *
         * @pre start must be <= end
         * @pre start must be <= size()
         * @pre end should be <= size() for valid elements
         * @post Returned array has length (end - start)
         * @post Elements maintain their original validity and values
         * @post If end > buffer size, trailing elements may be invalid
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(start <= end)
         */
        [[nodiscard]] constexpr D slice(size_type start, size_type end) const;

        /**
         * @brief Creates a sliced view of the array.
         *
         * Creates a view over elements between start and end indices without copying.
         * The underlying data buffers are shared with the original array.
         *
         * @param start Index of the first element to keep (inclusive)
         * @param end Index of the first element to exclude (exclusive)
         * @return Array view containing the sliced range
         *
         * @pre start must be <= end
         * @pre start must be <= size()
         * @pre end should be <= size() for valid elements
         * @pre Original array must remain valid while view is used
         * @post Returned view has length (end - start)
         * @post View shares data with original array
         * @post Changes to original array may affect the view
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(start <= end)
         */
        [[nodiscard]] constexpr D slice_view(size_type start, size_type end) const;

    protected:

        /**
         * @brief Protected constructor from Arrow proxy.
         *
         * @param proxy Arrow proxy containing array data and schema
         *
         * @pre proxy must contain valid Arrow array and schema
         * @post Array is initialized with data from proxy
         * @post Arrow proxy is moved and stored internally
         */
        explicit array_crtp_base(arrow_proxy);

        constexpr array_crtp_base(const array_crtp_base&) = default;
        constexpr array_crtp_base& operator=(const array_crtp_base&) = default;

        constexpr array_crtp_base(array_crtp_base&&) noexcept = default;
        constexpr array_crtp_base& operator=(array_crtp_base&&) noexcept = default;

        /**
         * @brief Gets mutable reference to the Arrow proxy.
         *
         * @return Mutable reference to internal Arrow proxy
         *
         * @post Returns valid reference to Arrow proxy
         */
        [[nodiscard]] constexpr arrow_proxy& get_arrow_proxy() noexcept;

        /**
         * @brief Gets const reference to the Arrow proxy.
         *
         * @return Const reference to internal Arrow proxy
         *
         * @post Returns valid const reference to Arrow proxy
         */
        [[nodiscard]] constexpr const arrow_proxy& get_arrow_proxy() const noexcept;

        /**
         * @brief Checks if element at index i has a valid value.
         *
         * @param i Index of element to check
         * @return Reference to validity flag for element i
         *
         * @pre i must be < size()
         * @post Returns reference to validity bit for element i
         * @post true indicates valid element, false indicates null
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(i < size())
         */
        constexpr bitmap_const_reference has_value(size_type i) const;

        /**
         * @brief Gets bitmap iterator to the beginning.
         *
         * @return Iterator to first validity bit
         *
         * @post Iterator accounts for array offset
         * @post Iterator is valid for bitmap traversal
         */
        constexpr const_bitmap_iterator bitmap_begin() const;

        /**
         * @brief Gets bitmap iterator to the end.
         *
         * @return Iterator past last validity bit
         *
         * @post Iterator marks end of bitmap range
         */
        constexpr const_bitmap_iterator bitmap_end() const;

        /**
         * @brief Gets const bitmap iterator to the beginning.
         *
         * @return Const iterator to first validity bit
         *
         * @post Iterator accounts for array offset
         * @post Guarantees const iterator
         */
        constexpr const_bitmap_iterator bitmap_cbegin() const;

        /**
         * @brief Gets const bitmap iterator to the end.
         *
         * @return Const iterator past last validity bit
         *
         * @post Iterator marks end of bitmap range
         * @post Guarantees const iterator
         */
        constexpr const_bitmap_iterator bitmap_cend() const;

    private:

        arrow_proxy m_proxy;  ///< Internal Arrow proxy containing array data

        // friend classes
        friend class layout_iterator<iterator_types>;
        friend class detail::array_access;
#if defined(__cpp_lib_format)
        friend struct std::formatter<D>;
#endif
    };

    template <class D>
    constexpr bool operator==(const array_crtp_base<D>& lhs, const array_crtp_base<D>& rhs);

    /**********************************
     * array_crtp_base implementation *
     **********************************/

    template <class D>
    constexpr std::optional<std::string_view> array_crtp_base<D>::name() const
    {
        return get_arrow_proxy().name();
    }

    template <class D>
    std::optional<key_value_view> array_crtp_base<D>::metadata() const
    {
        return get_arrow_proxy().metadata();
    }

    template <class D>
    constexpr bool array_crtp_base<D>::empty() const
    {
        return size() == size_type(0);
    }

    template <class D>
    constexpr auto array_crtp_base<D>::size() const -> size_type
    {
        return static_cast<size_type>(get_arrow_proxy().length());
    }

    template <class D>
    constexpr auto array_crtp_base<D>::at(size_type i) const -> const_reference
    {
        if (i >= size())
        {
            const std::string error_message = "Index " + std::to_string(i)
                                              + " is greater or equal to size of array ("
                                              + std::to_string(size()) + ")";
            throw std::out_of_range(error_message);
        }
        return (*this)[i];
    }

    template <class D>
    constexpr auto array_crtp_base<D>::operator[](size_type i) const -> const_reference
    {
        auto& derived_cast = this->derived_cast();
        SPARROW_ASSERT_TRUE(i < derived_cast.size());
        return const_reference(inner_const_reference(derived_cast.value(i)), derived_cast.has_value(i));
    }

    template <class D>
    constexpr auto array_crtp_base<D>::front() const -> const_reference
    {
        SPARROW_ASSERT_TRUE(!empty());
        return (*this)[size_type(0)];
    }

    template <class D>
    constexpr auto array_crtp_base<D>::back() const -> const_reference
    {
        SPARROW_ASSERT_TRUE(!empty());
        return (*this)[size() - 1];
    }

    template <class D>
    constexpr auto array_crtp_base<D>::begin() const -> const_iterator
    {
        return cbegin();
    }

    template <class D>
    constexpr auto array_crtp_base<D>::end() const -> const_iterator
    {
        return cend();
    }

    template <class D>
    constexpr auto array_crtp_base<D>::cbegin() const -> const_iterator
    {
        return const_iterator(this->derived_cast().value_cbegin(), bitmap_begin());
    }

    template <class D>
    constexpr auto array_crtp_base<D>::cend() const -> const_iterator
    {
        return const_iterator(this->derived_cast().value_cend(), bitmap_end());
    }

    template <class D>
    constexpr auto array_crtp_base<D>::rbegin() const -> const_reverse_iterator
    {
        return crbegin();
    }

    template <class D>
    constexpr auto array_crtp_base<D>::rend() const -> const_reverse_iterator
    {
        return crend();
    }

    template <class D>
    constexpr auto array_crtp_base<D>::crbegin() const -> const_reverse_iterator
    {
        return const_reverse_iterator(cend());
    }

    template <class D>
    constexpr auto array_crtp_base<D>::crend() const -> const_reverse_iterator
    {
        return const_reverse_iterator(cbegin());
    }

    template <class D>
    constexpr auto array_crtp_base<D>::bitmap() const -> const_bitmap_range
    {
        return const_bitmap_range(bitmap_begin(), bitmap_end());
    }

    template <class D>
    constexpr auto array_crtp_base<D>::values() const -> const_value_range
    {
        return const_value_range(this->derived_cast().value_cbegin(), this->derived_cast().value_cend());
    }

    template <class D>
    array_crtp_base<D>::array_crtp_base(arrow_proxy proxy)
        : m_proxy(std::move(proxy))
    {
    }

    template <class D>
    constexpr auto array_crtp_base<D>::get_arrow_proxy() noexcept -> arrow_proxy&
    {
        return m_proxy;
    }

    template <class D>
    constexpr auto array_crtp_base<D>::get_arrow_proxy() const noexcept -> const arrow_proxy&
    {
        return m_proxy;
    }

    template <class D>
    constexpr auto array_crtp_base<D>::has_value(size_type i) const -> bitmap_const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return *sparrow::next(bitmap_begin(), i);
    }

    template <class D>
    constexpr auto array_crtp_base<D>::bitmap_begin() const -> const_bitmap_iterator
    {
        return sparrow::next(this->derived_cast().get_bitmap().cbegin(), get_arrow_proxy().offset());
    }

    template <class D>
    constexpr auto array_crtp_base<D>::bitmap_end() const -> const_bitmap_iterator
    {
        return sparrow::next(bitmap_begin(), size());
    }

    template <class D>
    constexpr auto array_crtp_base<D>::bitmap_cbegin() const -> const_bitmap_iterator
    {
        return bitmap_begin();
    }

    template <class D>
    constexpr auto array_crtp_base<D>::bitmap_cend() const -> const_bitmap_iterator
    {
        return bitmap_end();
    }

    template <class D>
    constexpr D array_crtp_base<D>::slice(size_type start, size_type end) const
    {
        SPARROW_ASSERT_TRUE(start <= end);
        return D{get_arrow_proxy().slice(start, end)};
    }

    template <class D>
    constexpr D array_crtp_base<D>::slice_view(size_type start, size_type end) const
    {
        SPARROW_ASSERT_TRUE(start <= end);
        return D{get_arrow_proxy().slice_view(start, end)};
    }

    /*
     * @brief Equality comparison operator for arrays.
     *
     * Compares two arrays element-wise, including both values and validity flags.
     *
     * @tparam D Array type
     * @param lhs First array to compare
     * @param rhs Second array to compare
     * @return true if arrays are element-wise equal, false otherwise
     *
     * @post Returns true iff arrays have same size and all elements compare equal
     * @post Comparison includes both values and validity states
     */
    template <class D>
    constexpr bool operator==(const array_crtp_base<D>& lhs, const array_crtp_base<D>& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }
}

#if defined(__cpp_lib_format)

template <typename D>
    requires std::derived_from<D, sparrow::array_crtp_base<D>>
struct std::formatter<D>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const D& ar, std::format_context& ctx) const
    {
        const auto& proxy = ar.get_arrow_proxy();
        std::string type;
        if (proxy.dictionary())
        {
            std::format_to(ctx.out(), "Dictionary<{}>", proxy.dictionary()->data_type());
        }
        else
        {
            std::format_to(ctx.out(), "{}", proxy.data_type());
        }
        std::format_to(ctx.out(), " [name={} | size={}] <", ar.name().value_or("nullptr"), proxy.length());

        std::for_each(
            ar.cbegin(),
            std::prev(ar.cend()),
            [&ctx](const auto& value)
            {
                std::format_to(ctx.out(), "{}, ", value);
            }
        );
        return std::format_to(ctx.out(), "{}>", ar.back());
    }
};

namespace sparrow
{
    template <typename D>
        requires std::derived_from<D, array_crtp_base<D>>
    std::ostream& operator<<(std::ostream& os, const D& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

#endif
