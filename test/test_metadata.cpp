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
#include "metadata_sample.hpp"

TEST_SUITE("metadata")
{
    TEST_CASE("KeyValueView")
    {
        const sparrow::KeyValueView key_values(sparrow::metadata_buffer.data());
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

    TEST_CASE("get_metadata_from_key_values")
    {
        const auto metadata_result = sparrow::get_metadata_from_key_values(sparrow::metadata_sample);
        CHECK_EQ(sparrow::metadata_buffer, metadata_result);
    }
}
