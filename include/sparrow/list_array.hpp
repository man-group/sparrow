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

#include <limits>
#include <ranges>
#include <stdexcept>
#include <string>  // for std::stoull
#include <type_traits>
#include <utility>
#include <vector>

#include "sparrow/array_api.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/debug/copy_tracker.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/memory.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/utils/repeat_container.hpp"

namespace sparrow
{
    template <class DERIVED>
    class list_array_crtp_base;

    template <bool BIG>
    class list_array_impl;

    template <bool BIG>
    class list_view_array_impl;

    namespace copy_tracker
    {
        template <typename T>
            requires std::same_as<T, list_array_impl<false>> || std::same_as<T, list_array_impl<true>>
        std::string key()
        {
            return "list_array";
        }

        template <typename T>
            requires std::same_as<T, list_view_array_impl<false>> || std::same_as<T, list_view_array_impl<true>>
        std::string key()
        {
            return "list_view_array";
        }
    }

    /**
     * A list array implementation.
     * Stores variable-length lists of values, where each list can have a different length.
     * Uses 32-bit offsets for smaller datasets.
     *
     * Related Apache Arrow description and specification:
     * - https://arrow.apache.org/docs/dev/format/Intro.html#list
     * - https://arrow.apache.org/docs/format/Columnar.html#list-layout
     * @see big_list_array
     */
    using list_array = list_array_impl<false>;
    /**
     * A big list array implementation.
     * Stores variable-length lists of values, where each list can have a different length.
     * Uses 64-bit offsets for larger datasets.
     *
     * Related Apache Arrow description and specification:
     * - https://arrow.apache.org/docs/dev/format/Intro.html#list
     * - https://arrow.apache.org/docs/format/Columnar.html#list-layout
     * @see list_array
     */
    using big_list_array = list_array_impl<true>;

    /**
     * A list view array implementation.
     * Stores variable-length lists where each element can contain a different number of sub-elements.
     * Use the List layout when your data consists of variable-length lists and you want a straightforward,
     * efficient representation where the order of elements in the child array matches the logical order in
     * the parent array. This is the standard layout for most use cases involving variable-length lists, such
     * as arrays of strings or arrays of arrays of numbers.
     *
     * Related Apache Arrow description and specification:
     * - https://arrow.apache.org/docs/dev/format/Intro.html#list-view
     * - https://arrow.apache.org/docs/dev/format/Columnar.html#listview-layout
     */
    using list_view_array = list_view_array_impl<false>;
    using big_list_view_array = list_view_array_impl<true>;

    class fixed_sized_list_array;

    namespace copy_tracker
    {
        template <>
        inline std::string key<fixed_sized_list_array>()
        {
            return "fixed_sized_list_array";
        }
    }


    /**
     * Checks whether T is a list_array type.
     */
    template <class T>
    constexpr bool is_list_array_v = std::same_as<T, list_array>;

    /**
     * Checks whether T is a big_list_array type.
     */
    template <class T>
    constexpr bool is_big_list_array_v = std::same_as<T, big_list_array>;

    /**
     * Checks whether T is a list_view_array type.
     */
    template <class T>
    constexpr bool is_list_view_array_v = std::same_as<T, list_view_array>;

    /**
     * Checks whether T is a big_list_view_array type.
     */
    template <class T>
    constexpr bool is_big_list_view_array_v = std::same_as<T, big_list_view_array>;

    /**
     * Checks whether T is a fixed_sized_list_array type.
     */
    template <class T>
    constexpr bool is_fixed_sized_list_array_v = std::same_as<T, fixed_sized_list_array>;

    namespace detail
    {
        template <bool BIG>
        struct get_data_type_from_array<sparrow::list_array_impl<BIG>>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return BIG ? sparrow::data_type::LARGE_LIST : sparrow::data_type::LIST;
            }
        };

        template <bool BIG>
        struct get_data_type_from_array<sparrow::list_view_array_impl<BIG>>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return BIG ? sparrow::data_type::LARGE_LIST_VIEW : sparrow::data_type::LIST_VIEW;
            }
        };

        template <>
        struct get_data_type_from_array<sparrow::fixed_sized_list_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::FIXED_SIZED_LIST;
            }
        };

        // Helper to build arrow schema for list arrays
        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        inline ArrowSchema make_list_arrow_schema(
            std::string format,
            ArrowSchema&& flat_schema,
            std::optional<std::string_view> name,
            std::optional<METADATA_RANGE> metadata,
            bool nullable
        )
        {
            const repeat_view<bool> children_ownership{true, 1};
            std::optional<std::unordered_set<ArrowFlag>>
                flags = nullable ? std::optional<std::unordered_set<ArrowFlag>>{{ArrowFlag::NULLABLE}}
                                 : std::nullopt;

            return make_arrow_schema(
                std::move(format),
                name,
                metadata,
                flags,
                new ArrowSchema*[1]{new ArrowSchema(std::move(flat_schema))},
                children_ownership,
                nullptr,  // dictionary
                true      // dictionary ownership
            );
        }

        // Helper to build arrow array for list arrays
        inline ArrowArray make_list_arrow_array(
            std::int64_t size,
            std::int64_t null_count,
            std::vector<buffer<std::uint8_t>>&& arr_buffs,
            ArrowArray&& flat_arr
        )
        {
            const repeat_view<bool> children_ownership{true, 1};
            return make_arrow_array(
                size,
                null_count,
                0,  // offset
                std::move(arr_buffs),
                new ArrowArray*[1]{new ArrowArray(std::move(flat_arr))},
                children_ownership,
                nullptr,  // dictionary
                true      // dictionary ownership
            );
        }
    }

    template <bool BIG>
    struct array_inner_types<list_array_impl<BIG>> : array_inner_types_base
    {
        using list_size_type = std::conditional_t<BIG, std::uint64_t, std::uint32_t>;
        using array_type = list_array_impl<BIG>;
        using inner_value_type = list_value;
        using inner_reference = list_reference<array_type>;
        using inner_const_reference = list_value;
        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_reference>>;
        using const_value_iterator = functor_index_iterator<
            detail::layout_value_functor<const array_type, inner_const_reference>>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    template <bool BIG>
    struct array_inner_types<list_view_array_impl<BIG>> : array_inner_types_base
    {
        using list_size_type = std::conditional_t<BIG, std::uint64_t, std::uint32_t>;
        using array_type = list_view_array_impl<BIG>;
        using inner_value_type = list_value;
        using inner_reference = list_reference<array_type>;
        using inner_const_reference = list_value;
        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_reference>>;
        using const_value_iterator = functor_index_iterator<
            detail::layout_value_functor<const array_type, inner_const_reference>>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    template <>
    struct array_inner_types<fixed_sized_list_array> : array_inner_types_base
    {
        using list_size_type = std::uint64_t;
        using array_type = fixed_sized_list_array;
        using inner_value_type = list_value;
        using inner_reference = list_reference<array_type>;
        using inner_const_reference = list_value;
        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_reference>>;
        using const_value_iterator = functor_index_iterator<
            detail::layout_value_functor<const array_type, inner_const_reference>>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    /**
     * @brief CRTP base class for all list array implementations.
     *
     * This class provides common functionality for list-based array types including
     * list_array, big_list_array, list_view_array, big_list_view_array, and
     * fixed_sized_list_array. It manages the flat array of values and provides
     * iteration and access methods.
     *
     * @tparam DERIVED The derived list array type (CRTP pattern)
     *
     * @pre DERIVED must implement offset_range(size_type) method
     * @note Mutating operations are supported only on unsliced arrays.
     *       Arrays produced by slice() or slice_view() with a non-zero Arrow offset are
     *       treated as read-only for mutation.
     * @post Maintains Arrow array format compatibility for list types
     * @post Provides unified interface for all list array variants
     */
    template <class DERIVED>
    class list_array_crtp_base : public mutable_array_bitmap_base<DERIVED>
    {
    public:

        using self_type = list_array_crtp_base<DERIVED>;
        using base_type = mutable_array_bitmap_base<DERIVED>;
        using inner_types = array_inner_types<DERIVED>;
        using value_iterator = typename inner_types::value_iterator;
        using const_value_iterator = typename inner_types::const_value_iterator;
        using size_type = typename base_type::size_type;

        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_reference = typename base_type::bitmap_reference;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;

        using const_bitmap_range = typename base_type::const_bitmap_range;

        using inner_value_type = list_value;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;

        using value_type = nullable<inner_value_type>;
        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using iterator_tag = typename base_type::iterator_tag;

        /**
         * @brief Gets read-only access to the underlying flat array.
         *
         * @return Const pointer to the flat array containing all list elements
         *
         * @post Returns non-null pointer to a valid flat child handle
         */
        [[nodiscard]] constexpr const array* raw_flat_array() const;

        /**
         * @brief Gets mutable access to the underlying flat array.
         *
         * @return Pointer to the flat array containing all list elements
         *
         * @post Returns non-null pointer to a valid flat child handle
         */
        [[nodiscard]] constexpr array* raw_flat_array();

    protected:

        /**
         * @brief Constructs list array base from Arrow proxy.
         *
         * @param proxy Arrow proxy containing array data and schema
         *
         * @pre proxy must contain valid Arrow array and schema for list type
         * @pre proxy must have exactly one child array (the flat values array)
         */
        explicit list_array_crtp_base(arrow_proxy proxy);

        /**
         * @brief Copy constructor.
         *
         * @param rhs Source array to copy from
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post Flat array is reconstructed from copied data
         */
        constexpr list_array_crtp_base(const self_type&);

        /**
         * @brief Copy assignment operator.
         *
         * @param rhs Source array to copy from
         * @return Reference to this array
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post Previous data is properly released
         * @post Flat array is reconstructed from copied data
         */
        constexpr list_array_crtp_base& operator=(const self_type&);

        constexpr list_array_crtp_base(self_type&&) noexcept = default;
        constexpr list_array_crtp_base& operator=(self_type&&) noexcept = default;

    protected:

        /**
         * @brief Throws if a mutating operation is attempted on a sliced array.
         *
         * List-array mutation updates the parent offset or size buffers together with
         * the flat child array in place. The current implementation assumes
         * ArrowArray.offset == 0, so sliced arrays are intentionally unsupported for
         * mutating APIs.
         *
         * @param operation Name of the mutating operation for diagnostics.
         *
         * @throws std::logic_error if this array has a non-zero Arrow offset.
         */
        constexpr void throw_if_sliced_for_mutation(const char* operation) const;

        [[nodiscard]] constexpr value_iterator value_begin();
        [[nodiscard]] constexpr value_iterator value_end();
        [[nodiscard]] constexpr const_value_iterator value_cbegin() const;
        [[nodiscard]] constexpr const_value_iterator value_cend() const;

        // Inserts `count` copies of value's flat elements into the flat array at flat_pos.
        // No-op if value.size() == 0.
        constexpr void insert_flat_elements(size_type flat_pos, list_value value, size_type count);

        // Erases flat_count consecutive flat elements starting at flat_begin.
        // No-op if flat_count == 0.
        constexpr void erase_flat_elements(size_type flat_begin, size_type flat_count);

    private:

        using list_size_type = inner_types::list_size_type;

        constexpr void resize_values(size_type new_length, list_value value);

        template <std::input_iterator InputIt>
            requires std::convertible_to<typename std::iterator_traits<InputIt>::value_type, list_value>
        constexpr value_iterator insert_values(const_value_iterator pos, InputIt first, InputIt last);

        [[nodiscard]] constexpr inner_reference value(size_type i);
        [[nodiscard]] constexpr inner_const_reference value(size_type i) const;

        [[nodiscard]] array make_flat_array();

        // data members
        array p_flat_array;

        // friend classes
        friend class array_crtp_base<DERIVED>;
        friend class mutable_array_base<DERIVED>;
        friend class list_reference<DERIVED>;

        // needs access to this->value(i)
        friend class detail::layout_value_functor<DERIVED, inner_reference>;
        friend class detail::layout_value_functor<const DERIVED, inner_const_reference>;
    };

    template <bool BIG>
    class list_array_impl final : public list_array_crtp_base<list_array_impl<BIG>>
    {
    public:

        using self_type = list_array_impl<BIG>;
        using inner_types = array_inner_types<self_type>;
        using base_type = list_array_crtp_base<list_array_impl<BIG>>;
        using list_size_type = inner_types::list_size_type;
        using size_type = typename base_type::size_type;
        using value_iterator = typename base_type::value_iterator;
        using const_value_iterator = typename base_type::const_value_iterator;
        using offset_type = std::conditional_t<BIG, const std::int64_t, const std::int32_t>;
        using offset_buffer_type = u8_buffer<std::remove_const_t<offset_type>>;

        /**
         * @brief Constructs list array from Arrow proxy.
         *
         * @param proxy Arrow proxy containing list array data and schema
         *
         * @pre proxy must contain valid Arrow List or Large List array and schema
         * @pre proxy format must match BIG template parameter
         * @pre proxy must have offset buffer at index 1
         * @post Array is initialized with data from proxy
         * @post Offset pointers are set up for efficient access
         */
        explicit list_array_impl(arrow_proxy proxy);

        /**
         * @brief Copy constructor.
         *
         * @param rhs Source array to copy from
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post Offset pointers are recalculated for the new data
         */
        constexpr list_array_impl(const self_type&);

        /**
         * @brief Copy assignment operator.
         *
         * @param rhs Source array to copy from
         * @return Reference to this array
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post Previous data is properly released
         * @post Offset pointers are recalculated for the new data
         */
        constexpr list_array_impl& operator=(const self_type&);

        constexpr list_array_impl(self_type&&) noexcept = default;
        constexpr list_array_impl& operator=(self_type&&) noexcept = default;

        constexpr void slice_inplace(size_type start, size_type end);

        /**
         * @brief Generic constructor for creating list array from various inputs.
         *
         * @tparam ARGS Parameter pack for constructor arguments
         * @param args Constructor arguments (flat_values, offsets, validity, etc.)
         *
         * @pre First argument must be a valid array for flat values
         * @pre Second argument must be offset buffer or range convertible to offsets
         * @pre If validity is provided, it must match the number of lists
         * @post Array is created with the specified data and configuration
         */
        template <class... ARGS>
            requires(mpl::excludes_copy_and_move_ctor_v<list_array_impl<BIG>, ARGS...>)
        explicit list_array_impl(ARGS&&... args)
            : self_type(create_proxy(std::forward<ARGS>(args)...))
        {
        }

        /**
         * @brief Creates offset buffer from list sizes.
         *
         * Converts a range of list sizes into cumulative offsets.
         * The resulting offset buffer has size = sizes.size() + 1, with
         * the first element being 0 and subsequent elements being cumulative sums.
         *
         * @tparam SIZES_RANGE Type of input range containing list sizes
         * @param sizes Range of list sizes
         * @return Offset buffer suitable for list array construction
         *
         * @pre sizes must be a valid range of non-negative integers
         * @pre All sizes must fit within the offset_type range
         * @post Returned buffer has size = sizes.size() + 1
         * @post First offset is 0, last offset is sum of all sizes
         * @post Each offset[i+1] = offset[i] + sizes[i]
         */
        template <std::ranges::range SIZES_RANGE>
        [[nodiscard]] static constexpr auto offset_from_sizes(SIZES_RANGE&& sizes) -> offset_buffer_type;

    private:

        /**
         * @brief Creates Arrow proxy from flat values, offsets, and validity bitmap.
         *
         * @tparam VB Type of validity bitmap input
         * @tparam METADATA_RANGE Type of metadata container
         * @param flat_values Array containing all list elements in flattened form
         * @param list_offsets Buffer of offsets indicating list boundaries
         * @param validity_input Validity bitmap specification
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the list array data and schema
         *
         * @pre flat_values must be a valid array
         * @pre list_offsets.size() must be >= 1
         * @pre Last offset must not exceed flat_values.size()
         * @pre validity_input size must match list_offsets.size() - 1
         * @post Returns valid Arrow proxy with List or Large List format
         * @post Child array contains the flat_values
         */
        template <
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_values,
            offset_buffer_type&& list_offsets,
            VB&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates Arrow proxy from flat values, offset range, and validity bitmap.
         *
         * @tparam VB Type of validity bitmap input
         * @tparam OFFSET_BUFFER_RANGE Type of offset range
         * @tparam METADATA_RANGE Type of metadata container
         * @param flat_values Array containing all list elements
         * @param list_offsets_range Range of offsets convertible to offset_type
         * @param validity_input Validity bitmap specification
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the list array data and schema
         *
         * @pre flat_values must be a valid array
         * @pre list_offsets_range elements must be convertible to offset_type
         * @pre list_offsets_range.size() must be >= 1
         * @pre validity_input size must match list_offsets_range.size() - 1
         */
        template <
            validity_bitmap_input VB = validity_bitmap,
            std::ranges::input_range OFFSET_BUFFER_RANGE,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<std::ranges::range_value_t<OFFSET_BUFFER_RANGE>, offset_type>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_values,
            OFFSET_BUFFER_RANGE&& list_offsets_range,
            VB&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
        {
            offset_buffer_type list_offsets{std::forward<OFFSET_BUFFER_RANGE>(list_offsets_range)};
            return list_array_impl<BIG>::create_proxy(
                std::move(flat_values),
                std::move(list_offsets),
                std::forward<VB>(validity_input),
                std::forward<std::optional<std::string_view>>(name),
                std::forward<std::optional<METADATA_RANGE>>(metadata)
            );
        }

        template <
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_values,
            offset_buffer_type&& list_offsets,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        template <
            validity_bitmap_input VB = validity_bitmap,
            std::ranges::input_range OFFSET_BUFFER_RANGE,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<std::ranges::range_value_t<OFFSET_BUFFER_RANGE>, offset_type>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_values,
            OFFSET_BUFFER_RANGE&& list_offsets_range,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
        {
            offset_buffer_type list_offsets{std::forward<OFFSET_BUFFER_RANGE>(list_offsets_range)};
            return list_array_impl<BIG>::create_proxy(
                std::move(flat_values),
                std::move(list_offsets),
                nullable,
                std::forward<std::optional<std::string_view>>(name),
                std::forward<std::optional<METADATA_RANGE>>(metadata)
            );
        }

        static constexpr std::size_t OFFSET_BUFFER_INDEX = 1;
        [[nodiscard]] constexpr std::pair<offset_type, offset_type> offset_range(size_type i) const;

        [[nodiscard]] constexpr offset_type* make_list_offsets();

        void replace_value(size_type index, list_value value);

        constexpr value_iterator insert_value(const_value_iterator pos, list_value value, size_type count);

        constexpr value_iterator erase_values(const_value_iterator pos, size_type count);

        offset_type* p_list_offsets;

        // friend classes
        friend class array_crtp_base<self_type>;
        friend class mutable_array_base<self_type>;
        friend class list_array_crtp_base<self_type>;
        template <class L>
        friend class list_reference;
    };

    template <bool BIG>
    class list_view_array_impl final : public list_array_crtp_base<list_view_array_impl<BIG>>
    {
    public:

        using self_type = list_view_array_impl<BIG>;
        using inner_types = array_inner_types<self_type>;
        using base_type = list_array_crtp_base<list_view_array_impl<BIG>>;
        using list_size_type = inner_types::list_size_type;
        using size_type = typename base_type::size_type;
        using value_iterator = typename base_type::value_iterator;
        using const_value_iterator = typename base_type::const_value_iterator;
        using offset_type = std::conditional_t<BIG, const std::int64_t, const std::int32_t>;
        using offset_buffer_type = u8_buffer<std::remove_const_t<offset_type>>;
        using size_buffer_type = u8_buffer<std::remove_const_t<list_size_type>>;

        /**
         * @brief Constructs list view array from Arrow proxy.
         *
         * @param proxy Arrow proxy containing list view array data and schema
         *
         * @pre proxy must contain valid Arrow List View or Large List View array
         * @pre proxy format must match BIG template parameter
         * @pre proxy must have offset buffer at index 1 and size buffer at index 2
         * @post Array is initialized with data from proxy
         * @post Offset and size pointers are set up for efficient access
         */
        explicit list_view_array_impl(arrow_proxy proxy);

        /**
         * @brief Copy constructor.
         *
         * @param rhs Source array to copy from
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post Offset and size pointers are recalculated for the new data
         */
        constexpr list_view_array_impl(const self_type&);

        /**
         * @brief Copy assignment operator.
         *
         * @param rhs Source array to copy from
         * @return Reference to this array
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post Previous data is properly released
         * @post Offset and size pointers are recalculated for the new data
         */
        constexpr list_view_array_impl& operator=(const self_type&);

        constexpr list_view_array_impl(self_type&&) = default;
        constexpr list_view_array_impl& operator=(self_type&&) = default;

        constexpr void slice_inplace(size_type start, size_type end);

        /**
         * @brief Generic constructor for creating list view array from various inputs.
         *
         * @tparam ARGS Parameter pack for constructor arguments
         * @param args Constructor arguments (flat_values, offsets, sizes, validity, etc.)
         *
         * @pre First argument must be a valid array for flat values
         * @pre Second argument must be offset buffer or range
         * @pre Third argument must be size buffer or range
         * @pre Offset and size ranges must have the same length
         * @post Array is created with the specified data and configuration
         */
        template <class... ARGS>
            requires(mpl::excludes_copy_and_move_ctor_v<list_view_array_impl<BIG>, ARGS...>)
        list_view_array_impl(ARGS&&... args)
            : self_type(create_proxy(std::forward<ARGS>(args)...))
        {
        }

    private:

        /**
         * @brief Creates Arrow proxy from flat values, offsets, sizes, and validity.
         *
         * @tparam OFFSET_BUFFER_RANGE Type of offset buffer range
         * @tparam SIZE_RANGE Type of size buffer range
         * @tparam VB Type of validity bitmap input
         * @tparam METADATA_RANGE Type of metadata container
         * @param flat_values Array containing all list elements
         * @param list_offsets Range of starting offsets for each list
         * @param list_sizes Range of sizes for each list
         * @param validity_input Validity bitmap specification
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the list view array data and schema
         *
         * @pre flat_values must be a valid array
         * @pre list_offsets and list_sizes must have the same length
         * @pre All offsets + sizes must be within flat_values bounds
         * @pre validity_input size must match list_offsets.size()
         * @post Returns valid Arrow proxy with List View or Large List View format
         *
         * @note Internal assertion: SPARROW_ASSERT(list_offsets.size() == list_sizes.size())
         */
        template <
            std::ranges::input_range OFFSET_BUFFER_RANGE,
            std::ranges::input_range SIZE_RANGE,
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(
                std::convertible_to<std::ranges::range_value_t<OFFSET_BUFFER_RANGE>, offset_type>
                && std::convertible_to<std::ranges::range_value_t<SIZE_RANGE>, list_size_type>
            )
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_values,
            OFFSET_BUFFER_RANGE&& list_offsets,
            SIZE_RANGE&& list_sizes,
            VB&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
        {
            return list_view_array_impl<BIG>::create_proxy(
                std::move(flat_values),
                offset_buffer_type(std::forward<OFFSET_BUFFER_RANGE>(list_offsets)),
                size_buffer_type(std::forward<SIZE_RANGE>(list_sizes)),
                std::forward<VB>(validity_input),
                name,
                metadata
            );
        }

        template <
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_values,
            offset_buffer_type&& list_offsets,
            size_buffer_type&& list_sizes,
            VB&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        template <
            std::ranges::input_range OFFSET_BUFFER_RANGE,
            std::ranges::input_range SIZE_RANGE,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(
                std::convertible_to<std::ranges::range_value_t<OFFSET_BUFFER_RANGE>, offset_type>
                && std::convertible_to<std::ranges::range_value_t<SIZE_RANGE>, list_size_type>
            )
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_values,
            OFFSET_BUFFER_RANGE&& list_offsets,
            SIZE_RANGE&& list_sizes,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
        {
            return list_view_array_impl<BIG>::create_proxy(
                std::move(flat_values),
                offset_buffer_type(std::forward<OFFSET_BUFFER_RANGE>(list_offsets)),
                size_buffer_type(std::forward<SIZE_RANGE>(list_sizes)),
                nullable,
                name,
                metadata
            );
        }

        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_values,
            offset_buffer_type&& list_offsets,
            size_buffer_type&& list_sizes,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        static constexpr std::size_t OFFSET_BUFFER_INDEX = 1;
        static constexpr std::size_t SIZES_BUFFER_INDEX = 2;
        [[nodiscard]] constexpr std::pair<offset_type, offset_type> offset_range(size_type i) const;

        [[nodiscard]] constexpr offset_type* make_list_offsets() const;
        [[nodiscard]] constexpr const list_size_type* make_list_sizes() const;

        void replace_value(size_type index, list_value value);

        constexpr value_iterator insert_value(const_value_iterator pos, list_value value, size_type count);

        template <std::input_iterator InputIt>
            requires std::convertible_to<typename std::iterator_traits<InputIt>::value_type, list_value>
        constexpr value_iterator insert_values(const_value_iterator pos, InputIt first, InputIt last);

        constexpr value_iterator erase_values(const_value_iterator pos, size_type count);

        offset_type* p_list_offsets;
        const list_size_type* p_list_sizes;

        // friend classes
        friend class array_crtp_base<self_type>;
        friend class mutable_array_base<self_type>;
        friend class list_array_crtp_base<self_type>;
        template <class L>
        friend class list_reference;
    };

    class fixed_sized_list_array final : public list_array_crtp_base<fixed_sized_list_array>
    {
    public:

        using self_type = fixed_sized_list_array;
        using inner_types = array_inner_types<self_type>;
        using base_type = list_array_crtp_base<self_type>;
        using list_size_type = inner_types::list_size_type;
        using size_type = typename base_type::size_type;
        using value_iterator = typename base_type::value_iterator;
        using const_value_iterator = typename base_type::const_value_iterator;
        using offset_type = std::uint64_t;

        /**
         * @brief Constructs fixed size list array from Arrow proxy.
         *
         * @param proxy Arrow proxy containing fixed size list array data and schema
         *
         * @pre proxy must contain valid Arrow Fixed Size List array and schema
         * @pre proxy format must be "+w:[size]" where [size] is the list size
         * @pre proxy format size must be parseable as uint64_t
         * @post Array is initialized with data from proxy
         * @post List size is extracted from format string
         */
        explicit fixed_sized_list_array(arrow_proxy proxy);

        fixed_sized_list_array(const self_type&);
        fixed_sized_list_array& operator=(const self_type&);

        fixed_sized_list_array(self_type&&) = default;
        fixed_sized_list_array& operator=(self_type&&) = default;

        /**
         * @brief Generic constructor for creating fixed size list array.
         *
         * @tparam ARGS Parameter pack for constructor arguments
         * @param args Constructor arguments (list_size, flat_values, validity, etc.)
         *
         * @pre First argument must be the list size (uint64_t)
         * @pre Second argument must be a valid array for flat values
         * @pre flat_values.size() must be divisible by list_size
         * @post Array is created with the specified list size and data
         */
        template <class... ARGS>
            requires(mpl::excludes_copy_and_move_ctor_v<fixed_sized_list_array, ARGS...>)
        fixed_sized_list_array(ARGS&&... args)
            : self_type(create_proxy(std::forward<ARGS>(args)...))
        {
        }

    private:

        /**
         * @brief Creates Arrow proxy for fixed size list with validity bitmap.
         *
         * @tparam R Type of validity bitmap input
         * @tparam METADATA_RANGE Type of metadata container
         * @param list_size Size of each list (must be > 0)
         * @param flat_values Array containing all list elements
         * @param validity_input Validity bitmap specification
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the fixed size list array data and schema
         *
         * @pre list_size must be > 0
         * @pre flat_values.size() must be divisible by list_size
         * @pre validity_input size must equal flat_values.size() / list_size
         * @post Returns valid Arrow proxy with Fixed Size List format
         * @post Format string is "+w:<list_size>"
         */
        template <
            validity_bitmap_input R = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            std::uint64_t list_size,
            array&& flat_values,
            R&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates Arrow proxy for fixed size list with nullable flag.
         *
         * @tparam R Type of validity bitmap input
         * @tparam METADATA_RANGE Type of metadata container
         * @param list_size Size of each list (must be > 0)
         * @param flat_values Array containing all list elements
         * @param nullable Whether the array should support null values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the fixed size list array data and schema
         *
         * @pre list_size must be > 0
         * @pre flat_values.size() must be divisible by list_size
         * @post If nullable is true, array supports null values (though none initially set)
         * @post If nullable is false, array does not support null values
         * @post Returns valid Arrow proxy with Fixed Size List format
         */
        template <
            validity_bitmap_input R = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            std::uint64_t list_size,
            array&& flat_values,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Extracts list size from Arrow format string.
         *
         * @param format Arrow format string (expected: "+w:<size>")
         * @return List size as uint64_t
         *
         * @pre format must have at least 3 characters
         * @pre format must be in the form "+w:<number>"
         * @pre <number> part must be parseable as uint64_t
         * @post Returns the list size extracted from the format
         *
         */
        [[nodiscard]] static uint64_t list_size_from_format(const std::string_view format);

        /**
         * @brief Calculates offset range for list at given index.
         *
         * @param i Index of the list
         * @return Pair of (start_offset, end_offset) for the list
         *
         * @pre i must be < array.size()
         * @post Returns (i * list_size, (i + 1) * list_size)
         * @post end_offset - start_offset == list_size
         */
        [[nodiscard]] constexpr std::pair<offset_type, offset_type> offset_range(size_type i) const;

        [[nodiscard]] constexpr size_type flat_element_count(size_type list_count) const;

        void replace_value(size_type index, list_value value);

        value_iterator insert_value(const_value_iterator pos, list_value value, size_type count);

        value_iterator erase_values(const_value_iterator pos, size_type count);

        uint64_t m_list_size;

        // friend classes
        friend class array_crtp_base<self_type>;
        friend class mutable_array_base<self_type>;
        friend class list_array_crtp_base<self_type>;
        template <class L>
        friend class list_reference;
    };

    /***************************************
     * list_array_crtp_base implementation *
     ***************************************/

    template <class DERIVED>
    list_array_crtp_base<DERIVED>::list_array_crtp_base(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , p_flat_array(make_flat_array())
    {
    }

    template <class DERIVED>
    constexpr list_array_crtp_base<DERIVED>::list_array_crtp_base(const self_type& rhs)
        : base_type(rhs)
        , p_flat_array(make_flat_array())
    {
    }

    template <class DERIVED>
    constexpr auto list_array_crtp_base<DERIVED>::operator=(const self_type& rhs) -> self_type&
    {
        base_type::operator=(rhs);
        p_flat_array = make_flat_array();
        return *this;
    }

    template <class DERIVED>
    constexpr auto list_array_crtp_base<DERIVED>::raw_flat_array() const -> const array*
    {
        return &p_flat_array;
    }

    template <class DERIVED>
    constexpr auto list_array_crtp_base<DERIVED>::raw_flat_array() -> array*
    {
        return &p_flat_array;
    }

    template <class DERIVED>
    constexpr void list_array_crtp_base<DERIVED>::throw_if_sliced_for_mutation(const char* operation) const
    {
        if (this->get_arrow_proxy().offset() != 0)
        {
            throw std::logic_error(std::string(operation) + " does not support sliced arrays (non-zero offset)");
        }
    }

    template <class DERIVED>
    constexpr auto list_array_crtp_base<DERIVED>::value_begin() -> value_iterator
    {
        return value_iterator(detail::layout_value_functor<DERIVED, inner_reference>(&this->derived_cast()), 0);
    }

    template <class DERIVED>
    constexpr auto list_array_crtp_base<DERIVED>::value_end() -> value_iterator
    {
        return value_iterator(
            detail::layout_value_functor<DERIVED, inner_reference>(&this->derived_cast()),
            this->size()
        );
    }

    template <class DERIVED>
    constexpr auto list_array_crtp_base<DERIVED>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator(
            detail::layout_value_functor<const DERIVED, inner_const_reference>(&this->derived_cast()),
            0
        );
    }

    template <class DERIVED>
    constexpr auto list_array_crtp_base<DERIVED>::value_cend() const -> const_value_iterator
    {
        return const_value_iterator(
            detail::layout_value_functor<const DERIVED, inner_const_reference>(&this->derived_cast()),
            this->size()
        );
    }

    template <class DERIVED>
    constexpr auto list_array_crtp_base<DERIVED>::value(size_type i) -> inner_reference
    {
        return inner_reference(&this->derived_cast(), i);
    }

    template <class DERIVED>
    constexpr auto list_array_crtp_base<DERIVED>::value(size_type i) const -> inner_const_reference
    {
        const auto r = this->derived_cast().offset_range(i);
        using st = typename list_value::size_type;
        return list_value{&p_flat_array, static_cast<st>(r.first), static_cast<st>(r.second)};
    }

    template <class DERIVED>
    array list_array_crtp_base<DERIVED>::make_flat_array()
    {
        auto& child_proxy = this->get_arrow_proxy().children()[0];
        return array{&child_proxy.array(), &child_proxy.schema()};
    }

    template <class DERIVED>
    constexpr void
    list_array_crtp_base<DERIVED>::insert_flat_elements(size_type flat_pos, list_value value, size_type count)
    {
        if (value.size() == 0)
        {
            return;
        }
        SPARROW_ASSERT_TRUE(value.flat_array() != nullptr);
        array& flat_array = *raw_flat_array();
        flat_array.insert(
            flat_array.cbegin() + static_cast<std::ptrdiff_t>(flat_pos),
            value.flat_array()->cbegin() + static_cast<std::ptrdiff_t>(value.begin_index()),
            value.flat_array()->cbegin() + static_cast<std::ptrdiff_t>(value.end_index()),
            count
        );
    }

    template <class DERIVED>
    constexpr void
    list_array_crtp_base<DERIVED>::erase_flat_elements(size_type flat_begin, size_type flat_count)
    {
        if (flat_count == 0)
        {
            return;
        }
        array& flat_array = *raw_flat_array();
        flat_array.erase(
            flat_array.cbegin() + static_cast<std::ptrdiff_t>(flat_begin),
            flat_array.cbegin() + static_cast<std::ptrdiff_t>(flat_begin + flat_count)
        );
    }

    template <class DERIVED>
    constexpr void list_array_crtp_base<DERIVED>::resize_values(size_type new_length, list_value value)
    {
        DERIVED& derived = static_cast<DERIVED&>(*this);
        const size_type n = this->size();
        if (new_length < n)
        {
            derived.erase_values(
                sparrow::next(this->value_cbegin(), static_cast<std::ptrdiff_t>(new_length)),
                n - new_length
            );
        }
        else if (new_length > n)
        {
            derived.insert_value(this->value_cend(), value, new_length - n);
        }
    }

    template <class DERIVED>
    template <std::input_iterator InputIt>
        requires std::convertible_to<typename std::iterator_traits<InputIt>::value_type, list_value>
    constexpr auto
    list_array_crtp_base<DERIVED>::insert_values(const_value_iterator pos, InputIt first, InputIt last)
        -> value_iterator
    {
        DERIVED& derived = static_cast<DERIVED&>(*this);
        const auto idx = static_cast<size_type>(std::distance(this->value_cbegin(), pos));
        size_type count = 0;
        for (auto it = first; it != last; ++it, ++count)
        {
            auto cur_pos = sparrow::next(this->value_cbegin(), static_cast<std::ptrdiff_t>(idx + count));
            derived.insert_value(cur_pos, static_cast<list_value>(*it), 1);
        }
        return sparrow::next(this->value_begin(), static_cast<std::ptrdiff_t>(idx));
    }

    /**********************************
     * list_array_impl implementation *
     **********************************/

#ifdef __GNUC__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-align"
#endif

    template <bool BIG>
    list_array_impl<BIG>::list_array_impl(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , p_list_offsets(make_list_offsets())
    {
    }

    template <bool BIG>
    template <std::ranges::range SIZES_RANGE>
    constexpr auto list_array_impl<BIG>::offset_from_sizes(SIZES_RANGE&& sizes) -> offset_buffer_type
    {
        return detail::offset_buffer_from_sizes<std::remove_const_t<offset_type>>(
            std::forward<SIZES_RANGE>(sizes)
        );
    }

    template <bool BIG>
    template <validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
    arrow_proxy list_array_impl<BIG>::create_proxy(
        array&& flat_values,
        offset_buffer_type&& list_offsets,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto size = list_offsets.size() - 1;
        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<VB>(validity_input));
        const auto null_count = vbitmap.null_count();

        auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(flat_values));

        ArrowSchema schema = detail::make_list_arrow_schema(
            BIG ? std::string("+L") : std::string("+l"),
            std::move(flat_schema),
            name,
            metadata,
            true  // nullable
        );

        std::vector<buffer<std::uint8_t>> arr_buffs;
        arr_buffs.reserve(2);
        arr_buffs.emplace_back(std::move(vbitmap).extract_storage());
        arr_buffs.emplace_back(std::move(list_offsets).extract_storage());

        ArrowArray arr = detail::make_list_arrow_array(
            static_cast<std::int64_t>(size),
            static_cast<std::int64_t>(null_count),
            std::move(arr_buffs),
            std::move(flat_arr)
        );

        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <bool BIG>
    template <validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
    arrow_proxy list_array_impl<BIG>::create_proxy(
        array&& flat_values,
        offset_buffer_type&& list_offsets,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        if (nullable)
        {
            return list_array_impl<BIG>::create_proxy(
                std::move(flat_values),
                std::move(list_offsets),
                validity_bitmap{validity_bitmap::default_allocator()},
                name,
                metadata
            );
        }

        const auto size = list_offsets.size() - 1;
        auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(flat_values));

        ArrowSchema schema = detail::make_list_arrow_schema(
            BIG ? std::string("+L") : std::string("+l"),
            std::move(flat_schema),
            name,
            metadata,
            false  // not nullable
        );

        std::vector<buffer<std::uint8_t>> arr_buffs;
        arr_buffs.reserve(2);
        arr_buffs.emplace_back(nullptr, 0, buffer<std::uint8_t>::default_allocator());  // no validity bitmap
        arr_buffs.emplace_back(std::move(list_offsets).extract_storage());

        ArrowArray arr = detail::make_list_arrow_array(
            static_cast<std::int64_t>(size),
            0,  // null_count
            std::move(arr_buffs),
            std::move(flat_arr)
        );

        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <bool BIG>
    constexpr list_array_impl<BIG>::list_array_impl(const self_type& rhs)
        : base_type(rhs)
        , p_list_offsets(make_list_offsets())
    {
        copy_tracker::increase(copy_tracker::key<list_array_impl<BIG>>());
    }

    template <bool BIG>
    constexpr auto list_array_impl<BIG>::operator=(const self_type& rhs) -> self_type&
    {
        copy_tracker::increase(copy_tracker::key<self_type>());
        if (this != &rhs)
        {
            base_type::operator=(rhs);
            p_list_offsets = make_list_offsets();
        }
        return *this;
    }

    template <bool BIG>
    constexpr void list_array_impl<BIG>::slice_inplace(size_type start, size_type end)
    {
        base_type::slice_inplace(start, end);
        p_list_offsets = make_list_offsets();
    }

    template <bool BIG>
    constexpr auto list_array_impl<BIG>::offset_range(size_type i) const -> std::pair<offset_type, offset_type>
    {
        return std::make_pair(p_list_offsets[i], p_list_offsets[i + 1]);
    }

    template <bool BIG>
    constexpr auto list_array_impl<BIG>::make_list_offsets() -> offset_type*
    {
        return this->get_arrow_proxy().buffers()[OFFSET_BUFFER_INDEX].template data<offset_type>()
               + static_cast<size_type>(this->get_arrow_proxy().offset());
    }

    template <bool BIG>
    constexpr auto
    list_array_impl<BIG>::insert_value(const_value_iterator pos, list_value value, size_type count)
        -> value_iterator
    {
        using mutable_offset_type = std::remove_const_t<offset_type>;
        const auto idx = static_cast<size_type>(std::distance(this->value_cbegin(), pos));
        auto& proxy = this->get_arrow_proxy();

        this->throw_if_sliced_for_mutation("list_array_impl::insert_value");

        auto& offset_buffer = proxy.get_array_private_data()->buffers()[OFFSET_BUFFER_INDEX];
        auto offset_adaptor = make_buffer_adaptor<mutable_offset_type>(offset_buffer);
        const size_type n = offset_adaptor.size() - 1;

        // Insert flat elements: count copies of value's slice
        const auto flat_insert_pos = static_cast<size_type>(p_list_offsets[idx]);
        this->insert_flat_elements(flat_insert_pos, value, count);

        // Update offset buffer
        const auto value_size = value.size();
        const auto count_size = count;
        SPARROW_ASSERT_TRUE(std::in_range<mutable_offset_type>(value_size));
        SPARROW_ASSERT_TRUE(std::in_range<mutable_offset_type>(count_size));
        const auto val_sz = static_cast<mutable_offset_type>(value_size);
        const auto count_mt = static_cast<mutable_offset_type>(count_size);
        const auto max_offset = std::numeric_limits<mutable_offset_type>::max();
        // Check multiplication overflow for total_delta = count * val_sz
        if (val_sz != mutable_offset_type{})
        {
            SPARROW_ASSERT_TRUE(count_mt <= max_offset / val_sz);
        }
        const mutable_offset_type total_delta = count_mt * val_sz;
        // Existing offsets should fit in the representable range once delta is applied
        const mutable_offset_type existing_max = offset_adaptor[n];
        SPARROW_ASSERT_TRUE(existing_max <= max_offset - total_delta);

        offset_adaptor.resize(n + 1 + count, mutable_offset_type{});

        // Shift existing entries [idx+1..n] to [idx+count+1..n+count], adding delta
        for (size_type i = n; i > idx; --i)
        {
            offset_adaptor[i + count] = offset_adaptor[i] + total_delta;
        }

        const auto flat_insert_offset = static_cast<mutable_offset_type>(flat_insert_pos);
        SPARROW_ASSERT_TRUE(flat_insert_offset <= max_offset - total_delta);
        for (size_type k = 1; k <= count; ++k)
        {
            offset_adaptor[idx + k] = flat_insert_offset + static_cast<mutable_offset_type>(k) * val_sz;
        }

        proxy.update_buffers();
        p_list_offsets = make_list_offsets();
        return sparrow::next(this->value_begin(), static_cast<std::ptrdiff_t>(idx));
    }

    template <bool BIG>
    constexpr auto list_array_impl<BIG>::erase_values(const_value_iterator pos, size_type count)
        -> value_iterator
    {
        using mutable_offset_type = std::remove_const_t<offset_type>;
        const size_type idx = static_cast<size_type>(std::distance(this->value_cbegin(), pos));
        const size_type n = this->size();
        auto& proxy = this->get_arrow_proxy();

        if (count == 0)
        {
            return sparrow::next(this->value_begin(), static_cast<std::ptrdiff_t>(idx));
        }

        this->throw_if_sliced_for_mutation("list_array_impl::erase_values");

        // Erase flat elements
        const mutable_offset_type flat_erase_begin = p_list_offsets[idx];
        const mutable_offset_type flat_erase_end = p_list_offsets[idx + count];
        const mutable_offset_type flat_erase_count_val = flat_erase_end - flat_erase_begin;

        this->erase_flat_elements(
            static_cast<size_type>(flat_erase_begin),
            static_cast<size_type>(flat_erase_count_val)
        );

        // Update offset buffer: remove count entries, shift remaining down
        auto& offset_buffer = proxy.get_array_private_data()->buffers()[OFFSET_BUFFER_INDEX];
        auto offset_adaptor = make_buffer_adaptor<mutable_offset_type>(offset_buffer);
        std::copy(
            offset_adaptor.begin() + static_cast<std::ptrdiff_t>(idx + count + 1),
            offset_adaptor.begin() + static_cast<std::ptrdiff_t>(n + 1),
            offset_adaptor.begin() + static_cast<std::ptrdiff_t>(idx + 1)
        );
        auto delta_begin = offset_adaptor.begin() + static_cast<std::ptrdiff_t>(idx + 1);
        auto delta_end = offset_adaptor.begin() + static_cast<std::ptrdiff_t>(n + 1 - count);
        std::transform(delta_begin, delta_end, delta_begin, [flat_erase_count_val](mutable_offset_type v)
                       { return v - flat_erase_count_val; });
        offset_adaptor.resize(n + 1 - count);

        proxy.update_buffers();
        p_list_offsets = make_list_offsets();
        return sparrow::next(this->value_begin(), static_cast<std::ptrdiff_t>(idx));
    }

    template <bool BIG>
    void list_array_impl<BIG>::replace_value(size_type index, list_value value)
    {
        using mutable_offset_type = std::remove_const_t<offset_type>;

        this->throw_if_sliced_for_mutation("list_array_impl::replace_value");
        const size_type new_size = value.size();
        SPARROW_ASSERT_TRUE(std::in_range<mutable_offset_type>(new_size));

        const size_type n = this->size();
        const auto [flat_begin, flat_end] = offset_range(index);
        const auto old_size = static_cast<size_type>(flat_end - flat_begin);
        const auto old_size_mt = static_cast<mutable_offset_type>(old_size);
        const auto new_size_mt = static_cast<mutable_offset_type>(new_size);

        array source_owner;
        const array* source = value.flat_array();
        if (source != nullptr && this->raw_flat_array() == source)
        {
            source_owner = *source;
            source = &source_owner;
        }

        if (old_size == new_size)
        {
            if (new_size == 0)
            {
                return;
            }
            // Same element count: overwrite flat data in-place, no offset adjustment needed.
            SPARROW_ASSERT_TRUE(source != nullptr);
            const auto flat_begin_index = static_cast<size_type>(flat_begin);
            this->erase_flat_elements(flat_begin_index, old_size);
            array& flat_array = *this->raw_flat_array();
            flat_array.insert(
                flat_array.cbegin() + static_cast<std::ptrdiff_t>(flat_begin_index),
                source->cbegin() + static_cast<std::ptrdiff_t>(value.begin_index()),
                source->cbegin() + static_cast<std::ptrdiff_t>(value.end_index()),
                1
            );
            auto& proxy_eq = this->get_arrow_proxy();
            proxy_eq.update_buffers();
            p_list_offsets = make_list_offsets();
            return;
        }

        this->erase_flat_elements(static_cast<size_type>(flat_begin), old_size);
        if (new_size > 0)
        {
            SPARROW_ASSERT_TRUE(source != nullptr);
            array& flat_array = *this->raw_flat_array();
            flat_array.insert(
                flat_array.cbegin() + static_cast<std::ptrdiff_t>(flat_begin),
                source->cbegin() + static_cast<std::ptrdiff_t>(value.begin_index()),
                source->cbegin() + static_cast<std::ptrdiff_t>(value.end_index()),
                1
            );
        }

        auto& proxy = this->get_arrow_proxy();
        auto& offset_buffer = proxy.get_array_private_data()->buffers()[OFFSET_BUFFER_INDEX];
        auto offset_adaptor = make_buffer_adaptor<mutable_offset_type>(offset_buffer);

        // delta != 0 here (equal-size case already returned above)
        auto tail_begin = offset_adaptor.begin() + static_cast<std::ptrdiff_t>(index + 1);
        auto tail_end = offset_adaptor.begin() + static_cast<std::ptrdiff_t>(n + 1);
        if (new_size_mt > old_size_mt)
        {
            const mutable_offset_type delta = new_size_mt - old_size_mt;
            const auto max_offset = std::numeric_limits<mutable_offset_type>::max();
            SPARROW_ASSERT_TRUE(offset_adaptor[n] <= max_offset - delta);
            std::transform(tail_begin, tail_end, tail_begin, [delta](mutable_offset_type v)
                           { return v + delta; });
        }
        else
        {
            const mutable_offset_type delta = old_size_mt - new_size_mt;
            std::transform(tail_begin, tail_end, tail_begin, [delta](mutable_offset_type v)
                           { return v - delta; });
        }

        proxy.update_buffers();
        p_list_offsets = make_list_offsets();
    }

    /***************************************
     * list_view_array_impl implementation *
     ***************************************/

    template <bool BIG>
    inline list_view_array_impl<BIG>::list_view_array_impl(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , p_list_offsets(make_list_offsets())
        , p_list_sizes(make_list_sizes())
    {
    }

    template <bool BIG>
    template <validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
    arrow_proxy list_view_array_impl<BIG>::create_proxy(
        array&& flat_values,
        offset_buffer_type&& list_offsets,
        size_buffer_type&& list_sizes,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        SPARROW_ASSERT(list_offsets.size() == list_sizes.size(), "sizes and offset must have the same size");
        const auto size = list_sizes.size();
        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<VB>(validity_input));
        const auto null_count = vbitmap.null_count();

        auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(flat_values));

        ArrowSchema schema = detail::make_list_arrow_schema(
            BIG ? std::string("+vL") : std::string("+vl"),
            std::move(flat_schema),
            name,
            metadata,
            true  // nullable
        );

        std::vector<buffer<std::uint8_t>> arr_buffs;
        arr_buffs.reserve(3);
        arr_buffs.emplace_back(std::move(vbitmap).extract_storage());
        arr_buffs.emplace_back(std::move(list_offsets).extract_storage());
        arr_buffs.emplace_back(std::move(list_sizes).extract_storage());

        ArrowArray arr = detail::make_list_arrow_array(
            static_cast<std::int64_t>(size),
            static_cast<std::int64_t>(null_count),
            std::move(arr_buffs),
            std::move(flat_arr)
        );

        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <bool BIG>
    template <input_metadata_container METADATA_RANGE>
    arrow_proxy list_view_array_impl<BIG>::create_proxy(
        array&& flat_values,
        offset_buffer_type&& list_offsets,
        size_buffer_type&& list_sizes,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        if (nullable)
        {
            return list_view_array_impl<BIG>::create_proxy(
                std::move(flat_values),
                std::move(list_offsets),
                std::move(list_sizes),
                validity_bitmap{validity_bitmap::default_allocator()},
                name,
                metadata
            );
        }

        SPARROW_ASSERT(list_offsets.size() == list_sizes.size(), "sizes and offset must have the same size");
        const auto size = list_sizes.size();
        auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(flat_values));

        ArrowSchema schema = detail::make_list_arrow_schema(
            BIG ? std::string("+vL") : std::string("+vl"),
            std::move(flat_schema),
            name,
            metadata,
            false  // not nullable
        );

        std::vector<buffer<std::uint8_t>> arr_buffs;
        arr_buffs.reserve(3);
        arr_buffs.emplace_back(nullptr, 0, buffer<std::uint8_t>::default_allocator());  // no validity bitmap
        arr_buffs.emplace_back(std::move(list_offsets).extract_storage());
        arr_buffs.emplace_back(std::move(list_sizes).extract_storage());

        ArrowArray arr = detail::make_list_arrow_array(
            static_cast<std::int64_t>(size),
            0,  // null_count
            std::move(arr_buffs),
            std::move(flat_arr)
        );

        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <bool BIG>
    constexpr list_view_array_impl<BIG>::list_view_array_impl(const self_type& rhs)
        : base_type(rhs)
        , p_list_offsets(make_list_offsets())
        , p_list_sizes(make_list_sizes())
    {
        copy_tracker::increase(copy_tracker::key<list_view_array_impl<BIG>>());
    }

    template <bool BIG>
    constexpr auto list_view_array_impl<BIG>::operator=(const self_type& rhs) -> self_type&
    {
        copy_tracker::increase(copy_tracker::key<self_type>());
        if (this != &rhs)
        {
            base_type::operator=(rhs);
            p_list_offsets = make_list_offsets();
            p_list_sizes = make_list_sizes();
        }
        return *this;
    }

    template <bool BIG>
    constexpr void list_view_array_impl<BIG>::slice_inplace(size_type start, size_type end)
    {
        base_type::slice_inplace(start, end);
        p_list_offsets = make_list_offsets();
        p_list_sizes = make_list_sizes();
    }

    template <bool BIG>
    inline constexpr auto list_view_array_impl<BIG>::offset_range(size_type i) const
        -> std::pair<offset_type, offset_type>
    {
        const auto offset = p_list_offsets[i];
        SPARROW_ASSERT_TRUE(std::in_range<std::remove_const_t<offset_type>>(p_list_sizes[i]));
        return std::make_pair(offset, offset + static_cast<offset_type>(p_list_sizes[i]));
    }

    template <bool BIG>
    constexpr auto list_view_array_impl<BIG>::make_list_offsets() const -> offset_type*
    {
        return this->get_arrow_proxy().buffers()[OFFSET_BUFFER_INDEX].template data<offset_type>()
               + static_cast<size_type>(this->get_arrow_proxy().offset());
    }

    template <bool BIG>
    constexpr auto list_view_array_impl<BIG>::make_list_sizes() const -> const list_size_type*
    {
        return this->get_arrow_proxy().buffers()[SIZES_BUFFER_INDEX].template data<list_size_type>()
               + static_cast<size_type>(this->get_arrow_proxy().offset());
    }

    template <bool BIG>
    constexpr auto
    list_view_array_impl<BIG>::insert_value(const_value_iterator pos, list_value value, size_type count)
        -> value_iterator
    {
        using mutable_offset_type = std::remove_const_t<offset_type>;
        using mutable_size_type = std::remove_const_t<list_size_type>;
        const size_type idx = static_cast<size_type>(std::distance(this->value_cbegin(), pos));

        this->throw_if_sliced_for_mutation("list_view_array_impl::insert_value");

        // Append new element(s) at the end of the flat array
        SPARROW_ASSERT_TRUE(std::in_range<mutable_offset_type>(value.size()));
        SPARROW_ASSERT_TRUE(std::in_range<mutable_size_type>(value.size()));
        const mutable_offset_type val_sz = static_cast<mutable_offset_type>(value.size());
        const size_type flat_append_pos = this->raw_flat_array()->size();

        this->insert_flat_elements(flat_append_pos, value, count);

        auto& proxy = this->get_arrow_proxy();

        // Insert count new (offset, size) entries in the offset and sizes buffers at position idx
        const size_type n = this->size();

        auto& offset_buffer = proxy.get_array_private_data()->buffers()[OFFSET_BUFFER_INDEX];
        auto offset_adaptor = make_buffer_adaptor<mutable_offset_type>(offset_buffer);
        offset_adaptor.resize(n + count);

        auto& sizes_buffer = proxy.get_array_private_data()->buffers()[SIZES_BUFFER_INDEX];
        auto sizes_adaptor = make_buffer_adaptor<mutable_size_type>(sizes_buffer);
        sizes_adaptor.resize(n + count);

        // Shift existing entries [idx..n) → [idx+count..n+count): pure backward copy, no delta.
        // std::copy_backward lowers to memmove for trivially-copyable offset/size types.
        std::copy_backward(
            offset_adaptor.begin() + static_cast<std::ptrdiff_t>(idx),
            offset_adaptor.begin() + static_cast<std::ptrdiff_t>(n),
            offset_adaptor.begin() + static_cast<std::ptrdiff_t>(n + count)
        );
        std::copy_backward(
            sizes_adaptor.begin() + static_cast<std::ptrdiff_t>(idx),
            sizes_adaptor.begin() + static_cast<std::ptrdiff_t>(n),
            sizes_adaptor.begin() + static_cast<std::ptrdiff_t>(n + count)
        );

        // Write new entries for the count inserted lists
        // Before writing, assert that the computed offsets are representable
        if (count > 0 && val_sz > 0)
        {
            const auto base_offset_ull = static_cast<unsigned long long>(flat_append_pos);
            const auto step_ull = static_cast<unsigned long long>(val_sz);
            const auto max_k_ull = static_cast<unsigned long long>(count - 1);
            const auto max_offset_ull = static_cast<unsigned long long>(
                std::numeric_limits<mutable_offset_type>::max()
            );
            const auto last_offset_ull = base_offset_ull + max_k_ull * step_ull;
            SPARROW_ASSERT_TRUE(base_offset_ull <= max_offset_ull);
            SPARROW_ASSERT_TRUE(last_offset_ull <= max_offset_ull);
        }
        for (size_type k = 0; k < count; ++k)
        {
            offset_adaptor[idx + k] = static_cast<mutable_offset_type>(flat_append_pos)
                                      + static_cast<mutable_offset_type>(k) * val_sz;
            sizes_adaptor[idx + k] = static_cast<mutable_size_type>(val_sz);
        }

        proxy.update_buffers();
        p_list_offsets = make_list_offsets();
        p_list_sizes = make_list_sizes();
        return sparrow::next(this->value_begin(), static_cast<std::ptrdiff_t>(idx));
    }

    template <bool BIG>
    template <std::input_iterator InputIt>
        requires std::convertible_to<typename std::iterator_traits<InputIt>::value_type, list_value>
    constexpr auto
    list_view_array_impl<BIG>::insert_values(const_value_iterator pos, InputIt first, InputIt last)
        -> value_iterator
    {
        const size_type idx = static_cast<size_type>(std::distance(this->value_cbegin(), pos));
        auto& proxy = this->get_arrow_proxy();
        const size_type original_size = this->size();
        if constexpr (std::forward_iterator<InputIt>)
        {
            const auto count = static_cast<size_type>(std::distance(first, last));
            if (count == 0)
            {
                return sparrow::next(this->value_begin(), static_cast<std::ptrdiff_t>(idx));
            }
            size_type inserted_count = 0;
            // Temporarily extend logical length once for the whole bulk insertion.
            proxy.set_length(original_size + count);
            for (auto it = first; it != last; ++it)
            {
                auto cur_pos = sparrow::next(
                    this->value_cbegin(),
                    static_cast<std::ptrdiff_t>(idx + inserted_count)
                );
                insert_value(cur_pos, static_cast<list_value>(*it), 1);
                ++inserted_count;
            }
            proxy.set_length(original_size);
        }
        else
        {
            size_type inserted_count = 0;
            for (auto it = first; it != last; ++it)
            {
                auto cur_pos = sparrow::next(
                    this->value_cbegin(),
                    static_cast<std::ptrdiff_t>(idx + inserted_count)
                );
                insert_value(cur_pos, static_cast<list_value>(*it), 1);
                ++inserted_count;
                proxy.set_length(original_size + inserted_count);
            }
            proxy.set_length(original_size);
        }
        return sparrow::next(this->value_begin(), static_cast<std::ptrdiff_t>(idx));
    }

    template <bool BIG>
    constexpr auto list_view_array_impl<BIG>::erase_values(const_value_iterator pos, size_type count)
        -> value_iterator
    {
        using mutable_offset_type = std::remove_const_t<offset_type>;
        using mutable_size_type = std::remove_const_t<list_size_type>;
        const size_type idx = static_cast<size_type>(std::distance(this->value_cbegin(), pos));
        const size_type n = this->size();

        if (count == 0)
        {
            return sparrow::next(this->value_begin(), static_cast<std::ptrdiff_t>(idx));
        }

        this->throw_if_sliced_for_mutation("list_view_array_impl::erase_values");

        auto& proxy = this->get_arrow_proxy();
        auto& offset_buffer = proxy.get_array_private_data()->buffers()[OFFSET_BUFFER_INDEX];
        auto offset_adaptor = make_buffer_adaptor<mutable_offset_type>(offset_buffer);

        auto& sizes_buffer = proxy.get_array_private_data()->buffers()[SIZES_BUFFER_INDEX];
        auto sizes_adaptor = make_buffer_adaptor<mutable_size_type>(sizes_buffer);

        // Shift entries [idx+count..n) to [idx..n-count)
        std::copy(
            offset_adaptor.begin() + static_cast<std::ptrdiff_t>(idx + count),
            offset_adaptor.end(),
            offset_adaptor.begin() + static_cast<std::ptrdiff_t>(idx)
        );
        std::copy(
            sizes_adaptor.begin() + static_cast<std::ptrdiff_t>(idx + count),
            sizes_adaptor.end(),
            sizes_adaptor.begin() + static_cast<std::ptrdiff_t>(idx)
        );
        offset_adaptor.resize(n - count);
        sizes_adaptor.resize(n - count);
        proxy.update_buffers();
        p_list_offsets = make_list_offsets();
        p_list_sizes = make_list_sizes();
        return sparrow::next(this->value_begin(), static_cast<std::ptrdiff_t>(idx));
    }

    template <bool BIG>
    void list_view_array_impl<BIG>::replace_value(size_type index, list_value value)
    {
        using mutable_offset_type = std::remove_const_t<offset_type>;
        using mutable_size_type = std::remove_const_t<list_size_type>;

        this->throw_if_sliced_for_mutation("list_view_array_impl::replace_value");

        SPARROW_ASSERT_TRUE(std::in_range<mutable_offset_type>(value.size()));
        SPARROW_ASSERT_TRUE(std::in_range<mutable_size_type>(value.size()));

        const auto value_size = static_cast<mutable_offset_type>(value.size());
        const size_type flat_append_pos = this->raw_flat_array()->size();
        SPARROW_ASSERT_TRUE(std::in_range<mutable_offset_type>(flat_append_pos));

        this->insert_flat_elements(flat_append_pos, value, 1);

        auto& proxy = this->get_arrow_proxy();
        auto& offset_buffer = proxy.get_array_private_data()->buffers()[OFFSET_BUFFER_INDEX];
        auto offset_adaptor = make_buffer_adaptor<mutable_offset_type>(offset_buffer);
        auto& sizes_buffer = proxy.get_array_private_data()->buffers()[SIZES_BUFFER_INDEX];
        auto sizes_adaptor = make_buffer_adaptor<mutable_size_type>(sizes_buffer);
        offset_adaptor[index] = static_cast<mutable_offset_type>(flat_append_pos);
        sizes_adaptor[index] = static_cast<mutable_size_type>(value_size);
        proxy.update_buffers();
        p_list_offsets = make_list_offsets();
        p_list_sizes = make_list_sizes();
    }

#ifdef __GNUC__
#    pragma GCC diagnostic pop
#endif

    /*****************************************
     * fixed_sized_list_array implementation *
     *****************************************/

    inline auto fixed_sized_list_array::list_size_from_format(const std::string_view format) -> uint64_t
    {
        SPARROW_ASSERT(format.size() >= 3, "Invalid format string");
        const auto n_digits = format.size() - 3;
        const auto list_size_str = format.substr(3, n_digits);
        return std::stoull(std::string(list_size_str));
    }

    inline fixed_sized_list_array::fixed_sized_list_array(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , m_list_size(fixed_sized_list_array::list_size_from_format(this->get_arrow_proxy().format()))
    {
    }

    inline fixed_sized_list_array::fixed_sized_list_array(const self_type& rhs)
        : base_type(rhs)
        , m_list_size(rhs.m_list_size)
    {
        copy_tracker::increase(copy_tracker::key<self_type>());
    }

    inline fixed_sized_list_array& fixed_sized_list_array::operator=(const self_type& rhs)
    {
        copy_tracker::increase(copy_tracker::key<self_type>());
        if (this != &rhs)
        {
            base_type::operator=(rhs);
            m_list_size = rhs.m_list_size;
        }
        return *this;
    }

    constexpr auto fixed_sized_list_array::offset_range(size_type i) const
        -> std::pair<offset_type, offset_type>
    {
        const auto offset = (i + this->offset()) * m_list_size;
        return std::make_pair(offset, offset + m_list_size);
    }

    constexpr auto fixed_sized_list_array::flat_element_count(size_type list_count) const -> size_type
    {
        const offset_type max_flat_count = static_cast<offset_type>(std::numeric_limits<size_type>::max());
        SPARROW_ASSERT_TRUE(
            m_list_size == 0 || static_cast<offset_type>(list_count) <= max_flat_count / m_list_size
        );
        return static_cast<size_type>(static_cast<offset_type>(list_count) * m_list_size);
    }

    inline auto
    fixed_sized_list_array::insert_value(const_value_iterator pos, list_value value, size_type count)
        -> value_iterator
    {
        SPARROW_ASSERT_TRUE(value.size() == m_list_size);
        const auto idx = static_cast<size_type>(std::distance(this->value_cbegin(), pos));

        this->throw_if_sliced_for_mutation("fixed_sized_list_array::insert_value");

        const size_type flat_insert_pos = flat_element_count(idx);

        this->insert_flat_elements(flat_insert_pos, value, count);

        return sparrow::next(this->value_begin(), static_cast<std::ptrdiff_t>(idx));
    }

    inline auto fixed_sized_list_array::erase_values(const_value_iterator pos, size_type count) -> value_iterator
    {
        const auto idx = static_cast<size_type>(std::distance(this->value_cbegin(), pos));
        if (count == 0)
        {
            return sparrow::next(this->value_begin(), static_cast<std::ptrdiff_t>(idx));
        }

        this->throw_if_sliced_for_mutation("fixed_sized_list_array::erase_values");

        this->erase_flat_elements(flat_element_count(idx), flat_element_count(count));
        return sparrow::next(this->value_begin(), static_cast<std::ptrdiff_t>(idx));
    }

    inline void fixed_sized_list_array::replace_value(size_type index, list_value value)
    {
        SPARROW_ASSERT_TRUE(value.size() == m_list_size);

        this->throw_if_sliced_for_mutation("fixed_sized_list_array::replace_value");

        if (m_list_size == 0)
        {
            return;
        }

        array source_owner;
        const array* source = value.flat_array();
        if (source != nullptr && this->raw_flat_array() == source)
        {
            source_owner = *source;
            source = &source_owner;
        }

        SPARROW_ASSERT_TRUE(source != nullptr);
        const size_type flat_index = flat_element_count(index);
        this->erase_flat_elements(flat_index, static_cast<size_type>(m_list_size));
        array& flat_array = *this->raw_flat_array();
        flat_array.insert(
            flat_array.cbegin() + static_cast<std::ptrdiff_t>(flat_index),
            source->cbegin() + static_cast<std::ptrdiff_t>(value.begin_index()),
            source->cbegin() + static_cast<std::ptrdiff_t>(value.end_index()),
            1
        );
    }

    template <validity_bitmap_input R, input_metadata_container METADATA_RANGE>
    inline arrow_proxy fixed_sized_list_array::create_proxy(
        std::uint64_t list_size,
        array&& flat_values,
        R&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto size = flat_values.size() / static_cast<std::size_t>(list_size);
        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<R>(validity_input));
        const auto null_count = vbitmap.null_count();

        auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(flat_values));

        std::string format = "+w:" + std::to_string(list_size);
        ArrowSchema schema = detail::make_list_arrow_schema(
            std::move(format),
            std::move(flat_schema),
            name,
            metadata,
            true  // nullable
        );

        std::vector<buffer<std::uint8_t>> arr_buffs;
        arr_buffs.reserve(1);
        arr_buffs.emplace_back(vbitmap.extract_storage());

        ArrowArray arr = detail::make_list_arrow_array(
            static_cast<std::int64_t>(size),
            static_cast<std::int64_t>(null_count),
            std::move(arr_buffs),
            std::move(flat_arr)
        );

        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <validity_bitmap_input R, input_metadata_container METADATA_RANGE>
    inline arrow_proxy fixed_sized_list_array::create_proxy(
        std::uint64_t list_size,
        array&& flat_values,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        if (nullable)
        {
            return fixed_sized_list_array::create_proxy(
                list_size,
                std::move(flat_values),
                validity_bitmap{validity_bitmap::default_allocator()},
                name,
                metadata
            );
        }

        const auto size = flat_values.size() / static_cast<std::size_t>(list_size);
        auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(flat_values));

        std::string format = "+w:" + std::to_string(list_size);
        ArrowSchema schema = detail::make_list_arrow_schema(
            std::move(format),
            std::move(flat_schema),
            name,
            metadata,
            false  // not nullable
        );

        std::vector<buffer<std::uint8_t>> arr_buffs;
        arr_buffs.reserve(1);
        arr_buffs.emplace_back(nullptr, 0, buffer<std::uint8_t>::default_allocator());  // no validity bitmap

        ArrowArray arr = detail::make_list_arrow_array(
            static_cast<std::int64_t>(size),
            0,  // null_count
            std::move(arr_buffs),
            std::move(flat_arr)
        );

        return arrow_proxy{std::move(arr), std::move(schema)};
    }
}
