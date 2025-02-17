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

#include "sparrow/utils/metadata.hpp"

#include "doctest/doctest.h"

TEST_SUITE("metadata")
{
    // NOLINT
    const std::vector<char> metadata_buffer = {
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

    TEST_CASE("get_key_values_from_metadata")
    {
        const sparrow::KeyValueView key_values(metadata_buffer.data());
        CHECK_EQ(key_values.size(), 2);
        auto kv_it = key_values.cbegin();
        auto kv_1 = *kv_it;
        CHECK_EQ(kv_1.first, "key1");
        CHECK_EQ(kv_1.second, "val1");
        ++kv_it;
        auto kv_2 = *kv_it;
        CHECK_EQ(kv_2.first, "key2");
        CHECK_EQ(kv_2.second, "val2");
    }

    // TEST_CASE("get_metadata_from_key_values")
    // {
    //     // const std::vector<sparrow::metadata_pair> metadata = {{"key1", "val1"}, {"key2", "val2"}};
    //     // const std::vector<char> metadata_bytes = sparrow::get_metadata_from_key_values(metadata);

    //     const auto key_values = sparrow::get_key_values_from_metadata(metadata_buffer);
    //     CHECK_EQ(std::ranges::size(key_values), 2);
    //     CHECK_EQ(key_values[0].first, "key1");
    //     CHECK_EQ(key_values[0].second, "val1");
    //     CHECK_EQ(key_values[1].first, "key2");
    //     CHECK_EQ(key_values[1].second, "val2");
    // }
}
