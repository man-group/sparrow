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

#include "sparrow/c_data_integration/binaryview_parser.hpp"

#include <cstddef>
#include <cstring>
#include <vector>

#include "sparrow/c_data_integration/constant.hpp"
#include "sparrow/c_data_integration/utils.hpp"
#include "sparrow/variable_size_binary_view_array.hpp"

namespace sparrow::c_data_integration
{

    sparrow::u8_buffer<uint8_t> create_buffer_view_from_json(const nlohmann::json& views_json, bool is_binary_type)
    {
        constexpr std::size_t VIEW_STRUCTURE_SIZE = 16;
        const std::size_t element_count = views_json.size();

        sparrow::u8_buffer<uint8_t> buffer_view(element_count * VIEW_STRUCTURE_SIZE);

        for (std::size_t i = 0; i < element_count; ++i)
        {
            const auto& view_json = views_json[i];
            uint8_t* view_ptr = buffer_view.data() + (i * VIEW_STRUCTURE_SIZE);

            std::memset(view_ptr, 0, VIEW_STRUCTURE_SIZE);

            const bool inlined = view_json.contains(INLINED);

            if (inlined)
            {
                const std::string inlined_data = view_json.at(INLINED).get<std::string>();
                
                std::vector<std::byte> data_bytes;
                if (is_binary_type)
                {
                    // For binary view: data is hex-encoded
                    data_bytes = utils::hex_string_to_bytes(inlined_data);
                }
                else
                {
                    // For string view: data is UTF-8 string, convert directly to bytes
                    data_bytes.reserve(inlined_data.size());
                    for (char c : inlined_data)
                    {
                        data_bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
                    }
                }
                
                // Use signed int32_t for length as per specification
                const auto length = static_cast<std::int32_t>(data_bytes.size());

                std::memcpy(view_ptr, &length, sizeof(std::int32_t));

                const std::size_t inline_size = std::min(data_bytes.size(), static_cast<std::size_t>(12));
                std::memcpy(view_ptr + 4, data_bytes.data(), inline_size);
            }
            else
            {
                const std::size_t buffer_index = view_json.at(BUFFER_INDEX).get<std::size_t>();
                // Use signed int32_t for offset and size as per specification
                const auto offset = static_cast<std::int32_t>(view_json.at(OFFSET).get<int>());
                const auto size = static_cast<std::int32_t>(view_json.at(SIZE).get<int>());
                const std::string prefix_hex = view_json.at(PREFIX_HEX).get<std::string>();
                
                std::vector<std::byte> prefix_bytes;
                if (is_binary_type)
                {
                    // For binary view: prefix is hex-encoded
                    prefix_bytes = utils::hex_string_to_bytes(prefix_hex);
                }
                else
                {
                    // For string view: prefix should be UTF-8 string converted to bytes
                    prefix_bytes.reserve(prefix_hex.size());
                    for (char c : prefix_hex)
                    {
                        prefix_bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
                    }
                }

                std::memcpy(view_ptr, &size, sizeof(std::int32_t));

                const std::size_t prefix_size = std::min(prefix_bytes.size(), static_cast<std::size_t>(4));
                std::memcpy(view_ptr + 4, prefix_bytes.data(), prefix_size);

                // Convert logical buffer index to Arrow buffer index
                // Buffer index 0 in JSON refers to first variadic data buffer, which is at Arrow index 2
                constexpr std::size_t FIRST_VAR_DATA_BUFFER_INDEX = 2;
                const auto buf_idx = static_cast<std::int32_t>(buffer_index + FIRST_VAR_DATA_BUFFER_INDEX);
                std::memcpy(view_ptr + 8, &buf_idx, sizeof(std::int32_t));

                std::memcpy(view_ptr + 12, &offset, sizeof(std::int32_t));
            }
        }

        return buffer_view;
    }

    template <typename T>
    sparrow::array
    binaryview_array_from_json_impl(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json&, bool is_binary_type)
    {
        const auto& variadic_data_buffers_json = array.at(VARIADIC_DATA_BUFFERS);
        const std::vector<std::string> variadic_data_buffers_str = variadic_data_buffers_json
                                                                       .get<std::vector<std::string>>();
        const std::vector<std::vector<std::byte>> variadic_data_buffers_bytes = utils::hex_strings_to_bytes(
            variadic_data_buffers_str
        );
        const auto& views_json = array.at(VIEWS);

        auto buffer_view = create_buffer_view_from_json(views_json, is_binary_type);

        std::vector<sparrow::u8_buffer<uint8_t>> value_buffers;
        for (const auto& buf : variadic_data_buffers_bytes)
        {
            sparrow::u8_buffer<uint8_t> u8_buf(buf.size());
            std::memcpy(u8_buf.data(), buf.data(), buf.size());
            value_buffers.push_back(std::move(u8_buf));
        }

        const std::string name = schema.at("name").get<std::string>();
        auto metadata = utils::get_metadata(schema);
        const bool nullable = schema.at("nullable").get<bool>();

        if (nullable)
        {
            auto validity = utils::get_validity(array);
            T arr{
                views_json.size(),
                std::move(buffer_view),
                std::move(value_buffers),
                std::move(validity),
                name,
                std::move(metadata)
            };

            return sparrow::array{std::move(arr)};
        }
        else
        {
            std::vector<bool> all_valid(views_json.size(), true);
            T arr{
                views_json.size(),
                std::move(buffer_view),
                std::move(value_buffers),
                std::move(all_valid),
                name,
                std::move(metadata)
            };

            return sparrow::array{std::move(arr)};
        }
    }

    sparrow::array
    binaryview_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(schema, "binaryview");
        return binaryview_array_from_json_impl<binary_view_array>(array, schema, root, true); // true for binary type
    }

    sparrow::array
    utf8view_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(schema, "utf8view");
        return binaryview_array_from_json_impl<string_view_array>(array, schema, root, false); // false for string type
    }

}
