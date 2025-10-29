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

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/primitive_data_access.hpp"
#include "sparrow/u8_buffer.hpp"
#include "sparrow/utils/extension.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/repeat_container.hpp"

namespace sparrow
{
    template <trivial_copyable_type T, typename Ext = empty_extension, trivial_copyable_type T2 = T>
    class primitive_array_impl;

    template <trivial_copyable_type T, typename Ext, trivial_copyable_type T2>
    struct array_inner_types<primitive_array_impl<T, Ext, T2>> : array_inner_types_base
    {
        using array_type = primitive_array_impl<T, Ext, T2>;

        using data_access_type = details::primitive_data_access<T, T2>;
        using inner_value_type = typename data_access_type::inner_value_type;
        using inner_reference = typename data_access_type::inner_reference;
        using inner_const_reference = typename data_access_type::inner_const_reference;
        using pointer = typename data_access_type::inner_pointer;
        using const_pointer = typename data_access_type::inner_const_pointer;

        using value_iterator = typename data_access_type::value_iterator;
        using const_value_iterator = typename data_access_type::const_value_iterator;

        using bitmap_const_reference = bitmap_type::const_reference;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;

        using iterator_tag = std::random_access_iterator_tag;
    };

    namespace detail
    {
        template <class T>
        struct primitive_data_traits;

        template <trivial_copyable_type T, typename Ext, trivial_copyable_type T2>
        struct get_data_type_from_array<primitive_array_impl<T, Ext, T2>>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return primitive_data_traits<T>::type_id;
            }
        };
    }

    /**
     * @brief Array implementation for primitive (trivially copyable) types.
     *
     * This class provides a concrete implementation of an Arrow-compatible array
     * for primitive types such as integers, floating-point numbers, and other
     * trivially copyable types. It manages both the data buffer and validity bitmap.
     *
     * Related Apache Arrow description and specification:
     * - https://arrow.apache.org/docs/dev/format/Intro.html#fixed-size-primitive-layout
     * - https://arrow.apache.org/docs/dev/format/Columnar.html#fixed-size-primitive-layout
     *
     * @tparam T The primitive type to store. Must satisfy trivial_copyable_type concept.
     *
     * @pre T must be trivially copyable
     * @post Array maintains Arrow array format compatibility
     *
     * @example
     * ```cpp
     * // Create array from range
     * primitive_array_impl<int> arr(std::ranges::iota_view{0, 10});
     *
     * // Create array with missing values
     * std::vector<int> values{1, 2, 3, 4, 5};
     * std::vector<bool> validity{true, false, true, true, false};
     * primitive_array_impl<int> arr_with_nulls(values, validity);
     * ```
     */
    template <trivial_copyable_type T, typename Ext, trivial_copyable_type T2>
    class primitive_array_impl final : public mutable_array_bitmap_base<primitive_array_impl<T, Ext, T2>>,
                                       private details::primitive_data_access<T, T2>,
                                       public Ext
    {
    public:

        using self_type = primitive_array_impl<T, Ext, T2>;
        using base_type = mutable_array_bitmap_base<primitive_array_impl<T, Ext, T2>>;
        using access_class_type = details::primitive_data_access<T, T2>;
        using size_type = std::size_t;

        using inner_types = array_inner_types<self_type>;
        using inner_value_type = typename inner_types::inner_value_type;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;

        using pointer = typename inner_types::pointer;
        using const_pointer = typename inner_types::const_pointer;

        using value_iterator = typename base_type::value_iterator;
        using const_value_iterator = typename base_type::const_value_iterator;

        /**
         * @brief Constructs a primitive array from an existing Arrow proxy.
         *
         * @param proxy_param Arrow proxy containing the array data and schema
         *
         * @pre proxy_param must contain valid Arrow array and schema data
         * @pre Arrow schema format must match the primitive type T
         * @post Array is initialized with data from the proxy
         * @post Array maintains ownership of the proxy data
         */
        explicit primitive_array_impl(arrow_proxy);

        /**
         * @brief Constructs an array of trivial copyable type with values and optional bitmap.
         *
         * The first argument can be any range of values as long as its value type is convertible
         * to \c T.
         * The second argument can be:
         * - a bitmap range, i.e. a range of boolean-like values indicating the non-missing values.
         *   The bitmap range and the value range must have the same size.
         * - a range of indices indicating the missing values.
         * - omitted: this is equivalent as passing a bitmap range full of \c true.
         *
         * @tparam Args Parameter pack for constructor arguments
         *
         * @param args Constructor arguments (values, optional validity bitmap, optional metadata)
         *
         * @pre First argument must be convertible to a range of T values
         * @pre If bitmap is provided, it must have the same size as the value range
         * @post Array contains copies of the provided values
         * @post Validity bitmap is set according to the provided bitmap or defaults to all valid
         */
        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<primitive_array_impl<T, Ext, T2>, Args...>)
        explicit primitive_array_impl(Args&&... args)
            : base_type(create_proxy(std::forward<Args>(args)...))
            , access_class_type(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
        {
        }

        /**
         * @brief Constructs a primitive array from an initializer list of raw values.
         *
         * @tparam METADATA_RANGE Type of metadata container
         * @param init Initializer list of values
         * @param nullable Whether the array should support null values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         *
         * @pre All values in init must be valid instances of inner_value_type
         * @post Array contains copies of all values from the initializer list
         * @post If nullable is true, array supports null values (though none are set)
         * @post If nullable is false, array does not support null values
         */
        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        primitive_array_impl(
            std::initializer_list<inner_value_type> init,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
            : base_type(create_proxy(init, nullable, std::move(name), std::move(metadata)))
            , access_class_type(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
        {
        }

        /**
         * @brief Copy constructor.
         *
         * @param rhs Source array to copy from
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post This array is independent of rhs (no shared ownership)
         */
        constexpr primitive_array_impl(const primitive_array_impl&);

        /**
         * @brief Copy assignment operator.
         *
         * @param rhs Source array to copy from
         * @return Reference to this array
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post Previous data in this array is properly released
         * @post This array is independent of rhs (no shared ownership)
         */
        constexpr primitive_array_impl& operator=(const primitive_array_impl&);

        /**
         * @brief Move constructor.
         *
         * @param rhs Source array to move from
         *
         * @pre rhs must be in a valid state
         * @post This array takes ownership of rhs data
         * @post rhs is left in a valid but unspecified state
         */
        constexpr primitive_array_impl(primitive_array_impl&&) noexcept;

        /**
         * @brief Move assignment operator.
         *
         * @param rhs Source array to move from
         * @return Reference to this array
         *
         * @pre rhs must be in a valid state
         * @post This array takes ownership of rhs data
         * @post Previous data in this array is properly released
         * @post rhs is left in a valid but unspecified state
         */
        constexpr primitive_array_impl& operator=(primitive_array_impl&&) noexcept;

    private:

        /**
         * @brief Creates an Arrow proxy from a data buffer and validity bitmap.
         *
         * @tparam VALIDITY_RANGE Type of validity bitmap input
         * @tparam METADATA_RANGE Type of metadata container
         * @param data_buffer Buffer containing the array data
         * @param size Number of elements in the array
         * @param bitmaps Validity bitmap specification
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the array data and schema
         *
         * @pre data_buffer must contain at least size elements of type T
         * @pre If bitmaps specifies a bitmap, it must have exactly size bits
         * @post Returned proxy contains valid Arrow array and schema
         */
        template <
            validity_bitmap_input VALIDITY_RANGE = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static auto create_proxy(
            u8_buffer<T2>&& data_buffer,
            size_t size,
            VALIDITY_RANGE&& bitmaps,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        /**
         * @brief Creates an Arrow proxy from a data buffer with nullable flag.
         *
         * @tparam VALIDITY_RANGE Type of validity bitmap input
         * @tparam METADATA_RANGE Type of metadata container
         * @param data_buffer Buffer containing the array data
         * @param size Number of elements in the array
         * @param nullable Whether the array should support null values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the array data and schema
         *
         * @pre data_buffer must contain at least size elements of type T
         * @pre size must match the actual number of elements in data_buffer
         * @post Returned proxy contains valid Arrow array and schema
         * @post If nullable is true, array supports null values (though none are initially set)
         * @post If nullable is false, array does not support null values
         */
        template <
            validity_bitmap_input VALIDITY_RANGE = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static auto create_proxy(
            u8_buffer<T2>&& data_buffer,
            size_t size,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        /**
         * @brief Creates an Arrow proxy from a range of values (no missing values).
         *
         * @tparam R Type of input range containing values
         * @tparam METADATA_RANGE Type of metadata container
         * @param range Input range of values convertible to T
         * @param nullable Whether the array should support null values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the array data and schema
         *
         * @pre R must be an input range with values convertible to T
         * @pre R must not be a u8_buffer type
         * @pre range must be valid and accessible
         * @post Returned proxy contains copies of all values from the range
         * @post If nullable is true, array supports null values (though none are initially set)
         * @post All values in the array are marked as valid (non-null)
         */
        template <std::ranges::input_range R, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(
                std::convertible_to<std::ranges::range_value_t<R>, T2>
                && !mpl::is_type_instance_of_v<R, u8_buffer>
            )
        [[nodiscard]] static auto create_proxy(
            R&& range,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        /**
         * @brief Creates an Arrow proxy with n copies of the same value.
         *
         * @tparam U Type of the value to replicate (must be convertible to T)
         * @tparam METADATA_RANGE Type of metadata container
         * @param n Number of elements to create
         * @param value Value to replicate n times
         * @param nullable Whether the array should support null values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the array data and schema
         *
         * @pre n must be a valid size (typically n >= 0)
         * @pre value must be convertible to type T
         * @post Returned proxy contains exactly n elements, all equal to static_cast<T2>(value)
         * @post If nullable is true, array supports null values (though none are initially set)
         * @post All values in the array are marked as valid (non-null)
         */
        template <class U, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<U, T2>
        [[nodiscard]] static arrow_proxy create_proxy(
            size_type n,
            const U& value = U{},
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates an Arrow proxy from a range of values and validity bitmap.
         *
         * @tparam R Type of input range containing values
         * @tparam R2 Type of validity bitmap input
         * @tparam METADATA_RANGE Type of metadata container
         * @param values Input range of values convertible to T
         * @param validity_input Validity bitmap specification (bitmap or indices)
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the array data and schema
         *
         * @pre R must be an input range with values convertible to T
         * @pre R2 must satisfy validity_bitmap_input concept
         * @pre values and validity_input must have compatible sizes
         * @pre If validity_input is a bitmap, it must have the same size as values
         * @pre If validity_input is indices, all indices must be valid positions in values
         * @post Returned proxy contains copies of all values from the range
         * @post Validity bitmap is set according to validity_input
         * @post Array supports null values (nullable = true)
         */
        template <
            std::ranges::input_range R,
            validity_bitmap_input R2,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(std::convertible_to<std::ranges::range_value_t<R>, T2>)
        [[nodiscard]] static arrow_proxy create_proxy(
            R&&,
            R2&&,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates an Arrow proxy from a range of nullable values.
         *
         * @tparam NULLABLE_RANGE Type of input range containing nullable<T2> values
         * @tparam METADATA_RANGE Type of metadata container
         * @param nullable_range Input range of nullable<T2> values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the array data and schema
         *
         * @pre NULLABLE_RANGE must be an input range of exactly nullable<T2> values
         * @pre nullable_range must be valid and accessible
         * @post Returned proxy contains the unwrapped values from nullable_range
         * @post Validity bitmap reflects the has_value() status of each nullable<T2>
         * @post Array supports null values (nullable = true)
         * @post Elements where nullable<T2>.has_value() == false are marked as null
         */
        template <std::ranges::input_range NULLABLE_RANGE, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::is_same_v<std::ranges::range_value_t<NULLABLE_RANGE>, nullable<T2>>
        [[nodiscard]] static arrow_proxy create_proxy(
            NULLABLE_RANGE&&,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Implementation helper for creating Arrow proxy from components.
         *
         * @tparam METADATA_RANGE Type of metadata container
         * @param data_buffer Buffer containing the array data
         * @param size Number of elements in the array
         * @param bitmap Optional validity bitmap
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         * @return Arrow proxy containing the array data and schema
         *
         * @pre data_buffer must contain at least size elements of type T
         * @pre If bitmap has value, it must have exactly size bits
         * @pre size must accurately reflect the number of elements
         * @post Returned proxy contains valid Arrow array and schema
         * @post Arrow array format matches primitive type T requirements
         * @post Null count in Arrow array matches bitmap null count (if bitmap present)
         */
        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy_impl(
            u8_buffer<T2>&& data_buffer,
            size_t size,
            std::optional<validity_bitmap>&& bitmap,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        using access_class_type::value;
        using access_class_type::value_begin;
        using access_class_type::value_cbegin;
        using access_class_type::value_cend;
        using access_class_type::value_end;

        // Modifiers

        using access_class_type::erase_values;
        using access_class_type::insert_value;
        using access_class_type::insert_values;
        using access_class_type::resize_values;

        static constexpr size_type DATA_BUFFER_INDEX = 1;

        friend class run_end_encoded_array;
        friend base_type;
        friend base_type::base_type;
        friend base_type::base_type::base_type;
    };

    /********************************************************
     * primitive_array_impl implementation *
     ********************************************************/

    template <trivial_copyable_type T, typename Ext, trivial_copyable_type T2>
    primitive_array_impl<T, Ext, T2>::primitive_array_impl(arrow_proxy proxy_param)
        : base_type(std::move(proxy_param))
        , access_class_type(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
    {
    }

    template <trivial_copyable_type T, typename Ext, trivial_copyable_type T2>
    constexpr primitive_array_impl<T, Ext, T2>::primitive_array_impl(const primitive_array_impl& rhs)
        : base_type(rhs)
        , access_class_type(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
    {
    }

    template <trivial_copyable_type T, typename Ext, trivial_copyable_type T2>
    constexpr primitive_array_impl<T, Ext, T2>&
    primitive_array_impl<T, Ext, T2>::operator=(const primitive_array_impl& rhs)
    {
        base_type::operator=(rhs);
        access_class_type::reset_proxy(this->get_arrow_proxy());
        return *this;
    }

    template <trivial_copyable_type T, typename Ext, trivial_copyable_type T2>
    constexpr primitive_array_impl<T, Ext, T2>::primitive_array_impl(primitive_array_impl&& rhs) noexcept
        : base_type(std::move(rhs))
        , access_class_type(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
    {
    }

    template <trivial_copyable_type T, typename Ext, trivial_copyable_type T2>
    constexpr primitive_array_impl<T, Ext, T2>&
    primitive_array_impl<T, Ext, T2>::operator=(primitive_array_impl&& rhs) noexcept
    {
        base_type::operator=(std::move(rhs));
        access_class_type::reset_proxy(this->get_arrow_proxy());
        return *this;
    }

    template <trivial_copyable_type T, typename Ext, trivial_copyable_type T2>
    template <validity_bitmap_input VALIDITY_RANGE, input_metadata_container METADATA_RANGE>
    auto primitive_array_impl<T, Ext, T2>::create_proxy(
        u8_buffer<T2>&& data_buffer,
        size_t size,
        VALIDITY_RANGE&& bitmap_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        return create_proxy_impl(
            std::forward<u8_buffer<T2>>(data_buffer),
            size,
            ensure_validity_bitmap(size, std::forward<VALIDITY_RANGE>(bitmap_input)),
            std::move(name),
            std::move(metadata)
        );
    }

    template <trivial_copyable_type T, typename Ext, trivial_copyable_type T2>
    template <std::ranges::input_range VALUE_RANGE, validity_bitmap_input VALIDITY_RANGE, input_metadata_container METADATA_RANGE>
        requires(std::convertible_to<std::ranges::range_value_t<VALUE_RANGE>, T2>)
    arrow_proxy primitive_array_impl<T, Ext, T2>::create_proxy(
        VALUE_RANGE&& values,
        VALIDITY_RANGE&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        auto size = static_cast<size_t>(std::ranges::distance(values));
        u8_buffer<T2> data_buffer = details::primitive_data_access<T, T2>::make_data_buffer(
            std::forward<VALUE_RANGE>(values)
        );
        return create_proxy(
            std::move(data_buffer),
            size,
            std::forward<VALIDITY_RANGE>(validity_input),
            std::move(name),
            std::move(metadata)
        );
    }

    template <trivial_copyable_type T, typename Ext, trivial_copyable_type T2>
    template <class U, input_metadata_container METADATA_RANGE>
        requires std::convertible_to<U, T2>
    arrow_proxy primitive_array_impl<T, Ext, T2>::create_proxy(
        size_type n,
        const U& value,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        // create data_buffer
        u8_buffer<T2> data_buffer(n, value);
        return create_proxy_impl(
            std::move(data_buffer),
            n,
            nullable ? std::make_optional<validity_bitmap>(nullptr, 0) : std::nullopt,
            std::move(name),
            std::move(metadata)
        );
    }

    template <trivial_copyable_type T, typename Ext, trivial_copyable_type T2>
    template <validity_bitmap_input VALIDITY_RANGE, input_metadata_container METADATA_RANGE>
    arrow_proxy primitive_array_impl<T, Ext, T2>::create_proxy(
        u8_buffer<T2>&& data_buffer,
        size_t size,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        std::optional<validity_bitmap> bitmap = nullable ? std::make_optional<validity_bitmap>(nullptr, 0)
                                                         : std::nullopt;
        return create_proxy_impl(
            std::move(data_buffer),
            size,
            std::move(bitmap),
            std::move(name),
            std::move(metadata)
        );
    }

    template <trivial_copyable_type T, typename Ext, trivial_copyable_type T2>
    template <std::ranges::input_range R, input_metadata_container METADATA_RANGE>
        requires(std::convertible_to<std::ranges::range_value_t<R>, T2> && !mpl::is_type_instance_of_v<R, u8_buffer>)
    arrow_proxy primitive_array_impl<T, Ext, T2>::create_proxy(
        R&& range,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        auto data_buffer = details::primitive_data_access<T, T2>::make_data_buffer(std::forward<R>(range));
        auto distance = static_cast<size_t>(std::ranges::distance(range));
        std::optional<validity_bitmap> bitmap = nullable ? std::make_optional<validity_bitmap>(nullptr, 0)
                                                         : std::nullopt;
        return create_proxy_impl(
            std::move(data_buffer),
            distance,
            std::move(bitmap),
            std::move(name),
            std::move(metadata)
        );
    }

    // range of nullable values
    template <trivial_copyable_type T, typename Ext, trivial_copyable_type T2>
    template <std::ranges::input_range NULLABLE_RANGE, input_metadata_container METADATA_RANGE>
        requires std::is_same_v<std::ranges::range_value_t<NULLABLE_RANGE>, nullable<T2>>
    arrow_proxy primitive_array_impl<T, Ext, T2>::create_proxy(
        NULLABLE_RANGE&& nullable_range,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        // split into values and is_non_null ranges
        auto values = nullable_range
                      | std::views::transform(
                          [](const auto& v)
                          {
                              return v.get();
                          }
                      );
        auto is_non_null = nullable_range
                           | std::views::transform(
                               [](const auto& v)
                               {
                                   return v.has_value();
                               }
                           );
        return self_type::create_proxy(values, is_non_null, std::move(name), std::move(metadata));
    }

    template <trivial_copyable_type T, typename Ext, trivial_copyable_type T2>
    template <input_metadata_container METADATA_RANGE>
    [[nodiscard]] arrow_proxy primitive_array_impl<T, Ext, T2>::create_proxy_impl(
        u8_buffer<T2>&& data_buffer,
        size_t size,
        std::optional<validity_bitmap>&& bitmap,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const bool bitmap_has_value = bitmap.has_value();
        const auto null_count = bitmap_has_value ? bitmap->null_count() : 0;
        const auto flags = bitmap_has_value
                               ? std::make_optional<std::unordered_set<sparrow::ArrowFlag>>({ArrowFlag::NULLABLE})
                               : std::nullopt;

        // create arrow schema and array
        ArrowSchema schema = make_arrow_schema(
            data_type_to_format(detail::get_data_type_from_array<self_type>::get()),  // format
            std::move(name),                                                          // name
            std::move(metadata),                                                      // metadata
            flags,                                                                    // flags
            nullptr,                                                                  // children
            repeat_view<bool>(true, 0),                                               // children_ownership
            nullptr,                                                                  // dictionary
            true                                                                      // dictionary ownership
        );

        buffer<uint8_t> bitmap_buffer = bitmap_has_value ? std::move(*bitmap).extract_storage()
                                                         : buffer<uint8_t>{nullptr, 0};

        std::vector<buffer<uint8_t>> buffers(2);
        buffers[0] = std::move(bitmap_buffer);
        buffers[1] = std::move(data_buffer).extract_storage();

        // create arrow array
        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(buffers),
            nullptr,                     // children
            repeat_view<bool>(true, 0),  // children_ownership
            nullptr,                     // dictionary,
            true                         // dictionary ownership
        );
        arrow_proxy proxy(std::move(arr), std::move(schema));
        Ext::init(proxy);
        return proxy;
    }
}
