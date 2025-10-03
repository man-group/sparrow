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

#include <ranges>
#include <string_view>
#include <version>

#include "sparrow/utils/mp_utils.hpp"

#if defined(__cpp_lib_format)
#    include "sparrow/utils/format.hpp"
#endif

#include "sparrow/array_api.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/memory.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{
    class struct_array;

    namespace detail
    {
        template <>
        struct get_data_type_from_array<struct_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::STRUCT;
            }
        };
    }

    template <>
    struct array_inner_types<struct_array> : array_inner_types_base
    {
        using array_type = struct_array;
        using inner_value_type = struct_value;
        using inner_reference = struct_value;
        using inner_const_reference = struct_value;
        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_value_type>>;
        using const_value_iterator = functor_index_iterator<
            detail::layout_value_functor<const array_type, inner_value_type>>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    /**
     * @brief Type trait to check if a type is a struct_array.
     *
     * @tparam T Type to check
     */
    template <class T>
    constexpr bool is_struc_array_v = std::same_as<T, struct_array>;

    /**
     * @brief Array implementation for storing structured data with named fields.
     *
     * The struct_array provides a columnar storage format for structured data,
     * where each struct element consists of multiple named fields (children arrays).
     * This is similar to database records or C structs but optimized for analytical
     * workloads with columnar memory layout.
     *
     * Key features:
     * - Stores structured data with named fields
     * - Each field is a separate child array with its own type
     * - Supports nullable struct elements via validity bitmap
     * - Maintains Arrow struct format compatibility
     * - Efficient columnar access to field data
     *
     * The Arrow struct layout stores:
     * - A validity bitmap for the struct elements
     * - Child arrays for each field, all with the same length
     * - Schema information with field names and types
     *
     * Related Apache Arrow description and specification:
     * https://arrow.apache.org/docs/dev/format/Intro.html#struct
     * https://arrow.apache.org/docs/format/Columnar.html#struct-layout
     *
     * @pre All child arrays must have the same length
     * @pre Field names must be unique within the struct
     * @post Maintains Arrow struct format compatibility ("+s")
     * @post All child arrays remain synchronized in length
     * @post Thread-safe for read operations, requires external synchronization for writes
     *
     * @example
     * ```cpp
     * // Create child arrays for fields
     * primitive_array<int32_t> id_array({1, 2, 3});
     * primitive_array<std::string> name_array({"Alice", "Bob", "Charlie"});
     *
     * // Create struct array from children
     * std::vector<array> children = {
     *     std::move(id_array).with_name("id"),
     *     std::move(name_array).with_name("name")
     * };
     * struct_array persons(std::move(children));
     *
     * // Access struct elements
     * auto person = persons[0];  // Get struct_value
     * auto id_field = person["id"];  // Access field by name
     * ```
     */
    class struct_array final : public array_bitmap_base<struct_array>
    {
    public:

        using self_type = struct_array;
        using base_type = array_bitmap_base<self_type>;
        using inner_types = array_inner_types<self_type>;
        using value_iterator = typename inner_types::value_iterator;
        using const_value_iterator = typename inner_types::const_value_iterator;
        using size_type = typename base_type::size_type;

        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;

        using const_bitmap_range = base_type::const_bitmap_range;

        using inner_value_type = struct_value;
        using inner_reference = struct_value;
        using inner_const_reference = struct_value;

        using value_type = nullable<inner_value_type>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using iterator_tag = base_type::iterator_tag;

        /**
         * @brief Constructs struct array from Arrow proxy.
         *
         * @param proxy Arrow proxy containing struct array data and schema
         *
         * @pre proxy must contain valid Arrow struct array and schema
         * @pre proxy format must be "+s"
         * @pre proxy must have child arrays with consistent lengths
         * @post Array is initialized with data from proxy
         * @post Child arrays are accessible via field access methods
         * @post Validity bitmap is properly initialized
         */
        SPARROW_API explicit struct_array(arrow_proxy proxy);

        /**
         * @brief Generic constructor for creating struct array from various inputs.
         *
         * Creates a struct array from different input combinations. Arguments are
         * forwarded to compatible create_proxy() functions based on their types.
         *
         * @tparam Args Parameter pack for constructor arguments
         * @param args Constructor arguments (children, validity, metadata, etc.)
         *
         * @pre Arguments must match one of the create_proxy() overload signatures
         * @pre All child arrays must have the same length
         * @pre Child array names must be unique (if specified)
         * @post Array is created with the specified children and configuration
         * @post Field access is available for all child arrays
         */
        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<struct_array, Args...>)
        explicit struct_array(Args&&... args)
            : struct_array(create_proxy(std::forward<Args>(args)...))
        {
        }

        /**
         * @brief Copy constructor.
         *
         * @param rhs Source struct array to copy from
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post All child arrays are independently copied
         * @post Field structure and names are preserved
         */
        SPARROW_API struct_array(const struct_array& rhs);

        /**
         * @brief Copy assignment operator.
         *
         * @param rhs Source struct array to copy from
         * @return Reference to this array
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post Previous data is properly released
         * @post All child arrays are independently copied
         */
        SPARROW_API struct_array& operator=(const struct_array& rhs);

        struct_array(struct_array&&) = default;
        struct_array& operator=(struct_array&&) = default;

        /**
         * @brief Gets the number of child arrays (fields).
         *
         * @return Number of fields in the struct
         *
         * @post Returns non-negative count
         * @post Equals the number of distinct fields in the struct
         */
        [[nodiscard]] SPARROW_API size_type children_count() const;

        /**
         * @brief Gets const pointer to child array at specified index.
         *
         * @param i Index of the child array
         * @return Const pointer to the child array wrapper
         *
         * @pre i must be < children_count()
         * @post Returns non-null pointer to valid array_wrapper
         * @post Pointer remains valid while struct array exists
         */
        [[nodiscard]] SPARROW_API const array_wrapper* raw_child(std::size_t i) const;

        /**
         * @brief Gets mutable pointer to child array at specified index.
         *
         * @param i Index of the child array
         * @return Pointer to the child array wrapper
         *
         * @pre i must be < children_count()
         * @post Returns non-null pointer to valid array_wrapper
         * @post Modifications through pointer affect the struct array
         */
        [[nodiscard]] SPARROW_API array_wrapper* raw_child(std::size_t i);

        /**
         * @brief Gets the names of all child arrays.
         *
         * @return Range of child array names.
         */
        [[nodiscard]] auto names() const
        {
            return get_arrow_proxy().children()
                   | std::views::transform(
                       [](const auto& child)
                       {
                           return child.name();
                       }
                   );
        }

        /**
         * @brief Adds a child array to the struct.
         *
         * @param child The child array to add
         *
         * @post Increases the number of children by one
         */
        template <layout_or_array A>
        void add_child(A&& child);

        /**
         * @brief Adds multiple children to the struct array.
         *
         * This function template adds a range of children (layouts or arrays) to the struct array.
         * All children must have the same size as the current struct array.
         *
         * @tparam R An input range type whose value type satisfies the layout_or_array concept
         * @param children A range of child elements to be added to the struct array
         *
         * @throws Assertion error if any child's size doesn't match the struct array's size
         *
         * @note The function reserves memory upfront to optimize performance when adding multiple children
         * @note Children are forwarded to maintain their value category (lvalue/rvalue)
         */
        template <std::ranges::input_range R>
            requires layout_or_array<std::ranges::range_value_t<R>>
        void add_children(R&& children);

        /**
         * @brief Sets a child array at the specified index.
         *
         * @param child The child array to set
         * @param index The index at which to set the child
         *
         * @pre index must be < children_count()
         * @post Replaces the child array at the specified index. Release the array if it has the
         * ownership.
         */
        template <layout_or_array A>
        void set_child(A&& child, size_t index);

        /**
         * @brief Removes the last n children from the struct.
         *
         * @param n The number of children to remove
         *
         * @pre n must be <= children_count()
         * @post Decreases the number of children by n.
         * @post The owned arrays are released.
         */
        SPARROW_API void pop_children(size_t n);

    protected:

        /**
         * @brief Creates Arrow proxy from children arrays with explicit validity bitmap.
         *
         * @tparam CHILDREN_RANGE Type of children range
         * @tparam VB Type of validity bitmap input
         * @tparam METADATA_RANGE Type of metadata container
         * @param children Range of child arrays (one per field)
         * @param bitmaps Validity bitmap specification
         * @param name Optional name for the struct array
         * @param metadata Optional metadata for the struct array
         * @return Arrow proxy containing the struct array data and schema
         *
         * @pre CHILDREN_RANGE must be input range of array objects
         * @pre All children must have the same length
         * @pre bitmaps size must match children length (if not empty)
         * @post Returns valid Arrow proxy with struct format ("+s")
         * @post Child arrays are properly embedded in the structure
         * @post Validity bitmap reflects the provided bitmap data
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(child.size() == size) for each child
         */
        template <
            std::ranges::input_range CHILDREN_RANGE,
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::same_as<std::ranges::range_value_t<CHILDREN_RANGE>, array>
        [[nodiscard]] static auto create_proxy(
            CHILDREN_RANGE&& children,
            VB&& bitmaps,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        /**
         * @brief Creates Arrow proxy from children arrays with nullable flag.
         *
         * @tparam CHILDREN_RANGE Type of children range
         * @tparam METADATA_RANGE Type of metadata container
         * @param children Range of child arrays (one per field)
         * @param nullable Whether the struct array should support null values
         * @param name Optional name for the struct array
         * @param metadata Optional metadata for the struct array
         * @return Arrow proxy containing the struct array data and schema
         *
         * @pre CHILDREN_RANGE must be input range of array objects
         * @pre All children must have the same length
         * @post If nullable is true, array supports null values (though none initially set)
         * @post If nullable is false, array does not support null values
         * @post Returns valid Arrow proxy with struct format
         */
        template <std::ranges::input_range CHILDREN_RANGE, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::same_as<std::ranges::range_value_t<CHILDREN_RANGE>, array>
        [[nodiscard]] static auto create_proxy(
            CHILDREN_RANGE&& children,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        /**
         * @brief Implementation helper for creating Arrow proxy from components.
         *
         * @tparam CHILDREN_RANGE Type of children range
         * @tparam METADATA_RANGE Type of metadata container
         * @param children Range of child arrays
         * @param bitmap Optional validity bitmap
         * @param name Optional name for the struct array
         * @param metadata Optional metadata for the struct array
         * @return Arrow proxy containing the struct array data and schema
         *
         * @pre All children must have the same length
         * @pre If bitmap is provided, its size must match children length
         * @post Returns valid Arrow proxy with struct format ("+s")
         * @post Child arrays are embedded with proper ownership management
         * @post Schema includes field definitions from child arrays
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(child.size() == size) for each child
         */
        template <std::ranges::input_range CHILDREN_RANGE, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::same_as<std::ranges::range_value_t<CHILDREN_RANGE>, array>
        [[nodiscard]] static auto create_proxy_impl(
            CHILDREN_RANGE&& children,
            std::optional<validity_bitmap>&& bitmap,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        using children_type = std::vector<cloning_ptr<array_wrapper>>;

        /**
         * @brief Gets iterator to beginning of value range.
         *
         * @return Iterator pointing to the first struct element
         *
         * @post Returns valid iterator to array beginning
         */
        [[nodiscard]] SPARROW_API value_iterator value_begin();

        /**
         * @brief Gets iterator to end of value range.
         *
         * @return Iterator pointing past the last struct element
         *
         * @post Returns valid iterator to array end
         */
        [[nodiscard]] SPARROW_API value_iterator value_end();

        /**
         * @brief Gets const iterator to beginning of value range.
         *
         * @return Const iterator pointing to the first struct element
         *
         * @post Returns valid const iterator to array beginning
         */
        [[nodiscard]] SPARROW_API const_value_iterator value_cbegin() const;

        /**
         * @brief Gets const iterator to end of value range.
         *
         * @return Const iterator pointing past the last struct element
         *
         * @post Returns valid const iterator to array end
         */
        [[nodiscard]] SPARROW_API const_value_iterator value_cend() const;

        /**
         * @brief Gets mutable reference to struct at specified index.
         *
         * @param i Index of the struct to access
         * @return Mutable reference to the struct value
         *
         * @pre i must be < size()
         * @post Returns valid reference providing access to all fields
         */
        [[nodiscard]] SPARROW_API inner_reference value(size_type i);

        /**
         * @brief Gets const reference to struct at specified index.
         *
         * @param i Index of the struct to access
         * @return Const reference to the struct value
         *
         * @pre i must be < size()
         * @post Returns valid const reference providing access to all fields
         */
        [[nodiscard]] SPARROW_API inner_const_reference value(size_type i) const;

        /**
         * @brief Creates the children array wrappers.
         *
         * @return Vector of cloning pointers to child array wrappers
         *
         * @post Returns valid children collection
         * @post Each child corresponds to a field in the struct
         */
        [[nodiscard]] SPARROW_API children_type make_children();


        // data members
        children_type m_children;  ///< Collection of child arrays (fields)

        // friend classes
        friend class array_crtp_base<self_type>;

        // needs access to this->value(i)
        friend class detail::layout_value_functor<self_type, inner_value_type>;
        friend class detail::layout_value_functor<const self_type, inner_value_type>;
    };

    template <std::ranges::input_range CHILDREN_RANGE, validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
        requires std::same_as<std::ranges::range_value_t<CHILDREN_RANGE>, array>
    auto struct_array::create_proxy(
        CHILDREN_RANGE&& children,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        const auto size = children.empty() ? 0 : children[0].size();
        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<VB>(validity_input));
        return create_proxy_impl(
            std::forward<CHILDREN_RANGE>(children),
            std::move(vbitmap),
            std::move(name),
            std::move(metadata)
        );
    }

    template <std::ranges::input_range CHILDREN_RANGE, input_metadata_container METADATA_RANGE>
        requires std::same_as<std::ranges::range_value_t<CHILDREN_RANGE>, array>
    auto struct_array::create_proxy(
        CHILDREN_RANGE&& children,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        const size_t size = children.empty() ? 0 : children[0].size();
        return create_proxy_impl(
            std::forward<CHILDREN_RANGE>(children),
            nullable ? std::make_optional<validity_bitmap>(nullptr, size) : std::nullopt,
            std::move(name),
            std::move(metadata)
        );
    }

    template <std::ranges::input_range CHILDREN_RANGE, input_metadata_container METADATA_RANGE>
        requires std::same_as<std::ranges::range_value_t<CHILDREN_RANGE>, array>
    auto struct_array::create_proxy_impl(
        CHILDREN_RANGE&& children,
        std::optional<validity_bitmap>&& bitmap,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        const auto n_children = children.size();
        ArrowSchema** child_schemas = new ArrowSchema*[n_children];
        ArrowArray** child_arrays = new ArrowArray*[n_children];

        const auto size = children.empty() ? 0 : children[0].size();

        for (std::size_t i = 0; i < n_children; ++i)
        {
            auto& child = children[i];
            SPARROW_ASSERT_TRUE(child.size() == size);
            auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(child));
            child_arrays[i] = new ArrowArray(std::move(flat_arr));
            child_schemas[i] = new ArrowSchema(std::move(flat_schema));
        }

        const bool bitmap_has_value = bitmap.has_value();
        const auto null_count = bitmap_has_value ? bitmap->null_count() : 0;
        const auto flags = bitmap_has_value
                               ? std::make_optional<std::unordered_set<sparrow::ArrowFlag>>({ArrowFlag::NULLABLE})
                               : std::nullopt;

        ArrowSchema schema = make_arrow_schema(
            std::string("+s"),                    // format
            std::move(name),                      // name
            std::move(metadata),                  // metadata
            flags,                                // flags,
            child_schemas,                        // children
            repeat_view<bool>(true, n_children),  // children_ownership
            nullptr,                              // dictionary
            true                                  // dictionary ownership
        );

        buffer<uint8_t> bitmap_buffer = bitmap_has_value ? std::move(*bitmap).extract_storage()
                                                         : buffer<uint8_t>{nullptr, 0};

        std::vector<buffer<std::uint8_t>> arr_buffs(1);
        arr_buffs[0] = std::move(bitmap_buffer);

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),        // length
            static_cast<std::int64_t>(null_count),  // null_count
            0,                                      // offset
            std::move(arr_buffs),
            child_arrays,                         // children
            repeat_view<bool>(true, n_children),  // children_ownership
            nullptr,                              // dictionary
            true                                  // dictionary ownership
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <layout_or_array A>
    void struct_array::add_child(A&& child)
    {
        SPARROW_ASSERT_TRUE(child.size() == size());
        auto [array, schema] = extract_arrow_structures(std::forward<A>(child));
        get_arrow_proxy().add_child(std::move(array), std::move(schema));
        m_children.emplace_back(array_factory(get_arrow_proxy().children().back().view()));
    }

    template <std::ranges::input_range R>
        requires layout_or_array<std::ranges::range_value_t<R>>
    void struct_array::add_children(R&& children)
    {
        for (const auto& child : children)
        {
            SPARROW_ASSERT_TRUE(child.size() == size());
        }
        m_children.reserve(m_children.size() + children.size());
        for (auto&& child : children)
        {
            add_child(std::forward<decltype(child)>(child));
        }
    }

    template <layout_or_array A>
    void struct_array::set_child(A&& child, size_t index)
    {
        SPARROW_ASSERT_TRUE(child.size() == size());
        auto [array, schema] = extract_arrow_structures(std::forward<A>(child));
        get_arrow_proxy().set_child(index, std::move(array), std::move(schema));
        m_children[index] = array_factory(get_arrow_proxy().children()[index].view());
    }
}

#if defined(__cpp_lib_format)


template <>
struct std::formatter<sparrow::struct_array>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    SPARROW_API auto format(const sparrow::struct_array& struct_array, std::format_context& ctx) const
        -> decltype(ctx.out());
};

namespace sparrow
{
    SPARROW_API std::ostream& operator<<(std::ostream& os, const struct_array& value);
}

#endif
