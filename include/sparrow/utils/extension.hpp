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

#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/utils/fixed_string.hpp"

namespace sparrow
{
    class empty_extension
    {
    protected:

        static void init(arrow_proxy&)
        {
            // No-op for empty extension
        }
    };

    template <fixed_string extension_name>
    class simple_extension
    {
    public:

        static constexpr std::string_view EXTENSION_NAME = extension_name;

    protected:

        static void init(arrow_proxy& proxy)
        {
            // Add JSON extension metadata
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

}