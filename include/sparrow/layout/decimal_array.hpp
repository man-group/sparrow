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

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/buffer/u8_buffer.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/decimal_reference.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/utils/decimal.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/metadata.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{
    template <decimal_type T>
    class decimal_array;

    using decimal_32_array = decimal_array<decimal<int32_t>>;
    using decimal_64_array = decimal_array<decimal<int64_t>>;
    using decimal_128_array = decimal_array<decimal<int128_t>>;
    using decimal_256_array = decimal_array<decimal<int256_t>>;

    namespace detail
    {
        template <>
        struct get_data_type_from_array<decimal_32_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::DECIMAL32;
            }
        };

        template <>
        struct get_data_type_from_array<decimal_64_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::DECIMAL64;
            }
        };

        template <>
        struct get_data_type_from_array<decimal_128_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::DECIMAL128;
            }
        };

        template <>
        struct get_data_type_from_array<decimal_256_array>
        {
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


    template <class T>
    constexpr bool is_decimal_array_v = mpl::is_type_instance_of_v<T, decimal_array>;

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

        explicit decimal_array(arrow_proxy);

        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<decimal_array<T>, Args...>)
        explicit decimal_array(Args&&... args)
            : decimal_array(create_proxy(std::forward<Args>(args)...))
        {
        }

        [[nodiscard]] constexpr inner_reference value(size_type i);
        [[nodiscard]] constexpr inner_const_reference value(size_type i) const;

    private:

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

        template <validity_bitmap_input R, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static auto create_proxy(
            u8_buffer<storage_type>&& data_buffer,
            R&& bitmaps,
            std::size_t precision,
            int scale,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static auto create_proxy(
            u8_buffer<storage_type>&& data_buffer,
            std::size_t precision,
            int scale,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static auto create_proxy_impl(
            u8_buffer<storage_type>&& data_buffer,
            std::size_t precision,
            int scale,
            std::optional<validity_bitmap>,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        static constexpr std::string generate_format(std::size_t precision, int scale);

        [[nodiscard]] constexpr value_iterator value_begin();
        [[nodiscard]] constexpr value_iterator value_end();

        [[nodiscard]] constexpr const_value_iterator value_cbegin() const;
        [[nodiscard]] constexpr const_value_iterator value_cend() const;

        constexpr void assign(const T& rhs, size_type index);

        // Modifiers

        static constexpr size_type DATA_BUFFER_INDEX = 1;
        friend base_type;
        friend base_type::base_type;
        friend base_type::base_type::base_type;
        friend class detail::layout_value_functor<self_type, inner_value_type>;
        friend class detail::layout_value_functor<const self_type, inner_value_type>;
        friend class decimal_reference<self_type>;

        std::size_t m_precision;  // The precision of the decimal value
        int m_scale;              // The scale of the decimal value (can be negative)
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
            nullable ? std::make_optional<validity_bitmap>(nullptr, size) : std::nullopt,
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
            nullable ? std::make_optional<validity_bitmap>(nullptr, size) : std::nullopt,
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
        buffers[0] = bitmap.has_value() ? std::move(*bitmap).extract_storage() : buffer<uint8_t>{nullptr, 0};
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
}
