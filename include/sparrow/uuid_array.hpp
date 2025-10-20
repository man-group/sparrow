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

#include <array>
#include <cstddef>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#if defined(__cpp_lib_format)
#    include <format>
#    include <ostream>
#endif

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_flag_utils.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/fixed_width_binary_array.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/repeat_container.hpp"

namespace sparrow
{
    /**
     * @brief UUID array implementation following Arrow canonical extension specification.
     *
     * This class implements an Arrow-compatible array for storing UUID values according to
     * the Apache Arrow canonical extension specification for UUIDs. Each UUID is stored as
     * a 16-byte (128-bit) fixed-width binary value.
     *
     * The UUID extension type is defined as:
     * - Extension name: "arrow.uuid"
     * - Storage type: FixedSizeBinary(16)
     * - Extension metadata: none
     *
     * Related Apache Arrow specification:
     * https://arrow.apache.org/docs/format/CanonicalExtensions.html#uuid
     */
    class uuid_array final : public fixed_width_binary_array
    {
    public:

        /**
         * @brief UUID size in bytes (128 bits = 16 bytes).
         */
        static constexpr size_t UUID_SIZE = 16;

        /**
         * @brief Type representing a UUID value as a 16-byte array.
         */
        using uuid_type = std::array<byte_t, UUID_SIZE>;

        using base_type = fixed_width_binary_array;

        /**
         * @brief Extension name for UUID arrays in Arrow format.
         */
        static constexpr std::string_view EXTENSION_NAME = "arrow.uuid";

        /**
         * @brief Constructs UUID array from Arrow proxy.
         *
         * @param proxy Arrow proxy containing UUID array data and schema
         *
         * @pre proxy must contain valid Arrow FixedSizeBinary(16) array
         * @pre proxy format must be "w:16" (16-byte fixed-width binary)
         * @pre If extension type, extension name should be "arrow.uuid"
         * @post Array is initialized with data from proxy
         * @post Element size is validated to be 16 bytes
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(element_size == UUID_SIZE)
         */
        explicit uuid_array(arrow_proxy proxy);

        /**
         * @brief Creates empty UUID array.
         *
         * @tparam METADATA_RANGE Type of metadata container
         * @param nullable Whether the array should support null values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         *
         * @post Returns empty UUID array
         * @post If nullable is true, array supports null values
         * @post Extension name is "arrow.uuid"
         * @post Storage format is "w:16"
         */
        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        explicit uuid_array(
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates UUID array from range of UUID value ranges with validity bitmap.
         *
         * @tparam VALUES Type of input range containing UUID value ranges
         * @tparam VB Type of validity bitmap input
         * @tparam METADATA_RANGE Type of metadata container
         * @param values Range of ranges containing UUID data (each must be 16 bytes)
         * @param validity_input Validity bitmap specification
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         *
         * @pre VALUES must be a range of ranges of char-like elements
         * @pre All inner ranges must have exactly UUID_SIZE (16) bytes
         * @pre validity_input size must match values.size()
         * @post Returns valid UUID array
         * @post Extension name is "arrow.uuid"
         */
        template <
            std::ranges::input_range VALUES,
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(
                std::ranges::input_range<std::ranges::range_value_t<VALUES>>
                && mpl::char_like<std::ranges::range_value_t<std::ranges::range_value_t<VALUES>>>
            )
        uuid_array(
            VALUES&& values,
            VB&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates UUID array from range of UUID value ranges with nullable flag.
         *
         * @tparam VALUES Type of input range containing UUID value ranges
         * @tparam METADATA_RANGE Type of metadata container
         * @param values Range of ranges containing UUID data (each must be 16 bytes)
         * @param nullable Whether the array should support null values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         *
         * @pre VALUES must be a range of ranges of char-like elements
         * @pre All inner ranges must have exactly UUID_SIZE (16) bytes
         * @post If nullable is true, array supports null values (though none initially set)
         * @post If nullable is false, array does not support null values
         * @post Extension name is "arrow.uuid"
         */
        template <std::ranges::input_range VALUES, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(
                std::ranges::input_range<std::ranges::range_value_t<VALUES>>
                && mpl::char_like<std::ranges::range_value_t<std::ranges::range_value_t<VALUES>>>
            )
        uuid_array(
            VALUES&& values,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        /**
         * @brief Creates UUID array from range of nullable UUID values.
         *
         * @tparam NULLABLE_VALUES Type of input range containing nullable UUID values
         * @tparam METADATA_RANGE Type of metadata container
         * @param range Range of nullable UUID values (inner values must be 16 bytes)
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         *
         * @pre NULLABLE_VALUES must be a range of nullable<uuid_range>
         * @pre All non-null inner ranges must have exactly UUID_SIZE (16) bytes
         * @pre Inner ranges must contain byte_t elements
         * @post Returns valid UUID array
         * @post Validity bitmap reflects has_value() status of nullable elements
         * @post Array supports null values (nullable = true)
         * @post Extension name is "arrow.uuid"
         */
        template <std::ranges::input_range NULLABLE_VALUES, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires mpl::is_type_instance_of_v<std::ranges::range_value_t<NULLABLE_VALUES>, nullable>
                     && std::ranges::input_range<typename std::ranges::range_value_t<NULLABLE_VALUES>::value_type>
                     && std::is_same_v<
                         std::ranges::range_value_t<typename std::ranges::range_value_t<NULLABLE_VALUES>::value_type>,
                         byte_t>
        uuid_array(
            NULLABLE_VALUES&& range,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

    private:

        /**
         * @brief Adds UUID extension metadata to Arrow schema.
         *
         * @param proxy Arrow proxy to modify
         * @return Modified arrow proxy with UUID extension metadata
         *
         * @post Extension metadata includes "ARROW:extension:name" = "arrow.uuid"
         * @post Extension metadata includes "ARROW:extension:metadata" = ""
         */
        static arrow_proxy add_uuid_extension_metadata(arrow_proxy&& proxy);
    };

    namespace detail
    {
        template <>
        struct get_data_type_from_array<sparrow::uuid_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::FIXED_WIDTH_BINARY;
            }
        };
    }

    /********************************
     * uuid_array implementation    *
     ********************************/

    inline uuid_array::uuid_array(arrow_proxy proxy)
        : base_type(add_uuid_extension_metadata(std::move(proxy)))
    {
        const size_t element_size = num_bytes_for_fixed_sized_binary(this->get_arrow_proxy().format());
        SPARROW_ASSERT_TRUE(element_size == UUID_SIZE);
    }

    template <input_metadata_container METADATA_RANGE>
    uuid_array::uuid_array(bool nullable, std::optional<std::string_view> name, std::optional<METADATA_RANGE> metadata)
        : base_type(add_uuid_extension_metadata(
              base_type::create_proxy(UUID_SIZE, nullable, std::move(name), std::move(metadata))
          ))
    {
    }

    template <std::ranges::input_range VALUES, validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
        requires(
            std::ranges::input_range<std::ranges::range_value_t<VALUES>>
            && mpl::char_like<std::ranges::range_value_t<std::ranges::range_value_t<VALUES>>>
        )
    uuid_array::uuid_array(
        VALUES&& values,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
        : base_type(add_uuid_extension_metadata(
              base_type::create_proxy(
                  std::forward<VALUES>(values),
                  std::forward<VB>(validity_input),
                  std::move(name),
                  std::move(metadata)
              )
          ))
    {
        SPARROW_ASSERT_TRUE(!values.empty());
        SPARROW_ASSERT_TRUE(values.begin()->size() == UUID_SIZE);  // only need to check the first element as
                                                                   // the base type already ensures uniform
                                                                   // size
    }

    template <std::ranges::input_range VALUES, input_metadata_container METADATA_RANGE>
        requires(
            std::ranges::input_range<std::ranges::range_value_t<VALUES>>
            && mpl::char_like<std::ranges::range_value_t<std::ranges::range_value_t<VALUES>>>
        )
    uuid_array::uuid_array(
        VALUES&& values,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
        : base_type(add_uuid_extension_metadata(
              base_type::create_proxy(std::forward<VALUES>(values), nullable, std::move(name), std::move(metadata))
          ))
    {
        SPARROW_ASSERT_TRUE(!values.empty());
        SPARROW_ASSERT_TRUE(values.begin()->size() == UUID_SIZE);  // only need to check the first element as
                                                                   // the base type already ensures uniform
                                                                   // size
    }

    template <std::ranges::input_range NULLABLE_VALUES, input_metadata_container METADATA_RANGE>
        requires mpl::is_type_instance_of_v<std::ranges::range_value_t<NULLABLE_VALUES>, nullable>
                 && std::ranges::input_range<typename std::ranges::range_value_t<NULLABLE_VALUES>::value_type>
                 && std::is_same_v<
                     std::ranges::range_value_t<typename std::ranges::range_value_t<NULLABLE_VALUES>::value_type>,
                     byte_t>
    uuid_array::uuid_array(
        NULLABLE_VALUES&& range,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
        : base_type(add_uuid_extension_metadata(
              base_type::create_proxy(std::forward<NULLABLE_VALUES>(range), std::move(name), std::move(metadata))
          ))
    {
        // Validate non-null UUIDs
        for (const auto& nullable_uuid : range)
        {
            if (nullable_uuid.has_value())
            {
                SPARROW_ASSERT_TRUE(std::ranges::size(nullable_uuid.get()) == UUID_SIZE);
                break;  // only need to check the first non-null UUID as the base type already ensures uniform
                        // size
            }
        }
    }

    inline arrow_proxy uuid_array::add_uuid_extension_metadata(arrow_proxy&& proxy)
    {
        // Add UUID extension metadata
        std::vector<metadata_pair> extension_metadata;
        extension_metadata.emplace_back("ARROW:extension:name", std::string(EXTENSION_NAME));
        extension_metadata.emplace_back("ARROW:extension:metadata", "");

        // Get flags from existing schema
        const auto& schema = proxy.schema();
        std::optional<std::unordered_set<ArrowFlag>> flags;
        if (schema.flags != 0)
        {
            flags = to_set_of_ArrowFlags(schema.flags);
        }

        // Create new schema with UUID extension metadata
        ArrowSchema new_schema = make_arrow_schema(
            std::string(schema.format),
            schema.name ? std::make_optional<std::string_view>(schema.name) : std::nullopt,
            std::make_optional(std::move(extension_metadata)),
            flags,
            nullptr,                     // children
            repeat_view<bool>(true, 0),  // children_ownership
            nullptr,                     // dictionary
            true                         // dictionary_ownership
        );

        // Extract array from proxy and create new proxy
        ArrowArray arr = std::move(proxy).extract_array();

        return arrow_proxy{std::move(arr), std::move(new_schema)};
    }
}

#if defined(__cpp_lib_format)

template <>
struct std::formatter<sparrow::uuid_array>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::uuid_array& arr, std::format_context& ctx) const
    {
        std::format_to(ctx.out(), "uuid_array[{}](", arr.size());

        if (arr.size() > 0)
        {
            bool first = true;
            for (std::size_t i = 0; i < arr.size(); ++i)
            {
                if (!first)
                {
                    std::format_to(ctx.out(), ", ");
                }
                first = false;

                auto element = arr[i];
                if (!element.has_value())
                {
                    std::format_to(ctx.out(), "null");
                }
                else
                {
                    auto uuid_ref = element.get();
                    std::format_to(ctx.out(), "<");
                    for (std::size_t j = 0; j < uuid_ref.size(); ++j)
                    {
                        if (j > 0)
                        {
                            std::format_to(ctx.out(), " ");
                        }
                        std::format_to(ctx.out(), "{:02x}", static_cast<unsigned char>(uuid_ref[j]));
                    }
                    std::format_to(ctx.out(), ">");
                }
            }
        }

        return std::format_to(ctx.out(), ")");
    }
};

namespace sparrow
{
    inline std::ostream& operator<<(std::ostream& os, const uuid_array& arr)
    {
        os << std::format("{}", arr);
        return os;
    }
}

#endif
