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

#include <cstddef>
#include <ranges>
#include <unordered_map>

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/u8_buffer.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/utils/ranges.hpp"
#include "sparrow/utils/repeat_container.hpp"
#include "sparrow/utils/sequence_view.hpp"

namespace sparrow
{
    template <std::ranges::sized_range T, class CR>
    class variable_size_binary_view_array_impl;

    /**
     * A variable-size string view layout implementation.
     * Related Apache Arrow specification:
     * https://arrow.apache.org/docs/dev/format/Columnar.html#variable-size-binary-view-layout
     * @see binary_view_array
     * @see variable_size_binary_view_array_impl
     */
    using string_view_array = variable_size_binary_view_array_impl<
        arrow_traits<std::string>::value_type,
        arrow_traits<std::string>::const_reference>;

    /**
     * A variable-size binary view layout implementation.
     * Related Apache Arrow specification:
     * https://arrow.apache.org/docs/dev/format/Columnar.html#variable-size-binary-view-layout
     * @see string_view_array
     * @see variable_size_binary_view_array_impl
     */
    using binary_view_array = variable_size_binary_view_array_impl<
        arrow_traits<std::vector<byte_t>>::value_type,
        arrow_traits<std::vector<byte_t>>::const_reference>;

    namespace detail
    {
        template <>
        struct get_data_type_from_array<sparrow::string_view_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::STRING_VIEW;
            }
        };

        template <>
        struct get_data_type_from_array<sparrow::binary_view_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::BINARY_VIEW;
            }
        };
    }

    template <std::ranges::sized_range T, class CR>
    struct array_inner_types<variable_size_binary_view_array_impl<T, CR>> : array_inner_types_base
    {
        using array_type = variable_size_binary_view_array_impl<T, CR>;
        using inner_value_type = T;
        using inner_reference = CR;
        using inner_const_reference = inner_reference;

        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_reference>>;
        using const_value_iterator = functor_index_iterator<
            detail::layout_value_functor<const array_type, inner_const_reference>>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    template <class T>
    struct is_variable_size_binary_view_array_impl : std::false_type
    {
    };

    template <std::ranges::sized_range T, class CR>
    struct is_variable_size_binary_view_array_impl<variable_size_binary_view_array_impl<T, CR>> : std::true_type
    {
    };

    /**
     * Checks whether T is a variable_size_binary_view_array_impl type.
     */
    template <class T>
    constexpr bool is_variable_size_binary_view_array = is_variable_size_binary_view_array_impl<T>::value;

    /**
     * @brief Variable-size binary view array implementation for efficient string/binary data storage.
     *
     * This class implements an Arrow-compatible array for storing variable-length binary data
     * (strings or byte sequences) using the Binary View layout. This layout is optimized for
     * performance by storing short values inline and using references to external buffers for
     * longer values, reducing memory fragmentation and improving cache locality.
     *
     * The Binary View layout stores a 16-byte view structure for each element:
     * - Length (4 bytes): Size of the data in bytes
     * - Prefix (4 bytes): First 4 bytes of the data (for comparison)
     * - Buffer Index (4 bytes): Index of buffer containing full data (for long strings)
     * - Offset (4 bytes): Offset within the buffer (for long strings)
     *
     * For strings ≤ 12 bytes, the data is stored inline in the view structure.
     * For strings > 12 bytes, the data is stored in separate variadic buffers.
     *
     * Related Apache Arrow specification:
     * https://arrow.apache.org/docs/dev/format/Columnar.html#variable-size-binary-view-layout
     *
     * @tparam T The value type
     * @tparam CR const reference type
     *
     * @pre T must be a sized range
     * @post Maintains Arrow Binary View or String View format compatibility
     * @post Supports efficient random access with O(1) element retrieval
     * @post Optimizes memory usage for mixed short/long string workloads
     *
     * @example
     * ```cpp
     * // String view array
     * std::vector<std::string> data = {"short", "a very long string that exceeds 12 bytes"};
     * string_view_array arr(data);
     *
     * // Binary view array
     * std::vector<std::vector<std::byte>> binary_data = {...};
     * binary_view_array bin_arr(binary_data);
     * ```
     */
    template <std::ranges::sized_range T, class CR>
    class variable_size_binary_view_array_impl final
        : public mutable_array_bitmap_base<variable_size_binary_view_array_impl<T, CR>>
    {
    public:

        using self_type = variable_size_binary_view_array_impl<T, CR>;
        using base_type = mutable_array_bitmap_base<self_type>;

        using inner_types = array_inner_types<self_type>;
        using inner_value_type = typename inner_types::inner_value_type;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;

        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_reference = typename base_type::bitmap_reference;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;
        using bitmap_iterator = typename base_type::bitmap_iterator;
        using const_bitmap_iterator = typename base_type::const_bitmap_iterator;
        using bitmap_range = typename base_type::bitmap_range;
        using const_bitmap_range = typename base_type::const_bitmap_range;

        using value_type = nullable<inner_value_type>;
        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;

        using size_type = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using iterator_tag = typename base_type::iterator_tag;

        using value_iterator = typename base_type::value_iterator;
        using const_value_iterator = typename base_type::const_value_iterator;

        using iterator = typename base_type::iterator;
        using const_iterator = typename base_type::const_iterator;

        /**
         * @brief Constructs variable-size binary view array from Arrow proxy.
         *
         * @param proxy Arrow proxy containing binary/string view array data and schema
         *
         * @pre proxy must contain valid Arrow Binary View or String View array and schema
         * @pre proxy format must be "vu" (string view) or "vz" (binary view)
         * @pre proxy must have the required buffer layout for view arrays
         * @post Array is initialized with data from proxy
         * @post View structures are accessible for efficient element retrieval
         */
        explicit variable_size_binary_view_array_impl(arrow_proxy);

        /**
         * @brief Generic constructor for creating variable-size binary view array.
         *
         * Creates a variable-size binary view array from various input types.
         * Arguments are forwarded to compatible create_proxy() functions.
         *
         * @tparam Args Parameter pack for constructor arguments
         * @param args Constructor arguments (data ranges, validity, metadata, etc.)
         *
         * @pre Arguments must match one of the create_proxy() overload signatures
         * @pre Input data must be convertible to T (string_view or span<const byte>)
         * @post Array is created with optimized Binary View layout
         * @post Short strings (≤12 bytes) are stored inline, long strings in buffers
         */
        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<variable_size_binary_view_array_impl<T, CR>, Args...>)
        explicit variable_size_binary_view_array_impl(Args&&... args)
            : variable_size_binary_view_array_impl(create_proxy(std::forward<Args>(args)...))
        {
        }

    private:

        /**
         * @brief Buffer collection for Binary View layout.
         *
         * Encapsulates the three main buffers used in the Binary View format:
         * - length_buffer: 16-byte view structures for each element
         * - long_string_storage: Concatenated storage for strings > 12 bytes
         * - buffer_sizes: Size information for variadic buffers
         */
        struct buffers_collection
        {
            buffer<uint8_t> length_buffer;        ///< View structures (16 bytes per element)
            buffer<uint8_t> long_string_storage;  ///< Storage for long strings/binary data
            u8_buffer<int64_t> buffer_sizes;      ///< Buffer size metadata
        };

        /**
         * @brief Returns the Arrow format string for this array type.
         *
         * @return "vu" for string_view_array, "vz" for binary_view_array
         */
        [[nodiscard]] static constexpr std::string_view get_arrow_format()
        {
            return std::is_same_v<T, arrow_traits<std::string>::value_type> ? std::string_view("vu")
                                                                            : std::string_view("vz");
        }

        /**
         * @brief Helper function to create ArrowSchema with common parameters.
         *
         * @tparam METADATA_RANGE Type of metadata container
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @param flags Optional Arrow flags
         * @return ArrowSchema configured for binary/string view array
         */
        template <input_metadata_container METADATA_RANGE>
        [[nodiscard]] static ArrowSchema create_arrow_schema(
            std::optional<std::string_view> name,
            std::optional<METADATA_RANGE> metadata,
            std::optional<std::unordered_set<sparrow::ArrowFlag>> flags
        )
        {
            constexpr repeat_view<bool> children_ownership(true, 0);
            return make_arrow_schema(
                get_arrow_format(),
                std::move(name),
                std::move(metadata),
                flags,
                nullptr,  // children
                children_ownership,
                nullptr,  // dictionary
                true
            );
        }

        /**
         * @brief Creates optimized buffer layout from input range.
         *
         * Analyzes the input data and creates the appropriate buffer structure for
         * the Binary View layout. Short strings (≤12 bytes) are stored inline in
         * the view buffer, while longer strings are stored in separate buffers.
         *
         * @tparam R Type of input range
         * @param range Range of string/binary values to process
         * @return Optimized buffer structure for Binary View layout
         *
         * @pre R must be an input range with values convertible to T
         * @pre All values in range must be valid and accessible
         * @post Returns buffers with optimized layout for the given data
         * @post Short values (≤12 bytes) are inlined in length_buffer
         * @post Long values (>12 bytes) are stored in long_string_storage
         * @post buffer_sizes contains metadata for variadic buffer management
         */
        template <std::ranges::input_range R>
            requires std::convertible_to<std::ranges::range_value_t<R>, T>
        static buffers_collection create_buffers(R&& range);

        /**
         * @brief Creates Arrow proxy from range with validity bitmap.
         *
         * @tparam R Type of input range containing values
         * @tparam VB Type of validity bitmap input
         * @tparam METADATA_RANGE Type of metadata container
         * @param range Range of values convertible to T
         * @param bitmap_input Validity bitmap specification
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the binary view array data and schema
         *
         * @pre R must be input range with values convertible to T
         * @pre bitmap_input size must match range.size()
         * @post Returns valid Arrow proxy with Binary View or String View format
         * @post Format is "vu" for string_view, "vz" for span<const byte>
         * @post Array supports null values with NULLABLE flag
         * @post Optimized buffer layout is created based on string lengths
         */
        template <
            std::ranges::input_range R,
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<std::ranges::range_value_t<R>, T>
        [[nodiscard]] static arrow_proxy create_proxy(
            R&& range,
            VB&& bitmap_input = validity_bitmap{},
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates Arrow proxy from range of nullable values.
         *
         * @tparam NULLABLE_RANGE Type of input range containing nullable values
         * @tparam METADATA_RANGE Type of metadata container
         * @param nullable_range Range of nullable<T> values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the binary view array data and schema
         *
         * @pre NULLABLE_RANGE must be input range of nullable<T> values
         * @pre All non-null values must be valid and accessible
         * @post Returns valid Arrow proxy with Binary View or String View format
         * @post Validity bitmap reflects has_value() status of nullable elements
         * @post Array supports null values (nullable = true)
         * @post Optimized buffer layout based on actual (non-null) string lengths
         */
        template <std::ranges::input_range NULLABLE_RANGE, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<std::ranges::range_value_t<NULLABLE_RANGE>, nullable<T>>
        [[nodiscard]] static arrow_proxy create_proxy(
            NULLABLE_RANGE&& nullable_range,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates Arrow proxy from range with nullable flag.
         *
         * @tparam R Type of input range containing values
         * @tparam METADATA_RANGE Type of metadata container
         * @param range Range of values convertible to T
         * @param nullable Whether the array should support null values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the binary view array data and schema
         *
         * @pre R must be input range with values convertible to T
         * @pre All values in range must be valid and accessible
         * @post If nullable is true, array supports null values (though none initially set)
         * @post If nullable is false, array does not support null values
         */
        template <std::ranges::input_range R, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<std::ranges::range_value_t<R>, T>
        [[nodiscard]] static arrow_proxy create_proxy(
            R&& range,
            bool = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates Arrow proxy from pre-existing buffer view and value buffers.
         *
         * @tparam VALUE_BUFFERS_RANGE Type of range containing value buffers
         * @tparam METADATA_RANGE Type of metadata container
         * @param element_count Number of elements in the array
         * @param buffer_view Buffer view containing the view structures
         * @param value_buffers Range of u8_buffer containing the value data
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the binary view array data and schema
         *
         * @pre element_count must match the number of view structures in buffer_view
         * @pre buffer_view must contain valid 16-byte view structures
         * @pre value_buffers must contain the data referenced by the view structures
         * @pre Buffer indices in view structures must be valid for the provided buffers
         * @post Returns valid Arrow proxy with Binary View or String View format
         * @post Array uses the provided buffers directly without copying
         */
        template <
            std::ranges::input_range VALUE_BUFFERS_RANGE,
            validity_bitmap_input VB,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<std::ranges::range_value_t<VALUE_BUFFERS_RANGE>, u8_buffer<uint8_t>>
        [[nodiscard]] static arrow_proxy create_proxy(
            size_t element_count,
            u8_buffer<uint8_t>&& buffer_view,
            VALUE_BUFFERS_RANGE&& value_buffers,
            VB&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Gets mutable reference to element at specified index.
         *
         * @param i Index of the element to access
         * @return Mutable reference to the string/binary view
         *
         * @pre i must be < size()
         * @post Returns valid view referencing the element data
         * @post For short strings, view points to inline data
         * @post For long strings, view points to data in variadic buffer
         */
        [[nodiscard]] constexpr inner_reference value(size_type i);

        /**
         * @brief Gets const reference to element at specified index.
         *
         * @param i Index of the element to access
         * @return Const reference to the string/binary view
         *
         * @pre i must be < size()
         * @post Returns valid const view referencing the element data
         * @post For strings ≤12 bytes, view points to inline data in view buffer
         * @post For strings >12 bytes, view points to data in variadic buffer
         * @post Returned view is valid for the lifetime of the array
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(i < this->size())
         */
        [[nodiscard]] constexpr inner_const_reference value(size_type i) const;

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
         * @post For view arrays, assignment may require buffer reorganization
         * @post String/binary data may be moved between inline and external storage
         *
         * @warning This operation can be expensive for binary view arrays due to
         *          potential reorganization of storage layout when switching between
         *          inline (≤12 bytes) and external (>12 bytes) storage.
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(index < size())
         */
        template <std::ranges::sized_range U>
            requires mpl::convertible_ranges<U, T>
        constexpr void assign(U&& rhs, size_type index);

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
         * @post If shrinking, trailing elements are removed and buffers compacted
         * @post If growing, new elements are set to value
         * @post View structures and variadic buffers are reorganized as needed
         *
         * @warning This operation is expensive for binary view arrays due to
         *          potential complete reorganization of the storage layout.
         */
        template <std::ranges::sized_range U>
            requires mpl::convertible_ranges<U, T>
        void resize_values(size_type new_length, U value);

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
         * @post View structures and variadic buffers are reorganized as needed
         *
         * @warning This operation is expensive for binary view arrays due to
         *          potential reorganization of the entire storage layout.
         */
        template <std::ranges::sized_range U>
            requires mpl::convertible_ranges<U, T>
        value_iterator insert_value(const_value_iterator pos, U value, size_type count);

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
         * @post View structures and variadic buffers are reorganized as needed
         *
         * @warning This operation is expensive for binary view arrays due to
         *          potential reorganization of the entire storage layout.
         */
        template <mpl::iterator_of_type<T> InputIt>
        value_iterator insert_values(const_value_iterator pos, InputIt first, InputIt last);

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
         * @post View structures and variadic buffers are compacted as needed
         *
         * @warning This operation is expensive for binary view arrays due to
         *          potential reorganization of the entire storage layout.
         */
        value_iterator erase_values(const_value_iterator pos, size_type count);

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

        static constexpr size_type LENGTH_BUFFER_INDEX = 1;            ///< Index of length/view buffer
        static constexpr std::size_t DATA_BUFFER_SIZE = 16;            ///< Size of each view structure
        static constexpr std::size_t SHORT_STRING_SIZE = 12;           ///< Threshold for inline storage
        static constexpr std::size_t PREFIX_SIZE = 4;                  ///< Size of prefix for long strings
        static constexpr std::ptrdiff_t PREFIX_OFFSET = 4;             ///< Offset to prefix in view structure
        static constexpr std::ptrdiff_t SHORT_STRING_OFFSET = 4;       ///< Offset to inline data
        static constexpr std::ptrdiff_t BUFFER_INDEX_OFFSET = 8;       ///< Offset to buffer index
        static constexpr std::ptrdiff_t BUFFER_OFFSET_OFFSET = 12;     ///< Offset to buffer offset
        static constexpr std::size_t FIRST_VAR_DATA_BUFFER_INDEX = 2;  ///< Index of first variadic buffer

        friend base_type;
        friend base_type::base_type;
        friend base_type::base_type::base_type;
        friend class detail::layout_value_functor<self_type, inner_reference>;
        friend class detail::layout_value_functor<const self_type, inner_const_reference>;
    };

    template <std::ranges::sized_range T, class CR>
    variable_size_binary_view_array_impl<T, CR>::variable_size_binary_view_array_impl(arrow_proxy proxy)
        : base_type(std::move(proxy))
    {
    }

    template <std::ranges::sized_range T, class CR>
    template <std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_value_t<R>, T>
    auto variable_size_binary_view_array_impl<T, CR>::create_buffers(R&& range) -> buffers_collection
    {
#ifdef __GNUC__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-align"
#endif

        // Helper lambda to cast values to uint8_t
        auto to_uint8 = [](const auto& v)
        {
            return static_cast<std::uint8_t>(v);
        };

        const auto size = range_size(range);
        buffer<uint8_t> length_buffer(size * DATA_BUFFER_SIZE);

        std::size_t long_string_storage_size = 0;
        std::size_t i = 0;
        for (auto&& val : range)
        {
            auto val_casted = val | std::ranges::views::transform(to_uint8);

            const auto length = val.size();
            auto length_ptr = length_buffer.data() + (i * DATA_BUFFER_SIZE);

            // write length
            *reinterpret_cast<std::int32_t*>(length_ptr) = static_cast<std::int32_t>(length);

            if (length <= SHORT_STRING_SIZE)
            {
                // write data itself
                sparrow::ranges::copy(val_casted, length_ptr + SHORT_STRING_OFFSET);
                std::fill(
                    length_ptr + SHORT_STRING_OFFSET + length,
                    length_ptr + DATA_BUFFER_SIZE,
                    std::uint8_t(0)
                );
            }
            else
            {
                // write the prefix of the data
                auto prefix_sub_range = val_casted | std::ranges::views::take(PREFIX_SIZE);
                sparrow::ranges::copy(prefix_sub_range, length_ptr + PREFIX_OFFSET);

                // write the buffer index
                *reinterpret_cast<std::int32_t*>(length_ptr + BUFFER_INDEX_OFFSET) = 0;

                // write the buffer offset
                *reinterpret_cast<std::int32_t*>(
                    length_ptr + BUFFER_OFFSET_OFFSET
                ) = static_cast<std::int32_t>(long_string_storage_size);

                // count the size of the long string storage
                long_string_storage_size += length;
            }
            ++i;
        }

        // write the long string storage
        buffer<uint8_t> long_string_storage(long_string_storage_size);
        std::size_t long_string_storage_offset = 0;
        for (auto&& val : range)
        {
            const auto length = val.size();
            if (length > SHORT_STRING_SIZE)
            {
                auto val_casted = val | std::ranges::views::transform(to_uint8);

                sparrow::ranges::copy(val_casted, long_string_storage.data() + long_string_storage_offset);
                long_string_storage_offset += length;
            }
        }

        // For binary or utf-8 view arrays, an extra buffer is appended which stores
        // the lengths of each variadic data buffer as int64_t.
        // This buffer is necessary since these buffer lengths are not trivially
        // extractable from other data in an array of binary or utf-8 view type.
        u8_buffer<int64_t> buffer_sizes(
            static_cast<std::size_t>(1),
            static_cast<int64_t>(long_string_storage_size)
        );

        return {std::move(length_buffer), std::move(long_string_storage), std::move(buffer_sizes)};

#ifdef __GNUC__
#    pragma GCC diagnostic pop
#endif
    }

    template <std::ranges::sized_range T, class CR>
    template <std::ranges::input_range R, validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
        requires std::convertible_to<std::ranges::range_value_t<R>, T>
    arrow_proxy variable_size_binary_view_array_impl<T, CR>::create_proxy(
        R&& range,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto size = range_size(range);
        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<VB>(validity_input));
        const auto null_count = vbitmap.null_count();

        static const std::optional<std::unordered_set<sparrow::ArrowFlag>> flags{{ArrowFlag::NULLABLE}};

        // create arrow schema
        ArrowSchema schema = create_arrow_schema(std::move(name), std::move(metadata), flags);

        // create buffers
        auto buffers_parts = create_buffers(std::forward<R>(range));

        std::vector<buffer<uint8_t>> buffers{
            std::move(vbitmap).extract_storage(),
            std::move(buffers_parts.length_buffer),
            std::move(buffers_parts.long_string_storage),
            std::move(buffers_parts.buffer_sizes).extract_storage()
        };

        constexpr repeat_view<bool> children_ownership(true, 0);

        // create arrow array
        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(buffers),
            nullptr,  // children
            children_ownership,
            nullptr,  // dictionary
            true
        );

        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <std::ranges::sized_range T, class CR>
    template <std::ranges::input_range NULLABLE_RANGE, input_metadata_container METADATA_RANGE>
        requires std::convertible_to<std::ranges::range_value_t<NULLABLE_RANGE>, nullable<T>>
    [[nodiscard]] arrow_proxy variable_size_binary_view_array_impl<T, CR>::create_proxy(
        NULLABLE_RANGE&& nullable_range,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        auto values = nullable_range
                      | std::views::transform(
                          [](const auto& v)
                          {
                              return static_cast<T>(v.value());
                          }
                      );

        auto is_non_null = nullable_range
                           | std::views::transform(
                               [](const auto& v)
                               {
                                   return v.has_value();
                               }
                           );

        return create_proxy(
            std::forward<decltype(values)>(values),
            std::forward<decltype(is_non_null)>(is_non_null),
            name,
            metadata
        );
    }

    template <std::ranges::sized_range T, class CR>
    template <std::ranges::input_range R, input_metadata_container METADATA_RANGE>
        requires std::convertible_to<std::ranges::range_value_t<R>, T>
    [[nodiscard]] arrow_proxy variable_size_binary_view_array_impl<T, CR>::create_proxy(
        R&& range,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        if (nullable)
        {
            return create_proxy(std::forward<R>(range), validity_bitmap{}, std::move(name), std::move(metadata));
        }

        // create arrow schema
        ArrowSchema schema = create_arrow_schema(std::move(name), std::move(metadata), std::nullopt);

        // create buffers
        auto buffers_parts = create_buffers(std::forward<R>(range));

        std::vector<buffer<uint8_t>> buffers{
            buffer<uint8_t>{nullptr, 0},  // validity bitmap
            std::move(buffers_parts.length_buffer),
            std::move(buffers_parts.long_string_storage),
            std::move(buffers_parts.buffer_sizes).extract_storage()
        };
        const auto size = range_size(range);

        constexpr repeat_view<bool> children_ownership(true, 0);

        // create arrow array
        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<int64_t>(0),
            0,  // offset
            std::move(buffers),
            nullptr,  // children
            children_ownership,
            nullptr,  // dictionary
            true
        );

        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <std::ranges::sized_range T, class CR>
    template <std::ranges::input_range VALUE_BUFFERS_RANGE, validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
        requires std::convertible_to<std::ranges::range_value_t<VALUE_BUFFERS_RANGE>, u8_buffer<uint8_t>>
    arrow_proxy variable_size_binary_view_array_impl<T, CR>::create_proxy(
        size_t element_count,
        u8_buffer<uint8_t>&& buffer_view,
        VALUE_BUFFERS_RANGE&& value_buffers,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto size = buffer_view.size() / DATA_BUFFER_SIZE;
        SPARROW_ASSERT_TRUE(size == element_count);

        static const std::optional<std::unordered_set<sparrow::ArrowFlag>> flags{{ArrowFlag::NULLABLE}};

        ArrowSchema schema = create_arrow_schema(std::move(name), std::move(metadata), flags);

        auto bitmap = ensure_validity_bitmap(size, std::forward<VB>(validity_input));
        std::vector<buffer<uint8_t>> buffers{std::move(bitmap).extract_storage(), std::move(buffer_view)};
        for (auto&& buf : value_buffers)
        {
            buffers.push_back(std::forward<decltype(buf)>(buf));
        }

        // Create buffer sizes for the variadic buffers
        u8_buffer<int64_t> buffer_sizes(value_buffers.size());
        for (std::size_t i = 0; i < value_buffers.size(); ++i)
        {
            buffer_sizes[i] = static_cast<int64_t>(value_buffers[i].size());
        }
        buffers.push_back(std::move(buffer_sizes).extract_storage());

        constexpr repeat_view<bool> children_ownership(true, 0);

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),                 // length
            static_cast<std::int64_t>(bitmap.null_count()),  // null_count
            0,                                               // offset
            std::move(buffers),
            nullptr,  // children
            children_ownership,
            nullptr,  // dictionary
            true
        );

        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <std::ranges::sized_range T, class CR>
    constexpr auto variable_size_binary_view_array_impl<T, CR>::value(size_type i) -> inner_reference
    {
        return static_cast<const self_type*>(this)->value(i);
    }

    template <std::ranges::sized_range T, class CR>
    constexpr auto variable_size_binary_view_array_impl<T, CR>::value(size_type i) const
        -> inner_const_reference
    {
#ifdef __GNUC__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-align"
#endif

        SPARROW_ASSERT_TRUE(i < this->size());
        using char_or_byte = typename inner_const_reference::value_type;

        auto data_ptr = this->get_arrow_proxy().buffers()[LENGTH_BUFFER_INDEX].template data<uint8_t>()
                        + (i * DATA_BUFFER_SIZE);
        const auto length = static_cast<std::size_t>(*reinterpret_cast<const std::int32_t*>(data_ptr));

        if (length <= SHORT_STRING_SIZE)
        {
            constexpr std::ptrdiff_t data_offset = 4;
            const auto ptr = reinterpret_cast<const char_or_byte*>(data_ptr);
            const auto ret = inner_const_reference(ptr + data_offset, length);
            return ret;
        }
        else
        {
            const auto buffer_index = static_cast<std::size_t>(
                *reinterpret_cast<const std::int32_t*>(data_ptr + BUFFER_INDEX_OFFSET)
            );
            const auto buffer_offset = static_cast<std::size_t>(
                *reinterpret_cast<const std::int32_t*>(data_ptr + BUFFER_OFFSET_OFFSET)
            );
            const auto buffer = this->get_arrow_proxy()
                                    .buffers()[buffer_index + FIRST_VAR_DATA_BUFFER_INDEX]
                                    .template data<const char_or_byte>();
            return inner_const_reference(buffer + buffer_offset, length);
        }

#ifdef __GNUC__
#    pragma GCC diagnostic pop
#endif
    }

    template <std::ranges::sized_range T, class CR>
    constexpr auto variable_size_binary_view_array_impl<T, CR>::value_begin() -> value_iterator
    {
        return value_iterator(detail::layout_value_functor<self_type, inner_reference>(), 0);
    }

    template <std::ranges::sized_range T, class CR>
    constexpr auto variable_size_binary_view_array_impl<T, CR>::value_end() -> value_iterator
    {
        return value_iterator(detail::layout_value_functor<self_type, inner_reference>(this), this->size());
    }

    template <std::ranges::sized_range T, class CR>
    constexpr auto variable_size_binary_view_array_impl<T, CR>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator(detail::layout_value_functor<const self_type, inner_const_reference>(this), 0);
    }

    template <std::ranges::sized_range T, class CR>
    constexpr auto variable_size_binary_view_array_impl<T, CR>::value_cend() const -> const_value_iterator
    {
        return const_value_iterator(
            detail::layout_value_functor<const self_type, inner_const_reference>(this),
            this->size()
        );
    }

    template <std::ranges::sized_range T, class CR>
    template <std::ranges::sized_range U>
        requires mpl::convertible_ranges<U, T>
    constexpr void variable_size_binary_view_array_impl<T, CR>::assign(U&& rhs, size_type index)
    {
        SPARROW_ASSERT_TRUE(index < this->size());

        // Note: Binary view arrays have a complex layout optimized for read access.
        // Modifying elements requires careful handling of the view structure and
        // potential reorganization of storage buffers.
        //
        // This implementation provides a basic assign operation but may not be
        // as efficient as the variable_size_binary_array equivalent due to the
        // complexity of the view layout format.

        const auto new_length = static_cast<std::size_t>(std::ranges::size(rhs));

        // Get pointer to the view structure for this element
        auto& length_buffer = this->get_arrow_proxy().get_array_private_data()->buffers()[LENGTH_BUFFER_INDEX];
        auto view_ptr = length_buffer.data() + (index * DATA_BUFFER_SIZE);

        // Read current length
        const auto current_length = static_cast<std::size_t>(*reinterpret_cast<const std::int32_t*>(view_ptr));

        // Update the length in the view structure
        *reinterpret_cast<std::int32_t*>(view_ptr) = static_cast<std::int32_t>(new_length);

        if (new_length <= SHORT_STRING_SIZE)
        {
            // Store inline: copy data directly into the view structure
            auto data_ptr = view_ptr + SHORT_STRING_OFFSET;

            // Transform and copy the new data
            auto transformed = rhs
                               | std::ranges::views::transform(
                                   [](const auto& v)
                                   {
                                       using char_or_byte = typename T::value_type;
                                       return static_cast<char_or_byte>(v);
                                   }
                               );

            std::ranges::copy(transformed, reinterpret_cast<typename T::value_type*>(data_ptr));

            // Clear any remaining bytes in the inline storage
            if (new_length < SHORT_STRING_SIZE)
            {
                std::fill_n(
                    reinterpret_cast<typename T::value_type*>(data_ptr) + new_length,
                    SHORT_STRING_SIZE - new_length,
                    typename T::value_type{}
                );
            }
        }
        else
        {
            // Handle assignment of long strings (> 12 bytes)
            // This requires managing the variadic buffers and potentially reorganizing the layout

            auto& buffers = this->get_arrow_proxy().get_array_private_data()->buffers();
            auto& var_data_buffer = buffers[FIRST_VAR_DATA_BUFFER_INDEX];
            auto& buffer_sizes_buffer = buffers[buffers.size() - 1];  // Last buffer contains sizes

            // Check if current element was already a long string
            const bool was_long_string = current_length > SHORT_STRING_SIZE;
            std::size_t current_buffer_offset = 0;

            if (was_long_string)
            {
                // Read current buffer offset from view structure
                current_buffer_offset = static_cast<std::size_t>(
                    *reinterpret_cast<const std::int32_t*>(view_ptr + BUFFER_OFFSET_OFFSET)
                );
            }

            // Transform new data once for efficiency
            auto transformed_data = rhs
                                    | std::ranges::views::transform(
                                        [](const auto& v)
                                        {
                                            using char_or_byte = typename T::value_type;
                                            return static_cast<char_or_byte>(v);
                                        }
                                    );

            // Check for memory reuse optimization: if the new value is identical to existing data
            bool can_reuse_memory = false;
            if (was_long_string && new_length == current_length)
            {
                const auto* existing_data = var_data_buffer.data() + current_buffer_offset;
                can_reuse_memory = std::ranges::equal(
                    transformed_data,
                    std::span<const typename T::value_type>(
                        reinterpret_cast<const typename T::value_type*>(existing_data),
                        new_length
                    )
                );
            }

            if (can_reuse_memory)
            {
                // Data is identical - just update the view structure prefix and we're done
                auto prefix_range = rhs | std::ranges::views::take(PREFIX_SIZE);
                auto prefix_transformed = prefix_range
                                          | std::ranges::views::transform(
                                              [](const auto& v)
                                              {
                                                  return static_cast<std::uint8_t>(v);
                                              }
                                          );
                std::ranges::copy(prefix_transformed, view_ptr + PREFIX_OFFSET);
                return;  // Early exit - no buffer management needed
            }

            // Calculate space requirements and buffer management strategy
            const auto length_diff = static_cast<std::ptrdiff_t>(new_length)
                                     - static_cast<std::ptrdiff_t>(current_length);
            const bool can_fit_in_place = was_long_string && length_diff <= 0;

            std::size_t final_offset = 0;

            if (can_fit_in_place)
            {
                // We can reuse the existing space (new data is same size or smaller)
                final_offset = current_buffer_offset;

                // If the new data is smaller, we need to compact the buffer
                if (length_diff < 0)
                {
                    const auto bytes_to_compact = static_cast<std::size_t>(-length_diff);
                    const auto move_start = current_buffer_offset + current_length;
                    const auto move_end = var_data_buffer.size();
                    const auto bytes_to_move = move_end - move_start;

                    if (bytes_to_move > 0)
                    {
                        // Move data after current element to fill the gap
                        std::move(
                            var_data_buffer.data() + move_start,
                            var_data_buffer.data() + move_end,
                            var_data_buffer.data() + move_start - bytes_to_compact
                        );
                    }

                    // Resize buffer to remove unused space
                    var_data_buffer.resize(var_data_buffer.size() - bytes_to_compact);

                    // Update buffer offsets for all elements that come after this one
                    const auto array_size = this->size();
                    for (size_type i = 0; i < array_size; ++i)
                    {
                        if (i == index)
                        {
                            continue;  // Skip current element
                        }

                        auto other_view_ptr = length_buffer.data() + (i * DATA_BUFFER_SIZE);
                        auto other_length = static_cast<std::size_t>(
                            *reinterpret_cast<const std::int32_t*>(other_view_ptr)
                        );

                        if (other_length > SHORT_STRING_SIZE)
                        {
                            auto other_offset = static_cast<std::size_t>(
                                *reinterpret_cast<const std::int32_t*>(other_view_ptr + BUFFER_OFFSET_OFFSET)
                            );

                            // Update offset if this element comes after our current element
                            if (other_offset > current_buffer_offset + current_length)
                            {
                                *reinterpret_cast<std::int32_t*>(
                                    other_view_ptr + BUFFER_OFFSET_OFFSET
                                ) = static_cast<std::int32_t>(other_offset - bytes_to_compact);
                            }
                        }
                    }

                    // Update buffer sizes metadata
                    auto buffer_sizes_ptr = buffer_sizes_buffer.template data<std::int64_t>();
                    *buffer_sizes_ptr = static_cast<std::int64_t>(var_data_buffer.size());
                }
            }
            else
            {
                // Need to expand buffer or assign to new location
                const auto expansion_needed = was_long_string ? length_diff
                                                              : static_cast<std::ptrdiff_t>(new_length);
                const auto new_var_buffer_size = var_data_buffer.size() + expansion_needed;

                if (was_long_string && length_diff > 0)
                {
                    // Expand in-place: move data after current element to make space
                    final_offset = current_buffer_offset;
                    const auto expansion_bytes = static_cast<std::size_t>(length_diff);
                    const auto move_start = current_buffer_offset + current_length;
                    const auto bytes_to_move = var_data_buffer.size() - move_start;

                    // Resize buffer first
                    var_data_buffer.resize(new_var_buffer_size);

                    if (bytes_to_move > 0)
                    {
                        // Move data to make space for expansion
                        std::move_backward(
                            var_data_buffer.data() + move_start,
                            var_data_buffer.data() + move_start + bytes_to_move,
                            var_data_buffer.data() + move_start + bytes_to_move + expansion_bytes
                        );
                    }

                    // Update buffer offsets for all elements that come after this one
                    const auto array_size = this->size();
                    for (size_type i = 0; i < array_size; ++i)
                    {
                        if (i == index)
                        {
                            continue;  // Skip current element
                        }

                        auto other_view_ptr = length_buffer.data() + (i * DATA_BUFFER_SIZE);
                        auto other_length = static_cast<std::size_t>(
                            *reinterpret_cast<const std::int32_t*>(other_view_ptr)
                        );

                        if (other_length > SHORT_STRING_SIZE)
                        {
                            auto other_offset = static_cast<std::size_t>(
                                *reinterpret_cast<const std::int32_t*>(other_view_ptr + BUFFER_OFFSET_OFFSET)
                            );

                            // Update offset if this element comes after our expansion point
                            if (other_offset >= move_start)
                            {
                                *reinterpret_cast<std::int32_t*>(
                                    other_view_ptr + BUFFER_OFFSET_OFFSET
                                ) = static_cast<std::int32_t>(other_offset + expansion_bytes);
                            }
                        }
                    }
                }
                else
                {
                    // Append to end of buffer (new long string)
                    final_offset = var_data_buffer.size();
                    var_data_buffer.resize(new_var_buffer_size);
                }

                // Update buffer sizes metadata
                auto buffer_sizes_ptr = buffer_sizes_buffer.template data<std::int64_t>();
                *buffer_sizes_ptr = static_cast<std::int64_t>(new_var_buffer_size);
            }

            // Copy new data into variadic buffer at the determined offset
            std::ranges::copy(transformed_data, var_data_buffer.data() + final_offset);

            // Update view structure for long string format
            // Write prefix (first 4 bytes)
            auto prefix_range = rhs | std::ranges::views::take(PREFIX_SIZE);
            auto prefix_transformed = prefix_range
                                      | std::ranges::views::transform(
                                          [](const auto& v)
                                          {
                                              return static_cast<std::uint8_t>(v);
                                          }
                                      );
            std::ranges::copy(prefix_transformed, view_ptr + PREFIX_OFFSET);

            // Write buffer index
            *reinterpret_cast<std::int32_t*>(view_ptr + BUFFER_INDEX_OFFSET) = static_cast<std::int32_t>(
                FIRST_VAR_DATA_BUFFER_INDEX
            );

            // Write buffer offset
            *reinterpret_cast<std::int32_t*>(view_ptr + BUFFER_OFFSET_OFFSET) = static_cast<std::int32_t>(
                final_offset
            );
        }
    }

    template <std::ranges::sized_range T, class CR>
    template <std::ranges::sized_range U>
        requires mpl::convertible_ranges<U, T>
    void variable_size_binary_view_array_impl<T, CR>::resize_values(size_type new_length, U value)
    {
        const size_t current_size = this->size();

        if (new_length == current_size)
        {
            return;  // Nothing to do
        }

        if (new_length < current_size)
        {
            // Shrinking: remove elements from the end
            erase_values(sparrow::next(value_cbegin(), new_length), current_size - new_length);
        }
        else
        {
            // Growing: insert copies of value at the end
            insert_value(value_cend(), value, new_length - current_size);
        }
    }

    template <std::ranges::sized_range T, class CR>
    template <std::ranges::sized_range U>
        requires mpl::convertible_ranges<U, T>
    auto
    variable_size_binary_view_array_impl<T, CR>::insert_value(const_value_iterator pos, U value, size_type count)
        -> value_iterator
    {
        const auto repeat_view = sparrow::repeat_view<U>(value, count);
        return insert_values(pos, std::ranges::begin(repeat_view), std::ranges::end(repeat_view));
    }

    template <std::ranges::sized_range T, class CR>
    template <mpl::iterator_of_type<T> InputIt>
    auto
    variable_size_binary_view_array_impl<T, CR>::insert_values(const_value_iterator pos, InputIt first, InputIt last)
        -> value_iterator
    {
        SPARROW_ASSERT_TRUE(first <= last);
        const size_type count = static_cast<size_type>(std::distance(first, last));
        if (count == 0)
        {
            const auto insert_index = std::distance(value_cbegin(), pos);
            return value_begin() + insert_index;
        }

        const auto insert_index = static_cast<size_t>(std::distance(value_cbegin(), pos));
        const auto current_size = this->size();
        const auto new_size = current_size + count;

        // Calculate total additional variadic storage needed
        std::size_t additional_var_storage = 0;
        std::vector<std::size_t> value_lengths;
        value_lengths.reserve(count);

        for (auto it = first; it != last; ++it)
        {
            const auto length = static_cast<std::size_t>(std::ranges::size(*it));
            value_lengths.push_back(length);
            if (length > SHORT_STRING_SIZE)
            {
                additional_var_storage += length;
            }
        }

        // Get access to current buffers
        auto& proxy = this->get_arrow_proxy();
        auto* private_data = proxy.get_array_private_data();
        auto& buffers = private_data->buffers();

        // Resize view buffer
        const auto new_view_buffer_size = new_size * DATA_BUFFER_SIZE;
        buffers[LENGTH_BUFFER_INDEX].resize(new_view_buffer_size);

        // Resize variadic data buffer if needed
        if (additional_var_storage > 0)
        {
            const auto current_var_size = buffers[FIRST_VAR_DATA_BUFFER_INDEX].size();
            buffers[FIRST_VAR_DATA_BUFFER_INDEX].resize(current_var_size + additional_var_storage);
        }

        // Update buffer sizes metadata
        auto& buffer_sizes = buffers[buffers.size() - 1];
        auto* sizes_ptr = reinterpret_cast<std::int64_t*>(buffer_sizes.data());
        *sizes_ptr = static_cast<std::int64_t>(buffers[FIRST_VAR_DATA_BUFFER_INDEX].size());

        // Shift existing view structures after insertion point
        auto* view_data = buffers[LENGTH_BUFFER_INDEX].data();
        if (insert_index < current_size)
        {
            const auto bytes_to_move = (current_size - insert_index) * DATA_BUFFER_SIZE;
            const auto src_offset = insert_index * DATA_BUFFER_SIZE;
            const auto dst_offset = (insert_index + count) * DATA_BUFFER_SIZE;

            std::memmove(view_data + dst_offset, view_data + src_offset, bytes_to_move);

            // Update buffer offsets for moved long strings
            if (additional_var_storage > 0)
            {
                for (size_type i = insert_index + count; i < new_size; ++i)
                {
                    auto* view_ptr = view_data + (i * DATA_BUFFER_SIZE);
                    const auto length = static_cast<std::size_t>(
                        *reinterpret_cast<const std::int32_t*>(view_ptr)
                    );

                    if (length > SHORT_STRING_SIZE)
                    {
                        auto* offset_ptr = reinterpret_cast<std::int32_t*>(view_ptr + BUFFER_OFFSET_OFFSET);
                        *offset_ptr += static_cast<std::int32_t>(additional_var_storage);
                    }
                }
            }
        }

        // Insert new view structures
        std::size_t var_offset = buffers[FIRST_VAR_DATA_BUFFER_INDEX].size() - additional_var_storage;
        size_type value_idx = 0;

        for (auto it = first; it != last; ++it, ++value_idx)
        {
            const auto view_index = insert_index + value_idx;
            auto* view_ptr = view_data + (view_index * DATA_BUFFER_SIZE);
            const auto value_length = value_lengths[value_idx];

            std::vector<std::uint8_t> transformed_value;
            transformed_value.reserve((*it).size());
            std::transform(
                (*it).begin(),
                (*it).end(),
                std::back_inserter(transformed_value),
                [](const auto& v)
                {
                    return static_cast<std::uint8_t>(v);
                }
            );

            // Write length
            *reinterpret_cast<std::int32_t*>(view_ptr) = static_cast<std::int32_t>(value_length);

            if (value_length <= SHORT_STRING_SIZE)
            {
                // Store inline
                sparrow::ranges::copy(transformed_value, view_ptr + SHORT_STRING_OFFSET);
                std::fill(
                    view_ptr + SHORT_STRING_OFFSET + value_length,
                    view_ptr + DATA_BUFFER_SIZE,
                    std::uint8_t(0)
                );
            }
            else
            {
                // Store prefix
                auto prefix_iter = std::ranges::begin(transformed_value);
                auto prefix_end = std::ranges::end(transformed_value);
                std::size_t copied = 0;
                auto* prefix_dest = view_ptr + PREFIX_OFFSET;

                while (prefix_iter != prefix_end && copied < PREFIX_SIZE)
                {
                    *prefix_dest = *prefix_iter;
                    ++prefix_iter;
                    ++prefix_dest;
                    ++copied;
                }

                // Set buffer index
                *reinterpret_cast<std::int32_t*>(view_ptr + BUFFER_INDEX_OFFSET) = 0;

                // Set buffer offset
                *reinterpret_cast<std::int32_t*>(view_ptr + BUFFER_OFFSET_OFFSET) = static_cast<std::int32_t>(
                    var_offset
                );

                // Copy data to variadic buffer
                sparrow::ranges::copy(transformed_value, buffers[FIRST_VAR_DATA_BUFFER_INDEX].data() + var_offset);
                var_offset += value_length;
            }
        }

        // Update buffers
        proxy.update_buffers();

        return value_begin() + static_cast<difference_type>(insert_index);
    }

    template <std::ranges::sized_range T, class CR>
    auto variable_size_binary_view_array_impl<T, CR>::erase_values(const_value_iterator pos, size_type count)
        -> value_iterator
    {
        if (count == 0)
        {
            const auto erase_index = std::distance(value_cbegin(), pos);
            return value_begin() + erase_index;
        }

        const size_t erase_index = static_cast<size_t>(std::distance(value_cbegin(), pos));
        const size_t current_size = this->size();

        // Validate bounds
        if (erase_index + count > current_size)
        {
            count = current_size - erase_index;
        }

        if (count == 0)
        {
            const auto erase_index_bis = std::distance(value_cbegin(), pos);
            return value_begin() + erase_index_bis;
        }

        const auto new_size = current_size - count;

        // Calculate how much variadic storage will be freed
        std::size_t freed_var_storage = 0;
        auto& proxy = this->get_arrow_proxy();
        auto* private_data = proxy.get_array_private_data();
        auto& buffers = private_data->buffers();
        auto* view_data = buffers[LENGTH_BUFFER_INDEX].data();

        // Calculate freed storage from elements being erased
        for (size_type i = erase_index; i < erase_index + count; ++i)
        {
            auto* view_ptr = view_data + (i * DATA_BUFFER_SIZE);
            const auto length = static_cast<std::size_t>(*reinterpret_cast<const std::int32_t*>(view_ptr));
            if (length > SHORT_STRING_SIZE)
            {
                freed_var_storage += length;
            }
        }

        // Handle empty array case
        if (new_size == 0)
        {
            // Resize all buffers to empty
            if (buffers[0].size() > 0)
            {
                buffers[0].clear();
            }
            buffers[LENGTH_BUFFER_INDEX].clear();
            buffers[FIRST_VAR_DATA_BUFFER_INDEX].clear();

            auto& buffer_sizes = buffers[buffers.size() - 1];
            auto* sizes_ptr = reinterpret_cast<std::int64_t*>(buffer_sizes.data());
            *sizes_ptr = 0;

            proxy.update_buffers();
            return value_begin();
        }

        // Compact variadic buffer if needed
        if (freed_var_storage > 0)
        {
            auto& var_buffer = buffers[FIRST_VAR_DATA_BUFFER_INDEX];
            std::size_t write_offset = 0;

            // Create mapping of old offsets to new offsets
            std::unordered_map<std::size_t, std::size_t> offset_mapping;

            for (size_type i = 0; i < current_size; ++i)
            {
                if (i >= erase_index && i < erase_index + count)
                {
                    // Skip erased elements
                    continue;
                }

                auto* view_ptr = view_data + (i * DATA_BUFFER_SIZE);
                const auto length = static_cast<std::size_t>(*reinterpret_cast<const std::int32_t*>(view_ptr));
                if (length > SHORT_STRING_SIZE)
                {
                    const auto old_offset = static_cast<std::size_t>(
                        *reinterpret_cast<const std::int32_t*>(view_ptr + BUFFER_OFFSET_OFFSET)
                    );

                    // Record mapping for updating view structures later
                    offset_mapping[old_offset] = write_offset;

                    // Move data if needed
                    if (write_offset != old_offset)
                    {
                        std::memmove(var_buffer.data() + write_offset, var_buffer.data() + old_offset, length);
                    }

                    write_offset += length;
                }
            }

            // Resize variadic buffer
            var_buffer.resize(var_buffer.size() - freed_var_storage);

            // Update buffer sizes metadata
            auto& buffer_sizes = buffers[buffers.size() - 1];
            auto* sizes_ptr = reinterpret_cast<std::int64_t*>(buffer_sizes.data());
            *sizes_ptr = static_cast<std::int64_t>(var_buffer.size());

            // Update view structure offsets
            for (size_type i = 0; i < current_size; ++i)
            {
                if (i >= erase_index && i < erase_index + count)
                {
                    continue;  // Skip erased elements
                }

                auto* view_ptr = view_data + (i * DATA_BUFFER_SIZE);
                const auto length = static_cast<std::size_t>(*reinterpret_cast<const std::int32_t*>(view_ptr));
                if (length > SHORT_STRING_SIZE)
                {
                    const auto old_offset = static_cast<std::size_t>(
                        *reinterpret_cast<const std::int32_t*>(view_ptr + BUFFER_OFFSET_OFFSET)
                    );
                    auto it = offset_mapping.find(old_offset);
                    if (it != offset_mapping.end())
                    {
                        *reinterpret_cast<std::int32_t*>(
                            view_ptr + BUFFER_OFFSET_OFFSET
                        ) = static_cast<std::int32_t>(it->second);
                    }
                }
            }
        }

        // Compact view buffer - move elements after erase range
        if (erase_index + count < current_size)
        {
            const auto src_offset = (erase_index + count) * DATA_BUFFER_SIZE;
            const auto dst_offset = erase_index * DATA_BUFFER_SIZE;
            const auto bytes_to_move = (current_size - erase_index - count) * DATA_BUFFER_SIZE;

            std::memmove(view_data + dst_offset, view_data + src_offset, bytes_to_move);
        }

        // Resize view buffer
        buffers[LENGTH_BUFFER_INDEX].resize(new_size * DATA_BUFFER_SIZE);

        // Update buffers
        proxy.update_buffers();

        // Return iterator to element after last erased, or end if we erased to the end
        return erase_index < new_size ? sparrow::next(value_begin(), erase_index) : value_end();
    }

}
