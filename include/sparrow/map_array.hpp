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

#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/array_factory.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/array_api.hpp"
#include "sparrow/struct_array.hpp"

namespace sparrow
{
    class map_array;

    template <>
    struct array_inner_types<map_array> : array_inner_types_base
    {
        using array_type = map_array;
        using inner_value_type = map_value;
        using inner_reference = map_value;
        using inner_const_reference = map_value;
        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_value_type>>;
        using const_value_iterator = functor_index_iterator<
            detail::layout_value_functor<const array_type, inner_value_type>>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    template <class T>
    constexpr bool is_map_array_v = std::same_as<T, map_array>;

    namespace detail
    {
        template <>
        struct get_data_type_from_array<sparrow::map_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::MAP;
            }
        };
    }

    /**
     * @brief Map array implementation for storing key-value pairs in a list-like structure.
     *
     * This class implements an Arrow-compatible array for storing maps (associative arrays)
     * where each element is a collection of key-value pairs. The implementation uses a
     * struct array with two fields (keys and values) as the child array, combined with
     * an offset buffer to delimit individual maps within the flat structure.
     *
     * The map array follows the Apache Arrow Map specification:
     * https://arrow.apache.org/docs/format/Columnar.html#map-layout
     *
     * @pre Keys within each map should be unique (though not enforced)
     * @pre Keys can optionally be sorted within each map (affects ArrowFlag::MAP_KEYS_SORTED)
     * @post Maintains Arrow Map format compatibility
     * @post Uses struct array with "key" and "value" fields as child array
     * @post Offset buffer has size = array_size + 1
     *
     * @example
     * ```cpp
     * // Create arrays for keys and values
     * primitive_array<std::string> keys({"a", "b", "c", "d"});
     * primitive_array<int> values({1, 2, 3, 4});
     *
     * // Create offset buffer: map 0 has 2 items, map 1 has 2 items
     * std::vector<int32_t> offsets{0, 2, 4};
     *
     * map_array arr(std::move(keys), std::move(values), std::move(offsets));
     * ```
     */
    class SPARROW_API map_array final : public array_bitmap_base<map_array>
    {
    public:

        using self_type = map_array;
        using base_type = array_bitmap_base<self_type>;
        using inner_types = array_inner_types<self_type>;
        using value_iterator = inner_types::value_iterator;
        using const_value_iterator = inner_types::const_value_iterator;
        using size_type = typename base_type::size_type;
        using offset_type = const std::int32_t;
        using offset_buffer_type = u8_buffer<std::remove_const_t<offset_type>>;

        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;

        using const_bitmap_range = typename base_type::const_bitmap_range;

        using inner_value_type = inner_types::inner_value_type;
        using inner_reference = inner_types::inner_reference;
        using inner_const_reference = inner_types::inner_const_reference;

        using value_type = nullable<inner_value_type>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using iterator_tag = typename base_type::iterator_tag;

        /**
         * @brief Constructs map array from Arrow proxy.
         *
         * @param proxy Arrow proxy containing map array data and schema
         *
         * @pre proxy must contain valid Arrow Map array and schema
         * @pre proxy format must be "+m"
         * @pre proxy must have exactly one child array (struct with key/value fields)
         * @pre proxy must have offset buffer at index 1
         * @post Array is initialized with data from proxy
         * @post Keys sorted flag is extracted from Arrow flags
         * @post Offset pointers and child arrays are set up for efficient access
         */
        explicit map_array(arrow_proxy proxy);

        /**
         * @brief Generic constructor for creating map array from various inputs.
         *
         * Creates a map array from different input combinations. Arguments are forwarded
         * to compatible create_proxy() functions based on their types.
         *
         * @tparam Args Parameter pack for constructor arguments
         * @param args Constructor arguments (keys, values, offsets, validity, metadata, etc.)
         *
         * @pre Arguments must match one of the create_proxy() overload signatures
         * @pre Keys and values arrays must have compatible sizes with offsets
         * @pre All offsets must be within bounds of the key/value arrays
         * @post Array is created with the specified data and configuration
         * @post Keys sorted flag is determined from key ordering within each map
         */
        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<map_array, Args...>)
        explicit map_array(Args&&... args)
            : self_type(create_proxy(std::forward<Args>(args)...))
        {
        }

        /**
         * @brief Copy constructor.
         *
         * @param rhs Source array to copy from
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post Child arrays and offset pointers are reconstructed
         */
        map_array(const self_type& rhs);

        /**
         * @brief Copy assignment operator.
         *
         * @param rhs Source array to copy from
         * @return Reference to this array
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post Previous data is properly released
         * @post Child arrays and offset pointers are reconstructed
         */
        map_array& operator=(const self_type& rhs);

        map_array(map_array&&) noexcept = default;
        map_array& operator=(map_array&&) noexcept = default;

        /**
         * @brief Gets read-only access to the keys array.
         *
         * @return Const pointer to the flat keys array
         *
         * @post Returns non-null pointer to valid array_wrapper containing all keys
         */
        [[nodiscard]] const array_wrapper* raw_keys_array() const;

        /**
         * @brief Gets mutable access to the keys array.
         *
         * @return Pointer to the flat keys array
         *
         * @post Returns non-null pointer to valid array_wrapper containing all keys
         */
        [[nodiscard]] array_wrapper* raw_keys_array();

        /**
         * @brief Gets read-only access to the values array.
         *
         * @return Const pointer to the flat values array
         *
         * @post Returns non-null pointer to valid array_wrapper containing all values
         */
        [[nodiscard]] const array_wrapper* raw_items_array() const;

        /**
         * @brief Gets mutable access to the values array.
         *
         * @return Pointer to the flat values array
         *
         * @post Returns non-null pointer to valid array_wrapper containing all values
         */
        [[nodiscard]] array_wrapper* raw_items_array();

        /**
         * @brief Creates offset buffer from map sizes.
         *
         * Converts a range of map sizes (number of key-value pairs per map) into
         * cumulative offsets. The resulting offset buffer has size = sizes.size() + 1,
         * with the first element being 0 and subsequent elements being cumulative sums.
         *
         * @tparam SIZES_RANGE Type of input range containing map sizes
         * @param sizes Range of map sizes (number of pairs per map)
         * @return Offset buffer suitable for map array construction
         *
         * @pre sizes must be a valid range of non-negative integers
         * @pre All sizes must fit within the offset_type (int32_t) range
         * @post Returned buffer has size = sizes.size() + 1
         * @post First offset is 0, last offset is sum of all sizes
         * @post Each offset[i+1] = offset[i] + sizes[i]
         */
        template <std::ranges::range SIZES_RANGE>
        [[nodiscard]] static auto offset_from_sizes(SIZES_RANGE&& sizes) -> offset_buffer_type;

    private:

        /**
         * @brief Gets iterator to beginning of value range.
         *
         * @return Iterator pointing to the first map element
         *
         * @post Returns valid iterator to array beginning
         */
        [[nodiscard]] value_iterator value_begin();

        /**
         * @brief Gets iterator to end of value range.
         *
         * @return Iterator pointing past the last map element
         *
         * @post Returns valid iterator to array end
         */
        [[nodiscard]] value_iterator value_end();

        /**
         * @brief Gets const iterator to beginning of value range.
         *
         * @return Const iterator pointing to the first map element
         *
         * @post Returns valid const iterator to array beginning
         */
        [[nodiscard]] const_value_iterator value_cbegin() const;

        /**
         * @brief Gets const iterator to end of value range.
         *
         * @return Const iterator pointing past the last map element
         *
         * @post Returns valid const iterator to array end
         */
        [[nodiscard]] const_value_iterator value_cend() const;

        /**
         * @brief Gets mutable reference to map at specified index.
         *
         * @param i Index of the map to access
         * @return Mutable reference to the map value
         *
         * @pre i must be < size()
         * @post Returns valid reference that provides access to key-value pairs
         */
        [[nodiscard]] inner_reference value(size_type i);

        /**
         * @brief Gets const reference to map at specified index.
         *
         * @param i Index of the map to access
         * @return Const reference to the map value
         *
         * @pre i must be < size()
         * @post Returns valid const reference that provides access to key-value pairs
         */
        [[nodiscard]] inner_const_reference value(size_type i) const;

        /**
         * @brief Gets pointer to the offset buffer.
         *
         * @return Pointer to the beginning of the offset buffer
         *
         * @post Returns non-null pointer to offset data
         * @post Pointer is adjusted for any array offset
         */
        [[nodiscard]] offset_type* make_list_offsets() const;

        /**
         * @brief Creates the entries array (struct with key/value fields).
         *
         * @return Cloning pointer to the entries struct array
         *
         * @post Returns valid array_wrapper containing struct array
         * @post Struct array has exactly 2 fields: keys and values
         */
        [[nodiscard]] cloning_ptr<array_wrapper> make_entries_array() const;

        /**
         * @brief Gets whether keys are sorted within each map.
         *
         * @return True if keys are sorted, false otherwise
         *
         * @post Returns the value of ArrowFlag::MAP_KEYS_SORTED from schema flags
         */
        [[nodiscard]] bool get_keys_sorted() const;

        /**
         * @brief Checks if keys are sorted within each map.
         *
         * Analyzes the flat keys array and offset buffer to determine if keys
         * within each individual map are in sorted order.
         *
         * @param flat_keys The flat array of all keys
         * @param offsets The offset buffer delimiting maps
         * @return True if keys are sorted within each map, false otherwise
         *
         * @pre flat_keys must be a valid array
         * @pre offsets must be a valid offset buffer with offsets.size() >= 1
         * @pre All offsets must be within bounds of flat_keys
         * @post Returns true only if keys within each map are in sorted order
         */
        [[nodiscard]] static bool check_keys_sorted(const array& flat_keys, const offset_buffer_type& offsets);

        /**
         * @brief Creates Arrow proxy from keys, values, offsets, and validity bitmap.
         *
         * @tparam VB Type of validity bitmap input
         * @tparam METADATA_RANGE Type of metadata container
         * @param flat_keys Array containing all map keys in flattened form
         * @param flat_items Array containing all map values in flattened form
         * @param list_offsets Buffer of offsets indicating map boundaries
         * @param validity_input Validity bitmap specification
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the map array data and schema
         *
         * @pre flat_keys must be a valid array
         * @pre flat_items must be a valid array
         * @pre flat_keys.size() must equal flat_items.size()
         * @pre list_offsets.size() must be >= 1
         * @pre Last offset must not exceed flat_keys.size()
         * @pre validity_input size must match list_offsets.size() - 1
         * @post Returns valid Arrow proxy with Map format ("+m")
         * @post Child array is struct with key and value fields
         * @post Keys sorted flag is set based on actual key ordering
         */
        template <
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_keys,
            array&& flat_items,
            offset_buffer_type&& list_offsets,
            VB&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates Arrow proxy from keys, values, offset range, and validity bitmap.
         *
         * @tparam VB Type of validity bitmap input
         * @tparam OFFSET_BUFFER_RANGE Type of offset range
         * @tparam METADATA_RANGE Type of metadata container
         * @param flat_keys Array containing all map keys
         * @param flat_items Array containing all map values
         * @param list_offsets_range Range of offsets convertible to offset_type
         * @param validity_input Validity bitmap specification
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the map array data and schema
         *
         * @pre flat_keys must be a valid array
         * @pre flat_items must be a valid array
         * @pre flat_keys.size() must equal flat_items.size()
         * @pre list_offsets_range elements must be convertible to offset_type
         * @pre list_offsets_range.size() must be >= 1
         * @pre validity_input size must match list_offsets_range.size() - 1
         * @post Returns valid Arrow proxy with Map format
         */
        template <
            validity_bitmap_input VB = validity_bitmap,
            std::ranges::input_range OFFSET_BUFFER_RANGE,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<std::ranges::range_value_t<OFFSET_BUFFER_RANGE>, offset_type>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_keys,
            array&& flat_items,
            OFFSET_BUFFER_RANGE&& list_offsets_range,
            VB&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
        {
            offset_buffer_type list_offsets{std::move(list_offsets_range)};
            return map_array::create_proxy(
                std::move(flat_keys),
                std::move(flat_items),
                std::move(list_offsets),
                std::forward<VB>(validity_input),
                std::forward<std::optional<std::string_view>>(name),
                std::forward<std::optional<METADATA_RANGE>>(metadata)
            );
        }

        /*
         * @brief Creates Arrow proxy from keys, values, offsets, and nullable flag.
         *
         * @tparam METADATA_RANGE Type of metadata container
         * @param flat_keys Array containing all map keys
         * @param flat_values Array containing all map values
         * @param list_offsets Buffer of offsets indicating map boundaries
         * @param nullable Whether the array should support null values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the map array data and schema
         *
         * @pre flat_keys must be a valid array
         * @pre flat_values must be a valid array
         * @pre flat_keys.size() must equal flat_values.size()
         * @pre list_offsets.size() must be >= 1
         * @pre Last offset must not exceed flat_keys.size()
         * @post If nullable is true, array supports null values (though none initially set)
         * @post If nullable is false, array does not support null values
         * @post Keys sorted flag is determined from actual key ordering
         */
        template <
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_keys,
            array&& flat_values,
            offset_buffer_type&& list_offsets,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates Arrow proxy from keys, values, offset range, and nullable flag.
         *
         * @tparam VB Type of validity bitmap input
         * @tparam OFFSET_BUFFER_RANGE Type of offset range
         * @tparam METADATA_RANGE Type of metadata container
         * @param flat_keys Array containing all map keys
         * @param flat_items Array containing all map values
         * @param list_offsets_range Range of offsets convertible to offset_type
         * @param nullable Whether the array should support null values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the map array data and schema
         *
         * @pre flat_keys must be a valid array
         * @pre flat_items must be a valid array
         * @pre flat_keys.size() must equal flat_items.size()
         * @pre list_offsets_range elements must be convertible to offset_type
         * @pre list_offsets_range.size() must be >= 1
         * @post Returns valid Arrow proxy with Map format
         * @post Nullable behavior matches the nullable parameter
         */
        template <
            validity_bitmap_input VB = validity_bitmap,
            std::ranges::input_range OFFSET_BUFFER_RANGE,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<std::ranges::range_value_t<OFFSET_BUFFER_RANGE>, offset_type>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_keys,
            array&& flat_items,
            OFFSET_BUFFER_RANGE&& list_offsets_range,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
        {
            offset_buffer_type list_offsets{std::move(list_offsets_range)};
            return map_array::create_proxy(
                std::move(flat_keys),
                std::move(flat_items),
                std::move(list_offsets),
                nullable,
                std::forward<std::optional<std::string_view>>(name),
                std::forward<std::optional<METADATA_RANGE>>(metadata)
            );
        }

        static constexpr std::size_t OFFSET_BUFFER_INDEX = 1;
        offset_type* p_list_offsets;

        cloning_ptr<array_wrapper> p_entries_array;
        bool m_keys_sorted;

        // friend classes
        friend class array_crtp_base<map_array>;
        friend class detail::layout_value_functor<const map_array, map_value>;
    };

    template <std::ranges::range SIZES_RANGE>
    auto map_array::offset_from_sizes(SIZES_RANGE&& sizes) -> offset_buffer_type
    {
        return detail::offset_buffer_from_sizes<std::remove_const_t<offset_type>>(
            std::forward<SIZES_RANGE>(sizes)
        );
    }

    template <validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
    arrow_proxy map_array::create_proxy(
        array&& flat_keys,
        array&& flat_items,
        offset_buffer_type&& list_offsets,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto size = list_offsets.size() - 1;
        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<VB>(validity_input));

        std::optional<std::unordered_set<ArrowFlag>> flags{{ArrowFlag::NULLABLE}};
        bool keys_sorted = check_keys_sorted(flat_keys, list_offsets);
        if (keys_sorted)
        {
            flags.value().insert(ArrowFlag::MAP_KEYS_SORTED);
        }

        std::array<sparrow::array, 2> struct_children = {std::move(flat_keys), std::move(flat_items)};
        struct_array entries(std::move(struct_children), false, std::string("entries"));

        auto [entries_arr, entries_schema] = extract_arrow_structures(std::move(entries));

        const auto null_count = vbitmap.null_count();
        const repeat_view<bool> children_ownership{true, 1};

        ArrowSchema schema = make_arrow_schema(
            std::string("+m"),
            name,      // name
            metadata,  // metadata
            flags,     // flags,
            new ArrowSchema*[1]{new ArrowSchema(std::move(entries_schema))},
            children_ownership,  // children ownership
            nullptr,             // dictionary
            true                 // dictionary ownership

        );

        std::vector<buffer<std::uint8_t>> arr_buffs = {
            std::move(vbitmap).extract_storage(),
            std::move(list_offsets).extract_storage()
        };

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<std::int64_t>(null_count),
            0,  // offset
            std::move(arr_buffs),
            new ArrowArray*[1]{new ArrowArray(std::move(entries_arr))},
            children_ownership,  // children ownership
            nullptr,             // dictionary
            true                 // dictionary ownership
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
    arrow_proxy map_array::create_proxy(
        array&& flat_keys,
        array&& flat_items,
        offset_buffer_type&& list_offsets,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        if (nullable)
        {
            return map_array::create_proxy(
                std::move(flat_keys),
                std::move(flat_items),
                std::move(list_offsets),
                validity_bitmap{},
                name,
                metadata
            );
        }
        else
        {
            bool keys_sorted = check_keys_sorted(flat_keys, list_offsets);
            auto flags = keys_sorted
                             ? std::optional<std::unordered_set<ArrowFlag>>{{ArrowFlag::MAP_KEYS_SORTED}}
                             : std::nullopt;

            const auto size = list_offsets.size() - 1;

            std::array<sparrow::array, 2> struct_children = {std::move(flat_keys), std::move(flat_items)};
            struct_array entries(std::move(struct_children), false, std::string("entries"));

            auto [entries_arr, entries_schema] = extract_arrow_structures(std::move(entries));
            const repeat_view<bool> children_ownership{true, 1};

            ArrowSchema schema = make_arrow_schema(
                std::string_view("+m"),
                name,      // name
                metadata,  // metadata
                flags,     // flags,
                new ArrowSchema*[1]{new ArrowSchema(std::move(entries_schema))},
                children_ownership,  // children ownership
                nullptr,             // dictionary
                true                 // dictionary ownership

            );

            std::vector<buffer<std::uint8_t>> arr_buffs = {
                buffer<std::uint8_t>{nullptr, 0},  // no validity bitmap
                std::move(list_offsets).extract_storage()
            };

            ArrowArray arr = make_arrow_array(
                static_cast<std::int64_t>(size),  // length
                0,
                0,  // offset
                std::move(arr_buffs),
                new ArrowArray*[1]{new ArrowArray(std::move(entries_arr))},
                children_ownership,  // children ownership
                nullptr,             // dictionary
                true                 // dictionary ownership
            );
            return arrow_proxy{std::move(arr), std::move(schema)};
        }
    }
}
