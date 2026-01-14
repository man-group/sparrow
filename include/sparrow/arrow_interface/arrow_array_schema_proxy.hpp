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

#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_set>

#include "sparrow/arrow_interface/arrow_array/private_data.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_info_utils.hpp"
#include "sparrow/arrow_interface/arrow_schema/private_data.hpp"
#include "sparrow/buffer/buffer_view.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset_view.hpp"
#include "sparrow/buffer/dynamic_bitset/non_owning_dynamic_bitset.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/config/config.hpp"
#include "sparrow/types/data_type.hpp"
#include "sparrow/utils/metadata.hpp"
#include "sparrow/utils/ranges.hpp"

namespace sparrow
{
    /**
     * @brief Exception thrown by arrow_proxy operations.
     *
     * This exception is thrown when arrow_proxy operations fail due to invalid
     * state, ownership violations, or incompatible Arrow structures.
     */
    class arrow_proxy_exception : public std::runtime_error
    {
    public:

        /**
         * @brief Constructs an arrow_proxy_exception with a descriptive message.
         *
         * @param message Description of the error condition
         */
        explicit arrow_proxy_exception(const std::string& message)
            : std::runtime_error(message)
        {
        }
    };

    struct arrow_array_and_schema_pointers
    {
        ArrowArray* array;
        ArrowSchema* schema;
    };

    struct arrow_array_and_schema
    {
        ArrowArray array;
        ArrowSchema schema;
    };

    /**
     * @brief Proxy class providing a user-friendly interface over ArrowArray and ArrowSchema.
     *
     * The arrow_proxy class simplifies working with Apache Arrow C data interface structures
     * by providing a modern C++ interface with automatic memory management, type safety,
     * and convenient access methods. It can either take ownership of Arrow structures or
     * work with external references, making it suitable for various integration scenarios.
     *
     * Key features:
     * - Automatic memory management with clear ownership semantics
     * - Type-safe access to Arrow data and metadata
     * - Support for nested structures (children, dictionaries)
     * - Efficient buffer management and manipulation
     * - Validation and consistency checking
     * - Slicing and view operations
     *
     * @pre Arrow structures must be valid and properly initialized
     * @pre Owned structures are automatically released on destruction
     * @post Maintains Arrow C data interface compatibility
     * @post Provides thread-safe read access, requires external synchronization for writes
     * @post Preserves data integrity through validation and bounds checking
     *
     * @example
     * ```cpp
     * // Taking ownership of Arrow structures
     * ArrowArray array = create_arrow_array();
     * ArrowSchema schema = create_arrow_schema();
     * arrow_proxy proxy(std::move(array), std::move(schema));
     *
     * // Working with external references
     * arrow_proxy view_proxy(&external_array, &external_schema);
     *
     * // Accessing data
     * auto buffers = proxy.buffers();
     * auto data_type = proxy.data_type();
     * auto slice = proxy.slice(10, 20);
     * ```
     */
    class arrow_proxy
    {
    public:

        /**
         * @brief Constructs an arrow_proxy taking ownership of both ArrowArray and ArrowSchema.
         *
         * @param array ArrowArray to take ownership of
         * @param schema ArrowSchema to take ownership of
         *
         * @pre array and schema must be valid Arrow structures
         * @pre array and schema must be compatible (matching data types)
         * @post This proxy owns both structures and will release them on destruction
         * @post Buffers, children, and dictionary are properly initialized
         * @post Internal consistency is validated
         */
        SPARROW_API explicit arrow_proxy(ArrowArray&& array, ArrowSchema&& schema);

        /**
         * @brief Constructs an arrow_proxy taking ownership of ArrowArray, referencing ArrowSchema.
         *
         * @param array ArrowArray to take ownership of
         * @param schema Pointer to ArrowSchema (not owned)
         *
         * @pre array must be a valid Arrow structure
         * @pre schema must be a valid pointer to ArrowSchema
         * @pre array and schema must be compatible
         * @post This proxy owns the array but not the schema
         * @post Schema must remain valid for the lifetime of this proxy
         * @post Buffers and children are properly initialized
         */
        SPARROW_API explicit arrow_proxy(ArrowArray&& array, ArrowSchema* schema);

        /**
         * @brief Constructs an arrow_proxy taking ownership of ArrowArray, referencing const ArrowSchema.
         *
         * @param array ArrowArray to take ownership of
         * @param schema Pointer to ArrowSchema (not owned)
         *
         * @pre array must be a valid Arrow structure
         * @pre schema must be a valid const pointer to ArrowSchema
         * @pre array and schema must be compatible
         * @post This proxy owns the array but not the schema
         * @post Schema must remain valid for the lifetime of this proxy
         * @post Buffers and children are properly initialized
         */
        SPARROW_API explicit arrow_proxy(ArrowArray&& array, const ArrowSchema* schema);

        /**
         * @brief Constructs an arrow_proxy referencing external ArrowArray and ArrowSchema.
         *
         * @param array Pointer to ArrowArray (not owned)
         * @param schema Pointer to ArrowSchema (not owned)
         *
         * @pre array must be a valid pointer to ArrowArray
         * @pre schema must be a valid pointer to ArrowSchema
         * @pre array and schema must be compatible
         * @pre External structures must remain valid for the lifetime of this proxy
         * @post This proxy does not own either structure
         * @post External structures must be managed by the caller
         * @post Buffers and children are properly initialized as views
         */
        SPARROW_API explicit arrow_proxy(ArrowArray* array, ArrowSchema* schema);

        /**
         * @brief Constructs an arrow_proxy referencing external const ArrowArray and const ArrowSchema.
         *
         * @param array Const pointer to ArrowArray (not owned)
         * @param schema Const pointer to ArrowSchema (not owned)
         *
         * @pre array must be a valid pointer to ArrowArray
         * @pre schema must be a valid pointer to ArrowSchema
         * @pre array and schema must be compatible
         * @pre External structures must remain valid for the lifetime of this proxy
         * @post This proxy does not own either structure
         * @post External structures must be managed by the caller
         * @post Buffers and children are properly initialized as views
         */
        SPARROW_API explicit arrow_proxy(const ArrowArray* array, const ArrowSchema* schema);

        /**
         * @brief Copy constructor creating independent copy.
         *
         * @param other Source arrow_proxy to copy from
         *
         * @pre other must be in a valid state
         * @post This proxy is an independent copy of other
         * @post If other owns structures, this proxy owns copies
         * @post If other references external structures, this proxy references the same
         * @post Deep copy of all children and dictionary (if owned)
         */
        SPARROW_API arrow_proxy(const arrow_proxy& other);

        /**
         * @brief Copy assignment operator.
         *
         * @param other Source arrow_proxy to copy from
         * @return Reference to this arrow_proxy
         *
         * @pre other must be in a valid state
         * @post This proxy is an independent copy of other
         * @post Previous structures are properly released (if owned)
         * @post Deep copy of all children and dictionary (if owned)
         */
        SPARROW_API arrow_proxy& operator=(const arrow_proxy& other);

        /**
         * @brief Move constructor transferring ownership.
         *
         * @param other Source arrow_proxy to move from
         *
         * @pre other must be in a valid state
         * @post This proxy takes ownership from other
         * @post other is left in a valid but unspecified state
         * @post No copying of Arrow structures occurs
         */
        SPARROW_API arrow_proxy(arrow_proxy&& other) noexcept;

        /**
         * @brief Move assignment operator.
         *
         * @param other Source arrow_proxy to move from
         * @return Reference to this arrow_proxy
         *
         * @pre other must be in a valid state
         * @post This proxy takes ownership from other
         * @post Previous structures are properly released (if owned)
         * @post other is left in a valid but unspecified state
         */
        SPARROW_API arrow_proxy& operator=(arrow_proxy&& other) noexcept;

        /**
         * @brief Destructor releasing owned Arrow structures.
         *
         * @post All owned Arrow structures are properly released
         * @post Referenced external structures are not affected
         * @post Children and dictionary are recursively released (if owned)
         */
        SPARROW_API ~arrow_proxy();

        /**
         * @brief Gets the Arrow format string describing the data type.
         *
         * @return String view of the format specification
         *
         * @post Returns valid format string according to Arrow specification
         * @post Format string remains valid while proxy exists
         */
        [[nodiscard]] SPARROW_API const std::string_view format() const;

        /**
         * @brief Sets the Arrow format string.
         *
         * @param format New format string to set
         *
         * @pre ArrowSchema must be created with sparrow (owned by this proxy)
         * @pre format must be a valid Arrow format string
         * @post Schema format is updated to the specified value
         * @post Data type is updated accordingly
         *
         * @throws arrow_proxy_exception if schema is not owned by sparrow
         */
        SPARROW_API void set_format(const std::string_view format);

        /**
         * @brief Gets the data type enum corresponding to the format.
         *
         * @return Data type enumeration value
         *
         * @post Returns data type consistent with format string
         */
        [[nodiscard]] SPARROW_API enum data_type data_type() const;

        /**
         * @brief Sets the data type (updates format string accordingly).
         *
         * @param data_type New data type to set
         *
         * @pre ArrowSchema must be created with sparrow (owned by this proxy)
         * @pre data_type must be a valid sparrow data type
         * @post Schema format is updated to match the data type
         * @post Format string is updated accordingly
         *
         * @throws arrow_proxy_exception if schema is not owned by sparrow
         */
        void SPARROW_API set_data_type(enum data_type data_type);

        /**
         * @brief Gets the optional name of the array/field.
         *
         * @return Optional string view of the name
         *
         * @post Returns nullopt if no name is set
         * @post Returned string view remains valid while proxy exists
         */
        [[nodiscard]] SPARROW_API std::optional<std::string_view> name() const;

        /**
         * @brief Sets the name of the array/field.
         *
         * @param name Optional name to set (nullopt to clear)
         *
         * @pre ArrowSchema must be created with sparrow (owned by this proxy)
         * @post Schema name is updated to the specified value
         * @post Name can be retrieved via name() method
         *
         * @throws arrow_proxy_exception if schema is not owned by sparrow
         */
        SPARROW_API void set_name(std::optional<std::string_view> name);

        /**
         * @brief Gets the metadata key-value pairs.
         *
         * @return Optional view of metadata key-value pairs
         *
         * @post Returns nullopt if no metadata is set
         * @post Returned view remains valid while proxy exists
         */
        [[nodiscard]] SPARROW_API std::optional<key_value_view> metadata() const;

        /**
         * @brief Sets the metadata key-value pairs.
         *
         * @tparam R Container type for metadata pairs
         * @param metadata Optional metadata container (nullopt to clear)
         *
         * @pre ArrowSchema must be created with sparrow (owned by this proxy)
         * @pre R must satisfy input_metadata_container concept
         * @post Schema metadata is updated to the specified values
         * @post Metadata can be retrieved via metadata() method
         *
         * @throws arrow_proxy_exception if schema is not owned by sparrow
         */
        template <input_metadata_container R>
        void set_metadata(std::optional<R> metadata)
        {
            if (!schema_created_with_sparrow())
            {
                throw arrow_proxy_exception("Cannot set metadata on non-sparrow created ArrowArray");
            }
            auto private_data = get_schema_private_data();
            if (!metadata.has_value())
            {
                private_data->metadata() = std::nullopt;
            }
            else
            {
                private_data->metadata() = get_metadata_from_key_values(*metadata);
            }
            schema().metadata = private_data->metadata_ptr();
        }

        /**
         * @brief Gets the Arrow flags set for this array.
         *
         * @return Set of Arrow flags
         *
         * @post Returns current flag configuration
         * @post Flags reflect capabilities and constraints of the array
         */
        [[nodiscard]] SPARROW_API std::unordered_set<ArrowFlag> flags() const;

        /**
         * @brief Sets the Arrow flags for this array.
         *
         * @param flags Set of Arrow flags to apply
         *
         * @pre ArrowSchema must be created with sparrow (owned by this proxy)
         * @pre Flags must be compatible with the data type
         * @post Schema flags are updated to the specified values
         * @post Flags affect array behavior and validation
         *
         * @throws arrow_proxy_exception if schema is not owned by sparrow
         */
        SPARROW_API void set_flags(const std::unordered_set<ArrowFlag>& flags);

        /**
         * @brief Gets the number of elements in the array.
         *
         * @return Number of elements
         *
         * @post Returns non-negative value
         * @post Value is consistent with buffer sizes
         */
        [[nodiscard]] SPARROW_API size_t length() const;

        /**
         * @brief Sets the number of elements in the array.
         *
         * This method updates the length field but does not resize buffers.
         * Buffers should be resized separately to match the new length.
         *
         * @param length New number of elements
         *
         * @pre ArrowArray must be created with sparrow (owned by this proxy)
         * @pre length should be consistent with buffer sizes after buffer operations
         * @post Array length is updated to the specified value
         * @post Buffer operations should follow to maintain consistency
         *
         * @throws arrow_proxy_exception if array is not owned by sparrow
         */
        SPARROW_API void set_length(size_t length);

        /**
         * @brief Gets the number of null values in the array.
         *
         * @return Number of null values (-1 if unknown)
         *
         * @post Returns value consistent with validity bitmap
         * @post -1 indicates null count should be computed
         */
        [[nodiscard]] SPARROW_API int64_t null_count() const;

        /**
         * @brief Sets the number of null values in the array.
         *
         * This method updates the null count field but does not modify the bitmap.
         * The bitmap should be updated separately to reflect the actual null count.
         *
         * @param null_count New null count (-1 for unknown)
         *
         * @pre ArrowArray must be created with sparrow (owned by this proxy)
         * @pre null_count should be consistent with actual validity bitmap
         * @post Array null count is updated to the specified value
         * @post Bitmap operations should follow to maintain consistency
         *
         * @throws arrow_proxy_exception if array is not owned by sparrow
         */
        SPARROW_API void set_null_count(int64_t null_count);

        /**
         * @brief Gets the starting offset within the buffers.
         *
         * @return Starting offset for array data
         *
         * @post Returns non-negative value
         * @post Offset is used for array slicing operations
         */
        [[nodiscard]] SPARROW_API size_t offset() const;

        /**
         * @brief Sets the starting offset within the buffers.
         *
         * @param offset New starting offset
         *
         * @pre ArrowArray must be created with sparrow (owned by this proxy)
         * @pre offset must be within buffer bounds
         * @pre offset + length must not exceed buffer capacity
         * @post Array offset is updated to the specified value
         * @post Effective array data starts at the new offset
         *
         * @throws arrow_proxy_exception if array is not owned by sparrow
         */
        SPARROW_API void set_offset(size_t offset);

        /**
         * @brief Gets the number of buffers in the array.
         *
         * @return Number of buffers
         *
         * @post Returns value consistent with data type requirements
         * @post Value matches buffers().size()
         */
        [[nodiscard]] SPARROW_API size_t n_buffers() const;

        /**
         * @brief Sets the number of buffers and resizes the buffer vector.
         *
         * @param n_buffers New number of buffers
         *
         * @pre ArrowArray must be created with sparrow (owned by this proxy)
         * @pre n_buffers must be appropriate for the data type
         * @post Buffer vector is resized to n_buffers
         * @post Additional buffers are initialized as empty
         * @post Removed buffers are properly released
         *
         * @throws arrow_proxy_exception if array is not owned by sparrow
         */
        SPARROW_API void set_n_buffers(size_t n_buffers);

        /**
         * @brief Gets the number of child arrays.
         *
         * @return Number of child arrays
         *
         * @post Returns value consistent with data type (0 for primitive types)
         * @post Value matches children().size()
         */
        [[nodiscard]] SPARROW_API size_t n_children() const;

        /**
         * @brief Gets const reference to the buffer views.
         *
         * @return Const reference to vector of buffer views
         *
         * @post Buffer views remain valid while proxy exists
         * @post Number of buffers matches n_buffers()
         * @post Buffer sizes are consistent with array length and offset
         */
        [[nodiscard]] SPARROW_API const std::vector<sparrow::buffer_view<uint8_t>>& buffers() const;

        /**
         * @brief Gets mutable reference to the buffer views.
         *
         * @return Mutable reference to vector of buffer views
         *
         * @post Buffer views can be modified through the returned reference
         * @post Changes to buffers may require calling update_buffers()
         * @post Buffer modifications should maintain data type consistency
         */
        [[nodiscard]] SPARROW_API std::vector<sparrow::buffer_view<uint8_t>>& buffers();

        /**
         * @brief Sets a specific buffer at the given index.
         *
         * @param index Index of the buffer to set
         * @param buffer Buffer view to set
         *
         * @pre ArrowArray must be created with sparrow (owned by this proxy)
         * @pre index must be < n_buffers()
         * @pre set_length() should be called first for proper buffer sizing
         * @pre buffer must be appropriate for the data type and index
         * @post Buffer at index is updated to the specified buffer
         * @post Buffer views are refreshed automatically
         *
         * @throws arrow_proxy_exception if array is not owned by sparrow
         * @throws std::out_of_range if index >= n_buffers()
         */
        SPARROW_API void set_buffer(size_t index, const buffer_view<uint8_t>& buffer);

        /**
         * @brief Sets a specific buffer by moving it at the given index.
         *
         * @param index Index of the buffer to set
         * @param buffer Buffer to move and set
         *
         * @pre ArrowArray must be created with sparrow (owned by this proxy)
         * @pre index must be < n_buffers()
         * @pre set_length() should be called first for proper buffer sizing
         * @pre buffer must be appropriate for the data type and index
         * @post Buffer at index is updated to the moved buffer
         * @post Buffer views are refreshed automatically
         * @post Original buffer is moved and becomes invalid
         *
         * @throws arrow_proxy_exception if array is not owned by sparrow
         * @throws std::out_of_range if index >= n_buffers()
         */
        SPARROW_API void set_buffer(size_t index, buffer<uint8_t>&& buffer);

        /**
         * @brief Resizes the validity bitmap buffer.
         *
         * @param new_size New size for the bitmap buffer
         * @param value Default value for new bits (true = valid, false = null)
         *
         * @pre ArrowArray must be created with sparrow (owned by this proxy)
         * @pre Data type must support validity bitmap
         * @pre new_size should be consistent with array length requirements
         * @post Bitmap buffer is resized to accommodate new_size bits
         * @post New bits (if any) are set to the specified value
         * @post Existing bits are preserved
         *
         * @throws arrow_proxy_exception if array is not owned by sparrow
         * @throws arrow_proxy_exception if data type doesn't support validity bitmap
         */
        SPARROW_API void resize_bitmap(size_t new_size, bool value = true);

        /**
         * @brief Inserts validity bits with the same value at specified position.
         *
         * @param index Position where to insert bits
         * @param value Validity value to insert (true = valid, false = null)
         * @param count Number of bits to insert
         * @return Index of the first inserted bit
         *
         * @pre ArrowArray must be created with sparrow (owned by this proxy)
         * @pre Data type must support validity bitmap
         * @pre index must be <= current bitmap length
         * @pre count must be >= 0
         * @post count bits with specified value are inserted at index
         * @post Bitmap length increases by count
         * @post Subsequent bits are shifted right
         *
         * @throws arrow_proxy_exception if array is not owned by sparrow
         * @throws arrow_proxy_exception if data type doesn't support validity bitmap
         * @throws std::out_of_range if index > bitmap length
         */
        SPARROW_API size_t insert_bitmap(size_t index, bool value, size_t count = 1);

        /**
         * @brief Inserts a range of validity bits at specified position.
         *
         * @tparam R Range type containing boolean values
         * @param index Position where to insert bits
         * @param range Range of boolean values to insert
         * @return Index of the first inserted bit
         *
         * @pre ArrowArray must be created with sparrow (owned by this proxy)
         * @pre Data type must support validity bitmap
         * @pre index must be <= current bitmap length
         * @pre range must be a valid input range of boolean-convertible values
         * @post All bits from range are inserted at index
         * @post Bitmap length increases by range size
         * @post Subsequent bits are shifted right
         *
         * @throws arrow_proxy_exception if array is not owned by sparrow
         * @throws arrow_proxy_exception if data type doesn't support validity bitmap
         * @throws std::out_of_range if index > bitmap length
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(has_bitmap(data_type()))
         */
        template <std::ranges::input_range R>
        size_t insert_bitmap(size_t index, const R& range);

        /**
         * @brief Erases validity bits starting at specified position.
         *
         * @param index Position where to start erasing bits
         * @param count Number of bits to erase
         * @return Index of the first erased bit
         *
         * @pre ArrowArray must be created with sparrow (owned by this proxy)
         * @pre Data type must support validity bitmap
         * @pre index must be < current bitmap length
         * @pre index + count must not exceed bitmap length
         * @pre count must be >= 0
         * @post count bits starting at index are removed
         * @post Bitmap length decreases by count
         * @post Subsequent bits are shifted left
         *
         * @throws arrow_proxy_exception if array is not owned by sparrow
         * @throws arrow_proxy_exception if data type doesn't support validity bitmap
         * @throws std::out_of_range if index >= bitmap length
         */
        SPARROW_API size_t erase_bitmap(size_t index, size_t count = 1);

        /**
         * @brief Appends a validity bit at the end of the bitmap.
         *
         * @param value Validity value to append (true = valid, false = null)
         *
         * @pre ArrowArray must be created with sparrow (owned by this proxy)
         * @pre Data type must support validity bitmap
         * @post One bit with specified value is added at the end
         * @post Bitmap length increases by 1
         *
         * @throws arrow_proxy_exception if array is not owned by sparrow
         * @throws arrow_proxy_exception if data type doesn't support validity bitmap
         */
        SPARROW_API void push_back_bitmap(bool value);

        /**
         * @brief Removes the last validity bit from the bitmap.
         *
         * @pre ArrowArray must be created with sparrow (owned by this proxy)
         * @pre Data type must support validity bitmap
         * @pre Bitmap must not be empty
         * @post Last bit is removed from the bitmap
         * @post Bitmap length decreases by 1
         *
         * @throws arrow_proxy_exception if array is not owned by sparrow
         * @throws arrow_proxy_exception if data type doesn't support validity bitmap
         */
        SPARROW_API void pop_back_bitmap();

        /**
         * Add children without taking their ownership.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or the `ArrowSchema` wrapped
         * in this proxy were not created with sparrow.
         * @param arrow_array_and_schema_pointers The children to add.
         */
        template <std::ranges::input_range R>
            requires std::same_as<std::ranges::range_value_t<R>, arrow_array_and_schema_pointers>
        void add_children(const R& arrow_array_and_schema_pointers);

        /**
         * Add children and take their ownership.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or the `ArrowSchema` wrapped
         * in this proxy were not created with sparrow.
         * @param arrow_array_and_schemas The children to add.
         */
        template <std::ranges::input_range R>
            requires std::same_as<std::ranges::range_value_t<R>, arrow_array_and_schema>
        void add_children(R&& arrow_array_and_schemas);

        /**
         * Add a child without taking its ownership.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or the `ArrowSchema` wrapped
         * in this proxy were not created with sparrow.
         * @param array The `ArrowArray` to set as child.
         * @param schema The `ArrowSchema` to set as child.
         */
        SPARROW_API void add_child(ArrowArray* array, ArrowSchema* schema);

        /**
         * Add a child without taking its ownership.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or the `ArrowSchema` wrapped
         * in this proxy were not created with sparrow.
         * @param array The `ArrowArray` to set as child.
         * @param schema The `ArrowSchema` to set as child.
         */
        SPARROW_API void add_child(const ArrowArray* array, const ArrowSchema* schema);

        /**
         * Add a child and takes its ownership.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or the `ArrowSchema` wrapped
         * in this proxy were not created with sparrow.
         * @param array The `ArrowArray` to set as child.
         * @param schema The `ArrowSchema` to set as child.
         */
        SPARROW_API void add_child(ArrowArray&& array, ArrowSchema&& schema);

        /**
         * Pop n children. If the children were created by sparrow or are owned by the proxy,
         * it will delete them.
         * @param n The number of children to pop.
         */
        SPARROW_API void pop_children(size_t n);

        /**
         * Set the child at the given index. It does not take the ownership on the `ArrowArray` and
         * `ArrowSchema` passed by pointers.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or the `ArrowSchema` wrapped
         * in this proxy were not created with sparrow.
         * @param index The index of the child to set.
         * @param array The `ArrowArray` to set as child.
         * @param schema The `ArrowSchema` to set as child.
         */
        SPARROW_API void set_child(size_t index, ArrowArray* array, ArrowSchema* schema);

        /**
         * Set the child at the given index. It does not take the ownership on the `ArrowArray` and
         * `ArrowSchema` passed by const pointers.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or the `ArrowSchema` wrapped
         * in this proxy were not created with sparrow.
         * @param index The index of the child to set.
         * @param array The `ArrowArray` to set as child.
         * @param schema The `ArrowSchema` to set as child.
         */
        SPARROW_API void set_child(size_t index, const ArrowArray* array, const ArrowSchema* schema);

        /**
         * Set the child at the given index. It takes the ownership on the `ArrowArray` and`ArrowSchema`
         * passed by rvalue referencess.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or `ArrowSchema` wrapped
         * in this proxy were not created with
         * sparrow.
         * @param index The index of the child to set.
         * @param array The `ArrowArray` to set as child.
         * @param schema The `ArrowSchema` to set as child.
         */
        SPARROW_API void set_child(size_t index, ArrowArray&& array, ArrowSchema&& schema);


        /**
         * @brief Returns a constant reference to the vector of child arrow proxies.
         *
         * This method provides read-only access to the collection of child arrow proxies
         * associated with this arrow array schema proxy. The children represent nested
         * or structured data elements within the schema.
         *
         * @return const std::vector<arrow_proxy>& A constant reference to the vector
         *         containing the child arrow proxies.
         */
        [[nodiscard]] SPARROW_API const std::vector<arrow_proxy>& children() const;

        /**
         * @brief Returns a mutable reference to the vector of child arrow proxies.
         *
         * This method provides read-write access to the collection of child arrow proxies
         * associated with this arrow array schema proxy. The children represent nested
         * or structured data elements within the schema.
         *
         * @return std::vector<arrow_proxy>& A mutable reference to the vector
         *         containing the child arrow proxies.
         */
        [[nodiscard]] SPARROW_API std::vector<arrow_proxy>& children();

        /**
         * @brief Returns a constant reference to the dictionary arrow proxy.
         *
         * This method provides read-only access to the dictionary arrow proxy associated
         * with this arrow array schema proxy. The dictionary is used for encoding categorical
         * data types.
         *
         * @return const std::unique_ptr<arrow_proxy>& A constant reference to the dictionary
         *         arrow proxy, or nullptr if no dictionary is set.
         */
        [[nodiscard]] SPARROW_API const std::unique_ptr<arrow_proxy>& dictionary() const;

        /**
         * @brief Returns a mutable reference to the dictionary arrow proxy.
         *
         * This method provides read-write access to the dictionary arrow proxy associated
         * with this arrow array schema proxy. The dictionary is used for encoding categorical
         * data types.
         *
         * @return std::unique_ptr<arrow_proxy>& A mutable reference to the dictionary
         *         arrow proxy, or nullptr if no dictionary is set.
         */
        [[nodiscard]] SPARROW_API std::unique_ptr<arrow_proxy>& dictionary();

        /**
         * Set the dictionary. It does not take the ownership on the `ArrowArray` and
         * `ArrowSchema` passed by pointers.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or `ArrowSchema` were not created with
         * sparrow.
         * @param array The `ArrowArray` to set as dictionary.
         * @param schema The `ArrowSchema` to set as dictionary.
         */
        SPARROW_API void set_dictionary(ArrowArray* array, ArrowSchema* schema);

        /**
         * Set the dictionary. It does not take the ownership on the `ArrowArray` and
         * `ArrowSchema` passed by pointers.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or `ArrowSchema` were not created with
         * sparrow.
         * @param array The `ArrowArray` to set as dictionary.
         * @param schema The `ArrowSchema` to set as dictionary.
         */
        SPARROW_API void set_dictionary(const ArrowArray* array, const ArrowSchema* schema);

        /**
         * Set the dictionary. It takes the ownership on the `ArrowArray` and`ArrowSchema`
         * passed by rvalue referencess.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or `ArrowSchema` wrapped
         * in this proxy were not created with
         * sparrow.
         * @param array_dictionary The `ArrowArray` to set as dictionary.
         * @param schema_dictionary The `ArrowSchema` to set as dictionary.s
         */
        SPARROW_API void set_dictionary(ArrowArray&& array_dictionary, ArrowSchema&& schema_dictionary);

        /**
         * Check if the `ArrowArray` and `ArrowSchema` were created with sparrow.
         */
        [[nodiscard]] SPARROW_API bool is_created_with_sparrow() const;

        [[nodiscard]] SPARROW_API void* private_data() const;

        /**
         * Get a non-owning view of the arrow_proxy.
         */
        [[nodiscard]] SPARROW_API arrow_proxy view() const;

        /**
         * Check whether the proxy is a view.
         */
        [[nodiscard]] SPARROW_API bool is_view() const noexcept;

        /**
         * Check whether the proxy has ownership of its internal the `ArrowArray`.
         */
        [[nodiscard]] SPARROW_API bool owns_array() const;

        /**
         * Extract the `ArrowArray` from the proxy, and transfers the responsibility to release it after usage
         * to the caller.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @details The schema flags can be updated by adding sparrow::ArrowFlag::NULLABLE, if null_count is
         * greater than 0.
         * @return The array.
         */
        [[nodiscard]] SPARROW_API ArrowArray extract_array();

        /**
         * Get a reference to the `ArrowArray` of the proxy. The proxy is still reponsible for releasing it,
         * and the reference returned from this method should not outlive the proxy.
         * @details The schema flags can be updated by adding sparrow::ArrowFlag::NULLABLE, if null_count is
         * greater than 0.
         * @return The `ArrowArray`.
         */
        [[nodiscard]] SPARROW_API ArrowArray& array();

        /**
         * Get a const reference to the `ArrowArray` of the proxy. The proxy is still reponsible for releasing
         * it, and the reference returned from this method should not outlive the proxy.
         * @details The schema flags can be updated by adding sparrow::ArrowFlag::NULLABLE, if null_count is
         * greater than 0.
         * @return The `ArrowArray` const reference.
         */
        [[nodiscard]] SPARROW_API const ArrowArray& array() const;

        /**
         * Check whether the proxy has ownership of its internal the `ArrowSchema`.
         */
        [[nodiscard]] SPARROW_API bool owns_schema() const;

        /**
         * Extract the `ArrowSchema` from the proxy, and transfers the responsibility to release it after
         * usage to the caller.
         * @exception `arrow_proxy_exception` If the `ArrowSchema` was not created with sparrow.
         * @details The schema flags can be updated by adding sparrow::ArrowFlag::NULLABLE, if null_count is
         * greater than 0.
         * @return The `ArrowSchema`.
         */
        [[nodiscard]] SPARROW_API ArrowSchema extract_schema();

        /**
         * Get a reference to the `ArrowSchema` of the proxy. The proxy is still reponsible for releasing
         * it, and the reference returned from this method should not outlive the proxy.
         * @details The schema flags can be updated by adding sparrow::ArrowFlag::NULLABLE, if null_count is
         * greater than 0.
         * @return The `ArrowSchema` reference.
         */
        [[nodiscard]] SPARROW_API ArrowSchema& schema();

        /**
         * Get a const reference to the `ArrowSchema` of the proxy. The proxy is still reponsible for
         * releasing it, and the reference returned from this method should not outlive the proxy.
         * @details The schema flags can be updated by adding sparrow::ArrowFlag::NULLABLE, if null_count is
         * greater than 0.
         * @return The `ArrowSchema` const reference.
         */
        [[nodiscard]] SPARROW_API const ArrowSchema& schema() const;

        [[nodiscard]] SPARROW_API arrow_schema_private_data* get_schema_private_data();
        [[nodiscard]] SPARROW_API arrow_array_private_data* get_array_private_data();

        /**
         * Slices the array to keep only the elements between the given \p start and \p end.
         * A copy of the \ref array is modified. The data is not modified, only the ArrowArray.offset and
         * ArrowArray.length are updated. If \p end is greater than the size of the buffers, the following
         * elements will be invalid.
         *
         * @param start The index of the first element to keep. Must be less than \p end.
         * @param end The index of the first element to discard. Must be less than the size of the buffers.
         */
        [[nodiscard]] SPARROW_API arrow_proxy slice(size_t start, size_t end) const;

        /**
         * Slices the array to keep only the elements between the given \p start and \p end.
         * A view of the \ref array is returned. The data is not modified, only the ArrowArray.offset and
         * ArrowArray.length are updated. If \p end is greater than the size of the buffers, the following
         * elements will be invalid.
         *
         * @param start The index of the first element to keep. Must be less than \p end.
         * @param end The index of the first element to discard. Must be less than the size of the buffers.
         */
        [[nodiscard]] SPARROW_API arrow_proxy slice_view(size_t start, size_t end) const;

        /**
         * Refresh the buffers views. This method should be called after modifying the buffers of the array.
         */
        SPARROW_API void update_buffers();

        /**
         * Check if the array is const.
         */
        [[nodiscard]] SPARROW_API bool is_array_const() const;

        /**
         * Check if the schema is const.
         */
        [[nodiscard]] SPARROW_API bool is_schema_const() const;

        using mutable_bitmap_type = non_owning_dynamic_bitset<uint8_t>;
        using const_bitmap_type = dynamic_bitset_view<const uint8_t>;
        using bitmap_variant = std::variant<mutable_bitmap_type, const_bitmap_type>;

        [[nodiscard]] SPARROW_API std::optional<bitmap_variant>& bitmap()
        {
            return m_null_bitmap;
        }

        [[nodiscard]] SPARROW_API const std::optional<bitmap_variant>& bitmap() const
        {
            return m_null_bitmap;
        }

        [[nodiscard]] SPARROW_API std::optional<const_bitmap_type>& const_bitmap()
        {
            return m_const_bitmap;
        }

        [[nodiscard]] SPARROW_API const std::optional<const_bitmap_type>& const_bitmap() const
        {
            return m_const_bitmap;
        }

    private:

        std::variant<ArrowArray*, ArrowArray> m_array;
        std::variant<ArrowSchema*, ArrowSchema> m_schema;
        std::vector<sparrow::buffer_view<uint8_t>> m_buffers;
        std::vector<arrow_proxy> m_children;
        std::unique_ptr<arrow_proxy> m_dictionary;
        bool m_array_is_immutable = false;
        bool m_schema_is_immutable = false;
        bool m_is_dictionary_immutable = false;
        std::vector<bool> m_children_array_immutable;
        std::vector<bool> m_children_schema_immutable;
        std::optional<bitmap_variant> m_null_bitmap;
        std::optional<const_bitmap_type> m_const_bitmap;

        struct impl_tag
        {
        };

        // Build an empty proxy. Convenient for resizing vector of children
        arrow_proxy();

        template <typename AA, typename AS>
            requires std::same_as<std::remove_const_t<std::remove_pointer_t<std::remove_cvref_t<AA>>>, ArrowArray>
                     && std::same_as<std::remove_const_t<std::remove_pointer_t<std::remove_cvref_t<AS>>>, ArrowSchema>
        arrow_proxy(AA&& array, AS&& schema, impl_tag);

        [[nodiscard]] bool empty() const;
        SPARROW_API void resize_children(size_t children_count);

        void update_children();
        void update_dictionary();
        void update_null_count();
        void reset();
        void remove_dictionary();
        void remove_child(size_t index);
        void create_bitmap_view();

        [[nodiscard]] bool array_created_with_sparrow() const;
        [[nodiscard]] SPARROW_API bool schema_created_with_sparrow() const;

        void validate_array_and_schema() const;

        [[nodiscard]] bool is_arrow_array_valid() const;
        [[nodiscard]] bool is_arrow_schema_valid() const;
        [[nodiscard]] bool is_proxy_valid() const;

        [[nodiscard]] size_t get_null_count() const;

        [[nodiscard]] ArrowArray& array_without_sanitize();
        [[nodiscard]] const ArrowArray& array_without_sanitize() const;

        [[nodiscard]] ArrowSchema& schema_without_sanitize();
        [[nodiscard]] const ArrowSchema& schema_without_sanitize() const;

        /**
         * If the null_count of the proxy or the one from the dictionary is greater than 0, the schema
         * is updated to add the ArrowFlag::NULLABLE flag.
         */
        void sanitize_schema();

        void swap(arrow_proxy& other) noexcept;

        template <const char* function_name, bool check_array_is_mutable, bool check_schema_is_mutable>
        void throw_if_immutable() const;
    };

    template <std::ranges::input_range R>
        requires std::same_as<std::ranges::range_value_t<R>, arrow_array_and_schema_pointers>
    void arrow_proxy::add_children(const R& arrow_array_and_schema_pointers)
    {
        static constexpr const char function_name[] = "add_children";
        throw_if_immutable<function_name, true, true>();
        const size_t add_children_count = std::ranges::size(arrow_array_and_schema_pointers);
        const size_t original_children_count = n_children();
        const size_t new_children_count = original_children_count + add_children_count;

        resize_children(new_children_count);
        for (size_t i = 0; i < add_children_count; ++i)
        {
            set_child(
                i + original_children_count,
                arrow_array_and_schema_pointers[i].array,
                arrow_array_and_schema_pointers[i].schema
            );
        }
    }

    template <std::ranges::input_range R>
        requires std::same_as<std::ranges::range_value_t<R>, arrow_array_and_schema>
    void arrow_proxy::add_children(R&& arrow_arrays_and_schemas)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set n_buffers on non-sparrow created ArrowArray or ArrowSchema");
        }

        const size_t add_children_count = std::ranges::size(arrow_arrays_and_schemas);
        const size_t original_children_count = n_children();
        const size_t new_children_count = original_children_count + add_children_count;

        resize_children(new_children_count);
        for (size_t i = 0; i < add_children_count; ++i)
        {
            set_child(
                i + original_children_count,
                std::move(arrow_arrays_and_schemas[i].array),
                std::move(arrow_arrays_and_schemas[i].schema)
            );
        }
    }

    template <std::ranges::input_range R>
    inline size_t arrow_proxy::insert_bitmap(size_t index, const R& range)
    {
        static constexpr const char function_name[] = "insert_bitmap";
        throw_if_immutable<function_name, true, false>();
        SPARROW_ASSERT_TRUE(m_null_bitmap.has_value())
        auto& bitmap = std::get<mutable_bitmap_type>(*m_null_bitmap);
        const auto it = bitmap.insert(sparrow::next(bitmap.cbegin(), index), range.begin(), range.end());
        set_null_count(static_cast<int64_t>(bitmap.null_count()));
        return static_cast<size_t>(std::distance(bitmap.begin(), it));
    }

    template <typename AA, typename AS>
        requires std::same_as<std::remove_const_t<std::remove_pointer_t<std::remove_cvref_t<AA>>>, ArrowArray>
                 && std::same_as<std::remove_const_t<std::remove_pointer_t<std::remove_cvref_t<AS>>>, ArrowSchema>
    arrow_proxy::arrow_proxy(AA&& array, AS&& schema, impl_tag)
    {
        if constexpr (std::is_const_v<std::remove_pointer_t<std::remove_reference_t<AA>>>)
        {
            m_array_is_immutable = true;
            m_array = const_cast<ArrowArray*>(array);
        }
        else
        {
            m_array = std::forward<AA>(array);
        }

        if constexpr (std::is_const_v<std::remove_pointer_t<std::remove_reference_t<AS>>>)
        {
            m_schema_is_immutable = true;
            m_schema = const_cast<ArrowSchema*>(schema);
        }
        else
        {
            m_schema = std::forward<AS>(schema);
        }

        if constexpr (std::is_rvalue_reference_v<AA&&>)
        {
            array = {};
        }
        else if constexpr (std::is_pointer_v<std::remove_cvref_t<AA>>)
        {
            SPARROW_ASSERT_TRUE(array != nullptr);
        }

        if constexpr (std::is_rvalue_reference_v<AS&&>)
        {
            schema = {};
        }
        else if constexpr (std::is_pointer_v<std::remove_cvref_t<AS>>)
        {
            SPARROW_ASSERT_TRUE(schema != nullptr);
        }

        m_children_array_immutable = std::vector<bool>(n_children(), m_array_is_immutable);
        m_children_schema_immutable = std::vector<bool>(n_children(), m_schema_is_immutable);
        validate_array_and_schema();
        update_buffers();
        update_children();
        update_dictionary();
        create_bitmap_view();
    }

    template <const char* function_name, bool check_array_is_mutable, bool check_schema_is_mutable>
    void arrow_proxy::throw_if_immutable() const
    {
        static const std::string cannot_call = "Cannot call ";
        if (!is_created_with_sparrow())
        {
            auto error_message = cannot_call + std::string(function_name)
                                 + " on non-sparrow created ArrowArray or ArrowSchema";
            throw arrow_proxy_exception(error_message);
        }
        if constexpr (check_array_is_mutable || check_schema_is_mutable)
        {
            if (m_array_is_immutable || m_schema_is_immutable)
            {
                {
                    std::string error_message = cannot_call + std::string(function_name);
                    if constexpr (check_array_is_mutable && !check_schema_is_mutable)
                    {
                        if (m_array_is_immutable)
                        {
                            error_message += " on an immutable ArrowArray. You may have passed a const ArrowArray* at the creation.";
                        }
                    }
                    else if constexpr (check_schema_is_mutable && !check_array_is_mutable)
                    {
                        if (m_schema_is_immutable)
                        {
                            error_message += " on an immutable ArrowSchema. You may have passed a const ArrowSchema* at the creation.";
                        }
                    }
                    else if constexpr (check_array_is_mutable && check_schema_is_mutable)
                    {
                        if (m_array_is_immutable && m_schema_is_immutable)
                        {
                            error_message += " on an immutable ArrowArray and ArrowSchema. You may have passed const ArrowArray* and const ArrowSchema* at the creation.";
                        }
                    }
                    throw arrow_proxy_exception(error_message);
                }
            }
        }
    }
}

#if defined(__cpp_lib_format)

template <>
struct std::formatter<sparrow::buffer_view<uint8_t>>
{
private:

    char delimiter = ' ';
    static constexpr std::string_view opening = "[";
    static constexpr std::string_view closing = "]";

public:

    constexpr auto parse(std::format_parse_context& ctx)
    {
        auto it = ctx.begin();
        auto end = ctx.end();

        // Parse optional delimiter
        if (it != end && *it != '}')
        {
            delimiter = *it++;
        }

        if (it != end && *it != '}')
        {
            throw std::format_error("Invalid format specifier for range");
        }

        return it;
    }

    auto format(const sparrow::buffer_view<uint8_t>& range, std::format_context& ctx) const
    {
        auto out = ctx.out();

        // Write opening bracket
        out = sparrow::ranges::copy(opening, out).out;

        // Write range elements
        bool first = true;
        for (const auto& elem : range)
        {
            if (!first)
            {
                *out++ = delimiter;
            }
            out = std::format_to(out, "{}", elem);
            first = false;
        }

        // Write closing bracket
        out = sparrow::ranges::copy(closing, out).out;

        return out;
    }
};

namespace sparrow
{
    inline std::ostream& operator<<(std::ostream& os, const buffer_view<uint8_t>& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

template <>
struct std::formatter<sparrow::arrow_proxy>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::arrow_proxy& obj, std::format_context& ctx) const
    {
        std::string buffers_description_str;
        for (size_t i = 0; i < obj.n_buffers(); ++i)
        {
            std::format_to(
                std::back_inserter(buffers_description_str),
                "<{}[{} b]{}",
                "uint8_t",
                obj.buffers()[i].size() * sizeof(uint8_t),
                obj.buffers()[i]
            );
        }

        std::string children_str;
        for (const auto& child : obj.children())
        {
            std::format_to(std::back_inserter(children_str), "{}\n", child);
        }

        const std::string dictionary_str = obj.dictionary() ? std::format("{}", *obj.dictionary()) : "nullptr";

        return std::format_to(
            ctx.out(),
            "arrow_proxy\n- format: {}\n- name; {}\n- metadata: {}\n- data_type: {}\n- null_count:{}\n- length: {}\n- offset: {}\n- n_buffers: {}\n- buffers:\n{}\n- n_children: {}\n-children: {}\n- dictionary: {}",
            obj.format(),
            obj.name().value_or(""),
            obj.metadata().value_or(""),
            obj.data_type(),
            obj.null_count(),
            obj.length(),
            obj.offset(),
            obj.n_buffers(),
            buffers_description_str,
            obj.n_children(),
            children_str,
            dictionary_str
        );
    }
};

namespace sparrow
{
    inline std::ostream& operator<<(std::ostream& os, const arrow_proxy& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

#endif
