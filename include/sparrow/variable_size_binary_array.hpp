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

#include <cstdint>
#include <iterator>
#include <limits>
#include <numeric>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/variable_size_binary_iterator.hpp"
#include "sparrow/layout/variable_size_binary_reference.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/utils/extension.hpp"
#include "sparrow/utils/repeat_container.hpp"

namespace sparrow
{
    namespace detail
    {
        template <class T, class OT>
        struct variable_size_binary_format;

        template <>
        struct variable_size_binary_format<std::string, std::int32_t>
        {
            [[nodiscard]] static SPARROW_CONSTEXPR_GCC_11 std::string format() noexcept
            {
                return "u";
            }
        };

        template <>
        struct variable_size_binary_format<std::string, std::int64_t>
        {
            [[nodiscard]] static SPARROW_CONSTEXPR_GCC_11 std::string format() noexcept
            {
                return "U";
            }
        };

        template <>
        struct variable_size_binary_format<std::vector<byte_t>, std::int32_t>
        {
            [[nodiscard]] static SPARROW_CONSTEXPR_GCC_11 std::string format() noexcept
            {
                return "z";
            }
        };

        template <>
        struct variable_size_binary_format<std::vector<byte_t>, std::int64_t>
        {
            [[nodiscard]] static SPARROW_CONSTEXPR_GCC_11 std::string format() noexcept
            {
                return "Z";
            }
        };
    }


    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext = empty_extension>
    class variable_size_binary_array_impl;

    template <layout_offset OT, typename Ext = empty_extension>
    using string_array_impl = variable_size_binary_array_impl<
        arrow_traits<std::string>::value_type,
        arrow_traits<std::string>::const_reference,
        OT,
        Ext>;

    template <layout_offset OT, typename Ext = empty_extension>
    using binary_array_impl = variable_size_binary_array_impl<
        arrow_traits<std::vector<byte_t>>::value_type,
        arrow_traits<std::vector<byte_t>>::const_reference,
        OT,
        Ext>;

    /**
     * @brief Type alias for variable-size string arrays with 32-bit offsets.
     *
     * A variable-size string array implementation for storing UTF-8 strings
     * where the cumulative length of all strings does not exceed 2^31-1 bytes.
     * This is the standard choice for most string datasets.
     *
     * Related Apache Arrow specification:
     * https://arrow.apache.org/docs/dev/format/Columnar.html#variable-size-binary-layout
     *
     * @see big_string_array for larger datasets requiring 64-bit offsets
     */
    using string_array = string_array_impl<std::int32_t>;

    /**
     * @brief Type alias for variable-size string arrays with 64-bit offsets.
     *
     * A variable-size string array implementation for storing UTF-8 strings
     * where the cumulative length of all strings may exceed 2^31-1 bytes.
     * Use this for very large string datasets.
     *
     * Related Apache Arrow specification:
     * https://arrow.apache.org/docs/dev/format/Columnar.html#variable-size-binary-layout
     *
     * @see string_array for smaller datasets with 32-bit offsets
     */
    using big_string_array = string_array_impl<std::int64_t>;

    /**
     * @brief Type alias for variable-size binary arrays with 32-bit offsets.
     *
     * A variable-size binary array implementation for storing arbitrary binary data
     * where the cumulative length does not exceed 2^31-1 bytes.
     * This is the standard choice for most binary datasets.
     *
     * Related Apache Arrow specification:
     * https://arrow.apache.org/docs/dev/format/Columnar.html#variable-size-binary-layout
     *
     * @see big_binary_array for larger datasets requiring 64-bit offsets
     */
    using binary_array = binary_array_impl<std::int32_t>;

    /**
     * @brief Type alias for variable-size binary arrays with 64-bit offsets.
     *
     * A variable-size binary array implementation for storing arbitrary binary data
     * where the cumulative length may exceed 2^31-1 bytes.
     * Use this for very large binary datasets.
     *
     * Related Apache Arrow specification:
     * https://arrow.apache.org/docs/dev/format/Columnar.html#variable-size-binary-layout
     *
     * @see binary_array for smaller datasets with 32-bit offsets
     */
    using big_binary_array = binary_array_impl<std::int64_t>;

    namespace detail
    {
        template <>
        struct get_data_type_from_array<sparrow::string_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get() noexcept
            {
                return sparrow::data_type::STRING;
            }
        };

        template <>
        struct get_data_type_from_array<sparrow::big_string_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get() noexcept
            {
                return sparrow::data_type::LARGE_STRING;
            }
        };

        template <>
        struct get_data_type_from_array<sparrow::binary_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get() noexcept
            {
                return sparrow::data_type::BINARY;
            }
        };

        template <>
        struct get_data_type_from_array<sparrow::big_binary_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get() noexcept
            {
                return sparrow::data_type::LARGE_BINARY;
            }
        };
    }

    /**
     * Checks whether T is a string_array type.
     */
    template <class T>
    constexpr bool is_string_array_v = std::same_as<T, string_array>;

    /**
     * Checks whether T is a big_string_array type.
     */
    template <class T>
    constexpr bool is_big_string_array_v = std::same_as<T, big_string_array>;

    /**
     * Checks whether T is a binary_array type.
     */
    template <class T>
    constexpr bool is_binary_array_v = std::same_as<T, binary_array>;

    /**
     * Checks whether T is a big_binary_array type.
     */
    template <class T>
    constexpr bool is_big_binary_array_v = std::same_as<T, big_binary_array>;

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    struct array_inner_types<variable_size_binary_array_impl<T, CR, OT, Ext>> : array_inner_types_base
    {
        using array_type = variable_size_binary_array_impl<T, CR, OT, Ext>;

        using inner_value_type = T;
        using inner_reference = variable_size_binary_reference<array_type>;
        using inner_const_reference = CR;
        using offset_type = OT;

        using data_value_type = typename T::value_type;

        using offset_iterator = OT*;
        using const_offset_iterator = const OT*;

        using data_iterator = data_value_type*;
        using const_data_iterator = const data_value_type*;

        using iterator_tag = std::random_access_iterator_tag;

        using const_bitmap_iterator = bitmap_type::const_iterator;

        struct iterator_types
        {
            using value_type = inner_value_type;
            using reference = inner_reference;
            using value_iterator = data_iterator;
            using bitmap_iterator = bitmap_type::iterator;
            using iterator_tag = array_inner_types<variable_size_binary_array_impl<T, CR, OT>>::iterator_tag;
        };

        using value_iterator = variable_size_binary_value_iterator<array_type, iterator_types>;

        struct const_iterator_types
        {
            using value_type = inner_value_type;
            using reference = inner_const_reference;
            using value_iterator = const_data_iterator;
            using bitmap_iterator = const_bitmap_iterator;
            using iterator_tag = array_inner_types<variable_size_binary_array_impl<T, CR, OT>>::iterator_tag;
        };

        using const_value_iterator = variable_size_binary_value_iterator<array_type, const_iterator_types>;

        // using iterator = layout_iterator<array_type, false>;
        // using const_iterator = layout_iterator<array_type, true, CR>;
    };

    /**
     * A variable-size binary array.
     * This array is used to store variable-length binary data.
     *
     * Related Apache Arrow description and specification:
     * - https://arrow.apache.org/docs/dev/format/Intro.html#variable-length-binary-and-string
     * - https://arrow.apache.org/docs/dev/format/Columnar.html#variable-size-binary-layout
     */
    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    class variable_size_binary_array_impl final
        : public mutable_array_bitmap_base<variable_size_binary_array_impl<T, CR, OT, Ext>>,
          public Ext
    {
    private:

        static_assert(
            sizeof(std::ranges::range_value_t<T>) == sizeof(std::uint8_t),
            "Only sequences of types with the same size as uint8_t are supported"
        );

    public:

        using self_type = variable_size_binary_array_impl<T, CR, OT, Ext>;
        using base_type = mutable_array_bitmap_base<self_type>;

        using inner_types = array_inner_types<self_type>;
        using inner_value_type = typename inner_types::inner_value_type;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;

        using offset_type = typename inner_types::offset_type;
        using offset_buffer_type = u8_buffer<std::remove_const_t<offset_type>>;
        using char_buffer_type = u8_buffer<char>;
        using byte_buffer_type = u8_buffer<std::byte>;
        using uint8_buffer_type = u8_buffer<std::uint8_t>;

        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_reference = typename base_type::bitmap_reference;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;
        using const_bitmap_range = typename base_type::const_bitmap_range;

        using value_type = nullable<inner_value_type>;
        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;

        using offset_iterator = typename inner_types::offset_iterator;
        using const_offset_iterator = typename inner_types::const_offset_iterator;

        using size_type = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using iterator_tag = typename base_type::iterator_tag;
        using data_iterator = typename inner_types::data_iterator;

        using const_data_iterator = typename inner_types::const_data_iterator;
        using data_value_type = typename inner_types::data_value_type;

        using value_iterator = typename inner_types::value_iterator;
        using const_value_iterator = typename inner_types::const_value_iterator;

        /**
         * @brief Constructs array from Arrow proxy.
         *
         * @param proxy Arrow proxy containing variable-size binary array data and schema
         *
         * @pre proxy must contain valid Arrow variable-size binary array and schema
         * @pre proxy format must match the expected format for T and OT
         * @pre For 32-bit offsets: format must be "u" (string) or "z" (binary)
         * @pre For 64-bit offsets: format must be "U" (large string) or "Z" (large binary)
         * @post Array is initialized with data from proxy
         * @post All elements are accessible via array interface
         *
         * @note Internal assertions verify data type and offset type compatibility:
         *   - SPARROW_ASSERT_TRUE(type == expected_data_type)
         *   - SPARROW_ASSERT_TRUE(offset_type_matches_data_type)
         */
        explicit variable_size_binary_array_impl(arrow_proxy);

        /**
         * @brief Generic constructor for creating array from various inputs.
         *
         * Creates a variable-size binary array from different input combinations.
         * Arguments are forwarded to compatible create_proxy() functions.
         *
         * @tparam ARGS Parameter pack for constructor arguments
         * @param args Constructor arguments (data, offsets, validity, metadata, etc.)
         *
         * @pre Arguments must match one of the create_proxy() overload signatures
         * @pre ARGS must exclude copy and move constructor signatures
         * @post Array is created with the specified data and configuration
         */
        template <class... ARGS>
            requires(mpl::excludes_copy_and_move_ctor_v<variable_size_binary_array_impl<T, CR, OT>, ARGS...>)
        variable_size_binary_array_impl(ARGS&&... args)
            : self_type(create_proxy(std::forward<ARGS>(args)...))
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
         * @post Returns valid reference providing access to element data
         * @post Reference allows modification of the underlying data
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
         * @post Returns valid const reference to element data
         * @post Element data spans from offset[i] to offset[i+1]
         *
         * @note Internal assertions verify index bounds and offset validity:
         *   - SPARROW_ASSERT_TRUE(i < this->size())
         *   - SPARROW_ASSERT_TRUE(offset_begin >= 0)
         *   - SPARROW_ASSERT_TRUE(offset_end >= 0)
         */
        [[nodiscard]] constexpr inner_const_reference value(size_type i) const;

        /**
         * @brief Creates offset buffer from a range of sizes.
         *
         * Converts a range of element sizes into cumulative offsets suitable
         * for the variable-size binary format. The resulting buffer starts
         * with 0 and contains cumulative sums of the input sizes.
         *
         * @tparam SIZES_RANGE Type of the sizes range
         * @param sizes Range of individual element sizes
         * @return Offset buffer with cumulative offsets
         *
         * @pre sizes must be a valid range of size values
         * @post Returned buffer has size() + 1 elements
         * @post First element is 0, subsequent elements are cumulative sums
         * @post Can be used directly for array construction
         */
        template <std::ranges::range SIZES_RANGE>
        [[nodiscard]] static constexpr auto offset_from_sizes(SIZES_RANGE&& sizes) -> offset_buffer_type;

    private:

        /**
         * @brief Creates Arrow proxy from data buffer and offsets with explicit validity.
         *
         * @tparam C Character type for data buffer
         * @tparam VB Validity bitmap input type
         * @tparam METADATA_RANGE Metadata container type
         * @param data_buffer Buffer containing concatenated binary data
         * @param list_offsets Buffer containing cumulative offsets
         * @param validity_input Validity bitmap specification
         * @param name Optional name for the array
         * @param metadata Optional metadata key-value pairs
         * @return Arrow proxy containing the array data and schema
         *
         * @pre C must be char-like type
         * @pre data_buffer must contain valid binary data
         * @pre list_offsets must have size() + 1 elements with valid cumulative offsets
         * @pre validity_input size must match offset buffer size - 1
         * @post Returns valid Arrow proxy with appropriate format
         * @post Schema includes nullable flag when validity bitmap provided
         */
        template <
            mpl::char_like C,
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            u8_buffer<C>&& data_buffer,
            offset_buffer_type&& list_offsets,
            VB&& validity_input = validity_bitmap{validity_bitmap::default_allocator()},
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates Arrow proxy from range of ranges with explicit validity.
         *
         * @tparam R Range type containing ranges of char-like elements
         * @tparam VB Validity bitmap input type
         * @tparam METADATA_RANGE Metadata container type
         * @param values Range of binary/string values to store
         * @param validity_input Validity bitmap specification
         * @param name Optional name for the array
         * @param metadata Optional metadata key-value pairs
         * @return Arrow proxy containing the array data and schema
         *
         * @pre R must be input range of input ranges
         * @pre Inner ranges must contain char-like elements
         * @pre validity_input size must match values size
         * @post Returns valid Arrow proxy with data from values
         * @post Offsets are computed automatically from value sizes
         */
        template <
            std::ranges::input_range R,
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(
                std::ranges::input_range<std::ranges::range_value_t<R>>
                && mpl::char_like<std::ranges::range_value_t<std::ranges::range_value_t<R>>>
            )
        [[nodiscard]] static arrow_proxy create_proxy(
            R&& values,
            VB&& validity_input = validity_bitmap{validity_bitmap::default_allocator()},
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates Arrow proxy from range of ranges with nullable flag.
         *
         * @tparam R Range type containing ranges of char-like elements
         * @tparam METADATA_RANGE Metadata container type
         * @param values Range of binary/string values to store
         * @param nullable Whether the array should support null values
         * @param name Optional name for the array
         * @param metadata Optional metadata key-value pairs
         * @return Arrow proxy containing the array data and schema
         *
         * @pre R must be input range of input ranges
         * @pre Inner ranges must contain char-like elements
         * @post If nullable is true, array supports null values (none initially set)
         * @post If nullable is false, array does not support null values
         * @post Offsets are computed automatically from value sizes
         */
        template <std::ranges::input_range R, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(
                std::ranges::input_range<std::ranges::range_value_t<R>>
                && mpl::char_like<std::ranges::range_value_t<std::ranges::range_value_t<R>>>
            )
        [[nodiscard]] static arrow_proxy create_proxy(
            R&& values,
            bool nullable,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates Arrow proxy from range of nullable values.
         *
         * @tparam R Range type containing nullable<T> elements
         * @tparam METADATA_RANGE Metadata container type
         * @param range Range of nullable values
         * @param name Optional name for the array
         * @param metadata Optional metadata key-value pairs
         * @return Arrow proxy containing the array data and schema
         *
         * @pre R must be input range of nullable<T> elements
         * @post Returns valid Arrow proxy with null/non-null information extracted
         * @post Validity bitmap reflects the has_value() state of each element
         */
        template <std::ranges::input_range R, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::is_same_v<std::ranges::range_value_t<R>, nullable<T>>
        [[nodiscard]] static arrow_proxy create_proxy(
            R&&,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Implementation helper for creating Arrow proxy from components.
         *
         * @tparam C Character type for data buffer
         * @tparam METADATA_RANGE Metadata container type
         * @param data_buffer Buffer containing concatenated binary data
         * @param list_offsets Buffer containing cumulative offsets
         * @param bitmap Optional validity bitmap
         * @param name Optional name for the array
         * @param metadata Optional metadata key-value pairs
         * @return Arrow proxy containing the array data and schema
         *
         * @pre data_buffer and list_offsets must be consistent
         * @pre If bitmap is provided, its size must match offset buffer size - 1
         * @post Returns valid Arrow proxy with appropriate format string
         * @post Schema includes correct flags based on bitmap presence
         */
        template <mpl::char_like C, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy_impl(
            u8_buffer<C>&& data_buffer,
            offset_buffer_type&& list_offsets,
            std::optional<validity_bitmap>&&,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        static constexpr size_t OFFSET_BUFFER_INDEX = 1;
        static constexpr size_t DATA_BUFFER_INDEX = 2;

        /**
         * @brief Gets mutable pointer to offset at specified index.
         *
         * @param i Index of the offset to access
         * @return Mutable pointer to offset value
         *
         * @pre i must be <= size() + offset()
         * @post Returns valid pointer to offset in buffer
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(i <= size() + this->get_arrow_proxy().offset())
         */
        [[nodiscard]] constexpr offset_iterator offset(size_type i);

        /**
         * @brief Gets iterator to beginning of offset buffer.
         *
         * @return Iterator pointing to first offset
         *
         * @post Iterator is valid for offset traversal
         */
        [[nodiscard]] constexpr offset_iterator offsets_begin();

        /**
         * @brief Gets iterator to end of offset buffer.
         *
         * @return Iterator pointing past last offset
         *
         * @post Iterator marks end of offset range
         */
        [[nodiscard]] constexpr offset_iterator offsets_end();

        /**
         * @brief Gets mutable pointer to data at specified byte offset.
         *
         * @param i Byte offset into data buffer
         * @return Mutable pointer to data at offset i
         *
         * @pre i must be within data buffer bounds
         * @post Returns valid pointer to data byte
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(proxy.buffers()[DATA_BUFFER_INDEX].size() >= i)
         */
        [[nodiscard]] constexpr data_iterator data(size_type i);

        /**
         * @brief Gets iterator to beginning of value range.
         *
         * @return Iterator pointing to first value
         *
         * @post Iterator is valid for value traversal
         */
        [[nodiscard]] constexpr value_iterator value_begin();

        /**
         * @brief Gets iterator to end of value range.
         *
         * @return Iterator pointing past last value
         *
         * @post Iterator marks end of value range
         */
        [[nodiscard]] constexpr value_iterator value_end();

        /**
         * @brief Gets const iterator to beginning of value range.
         *
         * @return Const iterator pointing to first value
         *
         * @post Iterator is valid for value traversal
         */
        [[nodiscard]] constexpr const_value_iterator value_cbegin() const;

        /**
         * @brief Gets const iterator to end of value range.
         *
         * @return Const iterator pointing past last value
         *
         * @post Iterator marks end of value range
         */
        [[nodiscard]] constexpr const_value_iterator value_cend() const;

        /**
         * @brief Gets const pointer to offset at specified index.
         *
         * @param i Index of the offset to access
         * @return Const pointer to offset value
         *
         * @pre i must be <= size() + offset()
         * @post Returns valid const pointer to offset
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(i <= this->size() + this->get_arrow_proxy().offset())
         */
        [[nodiscard]] constexpr const_offset_iterator offset(size_type i) const;

        /**
         * @brief Gets const iterator to beginning of offset buffer.
         *
         * @return Const iterator pointing to first offset
         *
         * @post Iterator is valid for offset traversal
         */
        [[nodiscard]] constexpr const_offset_iterator offsets_cbegin() const;

        /**
         * @brief Gets const iterator to end of offset buffer.
         *
         * @return Const iterator pointing past last offset
         *
         * @post Iterator marks end of offset range
         */
        [[nodiscard]] constexpr const_offset_iterator offsets_cend() const;

        /**
         * @brief Gets const pointer to data at specified byte offset.
         *
         * @param i Byte offset into data buffer
         * @return Const pointer to data at offset i
         *
         * @pre i must be within data buffer bounds
         * @post Returns valid const pointer to data byte
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(proxy.buffers()[DATA_BUFFER_INDEX].size() >= i)
         */
        [[nodiscard]] constexpr const_data_iterator data(size_type i) const;

        // Modifiers

        /**
         * @brief Resizes the array to specified length, filling with given value.
         *
         * @tparam U Type of value to fill with (must be convertible to T)
         * @param new_length New size for the array
         * @param value Value to use for new elements (if growing)
         *
         * @pre U must be convertible to T
         * @post Array size equals new_length
         * @post If shrinking, trailing elements are removed
         * @post If growing, new elements are set to value
         */
        template <std::ranges::sized_range U>
            requires mpl::convertible_ranges<U, T>
        constexpr void resize_values(size_type new_length, U value);

        /**
         * @brief Resizes offset buffer to specified length.
         *
         * @param new_length New number of offsets
         * @param offset_value Value to use for new offsets
         *
         * @post Offset buffer has new_length + 1 elements
         * @post New offsets are set to offset_value
         */
        constexpr void resize_offsets(size_type new_length, offset_type offset_value);

        /**
         * @brief Inserts copies of a value at specified position.
         *
         * @tparam U Type of value to insert (must be convertible to T)
         * @param pos Iterator position where to insert
         * @param value Value to insert
         * @param count Number of copies to insert
         * @return Iterator pointing to first inserted element
         *
         * @pre pos must be valid iterator within [value_cbegin(), value_cend()]
         * @pre U must be convertible to T
         * @post count copies of value are inserted at pos
         * @post Array size increases by count
         * @post Offsets are adjusted accordingly
         */
        template <std::ranges::sized_range U>
            requires mpl::convertible_ranges<U, T>
        constexpr value_iterator insert_value(const_value_iterator pos, U value, size_type count);

        /**
         * @brief Inserts offset values at specified position.
         *
         * @param pos Iterator position where to insert
         * @param size Size value for the offsets
         * @param count Number of offsets to insert
         * @return Iterator pointing to first inserted offset
         *
         * @pre pos must be valid offset iterator
         * @post count offsets are inserted with appropriate cumulative values
         * @post Subsequent offsets are adjusted by cumulative size
         */
        constexpr offset_iterator insert_offset(const_offset_iterator pos, offset_type size, size_type count);

        /**
         * @brief Inserts range of values at specified position.
         *
         * @tparam InputIt Iterator type for values (must yield T elements)
         * @param pos Position where to insert
         * @param first Beginning of range to insert
         * @param last End of range to insert
         * @return Iterator pointing to first inserted element
         *
         * @pre InputIt must yield elements of type T
         * @pre pos must be valid value iterator
         * @pre [first, last) must be valid range
         * @post Elements from [first, last) are inserted at pos
         * @post Array size increases by distance(first, last)
         * @post Data buffer and offsets are adjusted accordingly
         */
        template <mpl::iterator_of_type<T> InputIt>
        constexpr value_iterator insert_values(const_value_iterator pos, InputIt first, InputIt last);

        /**
         * @brief Inserts range of offset sizes at specified position.
         *
         * @tparam InputIt Iterator type for offset sizes (must yield OT elements)
         * @param pos Position where to insert offsets
         * @param first_sizes Beginning of size range
         * @param last_sizes End of size range
         * @return Iterator pointing to first inserted offset
         *
         * @pre InputIt must yield elements of type OT
         * @pre pos must be valid offset iterator
         * @pre [first_sizes, last_sizes) must be valid range
         * @post Offsets corresponding to sizes are inserted
         * @post Subsequent offsets are adjusted by cumulative size
         *
         * @note Internal assertions verify iterator bounds:
         *   - SPARROW_ASSERT_TRUE(pos >= offsets_cbegin())
         *   - SPARROW_ASSERT_TRUE(pos <= offsets_cend())
         *   - SPARROW_ASSERT_TRUE(first_sizes <= last_sizes)
         */
        template <mpl::iterator_of_type<OT> InputIt>
        constexpr offset_iterator
        insert_offsets(const_offset_iterator pos, InputIt first_sizes, InputIt last_sizes);

        /**
         * @brief Erases specified number of values starting at position.
         *
         * @param pos Position where to start erasing
         * @param count Number of values to erase
         * @return Iterator pointing to element after last erased
         *
         * @pre pos must be valid value iterator
         * @pre pos + count must not exceed value_cend()
         * @post count values are removed starting at pos
         * @post Array size decreases by count
         * @post Data buffer and offsets are adjusted accordingly
         *
         * @note Internal assertions verify iterator bounds:
         *   - SPARROW_ASSERT_TRUE(pos >= value_cbegin())
         *   - SPARROW_ASSERT_TRUE(pos <= value_cend())
         */
        constexpr value_iterator erase_values(const_value_iterator pos, size_type count);

        /**
         * @brief Erases specified number of offsets starting at position.
         *
         * @param pos Position where to start erasing
         * @param count Number of offsets to erase
         * @return Iterator pointing to offset after last erased
         *
         * @pre pos must be valid offset iterator
         * @pre pos + count must not exceed offsets_cend()
         * @post count offsets are removed starting at pos
         * @post Subsequent offsets are adjusted accordingly
         *
         * @note Internal assertions verify iterator bounds:
         *   - SPARROW_ASSERT_TRUE(pos >= offsets_cbegin())
         *   - SPARROW_ASSERT_TRUE(pos <= offsets_cend())
         */
        constexpr offset_iterator erase_offsets(const_offset_iterator pos, size_type count);

        /**
         * @brief Assigns new value to element at specified index.
         *
         * @tparam U Type of value to assign (must be convertible to T)
         * @param rhs Value to assign
         * @param index Index of element to modify
         *
         * @pre U must be convertible to T
         * @pre index must be < size()
         * @post Element at index contains data from rhs
         * @post Data buffer may be resized if new value has different length
         * @post Offsets are adjusted if value length changes
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(index < size())
         */
        template <std::ranges::sized_range U>
            requires mpl::convertible_ranges<U, T>
        constexpr void assign(U&& rhs, size_type index);

        /**
         * @brief Checks if adding size to current offset would cause overflow.
         *
         * @param current_offset Current offset value
         * @param size_to_add Size that would be added to the offset
         * @throws std::overflow_error if adding size would exceed maximum offset value
         *
         * @note This is a helper method to avoid code duplication in overflow checks
         */
        constexpr void check_offset_overflow(offset_type current_offset, offset_type size_to_add) const;

        friend class variable_size_binary_reference<self_type>;
        friend const_value_iterator;
        friend base_type;
        friend base_type::base_type;
        friend base_type::base_type::base_type;
    };

    /*********************************************
     * variable_size_binary_array_impl implementation *
     *********************************************/

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    variable_size_binary_array_impl<T, CR, OT, Ext>::variable_size_binary_array_impl(arrow_proxy proxy)
        : base_type(std::move(proxy))
    {
        const auto type = this->get_arrow_proxy().data_type();
        SPARROW_ASSERT_TRUE(
            type == data_type::STRING || type == data_type::LARGE_STRING || type == data_type::BINARY
            || type == data_type::LARGE_BINARY
        );
        SPARROW_ASSERT_TRUE(
            (((type == data_type::STRING || type == data_type::BINARY) && std::same_as<OT, int32_t>)
             || ((type == data_type::LARGE_STRING || type == data_type::LARGE_BINARY)
                 && std::same_as<OT, int64_t>) )
        );
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    template <std::ranges::range SIZES_RANGE>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::offset_from_sizes(SIZES_RANGE&& sizes)
        -> offset_buffer_type
    {
        return detail::offset_buffer_from_sizes<std::remove_const_t<offset_type>>(
            std::forward<SIZES_RANGE>(sizes)
        );
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    template <mpl::char_like C, validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
    arrow_proxy variable_size_binary_array_impl<T, CR, OT, Ext>::create_proxy(
        u8_buffer<C>&& data_buffer,
        offset_buffer_type&& offsets,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto size = offsets.size() - 1;
        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<VB>(validity_input));
        const auto null_count = vbitmap.null_count();

        ArrowSchema schema = make_arrow_schema(
            detail::variable_size_binary_format<T, OT>::format(),
            std::move(name),                                                                    // name
            std::move(metadata),                                                                // metadata
            std::make_optional<std::unordered_set<sparrow::ArrowFlag>>({ArrowFlag::NULLABLE}),  // flags,
            nullptr,                                                                            // children
            repeat_view<bool>(true, 0),
            nullptr,  // dictionary
            true

        );
        std::vector<buffer<std::uint8_t>> arr_buffs = {
            std::move(vbitmap).extract_storage(),
            std::move(offsets).extract_storage(),
            std::move(data_buffer).extract_storage()
        };

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(arr_buffs),
            nullptr,  // children
            repeat_view<bool>(true, 0),
            nullptr,  // dictionary
            true
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    template <std::ranges::input_range R, validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
        requires(
            std::ranges::input_range<std::ranges::range_value_t<R>> &&                 // a range of ranges
            mpl::char_like<std::ranges::range_value_t<std::ranges::range_value_t<R>>>  // inner range is a
                                                                                       // range of char-like
        )
    arrow_proxy variable_size_binary_array_impl<T, CR, OT, Ext>::create_proxy(
        R&& values,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        using values_inner_value_type = std::ranges::range_value_t<std::ranges::range_value_t<R>>;

        auto size_range = values
                          | std::views::transform(
                              [](const auto& v)
                              {
                                  return std::ranges::size(v);
                              }
                          );
        auto offset_buffer = offset_from_sizes(size_range);
        auto data_buffer = u8_buffer<values_inner_value_type>(std::ranges::views::join(values));
        return create_proxy(
            std::move(data_buffer),
            std::move(offset_buffer),
            std::forward<VB>(validity_input),
            std::forward<std::optional<std::string_view>>(name),
            std::forward<std::optional<METADATA_RANGE>>(metadata)
        );
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    template <std::ranges::input_range R, input_metadata_container METADATA_RANGE>
        requires std::is_same_v<std::ranges::range_value_t<R>, nullable<T>>
    arrow_proxy variable_size_binary_array_impl<T, CR, OT, Ext>::create_proxy(
        R&& range,
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

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    template <
        std::ranges::input_range R,
        input_metadata_container METADATA_RANGE>
        requires(
            std::ranges::input_range<std::ranges::range_value_t<R>> &&                 // a range of ranges
            mpl::char_like<std::ranges::range_value_t<std::ranges::range_value_t<R>>>  // inner range is a
                                                                                       // range of
                                                                                       // char-like
        )
    [[nodiscard]] arrow_proxy variable_size_binary_array_impl<T, CR, OT, Ext>::create_proxy(
        R&& values,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        using values_inner_value_type = std::ranges::range_value_t<std::ranges::range_value_t<R>>;
        const size_t size = std::ranges::size(values);
        u8_buffer<values_inner_value_type> data_buffer(std::ranges::views::join(values));
        auto size_range = values
                          | std::views::transform(
                              [](const auto& v)
                              {
                                  return std::ranges::size(v);
                              }
                          );
        auto offset_buffer = offset_from_sizes(size_range);
        return create_proxy_impl(
            std::move(data_buffer),
            std::move(offset_buffer),
            nullable ? std::make_optional<validity_bitmap>(nullptr, size, validity_bitmap::default_allocator()) : std::nullopt,
            std::move(name),
            std::move(metadata)
        );
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    template <mpl::char_like C, input_metadata_container METADATA_RANGE>
    [[nodiscard]] arrow_proxy variable_size_binary_array_impl<T, CR, OT, Ext>::create_proxy_impl(
        u8_buffer<C>&& data_buffer,
        offset_buffer_type&& list_offsets,
        std::optional<validity_bitmap>&& bitmap,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto size = list_offsets.size() - 1;
        const auto null_count = bitmap.has_value() ? bitmap->null_count() : 0;

        const std::optional<std::unordered_set<sparrow::ArrowFlag>>
            flags = bitmap.has_value()
                        ? std::make_optional<std::unordered_set<sparrow::ArrowFlag>>({ArrowFlag::NULLABLE})
                        : std::nullopt;

        ArrowSchema schema = make_arrow_schema(
            detail::variable_size_binary_format<T, OT>::format(),
            std::move(name),      // name
            std::move(metadata),  // metadata
            flags,                // flags,
            nullptr,              // children
            repeat_view<bool>(true, 0),
            nullptr,  // dictionary
            true

        );
        std::vector<buffer<std::uint8_t>> arr_buffs = {
            bitmap.has_value() ? std::move(*bitmap).extract_storage() : buffer<std::uint8_t>{nullptr, 0, buffer<std::uint8_t>::default_allocator()},
            std::move(list_offsets).extract_storage(),
            std::move(data_buffer).extract_storage()
        };

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(arr_buffs),
            nullptr,  // children
            repeat_view<bool>(true, 0),
            nullptr,  // dictionary
            true
        );
        arrow_proxy proxy{std::move(arr), std::move(schema)};
        Ext::init(proxy);
        return proxy;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::data(size_type i) -> data_iterator
    {
        arrow_proxy& proxy = get_arrow_proxy();
        SPARROW_ASSERT_TRUE(proxy.buffers()[DATA_BUFFER_INDEX].size() >= i);
        return proxy.buffers()[DATA_BUFFER_INDEX].template data<data_value_type>() + i;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::data(size_type i) const
        -> const_data_iterator
    {
        const arrow_proxy& proxy = this->get_arrow_proxy();
        SPARROW_ASSERT_TRUE(proxy.buffers()[DATA_BUFFER_INDEX].size() >= i);
        return proxy.buffers()[DATA_BUFFER_INDEX].template data<const data_value_type>() + i;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    template <std::ranges::sized_range U>
        requires mpl::convertible_ranges<U, T>
    constexpr void variable_size_binary_array_impl<T, CR, OT, Ext>::assign(U&& rhs, size_type index)
    {
        SPARROW_ASSERT_TRUE(index < size());
        const auto offset_beg = *offset(index);
        const auto offset_end = *offset(index + 1);
        const auto initial_value_length = offset_end - offset_beg;
        const auto new_value_length = static_cast<OT>(std::ranges::size(rhs));
        const OT shift_byte_count = new_value_length - initial_value_length;
        auto& data_buffer = this->get_arrow_proxy().get_array_private_data()->buffers()[DATA_BUFFER_INDEX];
        if (shift_byte_count != 0)
        {
            // Check for offset overflow before adjusting
            if (shift_byte_count > 0)
            {
                const offset_type last_offset = *offset(size());
                check_offset_overflow(last_offset, shift_byte_count);
            }

            const auto shift_val_abs = static_cast<size_t>(std::abs(shift_byte_count));
            const auto new_data_buffer_size = shift_byte_count < 0 ? data_buffer.size() - shift_val_abs
                                                                   : data_buffer.size() + shift_val_abs;

            if (shift_byte_count > 0)
            {
                data_buffer.resize(new_data_buffer_size);
                // Move elements to make space for the new value
                std::move_backward(
                    sparrow::next(data_buffer.begin(), offset_end),
                    sparrow::next(data_buffer.end(), -shift_byte_count),
                    data_buffer.end()
                );
            }
            else
            {
                std::move(
                    sparrow::next(data_buffer.begin(), offset_end),
                    data_buffer.end(),
                    sparrow::next(data_buffer.begin(), offset_end + shift_byte_count)
                );
                data_buffer.resize(new_data_buffer_size);
            }
            // Adjust offsets for subsequent elements
            std::for_each(
                offset(index + 1),
                offset(size() + 1),
                [shift_byte_count](auto& offset)
                {
                    offset += shift_byte_count;
                }
            );
        }
        auto tmp = std::views::transform(
            rhs,
            [](const auto& val)
            {
                return static_cast<std::uint8_t>(val);
            }
        );
        // Copy the new value into the buffer
        std::copy(std::ranges::begin(tmp), std::ranges::end(tmp), sparrow::next(data_buffer.begin(), offset_beg));
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr void variable_size_binary_array_impl<T, CR, OT, Ext>::check_offset_overflow(
        offset_type current_offset,
        offset_type size_to_add
    ) const
    {
        constexpr offset_type max_offset = std::numeric_limits<offset_type>::max();
        if (current_offset > max_offset - size_to_add)
        {
            throw std::overflow_error("Offset overflow: adding elements would exceed maximum offset value");
        }
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::offset(size_type i) -> offset_iterator
    {
        SPARROW_ASSERT_TRUE(i <= size() + this->get_arrow_proxy().offset());
        return get_arrow_proxy().buffers()[OFFSET_BUFFER_INDEX].template data<OT>()
               + static_cast<size_type>(this->get_arrow_proxy().offset()) + i;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::offset(size_type i) const
        -> const_offset_iterator
    {
        SPARROW_ASSERT_TRUE(i <= this->size() + this->get_arrow_proxy().offset());
        return this->get_arrow_proxy().buffers()[OFFSET_BUFFER_INDEX].template data<OT>()
               + static_cast<size_type>(this->get_arrow_proxy().offset()) + i;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::offsets_begin() -> offset_iterator
    {
        return offset(0);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::offsets_cbegin() const
        -> const_offset_iterator
    {
        return offset(0);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::offsets_end() -> offset_iterator
    {
        return offset(size() + 1);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::offsets_cend() const
        -> const_offset_iterator
    {
        return offset(size() + 1);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::value(size_type i) -> inner_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return inner_reference(this, i);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::value(size_type i) const
        -> inner_const_reference
    {
        SPARROW_ASSERT_TRUE(i < this->size());
        const OT offset_begin = *offset(i);
        SPARROW_ASSERT_TRUE(offset_begin >= 0);
        const OT offset_end = *offset(i + 1);
        SPARROW_ASSERT_TRUE(offset_end >= 0);
        const const_data_iterator pointer_begin = data(static_cast<size_t>(offset_begin));
        const const_data_iterator pointer_end = data(static_cast<size_t>(offset_end));
        return inner_const_reference(pointer_begin, pointer_end);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::value_begin() -> value_iterator
    {
        return value_iterator{this, 0};
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::value_end() -> value_iterator
    {
        return sparrow::next(value_begin(), size());
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator{this, 0};
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::value_cend() const -> const_value_iterator
    {
        return sparrow::next(value_cbegin(), this->size());
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    template <std::ranges::sized_range U>
        requires mpl::convertible_ranges<U, T>
    constexpr void variable_size_binary_array_impl<T, CR, OT, Ext>::resize_values(size_type new_length, U value)
    {
        const size_t new_size = new_length + static_cast<size_t>(this->get_arrow_proxy().offset());
        auto& buffers = this->get_arrow_proxy().get_array_private_data()->buffers();
        if (new_length < size())
        {
            const auto offset_begin = static_cast<size_t>(*offset(new_length));
            auto& data_buffer = buffers[DATA_BUFFER_INDEX];
            data_buffer.resize(offset_begin);
            auto& offset_buffer = buffers[OFFSET_BUFFER_INDEX];
            auto offset_buffer_adaptor = make_buffer_adaptor<OT>(offset_buffer);
            offset_buffer_adaptor.resize(new_size + 1);
        }
        else if (new_length > size())
        {
            insert_value(value_cend(), value, new_length - size());
        }
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    template <std::ranges::sized_range U>
        requires mpl::convertible_ranges<U, T>
    constexpr auto
    variable_size_binary_array_impl<T, CR, OT, Ext>::insert_value(const_value_iterator pos, U value, size_type count)
        -> value_iterator
    {
        const auto idx = static_cast<size_t>(std::distance(value_cbegin(), pos));
        const OT offset_begin = *offset(idx);
        const std::vector<uint8_t> casted_value{value.cbegin(), value.cend()};
        const repeat_view<std::vector<uint8_t>> my_repeat_view{casted_value, count};
        const auto joined_repeated_value_range = std::ranges::views::join(my_repeat_view);
        auto& data_buffer = this->get_arrow_proxy().get_array_private_data()->buffers()[DATA_BUFFER_INDEX];
        const auto pos_to_insert = sparrow::next(data_buffer.cbegin(), offset_begin);
        data_buffer.insert(pos_to_insert, joined_repeated_value_range.begin(), joined_repeated_value_range.end());
        insert_offset(offsets_cbegin() + idx + 1, static_cast<offset_type>(value.size()), count);
        return sparrow::next(value_begin(), idx);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::insert_offset(
        const_offset_iterator pos,
        offset_type value_size,
        size_type count
    ) -> offset_iterator
    {
        auto& offset_buffer = get_arrow_proxy().get_array_private_data()->buffers()[OFFSET_BUFFER_INDEX];
        const auto idx = static_cast<size_t>(std::distance(offsets_cbegin(), pos));
        auto offset_buffer_adaptor = make_buffer_adaptor<OT>(offset_buffer);
        const offset_type cumulative_size = value_size * static_cast<offset_type>(count);

        // Check for offset overflow before adjusting
        if (!offset_buffer_adaptor.empty())
        {
            const offset_type last_offset = offset_buffer_adaptor.back();
            check_offset_overflow(last_offset, cumulative_size);
        }

        // Adjust offsets for subsequent elements
        std::for_each(
            sparrow::next(offset_buffer_adaptor.begin(), idx + 1),
            offset_buffer_adaptor.end(),
            [cumulative_size](auto& offset)
            {
                offset += cumulative_size;
            }
        );
        offset_buffer_adaptor.insert(sparrow::next(offset_buffer_adaptor.cbegin(), idx + 1), count, 0);
        // Put the right values in the new offsets
        for (size_t i = idx + 1; i < idx + 1 + count; ++i)
        {
            offset_buffer_adaptor[i] = offset_buffer_adaptor[i - 1] + value_size;
        }
        return offsets_begin() + idx;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    template <mpl::iterator_of_type<T> InputIt>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::insert_values(
        const_value_iterator pos,
        InputIt first,
        InputIt last
    ) -> value_iterator
    {
        auto& data_buffer = get_arrow_proxy().get_array_private_data()->buffers()[DATA_BUFFER_INDEX];
        auto data_buffer_adaptor = make_buffer_adaptor<data_value_type>(data_buffer);
        auto values = std::ranges::subrange(first, last);
        const size_t cumulative_sizes = std::accumulate(
            values.begin(),
            values.end(),
            size_t(0),
            [](size_t acc, const T& value)
            {
                return acc + value.size();
            }
        );
        data_buffer_adaptor.resize(data_buffer_adaptor.size() + cumulative_sizes);
        const auto idx = static_cast<size_t>(std::distance(value_cbegin(), pos));
        const OT offset_begin = *offset(idx);
        auto insert_pos = sparrow::next(data_buffer_adaptor.begin(), offset_begin);

        // Move elements to make space for the new value
        std::move_backward(
            insert_pos,
            sparrow::next(data_buffer_adaptor.end(), -static_cast<difference_type>(cumulative_sizes)),
            data_buffer_adaptor.end()
        );

        for (const T& value : values)
        {
            std::copy(value.begin(), value.end(), insert_pos);
            std::advance(insert_pos, value.size());
        }

        const auto sizes_of_each_value = std::ranges::views::transform(
            values,
            [](const T& value) -> offset_type
            {
                return static_cast<offset_type>(value.size());
            }
        );
        insert_offsets(offset(idx + 1), sizes_of_each_value.begin(), sizes_of_each_value.end());
        return sparrow::next(value_begin(), idx);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    template <mpl::iterator_of_type<OT> InputIt>
    constexpr auto variable_size_binary_array_impl<T, CR, OT, Ext>::insert_offsets(
        const_offset_iterator pos,
        InputIt first_sizes,
        InputIt last_sizes
    ) -> offset_iterator
    {
        SPARROW_ASSERT_TRUE(pos >= offsets_cbegin());
        SPARROW_ASSERT_TRUE(pos <= offsets_cend());
        SPARROW_ASSERT_TRUE(first_sizes <= last_sizes);
        auto& offset_buffer = get_arrow_proxy().get_array_private_data()->buffers()[OFFSET_BUFFER_INDEX];
        auto offset_buffer_adaptor = make_buffer_adaptor<OT>(offset_buffer);
        const auto idx = std::distance(offsets_cbegin(), pos);
        const OT cumulative_sizes = std::reduce(first_sizes, last_sizes, OT(0));

        // Check for offset overflow before adjusting
        if (!offset_buffer_adaptor.empty())
        {
            const offset_type last_offset = offset_buffer_adaptor.back();
            check_offset_overflow(last_offset, cumulative_sizes);
        }

        const auto sizes_count = std::distance(first_sizes, last_sizes);
        offset_buffer_adaptor.resize(offset_buffer_adaptor.size() + static_cast<size_t>(sizes_count));
        // Move the offsets to make space for the new offsets
        std::move_backward(
            offset_buffer_adaptor.begin() + idx,
            offset_buffer_adaptor.end() - sizes_count,
            offset_buffer_adaptor.end()
        );
        // Adjust offsets for subsequent elements
        std::for_each(
            offset_buffer_adaptor.begin() + idx + sizes_count,
            offset_buffer_adaptor.end(),
            [cumulative_sizes](auto& offset)
            {
                offset += cumulative_sizes;
            }
        );
        // Put the right values in the new offsets
        InputIt it = first_sizes;
        for (size_t i = static_cast<size_t>(idx + 1); i < static_cast<size_t>(idx + sizes_count + 1); ++i)
        {
            offset_buffer_adaptor[i] = offset_buffer_adaptor[i - 1] + *it;
            ++it;
        }
        return offset(static_cast<size_t>(idx));
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto
    variable_size_binary_array_impl<T, CR, OT, Ext>::erase_values(const_value_iterator pos, size_type count)
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
        const auto offset_begin = *offset(index);
        const auto offset_end = *offset(index + count);
        const size_t difference = static_cast<size_t>(offset_end - offset_begin);
        // move the values after the erased ones
        std::move(data_buffer.begin() + offset_end, data_buffer.end(), data_buffer.begin() + offset_begin);
        data_buffer.resize(data_buffer.size() - difference);
        // adjust the offsets for the subsequent elements
        erase_offsets(offset(index), count);
        return sparrow::next(value_begin(), index);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT, typename Ext>
    constexpr auto
    variable_size_binary_array_impl<T, CR, OT, Ext>::erase_offsets(const_offset_iterator pos, size_type count)
        -> offset_iterator
    {
        SPARROW_ASSERT_TRUE(pos >= offsets_cbegin());
        SPARROW_ASSERT_TRUE(pos <= offsets_cend());
        const size_t index = static_cast<size_t>(std::distance(offsets_cbegin(), pos));
        if (count == 0)
        {
            return offset(index);
        }
        auto& offset_buffer = get_arrow_proxy().get_array_private_data()->buffers()[OFFSET_BUFFER_INDEX];
        auto offset_buffer_adaptor = make_buffer_adaptor<OT>(offset_buffer);
        const OT offset_start_value = *offset(index);
        const OT offset_end_value = *offset(index + count);
        const OT difference = offset_end_value - offset_start_value;
        // move the offsets after the erased ones
        std::move(
            sparrow::next(offset_buffer_adaptor.begin(), index + count + 1),
            offset_buffer_adaptor.end(),
            sparrow::next(offset_buffer_adaptor.begin(), index + 1)
        );
        offset_buffer_adaptor.resize(offset_buffer_adaptor.size() - count);
        // adjust the offsets for the subsequent elements
        std::for_each(
            sparrow::next(offset_buffer_adaptor.begin(), index + 1),
            offset_buffer_adaptor.end(),
            [difference](OT& offset)
            {
                offset -= difference;
            }
        );
        return offset(index);
    }

}
