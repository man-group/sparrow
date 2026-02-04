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

#include <optional>

#include "sparrow/array_api.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/debug/copy_tracker.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/layout/array_factory.hpp"
#include "sparrow/layout/array_helper.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/primitive_array.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/memory.hpp"
#include "sparrow/utils/metadata.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    /**
     * Functor for accessing elements in a layout.
     *
     * @tparam Layout The layout type.
     * @tparam is_const Whether the functor provides const access.
     */
    template <class Layout, bool is_const>
    class layout_element_functor
    {
    public:

        using layout_type = Layout;
        using storage_type = std::conditional_t<is_const, const layout_type*, layout_type>;
        using return_type = std::
            conditional_t<is_const, typename layout_type::const_reference, typename layout_type::reference>;

        /**
         * Default constructor.
         */
        constexpr layout_element_functor() = default;

        /**
         * Constructs a functor with the given layout.
         *
         * @param layout_ Pointer to the layout to access.
         */
        constexpr explicit layout_element_functor(storage_type layout_)
            : p_layout(layout_)
        {
        }

        /**
         * Access operator for getting element at index.
         *
         * @param i The index of the element to access.
         * @return Reference to the element at the specified index.
         */
        [[nodiscard]] constexpr return_type operator()(std::size_t i) const
        {
            return p_layout->operator[](i);
        }

    private:

        /** Pointer to the layout being accessed. */
        storage_type p_layout;
    };

    /**
     * Forward declaration of dictionary_encoded_array.
     *
     * @tparam IT The integral type used for dictionary keys.
     */
    template <std::integral IT>
    class dictionary_encoded_array;

    namespace detail
    {
        template <std::integral IT>
        struct get_data_type_from_array<sparrow::dictionary_encoded_array<IT>>
        {
            /**
             * Gets the data type for the dictionary keys.
             *
             * @return The data type corresponding to the key type.
             */
            [[nodiscard]] static constexpr sparrow::data_type get() noexcept
            {
                return get_data_type_from_array<primitive_array<IT>>::get();
            }
        };

        /**
         * Specialization to identify dictionary_encoded_array types.
         *
         * @tparam IT The integral type used for dictionary keys.
         */
        template <std::integral IT>
        struct is_dictionary_encoded_array<sparrow::dictionary_encoded_array<IT>>
        {
            /**
             * Returns true for dictionary_encoded_array types.
             *
             * @return Always true.
             */
            [[nodiscard]] static constexpr bool get() noexcept
            {
                return true;
            }
        };
    }

    /**
     * Checks whether T is a dictionary_encoded_array type.
     *
     * @tparam T The type to check.
     */
    template <class T>
    constexpr bool is_dictionary_encoded_array_v = detail::is_dictionary_encoded_array<T>::get();

    /**
     * Dictionary encoded array class.
     * Dictionary encoding is a data representation technique to represent values by integers referencing a
     * dictionary usually consisting of unique values. It can be effective when you have data with many
     * repeated values.
     * Related Apache Arrow specification:
     * https://arrow.apache.org/docs/dev/format/Columnar.html#dictionary-encoded-layout
     *
     * @tparam IT The integral type used for dictionary keys.
     */
    template <std::integral IT>
    class dictionary_encoded_array
    {
    public:

        using self_type = dictionary_encoded_array<IT>;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        using inner_value_type = array_traits::inner_value_type;

        using value_type = array_traits::value_type;
        using reference = array_traits::const_reference;
        using const_reference = array_traits::const_reference;

        using functor_type = layout_element_functor<self_type, true>;
        using const_functor_type = layout_element_functor<self_type, true>;

        using iterator = functor_index_iterator<functor_type>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_iterator = functor_index_iterator<const_functor_type>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        using keys_buffer_type = u8_buffer<IT>;

        /**
         * Constructs a dictionary encoded array from an arrow proxy.
         *
         * @param proxy The arrow proxy containing the array data and schema.
         */
        explicit dictionary_encoded_array(arrow_proxy proxy);

        /**
         * Copy constructor.
         *
         * @param other The dictionary_encoded_array to copy from.
         */
        constexpr dictionary_encoded_array(const self_type& other);

        /**
         * Copy assignment operator.
         *
         * @param other The dictionary_encoded_array to copy from.
         * @return Reference to this dictionary_encoded_array.
         */
        constexpr self_type& operator=(const self_type& other);

        /**
         * Move constructor.
         *
         * @param other The dictionary_encoded_array to move from.
         */
        constexpr dictionary_encoded_array(self_type&& other);

        /**
         * Move assignment operator.
         *
         * @param other The dictionary_encoded_array to move from.
         * @return Reference to this dictionary_encoded_array.
         */
        constexpr self_type& operator=(self_type&& other);

        /**
         * Gets the name of the array.
         *
         * @return Optional name of the array.
         */
        [[nodiscard]] constexpr std::optional<std::string_view> name() const;

        /**
         * Gets the metadata of the array.
         *
         * @return Optional metadata of the array.
         */
        [[nodiscard]] std::optional<key_value_view> metadata() const;

        /**
         * Gets the number of elements in the array.
         *
         * @return The number of elements.
         */
        [[nodiscard]] constexpr size_type size() const;

        /**
         * Checks if the array is empty.
         *
         * @return true if the array is empty, false otherwise.
         */
        [[nodiscard]] constexpr bool empty() const;

        /**
         * Access operator for getting element at index.
         *
         * @param i The index of the element to access.
         * @return Constant reference to the element at the specified index.
         */
        [[nodiscard]] SPARROW_CONSTEXPR_CLANG const_reference operator[](size_type i) const;

        /**
         * Gets an iterator to the beginning of the array.
         *
         * @return Iterator to the beginning.
         */
        [[nodiscard]] constexpr iterator begin();

        /**
         * Gets an iterator to the end of the array.
         *
         * @return Iterator to the end.
         */
        [[nodiscard]] constexpr iterator end();

        /**
         * Gets a constant iterator to the beginning of the array.
         *
         * @return Constant iterator to the beginning.
         */
        [[nodiscard]] constexpr const_iterator begin() const;

        /**
         * Gets a constant iterator to the end of the array.
         *
         * @return Constant iterator to the end.
         */
        [[nodiscard]] constexpr const_iterator end() const;

        /**
         * Gets a constant iterator to the beginning of the array.
         *
         * @return Constant iterator to the beginning.
         */
        [[nodiscard]] constexpr const_iterator cbegin() const;

        /**
         * Gets a constant iterator to the end of the array.
         *
         * @return Constant iterator to the end.
         */
        [[nodiscard]] constexpr const_iterator cend() const;

        /**
         * Gets a reverse iterator to the beginning of the array.
         *
         * @return Reverse iterator to the beginning.
         */
        [[nodiscard]] constexpr reverse_iterator rbegin();

        /**
         * Gets a reverse iterator to the end of the array.
         *
         * @return Reverse iterator to the end.
         */
        [[nodiscard]] constexpr reverse_iterator rend();

        /**
         * Gets a constant reverse iterator to the beginning of the array.
         *
         * @return Constant reverse iterator to the beginning.
         */
        [[nodiscard]] constexpr const_reverse_iterator rbegin() const;

        /**
         * Gets a constant reverse iterator to the end of the array.
         *
         * @return Constant reverse iterator to the end.
         */
        [[nodiscard]] constexpr const_reverse_iterator rend() const;

        /**
         * Gets a constant reverse iterator to the beginning of the array.
         *
         * @return Constant reverse iterator to the beginning.
         */
        [[nodiscard]] constexpr const_reverse_iterator crbegin() const;

        /**
         * Gets a constant reverse iterator to the end of the array.
         *
         * @return Constant reverse iterator to the end.
         */
        [[nodiscard]] constexpr const_reverse_iterator crend() const;

        /**
         * Gets a reference to the first element.
         *
         * @return Constant reference to the first element.
         */
        [[nodiscard]] SPARROW_CONSTEXPR_CLANG const_reference front() const;

        /**
         * Gets a reference to the last element.
         *
         * @return Constant reference to the last element.
         */
        [[nodiscard]] SPARROW_CONSTEXPR_CLANG const_reference back() const;

        /**
         * Constructs a dictionary encoded array with the given arguments.
         *
         * @tparam Args The argument types.
         * @param args Arguments forwarded to create_proxy.
         */
        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<dictionary_encoded_array<IT>, Args...>)
        explicit dictionary_encoded_array(Args&&... args)
            : dictionary_encoded_array(create_proxy(std::forward<Args>(args)...))
        {
        }

        /**
         * Slices the array to keep only the elements between the given \p start and \p end.
         * A copy of the \ref array is modified. The data is not modified, only the ArrowArray.offset and
         * ArrowArray.length are updated. If \p end is greater than the size of the buffers, the following
         * elements will be invalid.
         *
         * @param start The index of the first element to keep. Must be less than \p end.
         * @param end The index of the first element to discard. Must be less than the size of the buffers.
         */
        [[nodiscard]] constexpr self_type slice(size_type start, size_type end) const;

        /**
         * Slices the array to keep only the elements between the given \p start and \p end.
         * A view of the \ref array is returned. The data is not modified, only the ArrowArray.offset and
         * ArrowArray.length are updated. If \p end is greater than the size of the buffers, the following
         * elements will be invalid.
         *
         * @param start The index of the first element to keep. Must be less than \p end.
         * @param end The index of the first element to discard. Must be less than the size of the buffers.
         */
        [[nodiscard]] constexpr self_type slice_view(size_type start, size_type end) const;

    private:

        /**
         * Creates an arrow proxy from keys buffer, values array, and validity bitmap.
         *
         * @tparam R The validity bitmap input type.
         * @tparam METADATA_RANGE The metadata container type.
         * @param keys The buffer containing dictionary keys.
         * @param values The array containing dictionary values.
         * @param bitmaps The validity bitmap.
         * @param name Optional name for the array.
         * @param metadata Optional metadata for the array.
         * @return An arrow proxy containing the dictionary encoded array data.
         */
        template <
            validity_bitmap_input R = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static auto create_proxy(
            keys_buffer_type&& keys,
            array&& values,
            R&& bitmaps,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        /**
         * Creates an arrow proxy from keys buffer and values array.
         *
         * @tparam R The validity bitmap input type.
         * @tparam METADATA_RANGE The metadata container type.
         * @param keys The buffer containing dictionary keys.
         * @param values The array containing dictionary values.
         * @param nullable Whether the array can contain null values.
         * @param name Optional name for the array.
         * @param metadata Optional metadata for the array.
         * @return An arrow proxy containing the dictionary encoded array data.
         */
        template <
            validity_bitmap_input R = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static auto create_proxy(
            keys_buffer_type&& keys,
            array&& values,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        /**
         * Internal implementation for creating an arrow proxy.
         *
         * @tparam METADATA_RANGE The metadata container type.
         * @param keys The buffer containing dictionary keys.
         * @param values The array containing dictionary values.
         * @param validity Optional validity bitmap.
         * @param name Optional name for the array.
         * @param metadata Optional metadata for the array.
         * @return An arrow proxy containing the dictionary encoded array data.
         */
        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static auto create_proxy_impl(
            keys_buffer_type&& keys,
            array&& values,
            std::optional<validity_bitmap> validity = std::nullopt,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        /**
         * Creates an arrow proxy from a key range and values array.
         *
         * @tparam KEY_RANGE The key range type.
         * @tparam R The validity bitmap input type.
         * @tparam METADATA_RANGE The metadata container type.
         * @param keys The range of keys to store.
         * @param values The array containing dictionary values.
         * @param bitmaps The validity bitmap.
         * @param name Optional name for the array.
         * @param metadata Optional metadata for the array.
         * @return An arrow proxy containing the dictionary encoded array data.
         */
        template <
            std::ranges::input_range KEY_RANGE,
            validity_bitmap_input R = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(
                !std::same_as<KEY_RANGE, keys_buffer_type>
                and std::same_as<IT, std::ranges::range_value_t<KEY_RANGE>>
            )
        [[nodiscard]] static arrow_proxy create_proxy(
            KEY_RANGE&& keys,
            array&& values,
            R&& bitmaps = validity_bitmap{validity_bitmap::default_allocator()},
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
        {
            keys_buffer_type keys_buffer(std::forward<KEY_RANGE>(keys));
            return create_proxy(
                std::move(keys_buffer),
                std::forward<array>(values),
                std::forward<R>(bitmaps),
                std::move(name),
                std::move(metadata)
            );
        }

        /**
         * Creates an arrow proxy from a range of nullable keys and values array.
         *
         * @tparam NULLABLE_KEY_RANGE The nullable key range type.
         * @tparam METADATA_RANGE The metadata container type.
         * @param nullable_keys The range of nullable keys to store.
         * @param values The array containing dictionary values.
         * @param name Optional name for the array.
         * @param metadata Optional metadata for the array.
         * @return An arrow proxy containing the dictionary encoded array data.
         */
        template <
            std::ranges::input_range NULLABLE_KEY_RANGE,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::is_same_v<std::ranges::range_value_t<NULLABLE_KEY_RANGE>, nullable<IT>>
        static arrow_proxy create_proxy(
            NULLABLE_KEY_RANGE&& nullable_keys,
            array&& values,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        using keys_layout = primitive_array<IT>;
        using values_layout = cloning_ptr<array_wrapper>;
        /**
         * Gets a dummy inner value for null references.
         *
         * @return Constant reference to a dummy inner value.
         */
        [[nodiscard]] const inner_value_type& dummy_inner_value() const;

        /**
         * Gets a dummy const reference for null values.
         *
         * @return A dummy const reference.
         */
        [[nodiscard]] const_reference dummy_const_reference() const;

        /**
         * Creates a keys layout from the arrow proxy.
         *
         * @param proxy The arrow proxy containing the keys data.
         * @return A keys layout object.
         */
        [[nodiscard]] static constexpr keys_layout create_keys_layout(arrow_proxy& proxy);

        /**
         * Creates a values layout from the arrow proxy.
         *
         * @param proxy The arrow proxy containing the values data.
         * @return A values layout object.
         */
        [[nodiscard]] static values_layout create_values_layout(arrow_proxy& proxy);

        /**
         * Gets a reference to the internal arrow proxy.
         *
         * @return Reference to the arrow proxy.
         */
        [[nodiscard]] constexpr arrow_proxy& get_arrow_proxy();

        /**
         * Gets a constant reference to the internal arrow proxy.
         *
         * @return Constant reference to the arrow proxy.
         */
        [[nodiscard]] constexpr const arrow_proxy& get_arrow_proxy() const;

        /** The arrow proxy containing the array data and schema. */
        arrow_proxy m_proxy;
        /** The layout for accessing dictionary keys. */
        keys_layout m_keys_layout;
        /** The layout for accessing dictionary values. */
        values_layout p_values_layout;

        friend class detail::array_access;
    };

    /**
     * Equality comparison operator for dictionary_encoded_array.
     *
     * @tparam IT The integral type used for dictionary keys.
     * @param lhs The first dictionary_encoded_array to compare.
     * @param rhs The second dictionary_encoded_array to compare.
     * @return true if the arrays are equal, false otherwise.
     */
    template <class IT>
    constexpr bool operator==(const dictionary_encoded_array<IT>& lhs, const dictionary_encoded_array<IT>& rhs);

    /*******************************************
     * dictionary_encoded_array implementation *
     *******************************************/

    template <std::integral IT>
    dictionary_encoded_array<IT>::dictionary_encoded_array(arrow_proxy proxy)
        : m_proxy(std::move(proxy))
        , m_keys_layout(create_keys_layout(m_proxy))
        , p_values_layout(create_values_layout(m_proxy))
    {
        SPARROW_ASSERT_TRUE(data_type_is_integer(m_proxy.data_type()));
    }

    template <std::integral IT>
    constexpr dictionary_encoded_array<IT>::dictionary_encoded_array(const self_type& rhs)
        : m_proxy(rhs.m_proxy)
        , m_keys_layout(create_keys_layout(m_proxy))
        , p_values_layout(create_values_layout(m_proxy))
    {
        copy_tracker::increase("dictionary_encoded_array<" + std::string(typeid(IT).name()) + ">");
    }

    template <std::integral IT>
    constexpr auto dictionary_encoded_array<IT>::operator=(const self_type& rhs) -> self_type&
    {
        if (this != &rhs)
        {
            m_proxy = rhs.m_proxy;
            m_keys_layout = create_keys_layout(m_proxy);
            p_values_layout = create_values_layout(m_proxy);
        }
        return *this;
    }

    template <std::integral IT>
    constexpr dictionary_encoded_array<IT>::dictionary_encoded_array(self_type&& rhs)
        : m_proxy(std::move(rhs.m_proxy))
        , m_keys_layout(create_keys_layout(m_proxy))
        , p_values_layout(create_values_layout(m_proxy))
    {
    }

    template <std::integral IT>
    constexpr auto dictionary_encoded_array<IT>::operator=(self_type&& rhs) -> self_type&
    {
        if (this != &rhs)
        {
            using std::swap;
            swap(m_proxy, rhs.m_proxy);
            m_keys_layout = create_keys_layout(m_proxy);
            p_values_layout = create_values_layout(m_proxy);
        }
        return *this;
    }

    template <std::integral IT>
    template <validity_bitmap_input VBI, input_metadata_container METADATA_RANGE>
    auto dictionary_encoded_array<IT>::create_proxy(
        keys_buffer_type&& keys,
        array&& values,
        VBI&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        const auto size = keys.size();
        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<VBI>(validity_input));
        return create_proxy_impl(
            std::forward<keys_buffer_type>(keys),
            std::forward<array>(values),
            std::make_optional<validity_bitmap>(std::move(vbitmap)),
            std::move(name),
            std::move(metadata)
        );
    }

    template <std::integral IT>
    template <validity_bitmap_input VBI, input_metadata_container METADATA_RANGE>
    auto dictionary_encoded_array<IT>::create_proxy(
        keys_buffer_type&& keys,
        array&& values,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        const auto size = keys.size();
        return create_proxy_impl(
            std::forward<keys_buffer_type>(keys),
            std::forward<array>(values),
            nullable ? std::make_optional<validity_bitmap>(nullptr, size, validity_bitmap::default_allocator())
                     : std::nullopt,
            std::move(name),
            std::move(metadata)
        );
    }

    template <std::integral IT>
    template <input_metadata_container METADATA_RANGE>
    [[nodiscard]] arrow_proxy dictionary_encoded_array<IT>::create_proxy_impl(
        keys_buffer_type&& keys,
        array&& values,
        std::optional<validity_bitmap> validity,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto size = keys.size();
        auto [value_array, value_schema] = extract_arrow_structures(std::move(values));
        static const repeat_view<bool> children_ownership{true, 0};

        const std::optional<std::unordered_set<sparrow::ArrowFlag>>
            flags = validity.has_value()
                        ? std::make_optional<std::unordered_set<sparrow::ArrowFlag>>({ArrowFlag::NULLABLE})
                        : std::nullopt;

        // create arrow schema and array
        ArrowSchema schema = make_arrow_schema(
            data_type_to_format(detail::get_data_type_from_array<self_type>::get()),  // format
            std::move(name),                                                          // name
            std::move(metadata),                                                      // metadata
            flags,                                                                    // flags
            nullptr,                                                                  // children
            children_ownership,                                                       // children_ownership
            new ArrowSchema(std::move(value_schema)),                                 // dictionary
            true                                                                      // dictionary ownership
        );

        const size_t null_count = validity.has_value() ? validity->null_count() : 0;

        std::vector<buffer<uint8_t>> buffers;
        buffers.reserve(2);
        buffers.emplace_back(
            validity.has_value() ? std::move(*validity).extract_storage()
                                 : buffer<uint8_t>{nullptr, 0, buffer<uint8_t>::default_allocator()}
        );
        buffers.emplace_back(std::move(keys).extract_storage());
        // create arrow array
        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),        // length
            static_cast<std::int64_t>(null_count),  // Null count
            0,                                      // offset
            std::move(buffers),
            nullptr,                                 // children
            children_ownership,                      // children_ownership
            new ArrowArray(std::move(value_array)),  // dictionary
            true                                     // dictionary ownership
        );
        return arrow_proxy(std::move(arr), std::move(schema));
    }

    template <std::integral IT>
    template <std::ranges::input_range NULLABLE_KEY_RANGE, input_metadata_container METADATA_RANGE>
        requires std::is_same_v<std::ranges::range_value_t<NULLABLE_KEY_RANGE>, nullable<IT>>
    arrow_proxy dictionary_encoded_array<IT>::create_proxy(
        NULLABLE_KEY_RANGE&& nullable_keys,
        array&& values,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        auto keys = nullable_keys
                    | std::views::transform(
                        [](const auto& v)
                        {
                            return v.get();
                        }
                    );
        auto is_non_null = nullable_keys
                           | std::views::transform(
                               [](const auto& v)
                               {
                                   return v.has_value();
                               }
                           );
        return create_proxy(
            std::move(keys),
            std::forward<array>(values),
            std::move(is_non_null),
            std::move(name),
            std::move(metadata)
        );
    }

    template <std::integral IT>
    constexpr std::optional<std::string_view> dictionary_encoded_array<IT>::name() const
    {
        return m_proxy.name();
    }

    template <std::integral IT>
    std::optional<key_value_view> dictionary_encoded_array<IT>::metadata() const
    {
        return m_proxy.metadata();
    }

    template <std::integral IT>
    constexpr auto dictionary_encoded_array<IT>::size() const -> size_type
    {
        return m_proxy.length();
    }

    template <std::integral IT>
    constexpr auto dictionary_encoded_array<IT>::empty() const -> bool
    {
        return size() == 0;
    }

    template <std::integral IT>
    SPARROW_CONSTEXPR_CLANG auto dictionary_encoded_array<IT>::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        const auto index = m_keys_layout[i];

        if (index.has_value())
        {
            SPARROW_ASSERT_TRUE(index.value() >= 0);
            return array_element(*p_values_layout, static_cast<std::size_t>(index.value()));
        }
        else
        {
            return dummy_const_reference();
        }
    }

    template <std::integral IT>
    constexpr auto dictionary_encoded_array<IT>::begin() -> iterator
    {
        return iterator(functor_type(this), 0u);
    }

    template <std::integral IT>
    constexpr auto dictionary_encoded_array<IT>::end() -> iterator
    {
        return iterator(functor_type(this), size());
    }

    template <std::integral IT>
    constexpr auto dictionary_encoded_array<IT>::begin() const -> const_iterator
    {
        return cbegin();
    }

    template <std::integral IT>
    constexpr auto dictionary_encoded_array<IT>::end() const -> const_iterator
    {
        return cend();
    }

    template <std::integral IT>
    constexpr auto dictionary_encoded_array<IT>::cbegin() const -> const_iterator
    {
        return const_iterator(const_functor_type(this), 0u);
    }

    template <std::integral IT>
    constexpr auto dictionary_encoded_array<IT>::cend() const -> const_iterator
    {
        return const_iterator(const_functor_type(this), size());
    }

    template <std::integral IT>
    constexpr auto dictionary_encoded_array<IT>::rbegin() -> reverse_iterator
    {
        return reverse_iterator(end());
    }

    template <std::integral IT>
    [[nodiscard]] constexpr auto dictionary_encoded_array<IT>::rend() -> reverse_iterator
    {
        return reverse_iterator(begin());
    }

    template <std::integral IT>
    [[nodiscard]] constexpr auto dictionary_encoded_array<IT>::rbegin() const -> const_reverse_iterator
    {
        return const_reverse_iterator(cend());
    }

    template <std::integral IT>
    [[nodiscard]] constexpr auto dictionary_encoded_array<IT>::rend() const -> const_reverse_iterator
    {
        return const_reverse_iterator(cbegin());
    }

    template <std::integral IT>
    [[nodiscard]] constexpr auto dictionary_encoded_array<IT>::crbegin() const -> const_reverse_iterator
    {
        return rbegin();
    }

    template <std::integral IT>
    [[nodiscard]] constexpr auto dictionary_encoded_array<IT>::crend() const -> const_reverse_iterator
    {
        return rend();
    }

    template <std::integral IT>
    SPARROW_CONSTEXPR_CLANG auto dictionary_encoded_array<IT>::front() const -> const_reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return operator[](0);
    }

    template <std::integral IT>
    SPARROW_CONSTEXPR_CLANG auto dictionary_encoded_array<IT>::back() const -> const_reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return operator[](size() - 1);
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::dummy_inner_value() const -> const inner_value_type&
    {
        static const inner_value_type instance = array_default_element_value(*p_values_layout);
        return instance;
    }

    template <std::integral IT>
    constexpr auto dictionary_encoded_array<IT>::slice(size_type start, size_type end) const -> self_type
    {
        SPARROW_ASSERT_TRUE(start <= end);
        return self_type{get_arrow_proxy().slice(start, end)};
    }

    template <std::integral IT>
    constexpr auto dictionary_encoded_array<IT>::slice_view(size_type start, size_type end) const -> self_type
    {
        SPARROW_ASSERT_TRUE(start <= end);
        return self_type{get_arrow_proxy().slice_view(start, end)};
    }

    /*template <std::integral IT>
    auto dictionary_encoded_array<IT>::dummy_inner_const_reference() const -> inner_const_reference
    {
        static const inner_const_reference instance =
            std::visit([](const auto& val) -> inner_const_reference { return val; }, dummy_inner_value());
        return instance;
    }*/

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::dummy_const_reference() const -> const_reference
    {
        static const const_reference instance = std::visit(
            [](const auto& val) -> const_reference
            {
                using inner_ref = typename arrow_traits<std::decay_t<decltype(val)>>::const_reference;
                return const_reference{nullable<inner_ref>(inner_ref(val), false)};
            },
            dummy_inner_value()
        );
        return instance;
    }

    template <std::integral IT>
    typename dictionary_encoded_array<IT>::values_layout
    dictionary_encoded_array<IT>::create_values_layout(arrow_proxy& proxy)
    {
        const auto& dictionary = proxy.dictionary();
        SPARROW_ASSERT_TRUE(dictionary);
        arrow_proxy ar_dictionary{&(dictionary->array()), &(dictionary->schema())};
        return array_factory(std::move(ar_dictionary));
    }

    template <std::integral IT>
    constexpr auto dictionary_encoded_array<IT>::create_keys_layout(arrow_proxy& proxy) -> keys_layout
    {
        return keys_layout{arrow_proxy{&proxy.array(), &proxy.schema()}};
    }

    template <std::integral IT>
    constexpr auto dictionary_encoded_array<IT>::get_arrow_proxy() -> arrow_proxy&
    {
        return m_proxy;
    }

    template <std::integral IT>
    constexpr auto dictionary_encoded_array<IT>::get_arrow_proxy() const -> const arrow_proxy&
    {
        return m_proxy;
    }

    template <class IT>
    constexpr bool operator==(const dictionary_encoded_array<IT>& lhs, const dictionary_encoded_array<IT>& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }
}

#if defined(__cpp_lib_format)
template <std::integral IT>
struct std::formatter<sparrow::dictionary_encoded_array<IT>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::dictionary_encoded_array<IT>& ar, std::format_context& ctx) const
    {
        std::format_to(ctx.out(), "Dictionary [size={}] <", ar.size());
        std::for_each(
            ar.cbegin(),
            std::prev(ar.cend()),
            [&ctx](const auto& value)
            {
                std::format_to(ctx.out(), "{}, ", value);
            }
        );
        std::format_to(ctx.out(), "{}>", ar.back());
        return ctx.out();
    }
};

namespace sparrow
{
    template <std::integral IT>
    std::ostream& operator<<(std::ostream& os, const dictionary_encoded_array<IT>& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

#endif
