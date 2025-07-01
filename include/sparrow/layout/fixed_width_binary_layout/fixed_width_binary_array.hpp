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
#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/fixed_width_binary_layout/fixed_width_binary_array_utils.hpp"
#include "sparrow/layout/fixed_width_binary_layout/fixed_width_binary_reference.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/repeat_container.hpp"

namespace sparrow
{
    template <std::ranges::sized_range T, class CR>
    class fixed_width_binary_array_impl;

    using fixed_width_binary_traits = arrow_traits<std::vector<byte_t>>;

    /**
     * @brief Fixed-width binary array implementation for storing binary data with uniform element sizes.
     *
     * This class implements an Arrow-compatible array for storing fixed-width binary data where all
     * elements have the same byte length. Each element is a sequence of bytes with a predetermined size
     * that is specified at array creation time and encoded in the Arrow schema format.
     *
     * Related Apache Arrow description and specification:
     * https://arrow.apache.org/docs/dev/format/Intro.html#list
     * https://arrow.apache.org/docs/dev/format/Columnar.html#fixed-width-binary-layout
     *
     * @tparam T The container type for binary data (must be a sized range of byte-like elements)
     * @tparam CR The const reference type returned when accessing elements
     *
     * @pre T must be a sized range where each element has sizeof(byte_t)
     * @pre All binary elements in the array must have the same size
     * @post Maintains Arrow Fixed Width Binary format compatibility
     * @post Element size is encoded in the Arrow format string as "w:<size>"
     *
     * @example
     * ```cpp
     * // Create array of 4-byte binary values
     * std::vector<std::vector<uint8_t>> data = {
     *     {0x01, 0x02, 0x03, 0x04},
     *     {0x05, 0x06, 0x07, 0x08}
     * };
     * fixed_width_binary_array arr(data);
     * ```
     */
    using fixed_width_binary_array = fixed_width_binary_array_impl<
        fixed_width_binary_traits::value_type,
        fixed_width_binary_traits::const_reference>;

    template <std::ranges::sized_range T, class CR>
    struct array_inner_types<fixed_width_binary_array_impl<T, CR>> : array_inner_types_base
    {
        using array_type = fixed_width_binary_array_impl<T, CR>;

        using inner_value_type = T;
        using inner_reference = fixed_width_binary_reference<array_type>;
        using inner_const_reference = CR;

        using data_value_type = typename T::value_type;

        using data_iterator = data_value_type*;
        using const_data_iterator = const data_value_type*;

        using iterator_tag = std::random_access_iterator_tag;

        using const_bitmap_iterator = bitmap_type::const_iterator;

        using functor_type = detail::layout_value_functor<array_type, inner_reference>;
        using const_functor_type = detail::layout_value_functor<const array_type, inner_const_reference>;
        using iterator = functor_index_iterator<functor_type>;
        using const_iterator = functor_index_iterator<const_functor_type>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        using value_iterator = iterator;
        using const_value_iterator = const_iterator;
    };

    namespace detail
    {
        template <>
        struct get_data_type_from_array<sparrow::fixed_width_binary_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::FIXED_WIDTH_BINARY;
            }
        };
    }

    template <std::ranges::sized_range T, class CR>
    class fixed_width_binary_array_impl final
        : public mutable_array_bitmap_base<fixed_width_binary_array_impl<T, CR>>
    {
    private:

        static_assert(
            sizeof(std::ranges::range_value_t<T>) == sizeof(byte_t),
            "Only sequences of types with the same size as byte_t are supported"
        );

    public:

        using self_type = fixed_width_binary_array_impl<T, CR>;
        using base_type = mutable_array_bitmap_base<self_type>;

        using inner_types = array_inner_types<self_type>;
        using inner_value_type = typename inner_types::inner_value_type;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;

        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_reference = typename base_type::bitmap_reference;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;
        using const_bitmap_range = typename base_type::const_bitmap_range;

        using value_type = nullable<inner_value_type>;
        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;

        using size_type = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using iterator_tag = typename base_type::iterator_tag;
        using data_iterator = typename inner_types::data_iterator;

        using const_data_iterator = typename inner_types::const_data_iterator;
        using data_value_type = typename inner_types::data_value_type;

        using value_iterator = typename inner_types::value_iterator;
        using const_value_iterator = typename inner_types::const_value_iterator;

        using functor_type = typename inner_types::functor_type;
        using const_functor_type = typename inner_types::const_functor_type;

        /**
         * @brief Constructs fixed-width binary array from Arrow proxy.
         *
         * @param proxy Arrow proxy containing fixed-width binary array data and schema
         *
         * @pre proxy must contain valid Arrow Fixed Width Binary array and schema
         * @pre proxy format must be "w:[size]" where [size] is the element byte size
         * @pre proxy data type must be FIXED_WIDTH_BINARY
         * @post Array is initialized with data from proxy
         * @post Element size is extracted from format string
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(proxy.data_type() == data_type::FIXED_WIDTH_BINARY)
         */
        explicit fixed_width_binary_array_impl(arrow_proxy);

        /**
         * @brief Generic constructor for creating fixed-width binary array.
         *
         * Creates a fixed-width binary array from various input types. The arguments are
         * forwarded to compatible create_proxy() functions based on their types.
         *
         * @tparam ARGS Parameter pack for constructor arguments
         * @param args Constructor arguments (data, element_size, validity, metadata, etc.)
         *
         * @pre Arguments must match one of the create_proxy() overload signatures
         * @pre If providing data ranges, all elements must have the same size
         * @pre Element size must be > 0 for non-empty arrays
         * @post Array is created with the specified data and configuration
         * @post Element size is properly set and encoded in format string
         */
        template <class... ARGS>
            requires(mpl::excludes_copy_and_move_ctor_v<fixed_width_binary_array_impl<T, CR>, ARGS...>)
        fixed_width_binary_array_impl(ARGS&&... args)
            : base_type(create_proxy(std::forward<ARGS>(args)...))
            , m_element_size(num_bytes_for_fixed_sized_binary(this->get_arrow_proxy().format()))
        {
        }

        using base_type::get_arrow_proxy;
        using base_type::size;

        /**
         * @brief Gets mutable reference to element at specified index.
         *
         * @param i Index of the element to access
         * @return Mutable reference to the binary element
         *
         * @pre i must be < size()
         * @post Returns valid reference that can modify the element
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(i < size())
         */
        [[nodiscard]] constexpr inner_reference value(size_type i);

        /**
         * @brief Gets const reference to element at specified index.
         *
         * @param i Index of the element to access
         * @return Const reference to the binary element
         *
         * @pre i must be < size()
         * @post Returns valid const reference to the element data
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(i < size())
         */
        [[nodiscard]] constexpr inner_const_reference value(size_type i) const;

    private:

        /**
         * @brief Creates Arrow proxy from data buffer with element specifications.
         *
         * @tparam C Character-like type for the buffer data
         * @tparam VB Type of validity bitmap input
         * @tparam METADATA_RANGE Type of metadata container
         * @param data_buffer Buffer containing the binary data
         * @param element_count Number of elements in the array
         * @param element_size Size in bytes of each element
         * @param validity_input Validity bitmap specification
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the fixed-width binary array data and schema
         *
         * @pre C must be char-like (sizeof(C) == sizeof(byte_t))
         * @pre element_count * element_size must equal data_buffer.size()
         * @pre element_size must be > 0 if element_count > 0
         * @pre validity_input size must match element_count
         * @post Returns valid Arrow proxy with Fixed Width Binary format
         * @post Format string is "w:<element_size>"
         */
        template <
            mpl::char_like C,
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            u8_buffer<C>&& data_buffer,
            size_t element_count,
            size_t element_size,
            VB&& validity_input = validity_bitmap{},
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates empty Arrow proxy with specified element size.
         *
         * @tparam METADATA_RANGE Type of metadata container
         * @param element_size Size in bytes of each element
         * @param nullable Whether the array should support null values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing empty fixed-width binary array
         *
         * @pre element_size must be >= 0
         * @post Returns valid Arrow proxy with no elements
         * @post If nullable is true, array supports null values
         * @post Format string is "w:<element_size>"
         */
        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            size_t element_size,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates Arrow proxy from range of binary value ranges with validity bitmap.
         *
         * @tparam VALUES Type of input range containing binary value ranges
         * @tparam VB Type of validity bitmap input
         * @tparam METADATA_RANGE Type of metadata container
         * @param values Range of ranges containing binary data (all must have same size)
         * @param validity_input Validity bitmap specification
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the fixed-width binary array data and schema
         *
         * @pre VALUES must be a range of ranges of char-like elements
         * @pre All inner ranges must have the same size
         * @pre validity_input size must match values.size()
         * @post Returns valid Arrow proxy with Fixed Width Binary format
         * @post Element size is determined from first non-empty element
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(all_same_size(values))
         */
        template <
            std::ranges::input_range VALUES,
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(std::ranges::input_range<std::ranges::range_value_t<VALUES>> && mpl::char_like<std::ranges::range_value_t<std::ranges::range_value_t<VALUES>>>)
        [[nodiscard]] static arrow_proxy create_proxy(
            VALUES&& values,
            VB&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates Arrow proxy from range of binary value ranges with nullable flag.
         *
         * @tparam VALUES Type of input range containing binary value ranges
         * @tparam METADATA_RANGE Type of metadata container
         * @param values Range of ranges containing binary data (all must have same size)
         * @param nullable Whether the array should support null values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the fixed-width binary array data and schema
         *
         * @pre VALUES must be a range of ranges of char-like elements
         * @pre All inner ranges must have the same size
         * @post If nullable is true, array supports null values (though none initially set)
         * @post If nullable is false, array does not support null values
         * @post Element size is determined from first non-empty element
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(all_same_size(values))
         */
        template <std::ranges::input_range VALUES, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(std::ranges::input_range<std::ranges::range_value_t<VALUES>> && mpl::char_like<std::ranges::range_value_t<std::ranges::range_value_t<VALUES>>>)
        [[nodiscard]] static arrow_proxy create_proxy(
            VALUES&& values,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates Arrow proxy from range of nullable binary values.
         *
         * @tparam NULLABLE_VALUES Type of input range containing nullable binary values
         * @tparam METADATA_RANGE Type of metadata container
         * @param range Range of nullable binary values (inner values must have same size)
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the fixed-width binary array data and schema
         *
         * @pre NULLABLE_VALUES must be a range of nullable<binary_range>
         * @pre All non-null inner ranges must have the same size
         * @pre Inner ranges must contain byte_t elements
         * @post Returns valid Arrow proxy with Fixed Width Binary format
         * @post Validity bitmap reflects has_value() status of nullable elements
         * @post Array supports null values (nullable = true)
         */
        template <std::ranges::input_range NULLABLE_VALUES, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires mpl::is_type_instance_of_v<std::ranges::range_value_t<NULLABLE_VALUES>, nullable>
                     && std::ranges::input_range<typename std::ranges::range_value_t<NULLABLE_VALUES>::value_type>
                     && std::is_same_v<
                         std::ranges::range_value_t<typename std::ranges::range_value_t<NULLABLE_VALUES>::value_type>,
                         byte_t>
        [[nodiscard]] static arrow_proxy create_proxy(
            NULLABLE_VALUES&&,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Implementation helper for creating Arrow proxy from components.
         *
         * @tparam C Character-like type for the buffer data
         * @tparam METADATA_RANGE Type of metadata container
         * @param data_buffer Buffer containing the binary data
         * @param element_count Number of elements in the array
         * @param element_size Size in bytes of each element
         * @param validity_input Optional validity bitmap
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the fixed-width binary array data and schema
         *
         * @pre element_size == 0 implies data_buffer.size() == 0
         * @pre element_size > 0 implies data_buffer.size() % element_size == 0
         * @pre element_count * element_size == data_buffer.size()
         * @post Returns valid Arrow proxy with Fixed Width Binary format
         * @post Format string is "w:<element_size>"
         * @post Null count matches validity bitmap null count (if present)
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(element_size == 0 ? (data_buffer.size() == 0) :
         * (data_buffer.size() % element_size == 0))
         */
        template <mpl::char_like C, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy_impl(
            u8_buffer<C>&& data_buffer,
            size_t element_count,
            size_t element_size,
            std::optional<validity_bitmap>&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        static constexpr size_t DATA_BUFFER_INDEX = 1;

        /**
         * @brief Gets mutable pointer to data at byte offset.
         *
         * @param i Byte offset into the data buffer
         * @return Mutable pointer to data at the specified offset
         *
         * @pre i must be within buffer bounds considering array offset
         * @post Returns valid pointer to mutable data
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(data_buffer_size >= index_offset)
         */
        [[nodiscard]] constexpr data_iterator data(size_type i);

        /**
         * @brief Gets iterator to beginning of value range.
         *
         * @return Iterator pointing to the first element
         *
         * @post Returns valid iterator to array beginning
         */
        [[nodiscard]] constexpr value_iterator value_begin();

        /**
         * @brief Gets iterator to end of value range.
         *
         * @return Iterator pointing past the last element
         *
         * @post Returns valid iterator to array end
         */
        [[nodiscard]] constexpr value_iterator value_end();

        /**
         * @brief Gets const iterator to beginning of value range.
         *
         * @return Const iterator pointing to the first element
         *
         * @post Returns valid const iterator to array beginning
         */
        [[nodiscard]] constexpr const_value_iterator value_cbegin() const;

        /**
         * @brief Gets const iterator to end of value range.
         *
         * @return Const iterator pointing past the last element
         *
         * @post Returns valid const iterator to array end
         */
        [[nodiscard]] constexpr const_value_iterator value_cend() const;

        /**
         * @brief Gets const pointer to data at byte offset.
         *
         * @param i Byte offset into the data buffer
         * @return Const pointer to data at the specified offset
         *
         * @pre i must be within buffer bounds considering array offset
         * @post Returns valid pointer to const data
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(data_buffer_size >= index_offset)
         */
        [[nodiscard]] constexpr const_data_iterator data(size_type i) const;

        // Modifiers

        /**
         * @brief Resizes the array to specified length, filling with given value.
         *
         * @tparam U Type of fill value (must be convertible to T)
         * @param new_length New size for the array
         * @param value Value to use for new elements (if growing)
         *
         * @pre U must be convertible to T and sized range
         * @pre value.size() must equal element_size
         * @pre new_length must be valid array size
         * @post Array size is exactly new_length
         * @post If growing, new elements are copies of value
         * @post If shrinking, excess elements are removed
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(m_element_size == value.size())
         */
        template <std::ranges::sized_range U>
            requires mpl::convertible_ranges<U, T>
        constexpr void resize_values(size_type new_length, U value);

        /**
         * @brief Inserts copies of a value at specified position.
         *
         * @tparam U Type of value to insert (must be convertible to T)
         * @param pos Iterator position where to insert
         * @param value Value to insert multiple times
         * @param count Number of copies to insert
         * @return Iterator pointing to first inserted element
         *
         * @pre U must be convertible to T and sized range
         * @pre pos must be valid iterator within [value_cbegin(), value_cend()]
         * @pre value.size() must equal element_size
         * @pre count must be >= 0
         * @post count copies of value are inserted at pos
         * @post Array size increases by count
         * @post Returns iterator to first inserted element
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(m_element_size == value.size())
         */
        template <std::ranges::sized_range U>
            requires mpl::convertible_ranges<U, T>
        constexpr value_iterator insert_value(const_value_iterator pos, U value, size_type count);

        /**
         * @brief Inserts range of values at specified position.
         *
         * @tparam InputIt Input iterator type
         * @param pos Iterator position where to insert
         * @param first Iterator to beginning of range to insert
         * @param last Iterator to end of range to insert
         * @return Iterator pointing to first inserted element
         *
         * @pre InputIt must be input iterator with value_type convertible to T
         * @pre pos must be valid iterator within [value_cbegin(), value_cend()]
         * @pre [first, last) must be valid range
         * @pre All values in [first, last) must have the same size
         * @pre All values must have size equal to element_size
         * @post All values in [first, last) are inserted at pos
         * @post Array size increases by distance(first, last)
         * @post Returns iterator to first inserted element
         *
         * @note Internal assertions:
         *   - SPARROW_ASSERT_TRUE(value_cbegin() <= pos)
         *   - SPARROW_ASSERT_TRUE(pos <= value_cend())
         *   - SPARROW_ASSERT_TRUE(first <= last)
         *   - SPARROW_ASSERT_TRUE(all_same_size(range))
         *   - SPARROW_ASSERT_TRUE(m_element_size == std::ranges::size(*first))
         */
        template <typename InputIt>
            requires std::input_iterator<InputIt>
                     && mpl::convertible_ranges<typename std::iterator_traits<InputIt>::value_type, T>
        constexpr value_iterator insert_values(const_value_iterator pos, InputIt first, InputIt last);

        /**
         * @brief Erases elements starting at specified position.
         *
         * @param pos Iterator position where to start erasing
         * @param count Number of elements to erase
         * @return Iterator pointing to element after the erased range
         *
         * @pre pos must be valid iterator within [value_cbegin(), value_cend()]
         * @pre count must be >= 0
         * @pre pos + count must not exceed value_cend()
         * @post count elements starting at pos are removed
         * @post Array size decreases by count
         * @post Returns iterator to element after erased range
         *
         * @note Internal assertions:
         *   - SPARROW_ASSERT_TRUE(pos >= value_cbegin())
         *   - SPARROW_ASSERT_TRUE(pos <= value_cend())
         */
        constexpr value_iterator erase_values(const_value_iterator pos, size_type count);

        /**
         * @brief Assigns new value to element at specified index.
         *
         * @tparam U Type of new value (must be convertible to T)
         * @param rhs New value to assign
         * @param index Index of element to modify
         *
         * @pre U must be convertible to T and sized range
         * @pre rhs.size() must equal element_size
         * @pre index must be < size()
         * @post Element at index is replaced with copy of rhs
         *
         * @note Internal assertions:
         *   - SPARROW_ASSERT_TRUE(std::ranges::size(rhs) == m_element_size)
         *   - SPARROW_ASSERT_TRUE(index < size())
         */
        template <std::ranges::sized_range U>
            requires mpl::convertible_ranges<U, T>
        constexpr void assign(U&& rhs, size_type index);

        size_t m_element_size = 0;

        friend class fixed_width_binary_reference<self_type>;
        friend const_value_iterator;
        friend base_type;
        friend base_type::base_type;
        friend base_type::base_type::base_type;
    };

    /************************************************
     * fixed_width_binary_array_impl implementation *
     ************************************************/

    template <std::ranges::sized_range T, class CR>
    fixed_width_binary_array_impl<T, CR>::fixed_width_binary_array_impl(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , m_element_size(num_bytes_for_fixed_sized_binary(this->get_arrow_proxy().format()))
    {
        SPARROW_ASSERT_TRUE(this->get_arrow_proxy().data_type() == data_type::FIXED_WIDTH_BINARY);
    }

    template <std::ranges::sized_range T, class CR>
    template <mpl::char_like C, validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
    arrow_proxy fixed_width_binary_array_impl<T, CR>::create_proxy(
        u8_buffer<C>&& data_buffer,
        size_t element_count,
        size_t element_size,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        validity_bitmap bitmap = ensure_validity_bitmap(element_count, std::forward<VB>(validity_input));
        return create_proxy_impl(
            std::move(data_buffer),
            element_count,
            element_size,
            std::move(bitmap),
            std::move(name),
            std::move(metadata)
        );
    }

    template <std::ranges::sized_range T, class CR>
    template <input_metadata_container METADATA_RANGE>
    arrow_proxy fixed_width_binary_array_impl<T, CR>::create_proxy(
        size_t element_size,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        u8_buffer<char> data_buffer{};
        std::optional<validity_bitmap> bitmap = nullable ? std::make_optional<validity_bitmap>(nullptr, 0)
                                                         : std::nullopt;
        return create_proxy_impl(
            std::move(data_buffer),
            0,
            element_size,
            std::move(bitmap),
            std::move(name),
            std::move(metadata)
        );
    }

    template <std::ranges::sized_range T, class CR>
    template <std::ranges::input_range R, validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
        requires(
            std::ranges::input_range<std::ranges::range_value_t<R>> &&                 // a range of ranges
            mpl::char_like<std::ranges::range_value_t<std::ranges::range_value_t<R>>>  // inner range is a
                                                                                       // range of char-like
        )
    arrow_proxy fixed_width_binary_array_impl<T, CR>::create_proxy(
        R&& values,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        using values_type = std::ranges::range_value_t<R>;
        using values_inner_value_type = std::ranges::range_value_t<values_type>;

        SPARROW_ASSERT_TRUE(all_same_size(values));
        const size_t element_size = std::ranges::empty(values) ? 0 : std::ranges::size(*values.begin());

        auto data_buffer = u8_buffer<values_inner_value_type>(std::ranges::views::join(values));
        return create_proxy(
            std::move(data_buffer),
            values.size(),
            element_size,
            std::forward<VB>(validity_input),
            std::forward<std::optional<std::string_view>>(name),
            std::forward<std::optional<METADATA_RANGE>>(metadata)
        );
    }

    template <std::ranges::sized_range T, class CR>
    template <std::ranges::input_range R, input_metadata_container METADATA_RANGE>
        requires(
            std::ranges::input_range<std::ranges::range_value_t<R>> &&                 // a range of ranges
            mpl::char_like<std::ranges::range_value_t<std::ranges::range_value_t<R>>>  // inner range is a
                                                                                       // range of char-like
        )
    arrow_proxy fixed_width_binary_array_impl<T, CR>::create_proxy(
        R&& values,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        if (nullable)
        {
            return create_proxy(std::forward<R>(values), validity_bitmap{}, std::move(name), std::move(metadata));
        }
        else
        {
            using values_type = std::ranges::range_value_t<R>;
            using values_inner_value_type = std::ranges::range_value_t<values_type>;

            SPARROW_ASSERT_TRUE(all_same_size(values));
            const size_t element_size = std::ranges::empty(values) ? 0 : std::ranges::size(*values.begin());
            auto data_buffer = u8_buffer<values_inner_value_type>(std::ranges::views::join(values));
            return create_proxy_impl(
                std::move(data_buffer),
                values.size(),  // element count
                element_size,
                std::nullopt,  // validity bitmap
                std::move(name),
                std::move(metadata)
            );
        }
    }

    template <std::ranges::sized_range T, class CR>
    template <std::ranges::input_range NULLABLE_RANGE, input_metadata_container METADATA_RANGE>
        requires mpl::is_type_instance_of_v<std::ranges::range_value_t<NULLABLE_RANGE>, nullable>
                 && std::ranges::input_range<typename std::ranges::range_value_t<NULLABLE_RANGE>::value_type>
                 && std::is_same_v<
                     std::ranges::range_value_t<typename std::ranges::range_value_t<NULLABLE_RANGE>::value_type>,
                     byte_t>
    arrow_proxy fixed_width_binary_array_impl<T, CR>::create_proxy(
        NULLABLE_RANGE&& range,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        // split into values and is_non_null ranges
        const auto values = range
                            | std::views::transform(
                                [](const auto& v)
                                {
                                    return v.get();
                                }
                            );
        const auto is_non_null = range
                                 | std::views::transform(
                                     [](const auto& v)
                                     {
                                         return v.has_value();
                                     }
                                 );
        return self_type::create_proxy(values, is_non_null, std::move(name), std::move(metadata));
    }

    template <std::ranges::sized_range T, class CR>
    template <mpl::char_like C, input_metadata_container METADATA_RANGE>
    arrow_proxy fixed_width_binary_array_impl<T, CR>::create_proxy_impl(
        u8_buffer<C>&& data_buffer,
        size_t element_count,
        size_t element_size,
        std::optional<validity_bitmap>&& bitmap,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        SPARROW_ASSERT_TRUE(
            element_size == 0 ? (data_buffer.size() == 0) : (data_buffer.size() % element_size == 0)
        );

        const auto null_count = bitmap.has_value() ? bitmap->null_count() : 0;
        std::string format_str = "w:" + std::to_string(element_size);
        const std::optional<std::unordered_set<ArrowFlag>>
            flags = bitmap.has_value()
                        ? std::make_optional<std::unordered_set<ArrowFlag>>({ArrowFlag::NULLABLE})
                        : std::nullopt;

        ArrowSchema schema = make_arrow_schema(
            std::move(format_str),
            std::move(name),             // name
            std::move(metadata),         // metadata
            flags,                       // flags,
            nullptr,                     // children
            repeat_view<bool>(true, 0),  // children_ownership
            nullptr,                     // dictionary
            true                         // dictionary ownership

        );
        std::vector<buffer<std::uint8_t>> arr_buffs = {
            bitmap.has_value() ? std::move(*bitmap).extract_storage() : buffer<std::uint8_t>{nullptr, 0},
            std::move(data_buffer).extract_storage()
        };

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(element_count),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(arr_buffs),
            nullptr,                     // children
            repeat_view<bool>(true, 0),  // children_ownership
            nullptr,                     // dictionary
            true                         // dictionary ownership
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <std::ranges::sized_range T, class CR>
    constexpr auto fixed_width_binary_array_impl<T, CR>::data(size_type i) -> data_iterator
    {
        const arrow_proxy& proxy = this->get_arrow_proxy();
        auto data_buffer = proxy.buffers()[DATA_BUFFER_INDEX];
        const size_t data_buffer_size = data_buffer.size();
        const size_type index_offset = (static_cast<size_type>(proxy.offset()) * m_element_size) + i;
        SPARROW_ASSERT_TRUE(data_buffer_size >= index_offset);
        return data_buffer.template data<data_value_type>() + index_offset;
    }

    template <std::ranges::sized_range T, class CR>
    constexpr auto fixed_width_binary_array_impl<T, CR>::data(size_type i) const -> const_data_iterator
    {
        const arrow_proxy& proxy = this->get_arrow_proxy();
        const auto data_buffer = proxy.buffers()[DATA_BUFFER_INDEX];
        const size_t data_buffer_size = data_buffer.size();
        const size_type index_offset = (static_cast<size_type>(proxy.offset()) * m_element_size) + i;
        SPARROW_ASSERT_TRUE(data_buffer_size >= index_offset);
        return data_buffer.template data<const data_value_type>() + index_offset;
    }

    template <std::ranges::sized_range T, class CR>
    template <std::ranges::sized_range U>
        requires mpl::convertible_ranges<U, T>
    constexpr void fixed_width_binary_array_impl<T, CR>::assign(U&& rhs, size_type index)
    {
        SPARROW_ASSERT_TRUE(std::ranges::size(rhs) == m_element_size);
        SPARROW_ASSERT_TRUE(index < size());
        std::copy(std::ranges::begin(rhs), std::ranges::end(rhs), data(index * m_element_size));
    }

    template <std::ranges::sized_range T, class CR>
    constexpr auto fixed_width_binary_array_impl<T, CR>::value(size_type i) -> inner_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return inner_reference(this, i);
    }

    template <std::ranges::sized_range T, class CR>
    constexpr auto fixed_width_binary_array_impl<T, CR>::value(size_type i) const -> inner_const_reference
    {
        SPARROW_ASSERT_TRUE(i < this->size());
        const auto offset_begin = i * m_element_size;
        const auto offset_end = offset_begin + m_element_size;
        const const_data_iterator pointer_begin = data(static_cast<size_type>(offset_begin));
        const const_data_iterator pointer_end = data(static_cast<size_type>(offset_end));
        return inner_const_reference(pointer_begin, pointer_end);
    }

    template <std::ranges::sized_range T, class CR>
    constexpr auto fixed_width_binary_array_impl<T, CR>::value_begin() -> value_iterator
    {
        return value_iterator{functor_type{&(this->derived_cast())}, 0};
    }

    template <std::ranges::sized_range T, class CR>
    constexpr auto fixed_width_binary_array_impl<T, CR>::value_end() -> value_iterator
    {
        return sparrow::next(value_begin(), size());
    }

    template <std::ranges::sized_range T, class CR>
    constexpr auto fixed_width_binary_array_impl<T, CR>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator{const_functor_type{&(this->derived_cast())}, 0};
    }

    template <std::ranges::sized_range T, class CR>
    constexpr auto fixed_width_binary_array_impl<T, CR>::value_cend() const -> const_value_iterator
    {
        return sparrow::next(value_cbegin(), this->size());
    }

    template <std::ranges::sized_range T, class CR>
    template <std::ranges::sized_range U>
        requires mpl::convertible_ranges<U, T>
    constexpr void fixed_width_binary_array_impl<T, CR>::resize_values(size_type new_length, U value)
    {
        SPARROW_ASSERT_TRUE(m_element_size == value.size());
        if (new_length < size())
        {
            arrow_proxy& proxy = this->get_arrow_proxy();
            const size_t new_size = new_length + static_cast<size_t>(proxy.offset());
            const auto offset = new_size * m_element_size;
            auto& data_buffer = proxy.get_array_private_data()->buffers()[DATA_BUFFER_INDEX];
            data_buffer.resize(offset);
        }
        else if (new_length > size())
        {
            insert_value(value_cend(), value, new_length - size());
        }
    }

    template <std::ranges::sized_range T, class CR>
    template <std::ranges::sized_range U>
        requires mpl::convertible_ranges<U, T>
    constexpr auto
    fixed_width_binary_array_impl<T, CR>::insert_value(const_value_iterator pos, U value, size_type count)
        -> value_iterator
    {
        SPARROW_ASSERT_TRUE(m_element_size == value.size());
        const auto idx = static_cast<size_t>(std::distance(value_cbegin(), pos));

        const uint8_t* uint8_ptr = reinterpret_cast<const uint8_t*>(value.data());
        const std::vector<uint8_t> casted_value(uint8_ptr, uint8_ptr + value.size());
        const repeat_view<std::vector<uint8_t>> my_repeat_view{casted_value, count};
        const auto joined_repeated_value_range = std::ranges::views::join(my_repeat_view);
        arrow_proxy& proxy = this->get_arrow_proxy();
        auto& data_buffer = proxy.get_array_private_data()->buffers()[DATA_BUFFER_INDEX];
        const auto offset_begin = (idx + proxy.offset()) * m_element_size;
        const auto pos_to_insert = sparrow::next(data_buffer.cbegin(), offset_begin);
        data_buffer.insert(pos_to_insert, joined_repeated_value_range.begin(), joined_repeated_value_range.end());
        return sparrow::next(value_begin(), idx);
    }

    template <std::ranges::sized_range T, class CR>
    template <typename InputIt>
        requires std::input_iterator<InputIt>
                 && mpl::convertible_ranges<typename std::iterator_traits<InputIt>::value_type, T>
    constexpr auto
    fixed_width_binary_array_impl<T, CR>::insert_values(const_value_iterator pos, InputIt first, InputIt last)
        -> value_iterator
    {
        SPARROW_ASSERT_TRUE(value_cbegin() <= pos)
        SPARROW_ASSERT_TRUE(pos <= value_cend());
        SPARROW_ASSERT_TRUE(first <= last);
        SPARROW_ASSERT_TRUE(all_same_size(std::ranges::subrange(first, last)));
        SPARROW_ASSERT_TRUE(m_element_size == std::ranges::size(*first));

        auto values = std::ranges::subrange(first, last);
        const size_t cumulative_sizes = values.size() * m_element_size;
        auto& data_buffer = get_arrow_proxy().get_array_private_data()->buffers()[DATA_BUFFER_INDEX];
        data_buffer.resize(data_buffer.size() + cumulative_sizes);
        const auto idx = static_cast<size_t>(std::distance(value_cbegin(), pos));
        sequence_view<byte_t> casted_values{reinterpret_cast<byte_t*>(data_buffer.data()), data_buffer.size()};
        const auto offset_begin = m_element_size * (idx + get_arrow_proxy().offset());
        auto insert_pos = sparrow::next(casted_values.begin(), offset_begin);

        // Move elements to make space for the new value
        std::move_backward(
            insert_pos,
            sparrow::next(casted_values.end(), -static_cast<difference_type>(cumulative_sizes)),
            casted_values.end()
        );

        for (const auto& val : values)
        {
            std::copy(val.begin(), val.end(), insert_pos);
            std::advance(insert_pos, m_element_size);
        }
        return sparrow::next(value_begin(), idx);
    }

    template <std::ranges::sized_range T, class CR>
    constexpr auto fixed_width_binary_array_impl<T, CR>::erase_values(const_value_iterator pos, size_type count)
        -> value_iterator
    {
        SPARROW_ASSERT_TRUE(pos >= value_cbegin());
        SPARROW_ASSERT_TRUE(pos <= value_cend());
        const size_t index = static_cast<size_t>(std::distance(value_cbegin(), pos));
        if (count == 0)
        {
            return sparrow::next(value_begin(), index);
        }
        auto& data_buffer = get_arrow_proxy().get_array_private_data()->buffers()[DATA_BUFFER_INDEX];
        const size_type byte_count = m_element_size * count;
        const auto offset_begin = m_element_size * (index + static_cast<size_type>(get_arrow_proxy().offset()));
        const auto offset_end = offset_begin + byte_count;
        // move the values after the erased ones
        std::move(
            data_buffer.begin() + static_cast<difference_type>(offset_end),
            data_buffer.end(),
            data_buffer.begin() + static_cast<difference_type>(offset_begin)
        );
        data_buffer.resize(data_buffer.size() - byte_count);
        return sparrow::next(value_begin(), index);
    }
}
