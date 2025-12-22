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

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/decimal_reference.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/u8_buffer.hpp"
#include "sparrow/utils/decimal.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/metadata.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{
    /**
     * Array implementation for decimal types.
     *
     * @tparam T The decimal type (e.g., decimal<int32_t>, decimal<int64_t>, etc.).
     */
    template <decimal_type T>
    class decimal_array;

    /** Type alias for 32-bit decimal array. */
    using decimal_32_array = decimal_array<decimal<int32_t>>;
    /** Type alias for 64-bit decimal array. */
    using decimal_64_array = decimal_array<decimal<int64_t>>;
    /** Type alias for 128-bit decimal array. */
    using decimal_128_array = decimal_array<decimal<int128_t>>;
    /** Type alias for 256-bit decimal array. */
    using decimal_256_array = decimal_array<decimal<int256_t>>;

    namespace detail
    {
        template <>
        struct get_data_type_from_array<decimal_32_array>
        {
            /**
             * Gets the data type for 32-bit decimal.
             *
             * @return The DECIMAL32 data type.
             */
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::DECIMAL32;
            }
        };

        /** Specialization for 64-bit decimal array. */
        template <>
        struct get_data_type_from_array<decimal_64_array>
        {
            /**
             * Gets the data type for 64-bit decimal.
             *
             * @return The DECIMAL64 data type.
             */
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::DECIMAL64;
            }
        };

        /** Specialization for 128-bit decimal array. */
        template <>
        struct get_data_type_from_array<decimal_128_array>
        {
            /**
             * Gets the data type for 128-bit decimal.
             *
             * @return The DECIMAL128 data type.
             */
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::DECIMAL128;
            }
        };

        /** Specialization for 256-bit decimal array. */
        template <>
        struct get_data_type_from_array<decimal_256_array>
        {
            /**
             * Gets the data type for 256-bit decimal.
             *
             * @return The DECIMAL256 data type.
             */
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::DECIMAL256;
            }
        };

    }

    template <decimal_type T>
    struct array_inner_types<decimal_array<T>> : array_inner_types_base
    {
        using array_type = decimal_array<T>;

        using inner_value_type = T;
        using inner_reference = decimal_reference<array_type>;
        using inner_const_reference = T;

        using bitmap_const_reference = bitmap_type::const_reference;

        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;

        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_reference>>;
        using const_value_iterator = functor_index_iterator<
            detail::layout_value_functor<const array_type, inner_value_type>>;
        using iterator_tag = std::random_access_iterator_tag;
    };


    /**
     * Type trait to check if a type is a decimal array.
     *
     * @tparam T The type to check.
     */
    template <class T>
    constexpr bool is_decimal_array_v = mpl::is_type_instance_of_v<T, decimal_array>;

    /**
     * Array implementation for decimal types with fixed precision and scale.
     *
     * This class provides a container for decimal values with a specified precision
     * and scale, stored as integer values with an associated scaling factor.
     *
     * @tparam T The decimal type, must satisfy the decimal_type concept.
     */
    template <decimal_type T>
    class decimal_array final : public mutable_array_bitmap_base<decimal_array<T>>
    {
    public:

        using self_type = decimal_array<T>;
        using base_type = mutable_array_bitmap_base<self_type>;

        using inner_types = array_inner_types<self_type>;
        using inner_value_type = typename inner_types::inner_value_type;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;

        // the integral value type used to store the bits
        using storage_type = typename T::integer_type;
        static_assert(
            sizeof(storage_type) == 4 || sizeof(storage_type) == 8 || sizeof(storage_type) == 16
                || sizeof(storage_type) == 32,
            "The storage type must be an integral type of size  4, 8, 16 or 32 bytes"
        );

        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;
        using const_bitmap_iterator = typename base_type::const_bitmap_iterator;
        using const_bitmap_range = typename base_type::const_bitmap_range;

        using value_type = nullable<inner_value_type>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;

        using size_type = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using iterator_tag = typename base_type::iterator_tag;

        using value_iterator = typename inner_types::value_iterator;
        using const_value_iterator = typename inner_types::const_value_iterator;

        /**
         * Constructs a decimal array from an arrow proxy.
         *
         * @param proxy The arrow proxy containing the array data and schema.
         */
        explicit decimal_array(arrow_proxy proxy);

        /**
         * Constructs a decimal array with the given arguments.
         *
         * @tparam Args The argument types.
         * @param args Arguments forwarded to create_proxy.
         */
        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<decimal_array<T>, Args...>)
        explicit decimal_array(Args&&... args)
            : decimal_array(create_proxy(std::forward<Args>(args)...))
        {
        }

        /**
         * Gets a mutable reference to the value at the specified index.
         *
         * @param i The index of the element.
         * @return Mutable reference to the decimal value.
         */
        [[nodiscard]] constexpr inner_reference value(size_type i);

        /**
         * Gets a constant reference to the value at the specified index.
         *
         * @param i The index of the element.
         * @return Constant reference to the decimal value.
         */
        [[nodiscard]] constexpr inner_const_reference value(size_type i) const;

    private:

        /**
         * Creates an arrow proxy from a value range and validity bitmap.
         *
         * @tparam VALUE_RANGE The value range type.
         * @tparam VALIDITY_RANGE The validity bitmap type.
         * @tparam METADATA_RANGE The metadata container type.
         * @param range The range of values to store.
         * @param bitmaps The validity bitmap.
         * @param precision The precision of the decimal values.
         * @param scale The scale of the decimal values.
         * @param name Optional name for the array.
         * @param metadata Optional metadata for the array.
         * @return An arrow proxy containing the decimal array data.
         */
        template <
            std::ranges::input_range VALUE_RANGE,
            validity_bitmap_input VALIDITY_RANGE,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<std::ranges::range_value_t<VALUE_RANGE>, typename T::integer_type>
        [[nodiscard]] static auto create_proxy(
            VALUE_RANGE&& range,
            VALIDITY_RANGE&& bitmaps,
            std::size_t precision,
            int scale,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        /**
         * Creates an arrow proxy from a range of nullable values.
         *
         * @tparam NULLABLE_VALUE_RANGE The nullable value range type.
         * @tparam METADATA_RANGE The metadata container type.
         * @param range The range of nullable values to store.
         * @param precision The precision of the decimal values.
         * @param scale The scale of the decimal values.
         * @param name Optional name for the array.
         * @param metadata Optional metadata for the array.
         * @return An arrow proxy containing the decimal array data.
         */
        template <
            std::ranges::input_range NULLABLE_VALUE_RANGE,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::is_same_v<std::ranges::range_value_t<NULLABLE_VALUE_RANGE>, nullable<typename T::integer_type>>
        [[nodiscard]] static auto create_proxy(
            NULLABLE_VALUE_RANGE&& range,
            std::size_t precision,
            int scale,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        /**
         * Creates an arrow proxy from a value range.
         *
         * @tparam VALUE_RANGE The value range type.
         * @tparam METADATA_RANGE The metadata container type.
         * @param range The range of values to store.
         * @param precision The precision of the decimal values.
         * @param scale The scale of the decimal values.
         * @param nullable Whether the array can contain null values.
         * @param name Optional name for the array.
         * @param metadata Optional metadata for the array.
         * @return An arrow proxy containing the decimal array data.
         */
        template <std::ranges::input_range VALUE_RANGE, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::is_same_v<std::ranges::range_value_t<VALUE_RANGE>, typename T::integer_type>
        [[nodiscard]] static auto create_proxy(
            VALUE_RANGE&& range,
            std::size_t precision,
            int scale,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        /**
         * Creates an arrow proxy from a data buffer and validity bitmap.
         *
         * @tparam R The validity bitmap input type.
         * @tparam METADATA_RANGE The metadata container type.
         * @param data_buffer The buffer containing the decimal storage values.
         * @param bitmaps The validity bitmap.
         * @param precision The precision of the decimal values.
         * @param scale The scale of the decimal values.
         * @param name Optional name for the array.
         * @param metadata Optional metadata for the array.
         * @return An arrow proxy containing the decimal array data.
         */
        template <validity_bitmap_input R, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static auto create_proxy(
            u8_buffer<storage_type>&& data_buffer,
            R&& bitmaps,
            std::size_t precision,
            int scale,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        /**
         * Creates an arrow proxy from a data buffer.
         *
         * @tparam METADATA_RANGE The metadata container type.
         * @param data_buffer The buffer containing the decimal storage values.
         * @param precision The precision of the decimal values.
         * @param scale The scale of the decimal values.
         * @param nullable Whether the array can contain null values.
         * @param name Optional name for the array.
         * @param metadata Optional metadata for the array.
         * @return An arrow proxy containing the decimal array data.
         */
        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static auto create_proxy(
            u8_buffer<storage_type>&& data_buffer,
            std::size_t precision,
            int scale,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        /**
         * Internal implementation for creating an arrow proxy.
         *
         * @tparam METADATA_RANGE The metadata container type.
         * @param data_buffer The buffer containing the decimal storage values.
         * @param precision The precision of the decimal values.
         * @param scale The scale of the decimal values.
         * @param bitmap Optional validity bitmap.
         * @param name Optional name for the array.
         * @param metadata Optional metadata for the array.
         * @return An arrow proxy containing the decimal array data.
         */
        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static auto create_proxy_impl(
            u8_buffer<storage_type>&& data_buffer,
            std::size_t precision,
            int scale,
            std::optional<validity_bitmap> bitmap,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        /**
         * Generates the format string for the decimal type.
         *
         * @param precision The precision of the decimal values.
         * @param scale The scale of the decimal values.
         * @return The format string.
         */
        static constexpr std::string generate_format(std::size_t precision, int scale);

        /**
         * Gets an iterator to the beginning of the values.
         *
         * @return Iterator to the beginning of values.
         */
        [[nodiscard]] constexpr value_iterator value_begin();

        /**
         * Gets an iterator to the end of the values.
         *
         * @return Iterator to the end of values.
         */
        [[nodiscard]] constexpr value_iterator value_end();

        /**
         * Gets a constant iterator to the beginning of the values.
         *
         * @return Constant iterator to the beginning of values.
         */
        [[nodiscard]] constexpr const_value_iterator value_cbegin() const;

        /**
         * Gets a constant iterator to the end of the values.
         *
         * @return Constant iterator to the end of values.
         */
        [[nodiscard]] constexpr const_value_iterator value_cend() const;

        /**
         * Assigns a decimal value to the specified index.
         *
         * @param rhs The decimal value to assign.
         * @param index The index where to assign the value.
         */
        constexpr void assign(const T& rhs, size_type index);

        // Modifiers

        /**
         * Resizes the value buffer to the specified length.
         *
         * @param new_length The new length of the value buffer.
         * @param value The value to use for new elements.
         */
        constexpr void resize_values(size_t new_length, const inner_value_type& value);

        /**
         * Inserts a single value multiple times at the specified position.
         *
         * @param pos The position where to insert.
         * @param value The value to insert.
         * @param count The number of times to insert the value.
         * @return Iterator to the first inserted element.
         */
        constexpr value_iterator insert_value(const_value_iterator pos, inner_value_type value, size_t count);

        /**
         * Inserts a range of values at the specified position.
         *
         * @tparam InputIt Input iterator type.
         * @param pos The position where to insert.
         * @param first Iterator to the beginning of the range.
         * @param last Iterator to the end of the range.
         * @return Iterator to the first inserted element.
         */
        template <std::input_iterator InputIt>
            requires std::convertible_to<
                typename std::iterator_traits<InputIt>::value_type,
                typename decimal_array<T>::inner_value_type>
        constexpr value_iterator insert_values(const_value_iterator pos, InputIt first, InputIt last);

        /**
         * Erases values starting at the specified position.
         *
         * @param pos The position where to start erasing.
         * @param count The number of values to erase.
         * @return Iterator to the element following the erased range.
         */
        constexpr value_iterator erase_values(const_value_iterator pos, size_t count);

        /**
         * Gets a buffer adaptor for the data buffer.
         *
         * @return A buffer adaptor that provides access to the data buffer.
         */
        [[nodiscard]] constexpr auto get_data_buffer()
        {
            auto& buffers = this->get_arrow_proxy().get_array_private_data()->buffers();
            return make_buffer_adaptor<storage_type>(buffers[DATA_BUFFER_INDEX]);
        }

        /** Index of the data buffer in the Arrow array buffers. */
        static constexpr size_type DATA_BUFFER_INDEX = 1;
        friend base_type;
        friend base_type::base_type;
        friend base_type::base_type::base_type;
        friend class detail::layout_value_functor<self_type, inner_value_type>;
        friend class detail::layout_value_functor<const self_type, inner_value_type>;
        friend class decimal_reference<self_type>;

        /** The precision of the decimal values (total number of digits). */
        std::size_t m_precision;
        /** The scale of the decimal values (number of digits after decimal point, can be negative). */
        int m_scale;
    };

    /**********************************
     * decimal_array implementation *
     **********************************/

    template <decimal_type T>
    decimal_array<T>::decimal_array(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , m_precision(0)
        , m_scale(0)
    {
        // parse the format string
        const auto format = this->get_arrow_proxy().format();

        // ensure that the format string starts with d:
        if (format.size() < 2 || format[0] != 'd' || format[1] != ':')
        {
            throw std::runtime_error("Invalid format string for decimal array");
        }

        // substring staring aftet d:
        const auto format_str = format.substr(2);

        std::stringstream ss;
        ss << format_str;
        char c = 0;
        ss >> m_precision >> c >> m_scale;

        // check for failure
        if (ss.fail())
        {
            throw std::runtime_error("Invalid format string for decimal array");
        }
    }

    template <decimal_type T>
    template <std::ranges::input_range VALUE_RANGE, validity_bitmap_input VALIDITY_RANGE, input_metadata_container METADATA_RANGE>
        requires std::convertible_to<std::ranges::range_value_t<VALUE_RANGE>, typename T::integer_type>
    arrow_proxy decimal_array<T>::create_proxy(
        VALUE_RANGE&& range,
        VALIDITY_RANGE&& bitmaps,
        std::size_t precision,
        int scale,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        u8_buffer<storage_type> u8_data_buffer(std::forward<VALUE_RANGE>(range));
        const auto size = u8_data_buffer.size();
        validity_bitmap bitmap = ensure_validity_bitmap(size, std::forward<VALIDITY_RANGE>(bitmaps));
        return create_proxy_impl(
            std::move(u8_data_buffer),
            precision,
            scale,
            std::move(bitmap),
            std::move(name),
            std::move(metadata)
        );
    }

    template <decimal_type T>
    template <std::ranges::input_range NULLABLE_VALUE_RANGE, input_metadata_container METADATA_RANGE>
        requires std::is_same_v<std::ranges::range_value_t<NULLABLE_VALUE_RANGE>, nullable<typename T::integer_type>>
    arrow_proxy decimal_array<T>::create_proxy(
        NULLABLE_VALUE_RANGE&& range,
        std::size_t precision,
        int scale,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
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
        return create_proxy(values, is_non_null, precision, scale, std::move(name), std::move(metadata));
    }

    template <decimal_type T>
    template <input_metadata_container METADATA_RANGE>
    auto decimal_array<T>::create_proxy(
        u8_buffer<storage_type>&& data_buffer,
        std::size_t precision,
        int scale,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        const size_t size = data_buffer.size();
        return create_proxy_impl(
            std::move(data_buffer),
            precision,
            scale,
            nullable ? std::make_optional<validity_bitmap>(nullptr, size, validity_bitmap::default_allocator())
                     : std::nullopt,
            name,
            metadata
        );
    }

    template <decimal_type T>
    template <std::ranges::input_range VALUE_RANGE, input_metadata_container METADATA_RANGE>
        requires std::is_same_v<std::ranges::range_value_t<VALUE_RANGE>, typename T::integer_type>
    arrow_proxy decimal_array<T>::create_proxy(
        VALUE_RANGE&& range,
        std::size_t precision,
        int scale,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        u8_buffer<storage_type> u8_data_buffer(std::forward<VALUE_RANGE>(range));
        const auto size = u8_data_buffer.size();
        return create_proxy_impl(
            std::move(u8_data_buffer),
            precision,
            scale,
            nullable ? std::make_optional<validity_bitmap>(nullptr, size, validity_bitmap::default_allocator())
                     : std::nullopt,
            name,
            metadata
        );
    }

    template <decimal_type T>
    template <validity_bitmap_input R, input_metadata_container METADATA_RANGE>
    auto decimal_array<T>::create_proxy(
        u8_buffer<storage_type>&& data_buffer,
        R&& bitmap_input,
        std::size_t precision,
        int scale,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        const auto size = data_buffer.size();
        validity_bitmap bitmap = ensure_validity_bitmap(size, std::forward<R>(bitmap_input));
        return create_proxy_impl(
            std::move(data_buffer),
            precision,
            scale,
            std::move(bitmap),
            std::move(name),
            std::move(metadata)
        );
    }

    template <decimal_type T>
    template <input_metadata_container METADATA_RANGE>
    [[nodiscard]] auto decimal_array<T>::create_proxy_impl(
        u8_buffer<storage_type>&& data_buffer,
        std::size_t precision,
        int scale,
        std::optional<validity_bitmap> bitmap,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        const std::optional<std::unordered_set<sparrow::ArrowFlag>>
            flags = bitmap.has_value()
                        ? std::make_optional<std::unordered_set<sparrow::ArrowFlag>>({ArrowFlag::NULLABLE})
                        : std::nullopt;
        static const repeat_view<bool> children_ownership{true, 0};
        const auto size = data_buffer.size();
        const size_t null_count = bitmap.has_value() ? bitmap->null_count() : 0;

        // create arrow schema and array
        ArrowSchema schema = make_arrow_schema(
            generate_format(precision, scale),
            name,      // name
            metadata,  // metadata
            flags,     // flags
            nullptr,   // children
            children_ownership,
            nullptr,  // dictionary
            true      // dictionary ownership
        );

        std::vector<buffer<uint8_t>> buffers(2);
        buffers[0] = bitmap.has_value() ? std::move(*bitmap).extract_storage()
                                        : buffer<uint8_t>{nullptr, 0, buffer<uint8_t>::default_allocator()};
        buffers[1] = std::move(data_buffer).extract_storage();

        // create arrow array
        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // lengths
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(buffers),
            nullptr,                     // children
            repeat_view<bool>(true, 0),  // children_ownership
            nullptr,                     // dictionary
            true
        );
        return arrow_proxy(std::move(arr), std::move(schema));
    }

    template <decimal_type T>
    constexpr auto decimal_array<T>::value(size_type i) -> inner_reference
    {
        SPARROW_ASSERT_TRUE(i < this->size());
        return inner_reference(this, i);
    }

    template <decimal_type T>
    constexpr auto decimal_array<T>::value(size_type i) const -> inner_const_reference
    {
        SPARROW_ASSERT_TRUE(i < this->size());
        const auto ptr = this->get_arrow_proxy().buffers()[DATA_BUFFER_INDEX].template data<const storage_type>();
        return inner_const_reference(ptr[i], m_scale);
    }

    template <decimal_type T>
    constexpr auto decimal_array<T>::value_begin() -> value_iterator
    {
        return value_iterator(detail::layout_value_functor<self_type, inner_reference>(this), 0);
    }

    template <decimal_type T>
    constexpr auto decimal_array<T>::value_end() -> value_iterator
    {
        return value_iterator(detail::layout_value_functor<self_type, inner_reference>(this), this->size());
    }

    template <decimal_type T>
    constexpr auto decimal_array<T>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator(detail::layout_value_functor<const self_type, inner_value_type>(this), 0);
    }

    template <decimal_type T>
    constexpr auto decimal_array<T>::value_cend() const -> const_value_iterator
    {
        return const_value_iterator(
            detail::layout_value_functor<const self_type, inner_value_type>(this),
            this->size()
        );
    }

    template <decimal_type T>
    constexpr void decimal_array<T>::assign(const T& rhs, size_type index)
    {
        SPARROW_ASSERT_TRUE(index < this->size());
        const auto ptr = this->get_arrow_proxy().buffers()[DATA_BUFFER_INDEX].template data<storage_type>();
        const auto storage = rhs.storage();
        // Scale the storage value to match the scale of the decimal type
        const auto scaled_storage = storage
                                    * static_cast<storage_type>(
                                        static_cast<size_t>(std::pow(10, m_scale - rhs.scale()))
                                    );
        ptr[index] = scaled_storage;
    }

    template <decimal_type T>
    constexpr std::string decimal_array<T>::generate_format(std::size_t precision, int scale)
    {
        constexpr std::size_t sizeof_decimal = sizeof(storage_type);
        std::string format_str = "d:" + std::to_string(precision) + "," + std::to_string(scale);
        if constexpr (sizeof_decimal != 16)  // We don't need to specify the size for 128-bit
                                             // decimals
        {
            format_str += "," + std::to_string(sizeof_decimal * 8);
        }
        return format_str;
    }

    template <decimal_type T>
    constexpr void decimal_array<T>::resize_values(size_t new_length, const inner_value_type& value)
    {
        const size_t offset = static_cast<size_t>(this->get_arrow_proxy().offset());
        const size_t new_size = new_length + offset;
        auto data_buffer = get_data_buffer();
        data_buffer.resize(new_size, value.storage());
    }

    template <decimal_type T>
    constexpr auto

    decimal_array<T>::insert_value(const_value_iterator pos, inner_value_type value, size_t count)
        -> value_iterator
    {
        SPARROW_ASSERT_TRUE(value_cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= value_cend());
        const auto distance = std::distance(value_cbegin(), pos);
        const auto offset = static_cast<difference_type>(this->get_arrow_proxy().offset());
        auto data_buffer = get_data_buffer();
        const auto insertion_pos = data_buffer.cbegin() + distance + offset;
        data_buffer.insert(insertion_pos, count, value.storage());
        return value_iterator(
            detail::layout_value_functor<self_type, inner_reference>(this),
            static_cast<size_type>(distance)
        );
    }

    template <decimal_type T>
    template <std::input_iterator InputIt>
        requires std::convertible_to<typename std::iterator_traits<InputIt>::value_type, typename decimal_array<T>::inner_value_type>
    constexpr auto decimal_array<T>::insert_values(const_value_iterator pos, InputIt first, InputIt last)
        -> value_iterator
    {
        SPARROW_ASSERT_TRUE(value_cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos <= value_cend());
        const auto distance = std::distance(value_cbegin(), pos);
        const auto offset = static_cast<difference_type>(this->get_arrow_proxy().offset());
        auto data_buffer = get_data_buffer();
        auto value_range = std::ranges::subrange(first, last);
        auto storage_view = std::ranges::transform_view(
            value_range,
            [](const auto& v)
            {
                return v.storage();
            }
        );
        const auto insertion_pos = data_buffer.cbegin() + distance + offset;
        data_buffer.insert(insertion_pos, storage_view.begin(), storage_view.end());
        return value_iterator(
            detail::layout_value_functor<self_type, inner_reference>(this),
            static_cast<size_type>(distance)
        );
    }

    template <decimal_type T>
    constexpr auto decimal_array<T>::erase_values(const_value_iterator pos, size_t count) -> value_iterator
    {
        SPARROW_ASSERT_TRUE(value_cbegin() <= pos);
        SPARROW_ASSERT_TRUE(pos < value_cend());
        const auto distance = std::distance(value_cbegin(), pos);
        const auto offset = static_cast<difference_type>(this->get_arrow_proxy().offset());
        auto data_buffer = get_data_buffer();
        const auto erase_begin = data_buffer.cbegin() + distance + offset;
        const auto erase_end = erase_begin + static_cast<difference_type>(count);
        data_buffer.erase(erase_begin, erase_end);
        return value_iterator(
            detail::layout_value_functor<self_type, inner_reference>(this),
            static_cast<size_type>(distance)
        );
    }
}
