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
        struct buffers
        {
            buffer<uint8_t> length_buffer;        ///< View structures (16 bytes per element)
            buffer<uint8_t> long_string_storage;  ///< Storage for long strings/binary data
            u8_buffer<int64_t> buffer_sizes;      ///< Buffer size metadata
        };

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
        static buffers create_buffers(R&& range);

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
        template <std::ranges::input_range R, validity_bitmap_input VB = validity_bitmap, input_metadata_container METADATA_RANGE>
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
        template <std::ranges::input_range NULLABLE_RANGE, input_metadata_container METADATA_RANGE>
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
        template <std::ranges::input_range R, input_metadata_container METADATA_RANGE>
            requires std::convertible_to<std::ranges::range_value_t<R>, T>
        [[nodiscard]] static arrow_proxy create_proxy(
            R&& range,
            bool = true,
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
    auto variable_size_binary_view_array_impl<T, CR>::create_buffers(R&& range) -> buffers
    {
#ifdef __GNUC__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-align"
#endif

        const auto size = range_size(range);
        buffer<uint8_t> length_buffer(size * DATA_BUFFER_SIZE);

        std::size_t long_string_storage_size = 0;
        std::size_t i = 0;
        for (auto&& val : range)
        {
            auto val_casted = val
                              | std::ranges::views::transform(
                                  [](const auto& v)
                                  {
                                      return static_cast<std::uint8_t>(v);
                                  }
                              );

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
                *reinterpret_cast<std::int32_t*>(
                    length_ptr + BUFFER_INDEX_OFFSET
                ) = static_cast<std::int32_t>(FIRST_VAR_DATA_BUFFER_INDEX);

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
                auto val_casted = val
                                  | std::ranges::views::transform(
                                      [](const auto& v)
                                      {
                                          return static_cast<std::uint8_t>(v);
                                      }
                                  );

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

        const repeat_view<bool> children_ownership(true, 0);

        static const std::optional<std::unordered_set<sparrow::ArrowFlag>> flags{{ArrowFlag::NULLABLE}};

        // create arrow schema and array
        ArrowSchema schema = make_arrow_schema(
            std::is_same<T, arrow_traits<std::string>::value_type>::value ? std::string_view("vu")
                                                                          : std::string_view("vz"),
            std::move(name),      // name
            std::move(metadata),  // metadata
            flags,                // flags
            nullptr,              // children
            children_ownership,
            nullptr,  // dictionary
            true
        );

        // create buffers
        auto buffers_parts = create_buffers(std::forward<R>(range));

        std::vector<buffer<uint8_t>> buffers{
            std::move(vbitmap).extract_storage(),
            std::move(buffers_parts.length_buffer),
            std::move(buffers_parts.long_string_storage),
            std::move(buffers_parts.buffer_sizes).extract_storage()
        };

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
                              return static_cast<std::string>(v.value());
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
        else
        {
            // create arrow schema and array
            const repeat_view<bool> children_ownership(true, 0);
            ArrowSchema schema = make_arrow_schema(
                std::is_same<T, arrow_traits<std::string>::value_type>::value ? std::string_view("vu")
                                                                              : std::string_view("vz"),
                std::move(name),      // name
                std::move(metadata),  // metadata
                std::nullopt,         // flags
                nullptr,              // children
                children_ownership,
                nullptr,  // dictionary
                true
            );

            // create buffers
            auto buffers_parts = create_buffers(std::forward<R>(range));

            std::vector<buffer<uint8_t>> buffers{
                buffer<uint8_t>{nullptr, 0},  // validity bitmap
                std::move(buffers_parts.length_buffer),
                std::move(buffers_parts.long_string_storage),
                std::move(buffers_parts.buffer_sizes).extract_storage()
            };
            const auto size = range_size(range);

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

        constexpr std::size_t element_size = 16;
        auto data_ptr = this->get_arrow_proxy().buffers()[LENGTH_BUFFER_INDEX].template data<uint8_t>()
                        + (i * element_size);

        auto length = static_cast<std::size_t>(*reinterpret_cast<const std::int32_t*>(data_ptr));
        using char_or_byte = typename inner_const_reference::value_type;

        if (length <= 12)
        {
            constexpr std::ptrdiff_t data_offset = 4;
            auto ptr = reinterpret_cast<const char_or_byte*>(data_ptr);
            const auto ret = inner_const_reference(ptr + data_offset, length);
            return ret;
        }
        else
        {
            constexpr std::ptrdiff_t buffer_index_offset = 8;
            constexpr std::ptrdiff_t buffer_offset_offset = 12;
            auto buffer_index = static_cast<std::size_t>(
                *reinterpret_cast<const std::int32_t*>(data_ptr + buffer_index_offset)
            );
            auto buffer_offset = static_cast<std::size_t>(
                *reinterpret_cast<const std::int32_t*>(data_ptr + buffer_offset_offset)
            );
            auto buffer = this->get_arrow_proxy().buffers()[buffer_index].template data<const char_or_byte>();
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
}
