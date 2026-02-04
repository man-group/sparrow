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

#include <ranges>
#include <string>  // for std::stoull
#include <type_traits>
#include <vector>

#include "sparrow/array_api.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/debug/copy_tracker.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/array_factory.hpp"
#include "sparrow/layout/array_wrapper.hpp"
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
        using inner_reference = list_value;
        using inner_const_reference = list_value;
        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_value_type>>;
        using const_value_iterator = functor_index_iterator<
            detail::layout_value_functor<const array_type, inner_value_type>>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    template <bool BIG>
    struct array_inner_types<list_view_array_impl<BIG>> : array_inner_types_base
    {
        using list_size_type = std::conditional_t<BIG, std::uint64_t, std::uint32_t>;
        using array_type = list_view_array_impl<BIG>;
        using inner_value_type = list_value;
        using inner_reference = list_value;
        using inner_const_reference = list_value;
        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_value_type>>;
        using const_value_iterator = functor_index_iterator<
            detail::layout_value_functor<const array_type, inner_value_type>>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    template <>
    struct array_inner_types<fixed_sized_list_array> : array_inner_types_base
    {
        using list_size_type = std::uint64_t;
        using array_type = fixed_sized_list_array;
        using inner_value_type = list_value;
        using inner_reference = list_value;
        using inner_const_reference = list_value;
        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_value_type>>;
        using const_value_iterator = functor_index_iterator<
            detail::layout_value_functor<const array_type, inner_value_type>>;
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
     * @post Maintains Arrow array format compatibility for list types
     * @post Provides unified interface for all list array variants
     */
    template <class DERIVED>
    class list_array_crtp_base : public array_bitmap_base<DERIVED>
    {
    public:

        using self_type = list_array_crtp_base<DERIVED>;
        using base_type = array_bitmap_base<DERIVED>;
        using inner_types = array_inner_types<DERIVED>;
        using value_iterator = typename inner_types::value_iterator;
        using const_value_iterator = typename inner_types::const_value_iterator;
        using size_type = typename base_type::size_type;

        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;

        using const_bitmap_range = typename base_type::const_bitmap_range;

        using inner_value_type = list_value;
        using inner_reference = list_value;
        using inner_const_reference = list_value;

        using value_type = nullable<inner_value_type>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using iterator_tag = typename base_type::iterator_tag;

        /**
         * @brief Gets read-only access to the underlying flat array.
         *
         * @return Const pointer to the flat array containing all list elements
         *
         * @post Returns non-null pointer to valid array_wrapper
         */
        [[nodiscard]] constexpr const array_wrapper* raw_flat_array() const;

        /**
         * @brief Gets mutable access to the underlying flat array.
         *
         * @return Pointer to the flat array containing all list elements
         *
         * @post Returns non-null pointer to valid array_wrapper
         */
        [[nodiscard]] constexpr array_wrapper* raw_flat_array();

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

    private:

        using list_size_type = inner_types::list_size_type;

        [[nodiscard]] constexpr value_iterator value_begin();
        [[nodiscard]] constexpr value_iterator value_end();
        [[nodiscard]] constexpr const_value_iterator value_cbegin() const;
        [[nodiscard]] constexpr const_value_iterator value_cend() const;

        [[nodiscard]] constexpr inner_reference value(size_type i);
        [[nodiscard]] constexpr inner_const_reference value(size_type i) const;

        [[nodiscard]] cloning_ptr<array_wrapper> make_flat_array();

        // data members
        cloning_ptr<array_wrapper> p_flat_array;

        // friend classes
        friend class array_crtp_base<DERIVED>;

        // needs access to this->value(i)
        friend class detail::layout_value_functor<DERIVED, inner_value_type>;
        friend class detail::layout_value_functor<const DERIVED, inner_value_type>;
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

        offset_type* p_list_offsets;

        // friend classes
        friend class array_crtp_base<self_type>;
        friend class list_array_crtp_base<self_type>;
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

        [[nodiscard]] constexpr offset_type* make_list_offsets();
        [[nodiscard]] constexpr offset_type* make_list_sizes();

        offset_type* p_list_offsets;
        offset_type* p_list_sizes;

        // friend classes
        friend class array_crtp_base<self_type>;
        friend class list_array_crtp_base<self_type>;
    };

    class fixed_sized_list_array final : public list_array_crtp_base<fixed_sized_list_array>
    {
    public:

        using self_type = fixed_sized_list_array;
        using inner_types = array_inner_types<self_type>;
        using base_type = list_array_crtp_base<self_type>;
        using list_size_type = inner_types::list_size_type;
        using size_type = typename base_type::size_type;
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
        fixed_sized_list_array& operator=(const self_type&) = default;

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

        uint64_t m_list_size;

        // friend classes
        friend class array_crtp_base<self_type>;
        friend class list_array_crtp_base<self_type>;
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
    constexpr auto list_array_crtp_base<DERIVED>::raw_flat_array() const -> const array_wrapper*
    {
        return p_flat_array.get();
    }

    template <class DERIVED>
    constexpr auto list_array_crtp_base<DERIVED>::raw_flat_array() -> array_wrapper*
    {
        return p_flat_array.get();
    }

    template <class DERIVED>
    constexpr auto list_array_crtp_base<DERIVED>::value_begin() -> value_iterator
    {
        return value_iterator(detail::layout_value_functor<DERIVED, inner_value_type>(&this->derived_cast()), 0);
    }

    template <class DERIVED>
    constexpr auto list_array_crtp_base<DERIVED>::value_end() -> value_iterator
    {
        return value_iterator(
            detail::layout_value_functor<DERIVED, inner_value_type>(&this->derived_cast()),
            this->size()
        );
    }

    template <class DERIVED>
    constexpr auto list_array_crtp_base<DERIVED>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator(
            detail::layout_value_functor<const DERIVED, inner_value_type>(&this->derived_cast()),
            0
        );
    }

    template <class DERIVED>
    constexpr auto list_array_crtp_base<DERIVED>::value_cend() const -> const_value_iterator
    {
        return const_value_iterator(
            detail::layout_value_functor<const DERIVED, inner_value_type>(&this->derived_cast()),
            this->size()
        );
    }

    template <class DERIVED>
    constexpr auto list_array_crtp_base<DERIVED>::value(size_type i) -> inner_reference
    {
        const auto r = this->derived_cast().offset_range(i);
        using st = typename list_value::size_type;
        return list_value{p_flat_array.get(), static_cast<st>(r.first), static_cast<st>(r.second)};
    }

    template <class DERIVED>
    constexpr auto list_array_crtp_base<DERIVED>::value(size_type i) const -> inner_const_reference
    {
        const auto r = this->derived_cast().offset_range(i);
        using st = typename list_value::size_type;
        return list_value{p_flat_array.get(), static_cast<st>(r.first), static_cast<st>(r.second)};
    }

    template <class DERIVED>
    cloning_ptr<array_wrapper> list_array_crtp_base<DERIVED>::make_flat_array()
    {
        return array_factory(this->get_arrow_proxy().children()[0].view());
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
        if constexpr (BIG)
        {
            copy_tracker::increase("big_list_array");
        }
        else
        {
            copy_tracker::increase("list_array");
        }
    }

    template <bool BIG>
    constexpr auto list_array_impl<BIG>::operator=(const self_type& rhs) -> self_type&
    {
        if (this != &rhs)
        {
            base_type::operator=(rhs);
            p_list_offsets = make_list_offsets();
        }
        return *this;
    }

    template <bool BIG>
    constexpr auto list_array_impl<BIG>::offset_range(size_type i) const -> std::pair<offset_type, offset_type>
    {
        return std::make_pair(p_list_offsets[i], p_list_offsets[i + 1]);
    }

    template <bool BIG>
    constexpr auto list_array_impl<BIG>::make_list_offsets() -> offset_type*
    {
        return reinterpret_cast<offset_type*>(
            this->get_arrow_proxy().buffers()[OFFSET_BUFFER_INDEX].data() + this->get_arrow_proxy().offset()
        );
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
        if constexpr (BIG)
        {
            copy_tracker::increase("big_list_view_array");
        }
        else
        {
            copy_tracker::increase("list_view_array");
        }
    }

    template <bool BIG>
    constexpr auto list_view_array_impl<BIG>::operator=(const self_type& rhs) -> self_type&
    {
        if (this != &rhs)
        {
            base_type::operator=(rhs);
            p_list_offsets = make_list_offsets();
            p_list_sizes = make_list_sizes();
        }
        return *this;
    }

    template <bool BIG>
    inline constexpr auto list_view_array_impl<BIG>::offset_range(size_type i) const
        -> std::pair<offset_type, offset_type>
    {
        const auto offset = p_list_offsets[i];
        return std::make_pair(offset, offset + p_list_sizes[i]);
    }

    template <bool BIG>
    constexpr auto list_view_array_impl<BIG>::make_list_offsets() -> offset_type*
    {
        return reinterpret_cast<offset_type*>(
            this->get_arrow_proxy().buffers()[OFFSET_BUFFER_INDEX].data() + this->get_arrow_proxy().offset()
        );
    }

    template <bool BIG>
    constexpr auto list_view_array_impl<BIG>::make_list_sizes() -> offset_type*
    {
        return reinterpret_cast<offset_type*>(
            this->get_arrow_proxy().buffers()[SIZES_BUFFER_INDEX].data() + this->get_arrow_proxy().offset()
        );
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
        copy_tracker::increase("fixed_sized_list_array");
    }

    constexpr auto fixed_sized_list_array::offset_range(size_type i) const
        -> std::pair<offset_type, offset_type>
    {
        const auto offset = i * m_list_size;
        return std::make_pair(offset, offset + m_list_size);
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
