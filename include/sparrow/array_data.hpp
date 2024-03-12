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

#include <vector>

#include "sparrow/buffer.hpp"
#include "sparrow/data_type.hpp"
#include "sparrow/dynamic_bitset.hpp"

namespace sparrow
{
    struct array_data
    {
        using block_type = std::uint8_t;
        using bitmap_type = dynamic_bitset<block_type>;
        using buffer_type = buffer<block_type>;

        data_descriptor type;
        std::int64_t length = 0;
        std::int64_t offset = 0;
        // bitmap buffer and null_count
        bitmap_type bitmap;
        // Other buffers
        std::vector<buffer_type> buffers;
        std::vector<array_data> child_data;
    };

    struct null_type
    {
    };
    constexpr null_type null;
}
