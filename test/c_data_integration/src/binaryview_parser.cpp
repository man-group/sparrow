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
#include <vector>

#include "sparrow/c_data_integration/constant.hpp"
#include "sparrow/c_data_integration/utils.hpp"
#include "sparrow/layout/variable_size_binary_view_array.hpp"

namespace sparrow::c_data_integration
{
    template <typename T>
    sparrow::array
    binaryview_array_from_json_impl(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json&)
    {
        using data_type = std::conditional_t<std::same_as<T, string_view_array>, std::string, std::vector<std::byte>>;
        constexpr bool is_binary = std::same_as<T, binary_view_array>;
        const auto& variadic_data_buffers_json = array.at(VARIADIC_DATA_BUFFERS);
        const std::vector<std::string> variadic_data_buffers_str = variadic_data_buffers_json
                                                                       .get<std::vector<std::string>>();
        const std::vector<std::vector<std::byte>> variadic_data_buffers_bytes = utils::hex_strings_to_bytes(
            variadic_data_buffers_str
        );
        const auto& views_json = array.at(VIEWS);
        std::vector<data_type> views_data;
        for (const auto& view_json : views_json)
        {
            const bool inlined = view_json.contains(INLINED);
            if (inlined)
            {
                const std::string inlined_data = view_json.at(INLINED).get<std::string>();
                if constexpr (!is_binary)
                {
                    views_data.push_back(inlined_data);
                }
                else
                {
                    views_data.push_back(utils::hex_string_to_bytes(inlined_data));
                }
            }
            else
            {
                const size_t buffer_index = view_json.at(BUFFER_INDEX).get<size_t>();
                const int offset = view_json.at(OFFSET).get<int>();
                const int size = view_json.at(SIZE).get<int>();
                const auto& prefix_hex_json = view_json.at(PREFIX_HEX);
                const std::vector<std::byte> prefix_bytes = utils::hex_string_to_bytes(
                    prefix_hex_json.get<std::string>()
                );
                const std::vector<std::byte>& buffer = variadic_data_buffers_bytes.at(buffer_index);

                std::vector<std::byte> view_data = prefix_bytes;
                view_data.insert(
                    view_data.end(),
                    buffer.begin() + offset,
                    buffer.begin() + offset + size - static_cast<int>(prefix_bytes.size())
                );

                if constexpr (!is_binary)
                {
                    // Convert to string if the type is string_view_array
                    std::string view_data_str(reinterpret_cast<const char*>(view_data.data()), view_data.size());
                    views_data.push_back(std::move(view_data_str));
                }
                else
                {
                    views_data.push_back(std::move(view_data));
                }
            }
        }

        const std::string name = schema.at("name").get<std::string>();
        auto metadata = utils::get_metadata(schema);
        const bool nullable = schema.at("nullable").get<bool>();
        if (nullable)
        {
            auto validity = utils::get_validity(array);
            T ar{std::move(views_data), std::move(validity), name, std::move(metadata)};
            return sparrow::array{std::move(ar)};
        }
        else
        {
            return sparrow::array{T{std::move(views_data), false, name, std::move(metadata)}};
        }
    }

    sparrow::array
    binaryview_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(schema, "binaryview");
        return binaryview_array_from_json_impl<binary_view_array>(array, schema, root);
    }

    sparrow::array
    utf8view_array_from_json(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root)
    {
        utils::check_type(schema, "utf8view");
        return binaryview_array_from_json_impl<string_view_array>(array, schema, root);
    }
}