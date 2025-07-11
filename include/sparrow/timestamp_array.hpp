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

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/layout/array_base.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/primitive_data_access.hpp"
#include "sparrow/layout/timestamp_concepts.hpp"
#include "sparrow/layout/timestamp_reference.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/u8_buffer.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/metadata.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/repeat_container.hpp"
#include "sparrow/utils/temporal.hpp"

// tts : timestamp<std::chrono::seconds>
// tsm : timestamp<std::chrono::milliseconds>
// tsu : timestamp<std::chrono::microseconds>
// tsn : timestamp<std::chrono::nanoseconds>

namespace sparrow
{
    template <timestamp_type T>
    class timestamp_array;

    template <timestamp_type T>
    struct array_inner_types<timestamp_array<T>> : array_inner_types_base
    {
        using self_type = timestamp_array<T>;

        using inner_value_type = T;
        using inner_reference = timestamp_reference<self_type>;
        using inner_const_reference = T;

        using functor_type = detail::layout_value_functor<self_type, inner_reference>;
        using const_functor_type = detail::layout_value_functor<const self_type, inner_const_reference>;

        using value_iterator = functor_index_iterator<functor_type>;
        using const_value_iterator = functor_index_iterator<const_functor_type>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    /**
     * @brief Type trait to check if a type is a timestamp_array.
     *
     * @tparam T Type to check
     */
    template <typename T>
    struct is_timestamp_array : std::false_type
    {
    };

    /**
     * @brief Specialization for timestamp_array types.
     *
     * @tparam T Timestamp type parameter
     */
    template <typename T>
    struct is_timestamp_array<timestamp_array<T>> : std::true_type
    {
    };

    /**
     * @brief Variable template for convenient access to is_timestamp_array.
     *
     * @tparam T Type to check
     */
    template <typename T>
    constexpr bool is_timestamp_array_v = is_timestamp_array<T>::value;

    /**
     * @brief Type aliases for common timestamp durations.
     */
    using timestamp_second = timestamp<std::chrono::seconds>;
    using timestamp_millisecond = timestamp<std::chrono::milliseconds>;
    using timestamp_microsecond = timestamp<std::chrono::microseconds>;
    using timestamp_nanosecond = timestamp<std::chrono::nanoseconds>;

    /**
     * @brief Type aliases for timestamp arrays with common durations.
     */
    using timestamp_seconds_array = timestamp_array<timestamp_second>;
    using timestamp_milliseconds_array = timestamp_array<timestamp_millisecond>;
    using timestamp_microseconds_array = timestamp_array<timestamp_microsecond>;
    using timestamp_nanoseconds_array = timestamp_array<timestamp_nanosecond>;

    namespace detail
    {
        template <>
        struct get_data_type_from_array<timestamp_seconds_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::TIMESTAMP_SECONDS;
            }
        };

        template <>
        struct get_data_type_from_array<timestamp_milliseconds_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::TIMESTAMP_MILLISECONDS;
            }
        };

        template <>
        struct get_data_type_from_array<timestamp_microseconds_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::TIMESTAMP_MICROSECONDS;
            }
        };

        template <>
        struct get_data_type_from_array<timestamp_nanoseconds_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::TIMESTAMP_NANOSECONDS;
            }
        };
    }

    /**
     * @brief Array implementation for storing timestamp values with timezone information.
     *
     * The timestamp_array provides efficient storage for datetime values with precise
     * duration types and timezone awareness. It stores timestamps as duration values
     * since the Unix epoch while maintaining timezone information for proper
     * interpretation and conversion.
     *
     * Key features:
     * - Timezone-aware timestamp storage
     * - Support for multiple precision levels (seconds to nanoseconds)
     * - Efficient columnar memory layout
     * - Arrow format compatibility
     * - Nullable timestamp support via validity bitmap
     * - Mutable operations (insert, erase, resize)
     *
     * The array internally stores:
     * - A validity bitmap for null/non-null timestamp tracking
     * - A contiguous buffer of duration values since Unix epoch
     * - Timezone pointer for proper timestamp interpretation
     *
     * Supported timestamp types:
     * - sparrow::timestamp<std::chrono::seconds>
     * - sparrow::timestamp<std::chrono::milliseconds>
     * - sparrow::timestamp<std::chrono::microseconds>
     * - sparrow::timestamp<std::chrono::nanoseconds>
     *
     * @tparam T The timestamp type with specific duration precision
     *
     * @pre T must satisfy the timestamp_type concept
     * @pre T must be one of the supported timestamp duration types
     * @post Maintains Arrow temporal format compatibility
     * @post All stored timestamps reference the same timezone
     * @post Thread-safe for read operations, requires external synchronization for writes
     *
     * @code{.cpp}
     * // Create timestamp array with New York timezone
     * const auto* ny_tz = date::locate_zone("America/New_York");
     * std::vector<timestamp_second> timestamps = {
     *     timestamp_second{ny_tz, std::chrono::sys_seconds{std::chrono::seconds{1609459200}}}, // 2021-01-01
     *     timestamp_second{ny_tz, std::chrono::sys_seconds{std::chrono::seconds{1609545600}}}  // 2021-01-02
     * };
     *
     * timestamp_seconds_array arr(ny_tz, timestamps);
     *
     * // Access timestamps
     * auto first = arr[0];
     * if (first.has_value()) {
     *     auto ts = first.value();  // timezone-aware timestamp
     * }
     * @endcode
     */
    template <timestamp_type T>
    class timestamp_array final : public mutable_array_bitmap_base<timestamp_array<T>>
    {
    public:

        using self_type = timestamp_array;
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

        using functor_type = typename inner_types::functor_type;
        using const_functor_type = typename inner_types::const_functor_type;

        using inner_value_type_duration = inner_value_type::duration;
        using buffer_inner_value_type = inner_value_type_duration::rep;
        using buffer_inner_value_iterator = pointer_iterator<buffer_inner_value_type*>;
        using buffer_inner_const_value_iterator = pointer_iterator<const buffer_inner_value_type*>;

        /**
         * @brief Constructs timestamp array from Arrow proxy.
         *
         * @param proxy Arrow proxy containing timestamp array data and schema
         *
         * @pre proxy must contain valid Arrow timestamp array and schema
         * @pre proxy format must match expected timestamp format for T
         * @pre proxy schema must include valid timezone information
         * @post Array is initialized with data from proxy
         * @post Timezone is extracted from schema metadata
         * @post Data access is properly configured for duration type
         */
        explicit timestamp_array(arrow_proxy);

        /**
         * @brief Generic constructor for creating timestamp arrays from various inputs.
         *
         * Creates a timestamp array from different input combinations including
         * timezone, values, validity information, and metadata. Arguments are
         * forwarded to compatible create_proxy() functions.
         *
         * The first argument must always be a const date::time_zone* pointer.
         * Subsequent arguments can include:
         * - Range of timestamp values
         * - Validity bitmap or boolean range
         * - Individual count and fill value
         * - Array name and metadata
         *
         * @tparam Args Parameter pack for constructor arguments
         * @param args Constructor arguments starting with timezone
         *
         * @pre First argument must be valid date::time_zone pointer
         * @pre Remaining arguments must match one of the create_proxy() signatures
         * @pre Args must exclude copy and move constructor signatures
         * @post Array is created with specified timezone and data
         * @post All timestamps reference the provided timezone
         *
         * @code{.cpp}
         * // Various construction patterns
         * const auto* utc = date::locate_zone("UTC");
         *
         * // From range with validity
         * std::vector<timestamp_second> values = {...};
         * std::vector<bool> validity = {...};
         * timestamp_seconds_array arr1(utc, values, validity);
         *
         * // From range, nullable
         * timestamp_seconds_array arr2(utc, values, true);
         *
         * // From count and fill value
         * timestamp_seconds_array arr3(utc, 100, timestamp_second{...});
         * @endcode
         */
        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<timestamp_array, Args...>)
        constexpr explicit timestamp_array(Args&&... args)
            : base_type(create_proxy(std::forward<Args>(args)...))
            , m_timezone(get_timezone(this->get_arrow_proxy()))
            , m_data_access(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
        {
        }

        /**
         * @brief Constructs timestamp array from initializer list.
         *
         * @tparam METADATA_RANGE Type of metadata container
         * @param timezone Timezone for all timestamps in the array
         * @param init Initializer list of timestamp values
         * @param name Optional name for the array
         * @param metadata Optional metadata key-value pairs
         *
         * @pre timezone must be a valid date::time_zone pointer
         * @pre All timestamps in init should reference the same timezone
         * @post Array contains timestamps from initializer list
         * @post All timestamps use the specified timezone
         */
        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        constexpr timestamp_array(
            const date::time_zone* timezone,
            std::initializer_list<inner_value_type> init,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
            : base_type(create_proxy(timezone, init, std::move(name), std::move(metadata)))
            , m_timezone(timezone)
            , m_data_access(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
        {
        }

        /**
         * @brief Copy constructor.
         *
         * @param rhs Source timestamp array to copy from
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post Timezone pointer is copied (shared reference)
         * @post Data access is reinitialized for this array
         */
        constexpr timestamp_array(const timestamp_array& rhs);

        /**
         * @brief Copy assignment operator.
         *
         * @param rhs Source timestamp array to copy from
         * @return Reference to this array
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post Previous data is properly released
         * @post Timezone and data access are updated
         */
        constexpr timestamp_array& operator=(const timestamp_array& rhs);

        /**
         * @brief Move constructor.
         *
         * @param rhs Source timestamp array to move from
         *
         * @post This array takes ownership of rhs data
         * @post rhs is left in a valid but unspecified state
         * @post Timezone and data access are properly transferred
         */
        constexpr timestamp_array(timestamp_array&& rhs);

        /**
         * @brief Move assignment operator.
         *
         * @param rhs Source timestamp array to move from
         * @return Reference to this array
         *
         * @post This array takes ownership of rhs data
         * @post Previous data is properly released
         * @post rhs is left in a valid but unspecified state
         */
        constexpr timestamp_array& operator=(timestamp_array&& rhs);

    private:

        /**
         * @brief Gets mutable reference to timestamp at specified index.
         *
         * @param i Index of the timestamp to access
         * @return Mutable reference to the timestamp
         *
         * @pre i must be < size()
         * @post Returns valid reference for timestamp modification
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(i < this->size())
         */
        [[nodiscard]] constexpr inner_reference value(size_type i);

        /**
         * @brief Gets const reference to timestamp at specified index.
         *
         * @param i Index of the timestamp to access
         * @return Const timestamp value with timezone information
         *
         * @pre i must be < size()
         * @post Returns timestamp constructed from stored duration and timezone
         * @post Returned timestamp reflects the array's timezone setting
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(i < this->size())
         */
        [[nodiscard]] constexpr inner_const_reference value(size_type i) const;

        /**
         * @brief Gets iterator to beginning of value range.
         *
         * @return Iterator pointing to first timestamp
         *
         * @post Iterator is valid for timestamp traversal
         */
        [[nodiscard]] constexpr value_iterator value_begin();

        /**
         * @brief Gets iterator to end of value range.
         *
         * @return Iterator pointing past last timestamp
         *
         * @post Iterator marks end of timestamp range
         */
        [[nodiscard]] constexpr value_iterator value_end();

        /**
         * @brief Gets const iterator to beginning of value range.
         *
         * @return Const iterator pointing to first timestamp
         *
         * @post Iterator is valid for timestamp traversal
         */
        [[nodiscard]] constexpr const_value_iterator value_cbegin() const;

        /**
         * @brief Gets const iterator to end of value range.
         *
         * @return Const iterator pointing past last timestamp
         *
         * @post Iterator marks end of timestamp range
         */
        [[nodiscard]] constexpr const_value_iterator value_cend() const;

        /// \cond PRIVATE_DOCS

        /**
         * @brief Creates Arrow proxy with specified count of default-initialized timestamp values.
         *
         * Creates a timestamp array proxy with n elements, each initialized to the default
         * timestamp value (Unix epoch: 1970-01-01 00:00:00 UTC). This is useful for creating
         * arrays that will be populated later or for allocating space with a known baseline.
         *
         * @tparam METADATA_RANGE Type of metadata container
         * @param timezone Timezone for interpreting all timestamp elements
         * @param n Number of elements to create
         * @param name Optional name for the array column
         * @param metadata Optional metadata key-value pairs
         * @return Arrow proxy containing n default-initialized timestamp elements
         *
         * @pre timezone must be a valid date::time_zone pointer
         * @pre n must be >= 0
         * @post Returns proxy with n timestamp elements
         * @post All elements contain Unix epoch duration (zero)
         * @post No validity bitmap (all elements considered valid)
         * @post All elements use the specified timezone for interpretation
         *
         * @note Efficient for creating large arrays that will be populated later
         * @note All elements start with the same baseline value (Unix epoch)
         * @note No null values - use other overloads if nullability is needed
         * @note Elements can be modified after array construction
         *
         * @code{.cpp}
         * // Create array of 1000 timestamps initialized to Unix epoch
         * const auto* utc = date::locate_zone("UTC");
         * auto proxy = timestamp_seconds_array::create_proxy(utc, 1000);
         *
         * // All elements will represent 1970-01-01 00:00:00 UTC
         * timestamp_seconds_array arr(std::move(proxy));
         * // Elements can be modified: arr[0] = some_timestamp;
         * @endcode
         */
        template <input_metadata_container METADATA_RANGE>
        [[nodiscard]] static arrow_proxy create_proxy(
            const date::time_zone* timezone,
            size_type n,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        //
        /**
         * @brief Creates Arrow proxy from pre-allocated data buffer and validity bitmap.
         *
         * Creates a timestamp array proxy from an existing data buffer containing duration
         * values and an optional validity bitmap. This is the most direct way to create
         * a timestamp array when you have pre-processed duration data.
         *
         * @tparam R Validity bitmap input type
         * @tparam METADATA_RANGE Type of metadata container
         * @param timezone Timezone for interpreting stored duration values
         * @param data_buffer Buffer containing duration values since Unix epoch
         * @param bitmaps Validity bitmap or input to create validity bitmap
         * @param name Optional name for the array column
         * @param metadata Optional metadata key-value pairs
         * @return Arrow proxy containing timestamp array with specified data
         *
         * @pre timezone must be a valid date::time_zone pointer
         * @pre data_buffer must contain valid duration values
         * @pre If provided, bitmaps must match data_buffer size
         * @post Returns proxy with timestamps constructed from data_buffer values
         * @post Validity information is properly configured from bitmaps
         * @post All timestamps use the specified timezone for interpretation
         *
         * @note Duration values are interpreted as time since Unix epoch
         * @note This method takes ownership of the data_buffer
         * @note Validity bitmap is ensured to match array size if provided
         */
        template <
            validity_bitmap_input R = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static auto create_proxy(
            const date::time_zone* timezone,
            u8_buffer<buffer_inner_value_type>&& data_buffer,
            R&& bitmaps = validity_bitmap{},
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        /**
         * @brief Creates Arrow proxy from range of timestamp values (no missing values).
         *
         * Creates a timestamp array proxy from a range of timestamp objects. This method
         * extracts duration components from timestamps and creates a dense array without
         * null values (unless nullable=true, which adds an empty validity bitmap).
         *
         * @tparam R Range type containing convertible timestamp values
         * @tparam METADATA_RANGE Type of metadata container
         * @param timezone Timezone for all timestamps (should match range values)
         * @param range Input range of timestamp values
         * @param nullable Whether to create validity bitmap (empty if true)
         * @param name Optional name for the array column
         * @param metadata Optional metadata key-value pairs
         * @return Arrow proxy containing timestamp array from range
         *
         * @pre timezone must be a valid date::time_zone pointer
         * @pre Range elements must be convertible to timestamp type T
         * @pre All timestamps in range should reference compatible timezone
         * @post Returns proxy with timestamps from range
         * @post Duration values are extracted and stored efficiently
         * @post If nullable=true, empty validity bitmap is created
         * @post If nullable=false, no validity bitmap (all values valid)
         *
         * @note Timezone compatibility is not enforced but recommended
         * @note Duration extraction preserves precision of timestamp type
         * @note This is optimal for dense timestamp data without nulls
         */
        template <std::ranges::input_range R, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<std::ranges::range_value_t<R>, T>
        [[nodiscard]] static auto create_proxy(
            const date::time_zone* timezone,
            R&& range,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        /**
         * @brief Creates Arrow proxy from value range and separate validity information.
         *
         * Creates a timestamp array proxy from a range of timestamp values and separate
         * validity information. This allows precise control over which elements are null
         * while providing timestamp values for non-null elements.
         *
         * @tparam VALUE_RANGE Range type containing timestamp values
         * @tparam VALIDITY_RANGE Range or bitmap type for validity information
         * @tparam METADATA_RANGE Type of metadata container
         * @param timezone Timezone for interpreting timestamp values
         * @param values Range of timestamp values (for non-null positions)
         * @param validity Validity bitmap or boolean range
         * @param name Optional name for the array column
         * @param metadata Optional metadata key-value pairs
         * @return Arrow proxy containing timestamp array with validity info
         *
         * @pre timezone must be a valid date::time_zone pointer
         * @pre VALUE_RANGE elements must be convertible to timestamp type T
         * @pre validity must provide validity information for each element
         * @pre Both ranges must have compatible sizes
         * @post Returns proxy with timestamps and validity bitmap
         * @post Null positions are marked according to validity input
         * @post Non-null positions contain duration values from range
         * @post All timestamps use the specified timezone
         *
         * @note Provides fine-grained control over null/non-null elements
         * @note Values range should provide meaningful data for non-null positions
         * @note Validity information determines the final null count
         */
        template <
            std::ranges::input_range VALUE_RANGE,
            validity_bitmap_input VALIDITY_RANGE,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(std::convertible_to<std::ranges::range_value_t<VALUE_RANGE>, T>)
        [[nodiscard]] static arrow_proxy create_proxy(
            const date::time_zone* timezone,
            VALUE_RANGE&&,
            VALIDITY_RANGE&&,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates Arrow proxy from range of nullable timestamp values.
         *
         * Creates a timestamp array proxy from a range of nullable timestamp objects.
         * Each element in the range specifies both its value (if non-null) and whether
         * it should be considered null, providing a convenient single-range interface.
         *
         * @tparam R Range type containing nullable<T> elements
         * @tparam METADATA_RANGE Type of metadata container
         * @param timezone Timezone for interpreting non-null timestamp values
         * @param range Range of nullable timestamp values
         * @param name Optional name for the array column
         * @param metadata Optional metadata key-value pairs
         * @return Arrow proxy containing timestamp array from nullable range
         *
         * @pre timezone must be a valid date::time_zone pointer
         * @pre Range elements must be of type nullable<T>
         * @pre Non-null timestamps should use compatible timezone
         * @post Returns proxy with timestamps and validity from nullable range
         * @post Validity bitmap reflects has_value() state of each element
         * @post Non-null elements contain duration values from get() calls
         * @post All timestamps use the specified timezone
         *
         * @note Convenient for ranges where null status is embedded in values
         * @note Automatically separates values and validity information
         * @note Handles mixed null/non-null data efficiently
         * @note Null elements don't require meaningful timestamp values
         */
        template <std::ranges::input_range R, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::is_same_v<std::ranges::range_value_t<R>, nullable<T>>
        [[nodiscard]] static arrow_proxy create_proxy(
            const date::time_zone* timezone,
            R&&,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Core implementation for creating Arrow proxy from processed data.
         *
         * This is the fundamental implementation that all other create_proxy methods
         * ultimately call. It creates a complete Arrow proxy with schema and array
         * from pre-processed duration data and validity information.
         *
         * The method handles:
         * - Arrow schema creation with proper timestamp format and timezone
         * - Arrow array creation with validity bitmap and duration data
         * - Proper buffer management and ownership transfer
         * - Metadata and naming integration
         * - Null count calculation and flag setting
         *
         * @tparam METADATA_RANGE Type of metadata container
         * @param timezone Timezone for timestamp interpretation (embedded in schema)
         * @param data_buffer Buffer containing duration values since Unix epoch
         * @param bitmap Optional validity bitmap (null if all values valid)
         * @param name Optional name for the array column
         * @param metadata Optional metadata key-value pairs
         * @return Complete Arrow proxy ready for timestamp_array construction
         *
         * @pre timezone must be a valid date::time_zone pointer
         * @pre data_buffer must contain valid duration values
         * @pre If bitmap provided, must match data_buffer size
         * @pre Metadata must be valid key-value pairs if provided
         * @post Returns complete Arrow proxy with schema and array
         * @post Schema includes timezone information in format string
         * @post Array buffers contain validity bitmap and duration data
         * @post Proper null count and nullable flags are set
         * @post All buffer ownership is properly transferred
         *
         * @note This is the core implementation used by all create_proxy overloads
         * @note Creates Arrow-compatible timestamp format with timezone suffix
         * @note Handles buffer ownership transfer to prevent memory leaks
         * @note Schema format follows Arrow timestamp specification
         * @note Supports both nullable and non-nullable configurations
         *
         * @see Arrow timestamp format specification
         * @see ArrowSchema and ArrowArray creation utilities
         */
        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy_impl(
            const date::time_zone* timezone,
            u8_buffer<buffer_inner_value_type>&& data_buffer,
            std::optional<validity_bitmap>&& bitmap,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /// \endcond

        // Modifiers

        /**
         * @brief Resizes the array to specified length, filling with given timestamp.
         *
         * @param new_length New size for the array
         * @param value Timestamp value to use for new elements (if growing)
         *
         * @post Array size equals new_length
         * @post If shrinking, trailing elements are removed
         * @post If growing, new elements are set to value's duration
         * @post All timestamps maintain the array's timezone
         */
        constexpr void resize_values(size_type new_length, inner_value_type value);

        /**
         * @brief Inserts copies of a timestamp at specified position.
         *
         * @param pos Iterator position where to insert
         * @param value Timestamp value to insert
         * @param count Number of copies to insert
         * @return Iterator pointing to first inserted timestamp
         *
         * @pre pos must be valid iterator within [value_cbegin(), value_cend()]
         * @pre value should use the same timezone as the array
         * @post count copies of value are inserted at pos
         * @post Array size increases by count
         * @post Inserted timestamps use value's duration component
         */
        constexpr value_iterator
        insert_value(const_value_iterator pos, inner_value_type value, size_type count);

        /**
         * @brief Inserts range of timestamps at specified position.
         *
         * @tparam InputIt Iterator type for timestamps (must yield inner_value_type)
         * @param pos Position where to insert
         * @param first Beginning of range to insert
         * @param last End of range to insert
         * @return Iterator pointing to first inserted timestamp
         *
         * @pre InputIt must yield elements of type inner_value_type
         * @pre pos must be valid value iterator
         * @pre [first, last) must be valid range
         * @pre Timestamps in range should use compatible timezone
         * @post Timestamps from [first, last) are inserted at pos
         * @post Array size increases by distance(first, last)
         * @post Duration components are extracted and stored
         */
        template <mpl::iterator_of_type<typename timestamp_array<T>::inner_value_type> InputIt>
        constexpr auto insert_values(const_value_iterator pos, InputIt first, InputIt last) -> value_iterator
        {
            const auto input_range = std::ranges::subrange(first, last);
            const auto values = input_range
                                | std::views::transform(
                                    [](const auto& v)
                                    {
                                        return v.get_sys_time().time_since_epoch();
                                    }
                                );
            const size_t idx = static_cast<size_t>(std::distance(value_cbegin(), pos));
            m_data_access.insert_values(idx, values.begin(), values.end());
            return sparrow::next(value_begin(), idx);
        }

        /**
         * @brief Erases specified number of timestamps starting at position.
         *
         * @param pos Position where to start erasing
         * @param count Number of timestamps to erase
         * @return Iterator pointing to timestamp after last erased
         *
         * @pre pos must be valid value iterator
         * @pre pos must be < value_cend()
         * @pre pos + count must not exceed array bounds
         * @post count timestamps are removed starting at pos
         * @post Array size decreases by count
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(pos < value_cend())
         */
        constexpr value_iterator erase_values(const_value_iterator pos, size_type count);

        /**
         * @brief Assigns new timestamp value to element at specified index.
         *
         * @param rhs Timestamp value to assign
         * @param index Index of element to modify
         *
         * @pre index must be < size()
         * @pre rhs should use compatible timezone
         * @post Element at index contains duration from rhs
         * @post Timezone interpretation remains unchanged
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(index < this->size())
         */
        constexpr void assign(const T& rhs, size_type index);

        /**
         * @brief Assigns new timestamp value (by move) to element at specified index.
         *
         * @param rhs Timestamp value to assign (moved)
         * @param index Index of element to modify
         *
         * @pre index must be < size()
         * @pre rhs should use compatible timezone
         * @post Element at index contains duration from rhs
         * @post Timezone interpretation remains unchanged
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(index < this->size())
         */
        constexpr void assign(T&& rhs, size_type index);


        const date::time_zone* m_timezone;  ///< Timezone for interpreting stored durations
        details::primitive_data_access<inner_value_type_duration> m_data_access;  ///< Access to duration data

        static constexpr size_type DATA_BUFFER_INDEX = 1;  ///< Index of data buffer in Arrow array
        friend class timestamp_reference<self_type>;
        friend base_type;
        friend base_type::base_type;
        friend base_type::base_type::base_type;
        friend functor_type;
        friend const_functor_type;
    };

    template <timestamp_type T>
    constexpr timestamp_array<T>::timestamp_array(const timestamp_array& rhs)
        : base_type(rhs)
        , m_timezone(rhs.m_timezone)
        , m_data_access(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
    {
    }

    template <timestamp_type T>
    constexpr timestamp_array<T>& timestamp_array<T>::operator=(const timestamp_array& rhs)
    {
        base_type::operator=(rhs);
        m_timezone = rhs.m_timezone;
        m_data_access.reset_proxy(this->get_arrow_proxy());
        return *this;
    }

    template <timestamp_type T>
    constexpr timestamp_array<T>::timestamp_array(timestamp_array&& rhs)
        : base_type(std::move(rhs))
        , m_timezone(rhs.m_timezone)
        , m_data_access(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
    {
    }

    template <timestamp_type T>
    constexpr timestamp_array<T>& timestamp_array<T>::operator=(timestamp_array&& rhs)
    {
        base_type::operator=(std::move(rhs));
        m_timezone = rhs.m_timezone;
        m_data_access.reset_proxy(this->get_arrow_proxy());
        return *this;
    }

    template <timestamp_type T>
    timestamp_array<T>::timestamp_array(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , m_timezone(get_timezone(this->get_arrow_proxy()))
        , m_data_access(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
    {
    }

    template <timestamp_type T>
    template <validity_bitmap_input R, input_metadata_container METADATA_RANGE>
    auto timestamp_array<T>::create_proxy(
        const date::time_zone* timezone,
        u8_buffer<buffer_inner_value_type>&& data_buffer,
        R&& bitmap_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        const auto size = data_buffer.size();
        validity_bitmap bitmap = ensure_validity_bitmap(size, std::forward<R>(bitmap_input));
        return create_proxy_impl(
            timezone,
            std::forward<u8_buffer<buffer_inner_value_type>>(data_buffer),
            std::move(bitmap),
            std::move(name),
            std::move(metadata)
        );
    }

    template <timestamp_type T>
    template <std::ranges::input_range VALUE_RANGE, validity_bitmap_input VALIDITY_RANGE, input_metadata_container METADATA_RANGE>
        requires(std::convertible_to<std::ranges::range_value_t<VALUE_RANGE>, T>)
    arrow_proxy timestamp_array<T>::create_proxy(
        const date::time_zone* timezone,
        VALUE_RANGE&& values,
        VALIDITY_RANGE&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto range = values
                           | std::views::transform(
                               [](const auto& v)
                               {
                                   return v.get_sys_time().time_since_epoch().count();
                               }
                           );


        u8_buffer<buffer_inner_value_type> data_buffer(range);
        return create_proxy(
            timezone,
            std::move(data_buffer),
            std::forward<VALIDITY_RANGE>(validity_input),
            std::move(name),
            std::move(metadata)
        );
    }

    template <timestamp_type T>
    template <typename U, input_metadata_container METADATA_RANGE>
        requires std::convertible_to<U, T>
    arrow_proxy timestamp_array<T>::create_proxy(
        const date::time_zone* timezone,
        size_type n,
        const U& value,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        // create data_buffer
        u8_buffer<buffer_inner_value_type> data_buffer(n, to_days_since_the_UNIX_epoch(value));
        return create_proxy(timezone, std::move(data_buffer), std::move(name), std::move(metadata));
    }

    template <timestamp_type T>
    template <std::ranges::input_range R, input_metadata_container METADATA_RANGE>
        requires std::convertible_to<std::ranges::range_value_t<R>, T>
    arrow_proxy timestamp_array<T>::create_proxy(
        const date::time_zone* timezone,
        R&& range,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        std::optional<validity_bitmap> bitmap = nullable ? std::make_optional<validity_bitmap>(nullptr, 0)
                                                         : std::nullopt;
        const auto values = range
                            | std::views::transform(
                                [](const auto& v)
                                {
                                    return v.get_sys_time().time_since_epoch().count();
                                }
                            );
        u8_buffer<buffer_inner_value_type> data_buffer(values);
        return self_type::create_proxy_impl(
            timezone,
            std::move(data_buffer),
            std::move(bitmap),
            std::move(name),
            std::move(metadata)
        );
    }

    // range of nullable values
    template <timestamp_type T>
    template <std::ranges::input_range R, input_metadata_container METADATA_RANGE>
        requires std::is_same_v<std::ranges::range_value_t<R>, nullable<T>>
    arrow_proxy timestamp_array<T>::create_proxy(
        const date::time_zone* timezone,
        R&& range,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {  // split into values and is_non_null ranges
        auto values = range
                      | std::views::transform(
                          [](const auto& v)
                          {
                              return v.get();
                          }
                      );
        auto is_non_null = range
                           | std::views::transform(
                               [](const auto& v)
                               {
                                   return v.has_value();
                               }
                           );
        return self_type::create_proxy(timezone, values, is_non_null, std::move(name), std::move(metadata));
    }

    template <timestamp_type T>
    template <input_metadata_container METADATA_RANGE>
    arrow_proxy timestamp_array<T>::create_proxy_impl(
        const date::time_zone* timezone,
        u8_buffer<buffer_inner_value_type>&& data_buffer,
        std::optional<validity_bitmap>&& bitmap,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto size = data_buffer.size();
        const auto null_count = bitmap.has_value() ? bitmap->null_count() : 0;

        std::string format(data_type_to_format(detail::get_data_type_from_array<self_type>::get()));
        format += timezone->name();

        const repeat_view<bool> children_ownership{true, 0};

        const std::optional<std::unordered_set<sparrow::ArrowFlag>>
            flags = bitmap.has_value()
                        ? std::make_optional<std::unordered_set<sparrow::ArrowFlag>>({ArrowFlag::NULLABLE})
                        : std::nullopt;

        // create arrow schema and array
        ArrowSchema schema = make_arrow_schema(
            std::move(format),    // format
            std::move(name),      // name
            std::move(metadata),  // metadata
            flags,                // flags
            nullptr,              // children
            children_ownership,   // children ownership
            nullptr,              // dictionary,
            true                  // dictionary ownership
        );

        std::vector<buffer<uint8_t>> buffers{
            bitmap.has_value() ? std::move(bitmap.value()).extract_storage() : buffer<uint8_t>{nullptr, 0},
            std::move(data_buffer).extract_storage()
        };

        // create arrow array
        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(buffers),
            nullptr,             // children
            children_ownership,  // children ownership
            nullptr,             // dictionary
            true                 // dicitonary ownership
        );
        return arrow_proxy(std::move(arr), std::move(schema));
    }

    template <timestamp_type T>
    constexpr void timestamp_array<T>::assign(const T& rhs, size_type index)
    {
        SPARROW_ASSERT_TRUE(index < this->size());
        m_data_access.value(index) = rhs.get_sys_time().time_since_epoch();
    }

    template <timestamp_type T>
    constexpr void timestamp_array<T>::assign(T&& rhs, size_type index)
    {
        SPARROW_ASSERT_TRUE(index < this->size());
        m_data_access.value(index) = rhs.get_sys_time().time_since_epoch();
    }

    template <timestamp_type T>
    constexpr auto timestamp_array<T>::value(size_type i) -> inner_reference
    {
        SPARROW_ASSERT_TRUE(i < this->size());
        return inner_reference(this, i);
    }

    template <timestamp_type T>
    constexpr auto timestamp_array<T>::value(size_type i) const -> inner_const_reference
    {
        SPARROW_ASSERT_TRUE(i < this->size());
        const auto& val = m_data_access.value(i);
        using time_duration = typename T::duration;
        const auto sys_time = std::chrono::sys_time<time_duration>{val};
        return T{m_timezone, sys_time};
    }

    template <timestamp_type T>
    constexpr auto timestamp_array<T>::value_begin() -> value_iterator
    {
        return value_iterator(functor_type(this), 0);
    }

    template <timestamp_type T>
    constexpr auto timestamp_array<T>::value_end() -> value_iterator
    {
        return value_iterator(functor_type(this), this->size());
    }

    template <timestamp_type T>
    constexpr auto timestamp_array<T>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator(const_functor_type(this), 0);
    }

    template <timestamp_type T>
    constexpr auto timestamp_array<T>::value_cend() const -> const_value_iterator
    {
        return const_value_iterator(const_functor_type(this), this->size());
    }

    template <timestamp_type T>
    constexpr void timestamp_array<T>::resize_values(size_type new_length, inner_value_type value)
    {
        m_data_access.resize_values(new_length, value.get_sys_time().time_since_epoch());
    }

    template <timestamp_type T>
    constexpr auto
    timestamp_array<T>::insert_value(const_value_iterator pos, inner_value_type value, size_type count)
        -> value_iterator
    {
        SPARROW_ASSERT_TRUE(pos <= value_cend());
        const size_t idx = static_cast<size_t>(std::distance(value_cbegin(), pos));
        m_data_access.insert_value(idx, value.get_sys_time().time_since_epoch(), count);
        return value_iterator(functor_type(this), idx);
    }

    template <timestamp_type T>
    constexpr auto timestamp_array<T>::erase_values(const_value_iterator pos, size_type count) -> value_iterator
    {
        SPARROW_ASSERT_TRUE(pos < value_cend());
        const size_t idx = static_cast<size_t>(std::distance(value_cbegin(), pos));
        m_data_access.erase_values(idx, count);
        return value_iterator(functor_type(this), idx);
    }
}
