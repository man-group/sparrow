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
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "sparrow/array.hpp"
#include "sparrow/utils/large_int.hpp"

namespace sparrow::c_data_integration::utils
{
    std::vector<std::vector<std::byte>> hexStringsToBytes(const std::vector<std::string>& hexStrings);

    std::vector<std::pair<const nlohmann::json&, const nlohmann::json&>>
    get_children(const nlohmann::json& array, const nlohmann::json& schema);

    std::vector<bool> get_validity(const nlohmann::json& array);

    void check_type(const nlohmann::json& schema, const std::string& type);

    std::optional<std::vector<sparrow::metadata_pair>> get_metadata(const nlohmann::json& schema);

    template <std::integral I>
    auto from_strings_to_Is(const std::vector<std::string>& data_str)
    {
        return data_str
               | std::views::transform(
                   [](const std::string& str)
                   {
                       if constexpr (std::is_same_v<I, int64_t>)
                       {
                           return static_cast<I>(std::stoll(str));
                       }
                       else if constexpr (std::is_same_v<I, uint64_t>)
                       {
                           return static_cast<I>(std::stoull(str));
                       }
#ifndef SPARROW_USE_LARGE_INT_PLACEHOLDERS
                       else if constexpr (std::is_same_v<I, sparrow::int128_t>)
                       {
                           return sparrow::stobigint<sparrow::int128_t>(str);
                       }
                       else if constexpr (std::is_same_v<I, sparrow::int256_t>)
                       {
                           return sparrow::stobigint<sparrow::int256_t>(str);
                       }
#endif
                       else
                       {
                           throw std::runtime_error("Unsupported type for conversion");
                       }
                   }
               );
    }
}  // namespace sparrow::c_data_integration::utils