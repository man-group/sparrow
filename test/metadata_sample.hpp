// Man Group Operations Limited
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

#include <optional>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "sparrow/utils/metadata.hpp"

namespace sparrow
{

    static const std::string metadata_buffer = []()
    {
        if constexpr (std::endian::native == std::endian::big)
        {
            return std::string{
                0x00, 0x00, 0x00, 0x02,  // Number of keys/values
                0x00, 0x00, 0x00, 0x04,  // Length of key1
                'k',  'e',  'y',  '1',   // Key 1
                0x00, 0x00, 0x00, 0x04,  // Length of value1
                'v',  'a',  'l',  '1',   // Value 1
                0x00, 0x00, 0x00, 0x04,  // Length of key2
                'k',  'e',  'y',  '2',   // Key 2
                0x00, 0x00, 0x00, 0x04,  // Length of value2
                'v',  'a',  'l',  '2'    // Value 2
            };
        }
        else if constexpr (std::endian::native == std::endian::little)
        {
            return std::string{
                0x02, 0x00, 0x00, 0x00,  // Number of keys/values
                0x04, 0x00, 0x00, 0x00,  // Length of key1
                'k',  'e',  'y',  '1',   // Key 1
                0x04, 0x00, 0x00, 0x00,  // Length of value1
                'v',  'a',  'l',  '1',   // Value 1
                0x04, 0x00, 0x00, 0x00,  // Length of key2
                'k',  'e',  'y',  '2',   // Key 2
                0x04, 0x00, 0x00, 0x00,  // Length of value2
                'v',  'a',  'l',  '2'    // Value 2
            };
        }
    }();

    static const std::vector<metadata_pair> metadata_sample = {{"key1", "val1"}, {"key2", "val2"}};
    static const std::optional<std::vector<metadata_pair>> metadata_sample_opt = metadata_sample;

    inline void test_metadata(const std::vector<metadata_pair>& metadata_1, const key_value_view& metadata_2)
    {
        REQUIRE_EQ(metadata_1.size(), metadata_2.size());

        auto it = metadata_2.cbegin();
        for (const auto& [key, value] : metadata_1)
        {
            CHECK_EQ(key, (*it).first);
            CHECK_EQ(value, (*it).second);
            ++it;
        }
    }
}
