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

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

#include "sparrow/fixed_width_binary_array.hpp"
#include "sparrow/utils/contracts.hpp"

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
    struct uuid_extension
    {
    public:

        static constexpr size_t UUID_SIZE = 16;
        static constexpr std::string_view EXTENSION_NAME = "arrow.uuid";

    protected:

        static void init(arrow_proxy& proxy)
        {
            const size_t element_size = num_bytes_for_fixed_sized_binary(proxy.format());
            SPARROW_ASSERT_TRUE(element_size == UUID_SIZE);
            // Add UUID extension metadata
            std::optional<key_value_view> metadata = proxy.metadata();
            std::vector<metadata_pair> extension_metadata = metadata.has_value()
                                                                ? std::vector<metadata_pair>(
                                                                      metadata->begin(),
                                                                      metadata->end()
                                                                  )
                                                                : std::vector<metadata_pair>{};

            // Check if extension metadata already exists
            const bool has_extension_name = std::ranges::find_if(
                                                extension_metadata,
                                                [](const auto& pair)
                                                {
                                                    return pair.first == "ARROW:extension:name"
                                                           && pair.second == EXTENSION_NAME;
                                                }
                                            )
                                            != extension_metadata.end();
            if (!has_extension_name)
            {
                extension_metadata.emplace_back("ARROW:extension:name", EXTENSION_NAME);
                extension_metadata.emplace_back("ARROW:extension:metadata", "");
            }
            proxy.set_metadata(std::make_optional(extension_metadata));
        }
    };

    using uuid_array = fixed_width_binary_array_impl<
        fixed_width_binary_traits::value_type,
        fixed_width_binary_traits::const_reference,
        uuid_extension>;

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
}
