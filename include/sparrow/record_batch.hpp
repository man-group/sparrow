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
#include <initializer_list>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

#include "sparrow/array.hpp"
#include "sparrow/struct_array.hpp"
#include "sparrow/utils/contracts.hpp"

#if defined(__cpp_lib_format)
#    include "sparrow/utils/format.hpp"
#endif

namespace sparrow
{
    /**
     * @brief Table-like data structure for storing columnar data with named fields.
     *
     * A record batch is a collection of equal-length arrays mapped to unique names,
     * representing a table where each array forms a column. This provides a convenient
     * unit of work for various serialization, computation, and data manipulation functions
     * while maintaining Arrow compatibility.
     *
     * The record batch ensures that:
     * - All arrays have the same length (number of rows)
     * - Column names are unique within the batch
     * - Efficient name-based and index-based column access
     * - Consistent internal state through validation
     *
     * @pre All arrays must have the same length
     * @pre Column names must be unique within the record batch
     * @post Maintains consistent mapping between names and arrays
     * @post Provides O(1) access to columns by index and O(1) average access by name
     * @post Thread-safe for read operations, requires external synchronization for writes
     *
     * @example
     * ```cpp
     * // Create from separate names and arrays
     * std::vector<std::string> names = {"id", "name", "age"};
     * std::vector<array> columns = {id_array, name_array, age_array};
     * record_batch batch(names, columns, "employee_data");
     *
     * // Create from named arrays
     * std::vector<array> named_columns;
     * named_columns.emplace_back(id_array.with_name("id"));
     * named_columns.emplace_back(name_array.with_name("name"));
     * record_batch batch2(named_columns);
     * ```
     */
    class record_batch
    {
    public:

        using name_type = std::string;
        using size_type = std::size_t;
        using initializer_type = std::initializer_list<std::pair<name_type, array>>;

        using name_range = std::ranges::ref_view<const std::vector<name_type>>;
        using column_range = std::ranges::ref_view<const std::vector<array>>;

        /**
         * @brief Constructs a record_batch from separate name and array ranges.
         *
         * Each array is mapped to the name at the corresponding position in the names range.
         * The ranges must have the same size, and all arrays must have equal length.
         *
         * @tparam NR Input range type for names (convertible to std::string)
         * @tparam CR Input range type for arrays
         * @tparam METADATA_RANGE Type of metadata container (default: std::vector<metadata_pair>)
         * @param names Input range of column names (must be unique)
         * @param columns Input range of arrays (must have equal lengths)
         * @param name Optional name for the record batch itself
         * @param metadata Optional metadata for the record batch
         *
         * @pre std::ranges::size(names) == std::ranges::size(columns)
         * @pre All names in the range must be unique
         * @pre All arrays must have the same length
         * @pre Names must be convertible to std::string
         * @post Record batch contains mapping from names to arrays
         * @post Internal consistency is maintained
         * @post Array map cache is properly initialized
         *
         * @throws std::invalid_argument if preconditions are violated
         */
        template <
            std::ranges::input_range NR,
            std::ranges::input_range CR,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(std::convertible_to<std::ranges::range_value_t<NR>, std::string> and std::same_as<std::ranges::range_value_t<CR>, array>)
        constexpr record_batch(
            NR&& names,
            CR&& columns,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Constructs a record_batch from arrays with existing names.
         *
         * Each array must have a non-empty name. The array names are extracted
         * and used as column names in the record batch.
         *
         * @tparam CR Input range type for arrays
         * @tparam METADATA_RANGE Type of metadata container (default: std::vector<metadata_pair>)
         * @param columns Input range of named arrays
         * @param name Optional name for the record batch itself
         * @param metadata Optional metadata for the record batch
         *
         * @pre All arrays must have non-empty names (arr.name().has_value() && !arr.name()->empty())
         * @pre All array names must be unique within the range
         * @pre All arrays must have the same length
         * @post Record batch contains arrays mapped to their internal names
         * @post Internal consistency is maintained
         *
         * @throws std::invalid_argument if any array lacks a name or names are not unique
         * @throws std::invalid_argument if arrays have different lengths
         */
        template <std::ranges::input_range CR, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::same_as<std::ranges::range_value_t<CR>, array>
        record_batch(
            CR&& columns,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Constructs a record_batch from initializer list of name-array pairs.
         *
         * @param init Initializer list of std::pair<name_type, array>
         *
         * @pre All names in the initializer list must be unique
         * @pre All arrays must have the same length
         * @post Record batch contains the specified name-array mappings
         * @post Internal consistency is maintained
         *
         * @throws std::invalid_argument if names are not unique or arrays have different lengths
         */
        SPARROW_API record_batch(initializer_type init);

        /**
         * @brief Constructs a record_batch from a struct_array.
         *
         * The struct array's fields become the columns of the record batch,
         * with field names becoming column names.
         *
         * @param ar Struct array to convert (must own its internal Arrow structures)
         *
         * @pre ar must be a valid struct array with owned Arrow structures
         * @pre ar must have at least one field
         * @pre All field names must be unique (guaranteed by struct_array invariants)
         * @post Record batch contains columns corresponding to struct fields
         * @post Struct array is moved and becomes invalid
         * @post Internal consistency is maintained
         */
        SPARROW_API record_batch(struct_array&& ar);

        /**
         * @brief Copy constructor.
         *
         * @param other The record_batch to copy from
         *
         * @pre other must be in a valid state
         * @post This record batch is an independent copy of other
         * @post All arrays are deep-copied
         * @post Internal consistency is maintained
         */
        SPARROW_API record_batch(const record_batch& other);

        /**
         * @brief Copy assignment operator.
         *
         * @param other The record_batch to copy from
         * @return Reference to this record_batch
         *
         * @pre other must be in a valid state
         * @post This record batch is an independent copy of other
         * @post Previous data is properly released
         * @post All arrays are deep-copied
         * @post Internal consistency is maintained
         */
        SPARROW_API record_batch& operator=(const record_batch& other);

        record_batch(record_batch&&) = default;
        record_batch& operator=(record_batch&&) = default;

        /**
         * @brief Gets the number of columns in the record batch.
         *
         * @return Number of columns (arrays) in the record batch
         *
         * @post Returns non-negative value
         * @post Return value equals the number of unique column names
         */
        SPARROW_API size_type nb_columns() const;

        /**
         * @brief Gets the number of rows in the record batch.
         *
         * @return Number of rows (length of each array) in the record batch
         *
         * @post Returns non-negative value
         * @post If nb_columns() > 0, all arrays have this length
         * @post If nb_columns() == 0, returns 0
         */
        SPARROW_API size_type nb_rows() const;

        /**
         * @brief Checks if the record batch contains a column with the specified name.
         *
         * @param key The name of the column to search for
         * @return true if the column exists, false otherwise
         *
         * @post Return value is consistent with get_column(key) success/failure
         * @post O(1) average time complexity due to internal hash map
         */
        SPARROW_API bool contains_column(const name_type& key) const;

        /**
         * @brief Gets the name of the column at the specified index.
         *
         * @param index The index of the column (0-based)
         * @return Const reference to the column name
         *
         * @pre index must be < nb_columns()
         * @post Returns valid reference to the column name
         * @post Returned reference remains valid while record batch exists
         *
         * @throws std::out_of_range if index >= nb_columns()
         */
        SPARROW_API const name_type& get_column_name(size_type index) const;

        /**
         * @brief Gets the column with the specified name.
         *
         * @param key The name of the column to retrieve
         * @return Const reference to the array
         *
         * @pre Column with the specified name must exist
         * @post Returns valid reference to the column array
         * @post Returned reference remains valid while record batch exists
         *
         * @throws std::out_of_range if column with key does not exist
         */
        SPARROW_API const array& get_column(const name_type& key) const;

        /**
         * @brief Gets the column at the specified index.
         *
         * @param index The index of the column (0-based)
         * @return Const reference to the array
         *
         * @pre index must be < nb_columns()
         * @post Returns valid reference to the column array
         * @post Returned reference remains valid while record batch exists
         *
         * @throws std::out_of_range if index >= nb_columns()
         */
        SPARROW_API const array& get_column(size_type index) const;

        /**
         * @brief Gets the name of the record batch.
         *
         * @return Optional name of the record batch
         *
         * @post Returns the name specified during construction (if any)
         */
        SPARROW_API const std::optional<name_type>& name() const;

        /**
         * @brief Gets a range view of the column names.
         *
         * @return Range view over the column names in insertion order
         *
         * @post Range size equals nb_columns()
         * @post Range elements are in the same order as columns
         * @post Range remains valid while record batch exists and is not modified
         */
        SPARROW_API name_range names() const;

        /**
         * @brief Gets a range view of the columns.
         *
         * @return Range view over the arrays in insertion order
         *
         * @post Range size equals nb_columns()
         * @post Range elements correspond to names() in the same order
         * @post Range remains valid while record batch exists and is not modified
         */
        SPARROW_API column_range columns() const;

        /**
         * @brief Moves the internal columns into a struct_array and empties the record batch.
         *
         * After this operation, the record batch becomes empty and should not be used
         * until new data is added.
         *
         * @return struct_array containing the moved columns
         *
         * @pre Record batch must not be empty (nb_columns() > 0)
         * @post Record batch becomes empty (nb_columns() == 0, nb_rows() == 0)
         * @post Returned struct_array contains all previous columns as fields
         * @post Column data is moved, not copied
         * @post Internal state is reset to empty but valid state
         */
        SPARROW_API struct_array extract_struct_array();

        /**
         * @brief Adds a new column to the record batch with the specified name.
         *
         * @param name The name for the new column (must be unique)
         * @param column The array to add as a column
         *
         * @pre name must not already exist in the record batch
         * @pre If record batch is not empty, column.size() must equal nb_rows()
         * @pre name must not be empty
         * @post Record batch contains the new column mapped to name
         * @post nb_columns() increases by 1
         * @post If this was the first column, nb_rows() equals column.size()
         * @post Internal consistency is maintained
         * @post Array map cache is updated
         *
         * @throws std::invalid_argument if name already exists or column size is incompatible
         */
        SPARROW_API void add_column(name_type name, array column);

        /**
         * @brief Adds a new column using the array's internal name.
         *
         * @param column The array to add (must have a non-empty name)
         *
         * @pre column must have a non-empty name (column.name().has_value() && !column.name()->empty())
         * @pre column.name() must not already exist in the record batch
         * @pre If record batch is not empty, column.size() must equal nb_rows()
         * @post Record batch contains the new column mapped to column.name()
         * @post nb_columns() increases by 1
         * @post If this was the first column, nb_rows() equals column.size()
         * @post Internal consistency is maintained
         *
         * @throws std::invalid_argument if column lacks name, name exists, or size is incompatible
         */
        SPARROW_API void add_column(array column);

    private:

        /**
         * @brief Converts a range to a vector of the specified type.
         *
         * @tparam U The element type of the resulting vector
         * @tparam R The range type
         * @param range The input range to convert
         * @return Vector containing the elements from the range
         *
         * @post Returned vector contains all elements from range in order
         * @post If range is sized, vector capacity is pre-allocated
         */
        template <class U, class R>
        [[nodiscard]] std::vector<U> to_vector(R&& range) const;

        /**
         * @brief Updates the internal array map cache for fast name-based lookups.
         *
         * @post Internal map cache reflects current name-to-array mappings
         * @post Cache dirty flag is cleared
         * @post Subsequent name-based lookups will be O(1) average time
         */
        SPARROW_API void update_array_map_cache() const;

        /**
         * @brief Checks the internal consistency of the record batch.
         *
         * Validates that:
         * - Name list and array list have the same size
         * - All names are unique
         * - All arrays have the same length
         * - Array map cache is consistent (if not dirty)
         *
         * @return true if the record batch is consistent, false otherwise
         *
         * @post Return value indicates whether all invariants are satisfied
         *
         * @note This is primarily used for debugging and testing
         */
        SPARROW_API void check_consistency() const;

        std::optional<name_type> m_name;                       ///< Optional name of the record batch
        std::optional<std::vector<metadata_pair>> m_metadata;  ///< Optional metadata for the record batch
        std::vector<name_type> m_name_list;                    ///< Ordered list of column names
        std::vector<array> m_array_list;                       ///< Ordered list of column arrays
        mutable std::unordered_map<name_type, const array*> m_array_map;  ///< Cache for fast name-based
                                                                          ///< lookup
        mutable bool m_dirty_map = true;  ///< Flag indicating cache needs update
    };

    /**
     * @brief Compares two record_batch objects for equality.
     *
     * Two record batches are considered equal if:
     * - They have the same number of columns
     * - Column names match in the same order
     * - Corresponding arrays are equal
     * - Record batch names match (both present and equal, or both absent)
     *
     * @param lhs First record batch to compare
     * @param rhs Second record batch to compare
     * @return true if the record batches are equal, false otherwise
     *
     * @post Comparison is symmetric: lhs == rhs iff rhs == lhs
     * @post Comparison includes both structure and data equality
     */
    SPARROW_API
    bool operator==(const record_batch& lhs, const record_batch& rhs);

    /*******************************
     * record_batch implementation *
     *******************************/

    template <std::ranges::input_range NR, std::ranges::input_range CR, input_metadata_container METADATA_RANGE>
        requires(std::convertible_to<std::ranges::range_value_t<NR>, std::string>
                 and std::same_as<std::ranges::range_value_t<CR>, array>)
    constexpr record_batch::record_batch(
        NR&& names,
        CR&& columns,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
        : m_name(name)
        , m_metadata(std::move(metadata))
        , m_name_list(to_vector<name_type>(std::forward<NR>(names)))
        , m_array_list(to_vector<array>(std::forward<CR>(columns)))
    {
        update_array_map_cache();
    }

    namespace detail
    {
        inline std::vector<record_batch::name_type> get_names(const std::vector<array>& array_list)
        {
            const auto names = array_list
                               | std::views::transform(
                                   [](const array& ar)
                                   {
                                       return ar.name().value();
                                   }
                               );
            return {names.begin(), names.end()};
        }
    }

    template <std::ranges::input_range CR, input_metadata_container METADATA_RANGE>
        requires std::same_as<std::ranges::range_value_t<CR>, array>
    record_batch::record_batch(CR&& columns, std::optional<std::string_view> name, std::optional<METADATA_RANGE> metadata)
        : m_name(name)
        , m_name_list(detail::get_names(columns))
        , m_array_list(to_vector<array>(std::move(columns)))
        , m_metadata(std::move(metadata))
    {
        update_array_map_cache();
    }

    template <class U, class R>
    std::vector<U> record_batch::to_vector(R&& range) const
    {
        std::vector<U> v;
        if constexpr (std::ranges::sized_range<decltype(range)>)
        {
            v.reserve(std::ranges::size(range));
        }
        std::ranges::move(range, std::back_inserter(v));
        return v;
    }
}

#if defined(__cpp_lib_format)
template <>
struct std::formatter<sparrow::record_batch>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::record_batch& rb, std::format_context& ctx) const
    {
        const auto values_by_columns = rb.columns()
                                       | std::views::transform(
                                           [&rb](const auto& ar)
                                           {
                                               return std::views::iota(0u, rb.nb_rows())
                                                      | std::views::transform(
                                                          [&ar](const auto i)
                                                          {
                                                              return ar[i];
                                                          }
                                                      );
                                           }
                                       );

        sparrow::to_table_with_columns(ctx.out(), rb.names(), values_by_columns);
        return ctx.out();
    }
};

inline std::ostream& operator<<(std::ostream& os, const sparrow::record_batch& value)
{
    os << std::format("{}", value);
    return os;
}

#endif
