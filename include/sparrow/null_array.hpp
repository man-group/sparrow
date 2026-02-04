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

#include <cstddef>
#include <optional>
#include <ranges>

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/metadata.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/utils/repeat_container.hpp"

namespace sparrow
{
    /**
     * @brief Iterator for null arrays where all elements are null.
     *
     * This iterator provides a memory-efficient way to iterate over null arrays
     * without storing actual data. It generates null values on-demand and maintains
     * position information to support all iterator operations.
     *
     * The iterator satisfies the contiguous iterator concept despite not pointing
     * to actual memory, as it represents a conceptually contiguous sequence of
     * identical null values.
     *
     * @tparam T The value type of the iterator (typically nullable<null_type>)
     *
     * @pre T must be default constructible to represent null values
     * @post Iterator provides contiguous iterator semantics
     * @post Dereferencing always yields default-constructed T (null value)
     * @post Iterator operations have O(1) complexity
     *
     * @code{.cpp}
     * empty_iterator<nullable<null_type>> it(0);
     * auto null_val = *it;  // Always returns default-constructed nullable
     * ++it;                 // Advances position counter
     * @endcode
     */
    template <class T>
    class empty_iterator : public iterator_base<empty_iterator<T>, T, std::contiguous_iterator_tag, T>
    {
    public:

        using self_type = empty_iterator<T>;
        using base_type = iterator_base<self_type, T, std::contiguous_iterator_tag, T>;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;

        /**
         * @brief Constructs an empty iterator at the specified position.
         *
         * @param index Position index for the iterator (defaults to 0)
         *
         * @post Iterator is positioned at the specified index
         * @post Iterator is ready for all iterator operations
         */
        explicit empty_iterator(difference_type index = difference_type()) noexcept;

    private:

        /**
         * @brief Dereferences the iterator to get a null value.
         *
         * @return Default-constructed T representing a null value
         *
         * @post Always returns the same null value regardless of position
         * @post Return value represents a null element
         */
        [[nodiscard]] reference dereference() const;

        /**
         * @brief Advances the iterator to the next position.
         *
         * @post m_index is incremented by 1
         * @post Iterator points to next conceptual position
         */
        void increment();

        /**
         * @brief Moves the iterator to the previous position.
         *
         * @post m_index is decremented by 1
         * @post Iterator points to previous conceptual position
         */
        void decrement();

        /**
         * @brief Advances the iterator by a specified offset.
         *
         * @param n Number of positions to advance (can be negative)
         *
         * @post m_index is advanced by n
         * @post Iterator points to position (original_index + n)
         */
        void advance(difference_type n);

        /**
         * @brief Calculates distance to another iterator.
         *
         * @param rhs Target iterator to measure distance to
         * @return Number of positions between this and rhs
         *
         * @post Returns rhs.m_index - this.m_index
         * @post Result can be negative if rhs comes before this
         */
        [[nodiscard]] difference_type distance_to(const self_type& rhs) const;

        /**
         * @brief Checks equality with another iterator.
         *
         * @param rhs Iterator to compare with
         * @return true if iterators point to the same position
         *
         * @post Returns true iff m_index == rhs.m_index
         */
        [[nodiscard]] bool equal(const self_type& rhs) const;

        /**
         * @brief Checks if this iterator is less than another.
         *
         * @param rhs Iterator to compare with
         * @return true if this iterator comes before rhs
         *
         * @post Returns true iff m_index < rhs.m_index
         */
        [[nodiscard]] bool less_than(const self_type& rhs) const;

        difference_type m_index;  ///< Current position index

        friend class iterator_access;
    };

    class null_array;

    /**
     * @brief Type trait to check if a type is a null_array.
     *
     * @tparam T Type to check
     */
    template <class T>
    constexpr bool is_null_array_v = std::same_as<T, null_array>;

    namespace detail
    {
        template <>
        struct get_data_type_from_array<null_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::NA;
            }
        };
    }

    /**
     * @brief Memory-efficient array implementation for null data types.
     *
     * The null_array provides a specialized implementation for storing arrays where
     * all values are null. This is a significant optimization that avoids allocating
     * any memory buffers while still providing the full array interface.
     *
     * Key features:
     * - Zero memory allocation for data storage
     * - All elements are conceptually null
     * - Full STL-compatible container interface
     * - Arrow format compatibility with "n" format
     * - Efficient iteration without data access
     *
     * This implementation is particularly useful for:
     * - Placeholder columns in data processing
     * - Testing and development scenarios
     * - Memory-constrained environments
     * - Large arrays of conceptually missing data
     *
     * Related Apache Arrow specification:
     * https://arrow.apache.org/docs/dev/format/Columnar.html#null-layout
     *
     * @pre All operations assume null semantics
     * @post Maintains Arrow null format compatibility ("n")
     * @post All elements are semantically null
     * @post Memory usage is O(1) regardless of array size
     * @post Thread-safe for read operations
     *
     * @code{.cpp}
     * // Create null array with 1000 elements
     * null_array arr(1000, "null_column");
     *
     * // All elements are null
     * auto elem = arr[500];  // Returns nullable<null_type> in null state
     * assert(!elem.has_value());
     *
     * // Iteration works normally
     * for (const auto& null_elem : arr) {
     *     assert(!null_elem.has_value());
     * }
     * @endcode
     */
    class null_array
    {
    public:

        using inner_value_type = null_type;
        using value_type = nullable<inner_value_type>;
        using iterator = empty_iterator<value_type>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_iterator = empty_iterator<value_type>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using reference = iterator::reference;
        using const_reference = const_iterator::reference;
        using size_type = std::size_t;
        using difference_type = iterator::difference_type;
        using iterator_tag = std::random_access_iterator_tag;

        using const_value_iterator = empty_iterator<int>;
        using const_bitmap_iterator = empty_iterator<bool>;

        using const_value_range = std::ranges::subrange<const_value_iterator>;
        using const_bitmap_range = std::ranges::subrange<const_bitmap_iterator>;

        /**
         * @brief Constructs a null array with specified length and metadata.
         *
         * @tparam METADATA_RANGE Type of metadata container
         * @param length Number of null elements in the array
         * @param name Optional name for the array
         * @param metadata Optional metadata key-value pairs
         *
         * @pre length must be non-negative
         * @post Array contains length null elements
         * @post All elements are in null state
         * @post Array has Arrow format "n"
         * @post Memory usage is independent of length
         */
        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        null_array(
            size_t length,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
            : m_proxy(create_proxy(length, std::move(name), std::move(metadata)))
        {
        }

        /**
         * @brief Constructs null array from Arrow proxy.
         *
         * @param proxy Arrow proxy containing null array data and schema
         *
         * @pre proxy must contain valid Arrow null array and schema
         * @pre proxy format must be "n"
         * @post Array is initialized with data from proxy
         * @post All elements are conceptually null
         */
        SPARROW_API explicit null_array(arrow_proxy);

        /** Copy constructor */
        SPARROW_API null_array(const null_array&);
        
        /** Copy assignment operator */
        null_array& operator=(const null_array&) = default;

        /**
         * @brief Gets the optional name of the array.
         *
         * @return Optional string view of the array name from Arrow schema
         *
         * @post Returns nullopt if no name is set
         * @post Returned string view remains valid while array exists
         */
        [[nodiscard]] SPARROW_API std::optional<std::string_view> name() const;

        /**
         * @brief Gets the metadata associated with the array.
         *
         * @return Optional view of key-value metadata pairs from Arrow schema
         *
         * @post Returns nullopt if no metadata is set
         * @post Returned view remains valid while array exists
         */
        [[nodiscard]] SPARROW_API std::optional<key_value_view> metadata() const;

        /**
         * @brief Gets the number of elements in the array.
         *
         * @return Number of null elements in the array
         *
         * @post Returns non-negative value
         * @post All size() elements are null
         */
        [[nodiscard]] SPARROW_API size_type size() const;

        /**
         * @brief Gets mutable reference to element at specified position.
         *
         * @param i Index of the element to access
         * @return Reference to null value at position i
         *
         * @pre i must be < size()
         * @post Returns null value regardless of index
         * @post Reference represents conceptually null element
         */
        [[nodiscard]] SPARROW_API reference operator[](size_type i);

        /**
         * @brief Gets const reference to element at specified position.
         *
         * @param i Index of the element to access
         * @return Const reference to null value at position i
         *
         * @pre i must be < size()
         * @post Returns null value regardless of index
         * @post Reference represents conceptually null element
         */
        [[nodiscard]] SPARROW_API const_reference operator[](size_type i) const;

        /**
         * @brief Gets iterator to the beginning of the array.
         *
         * @return Iterator pointing to the first null element
         *
         * @post Iterator is valid for array traversal
         * @post For empty array, equals end()
         */
        [[nodiscard]] SPARROW_API iterator begin();

        /**
         * @brief Gets iterator to the end of the array.
         *
         * @return Iterator pointing past the last null element
         *
         * @post Iterator marks the end of the array range
         * @post Not dereferenceable
         */
        [[nodiscard]] SPARROW_API iterator end();

        /**
         * @brief Gets const iterator to the beginning of the array.
         *
         * @return Const iterator pointing to the first null element
         *
         * @post Iterator is valid for array traversal
         * @post For empty array, equals end()
         */
        [[nodiscard]] SPARROW_API const_iterator begin() const;

        /**
         * @brief Gets const iterator to the end of the array.
         *
         * @return Const iterator pointing past the last null element
         *
         * @post Iterator marks the end of the array range
         * @post Not dereferenceable
         */
        [[nodiscard]] SPARROW_API const_iterator end() const;

        /**
         * @brief Gets reverse iterator to the beginning of reversed array.
         *
         * @return Reverse iterator pointing to the last null element
         *
         * @post Iterator is valid for reverse traversal
         * @post For empty array, equals rend()
         */
        [[nodiscard]] SPARROW_API reverse_iterator rbegin();

        /**
         * @brief Gets reverse iterator to the end of reversed array.
         *
         * @return Reverse iterator pointing before the first null element
         *
         * @post Iterator marks the end of reverse traversal
         * @post Not dereferenceable
         */
        [[nodiscard]] SPARROW_API reverse_iterator rend();

        /**
         * @brief Gets const reverse iterator to the beginning of reversed array.
         *
         * @return Const reverse iterator pointing to the last null element
         *
         * @post Iterator is valid for reverse traversal
         * @post For empty array, equals rend()
         */
        [[nodiscard]] SPARROW_API const_reverse_iterator rbegin() const;

        /**
         * @brief Gets const reverse iterator to the end of reversed array.
         *
         * @return Const reverse iterator pointing before the first null element
         *
         * @post Iterator marks the end of reverse traversal
         * @post Not dereferenceable
         */
        [[nodiscard]] SPARROW_API const_reverse_iterator rend() const;

        /**
         * @brief Gets const iterator to the beginning of the array.
         *
         * @return Const iterator pointing to the first null element
         *
         * @post Iterator is valid for array traversal
         * @post Guarantees const iterator even for non-const array
         */
        [[nodiscard]] SPARROW_API const_iterator cbegin() const;

        /**
         * @brief Gets const iterator to the end of the array.
         *
         * @return Const iterator pointing past the last null element
         *
         * @post Iterator marks the end of the array range
         * @post Guarantees const iterator even for non-const array
         */
        [[nodiscard]] SPARROW_API const_iterator cend() const;

        /**
         * @brief Gets const reverse iterator to the beginning of reversed array.
         *
         * @return Const reverse iterator pointing to the last null element
         *
         * @post Iterator is valid for reverse traversal
         * @post For empty array, equals crend()
         */
        [[nodiscard]] SPARROW_API const_reverse_iterator crbegin() const;

        /**
         * @brief Gets const reverse iterator to the end of reversed array.
         *
         * @return Const reverse iterator pointing before the first null element
         *
         * @post Iterator marks the end of reverse traversal
         * @post Not dereferenceable
         */
        [[nodiscard]] SPARROW_API const_reverse_iterator crend() const;

        /**
         * @brief Gets reference to the first element.
         *
         * @return Reference to the first null element
         *
         * @pre Array must not be empty (size() > 0)
         * @post Returns null value
         * @post Equivalent to (*this)[0]
         */
        [[nodiscard]] SPARROW_API reference front();

        /**
         * @brief Gets const reference to the first element.
         *
         * @return Const reference to the first null element
         *
         * @pre Array must not be empty (size() > 0)
         * @post Returns null value
         * @post Equivalent to (*this)[0]
         */
        [[nodiscard]] SPARROW_API const_reference front() const;

        /**
         * @brief Gets reference to the last element.
         *
         * @return Reference to the last null element
         *
         * @pre Array must not be empty (size() > 0)
         * @post Returns null value
         * @post Equivalent to (*this)[size() - 1]
         */
        [[nodiscard]] SPARROW_API reference back();

        /**
         * @brief Gets const reference to the last element.
         *
         * @return Const reference to the last null element
         *
         * @pre Array must not be empty (size() > 0)
         * @post Returns null value
         * @post Equivalent to (*this)[size() - 1]
         */
        [[nodiscard]] SPARROW_API const_reference back() const;

        /**
         * @brief Gets the values as a range (conceptually empty for null arrays).
         *
         * @return Range over conceptual values (empty for null arrays)
         *
         * @post Range represents the conceptual values in the null array
         * @post All values are conceptually absent/null
         */
        [[nodiscard]] SPARROW_API const_value_range values() const;

        /**
         * @brief Gets the validity bitmap as a range (all false for null arrays).
         *
         * @return Range over validity flags (all false)
         *
         * @post Range size equals array size
         * @post All bitmap values are false (indicating null)
         */
        [[nodiscard]] SPARROW_API const_bitmap_range bitmap() const;

        /**
         * @brief Resizes the null array to the specified size.
         *
         * Changes the number of null elements in the array. Since all elements
         * are conceptually null, this operation only updates the size metadata
         * without requiring any data buffer reallocation.
         *
         * @param new_size The new number of null elements
         *
         * @pre new_size must be non-negative
         * @post size() returns new_size
         * @post All elements (old and new) remain null
         * @post Memory usage remains O(1) regardless of new_size
         * @post Iterator ranges are updated to reflect new size
         *
         * @code{.cpp}
         * null_array arr(100);
         * arr.resize(200);  // Array now has 200 null elements
         * assert(arr.size() == 200);
         * @endcode
         */
        SPARROW_API void resize(size_type new_size);

    private:

        /**
         * @brief Creates Arrow proxy for null array with specified parameters.
         *
         * @tparam METADATA_RANGE Type of metadata container
         * @param length Number of null elements
         * @param name Optional array name
         * @param metadata Optional metadata pairs
         * @return Arrow proxy containing null array data and schema
         *
         * @pre length must be non-negative
         * @post Returns valid Arrow proxy with null format ("n")
         * @post Schema indicates nullable with all elements null
         * @post No data buffers are allocated
         */
        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy
        create_proxy(size_t length, std::optional<std::string_view> name, std::optional<METADATA_RANGE> metadata);

        /**
         * @brief Gets the size as a signed difference type.
         *
         * @return Array size as difference_type
         *
         * @post Returns size() cast to difference_type
         */
        [[nodiscard]] SPARROW_API difference_type ssize() const;

        /**
         * @brief Gets mutable reference to the Arrow proxy.
         *
         * @return Mutable reference to internal Arrow proxy
         *
         * @post Returns valid reference to Arrow proxy
         */
        [[nodiscard]] SPARROW_API arrow_proxy& get_arrow_proxy();

        /**
         * @brief Gets const reference to the Arrow proxy.
         *
         * @return Const reference to internal Arrow proxy
         *
         * @post Returns valid const reference to Arrow proxy
         */
        [[nodiscard]] SPARROW_API const arrow_proxy& get_arrow_proxy() const;

        arrow_proxy m_proxy;  ///< Internal Arrow proxy containing null array data

        friend class detail::array_access;
    };

    /**
     * @brief Equality comparison operator for null arrays.
     *
     * Two null arrays are considered equal if they have the same size,
     * since all elements are conceptually null.
     *
     * @param lhs First null array to compare
     * @param rhs Second null array to compare
     * @return true if arrays have the same size, false otherwise
     *
     * @post Returns true iff lhs.size() == rhs.size()
     * @post Content comparison is trivial since all elements are null
     */
    SPARROW_API
    bool operator==(const null_array& lhs, const null_array& rhs);

    /*********************************
     * empty_iterator implementation *
     *********************************/

    template <class T>
    empty_iterator<T>::empty_iterator(difference_type index) noexcept
        : m_index(index)
    {
    }

    template <class T>
    auto empty_iterator<T>::dereference() const -> reference
    {
        return T();
    }

    template <class T>
    void empty_iterator<T>::increment()
    {
        ++m_index;
    }

    template <class T>
    void empty_iterator<T>::decrement()
    {
        --m_index;
    }

    template <class T>
    void empty_iterator<T>::advance(difference_type n)
    {
        m_index += n;
    }

    template <class T>
    auto empty_iterator<T>::distance_to(const self_type& rhs) const -> difference_type
    {
        return rhs.m_index - m_index;
    }

    template <class T>
    bool empty_iterator<T>::equal(const self_type& rhs) const
    {
        return m_index == rhs.m_index;
    }

    template <class T>
    bool empty_iterator<T>::less_than(const self_type& rhs) const
    {
        return m_index < rhs.m_index;
    }

    template <input_metadata_container METADATA_RANGE>
    arrow_proxy
    null_array::create_proxy(size_t length, std::optional<std::string_view> name, std::optional<METADATA_RANGE> metadata)
    {
        using namespace std::literals;
        static const std::optional<std::unordered_set<sparrow::ArrowFlag>> flags{{ArrowFlag::NULLABLE}};
        ArrowSchema schema = make_arrow_schema(
            "n"sv,
            std::move(name),
            std::move(metadata),
            flags,
            0,
            repeat_view<bool>(false, 0),
            nullptr,
            false
        );

        using buffer_type = sparrow::buffer<std::uint8_t>;
        std::vector<buffer_type> arr_buffs = {};

        ArrowArray arr = make_arrow_array(
            static_cast<int64_t>(length),
            static_cast<int64_t>(length),
            0,
            std::move(arr_buffs),
            nullptr,
            repeat_view<bool>(false, 0),
            nullptr,
            false
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }
}

#if defined(__cpp_lib_format)


template <>
struct std::formatter<sparrow::null_array>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::null_array& ar, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "Null array [{}]", ar.size());
    }
};

namespace sparrow
{
    inline std::ostream& operator<<(std::ostream& os, const null_array& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

#endif
