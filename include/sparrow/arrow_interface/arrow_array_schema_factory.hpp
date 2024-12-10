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

#include <cstdint>

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/buffer.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/types/data_type.hpp"
#include "sparrow/utils/buffers.hpp"
#include "sparrow/utils/offsets.hpp"

namespace sparrow
{
    template <std::ranges::range R>
        requires(std::integral<std::ranges::range_value_t<R>> && !std::same_as<std::ranges::range_value_t<R>, bool>)
    buffer<uint8_t> make_bitmap_buffer(size_t count, R&& nulls)
    {
        if (!std::ranges::empty(nulls))
        {
            SPARROW_ASSERT_TRUE(*std::ranges::max_element(nulls) < count);
        }
        dynamic_bitset<uint8_t> bitmap(count, true);
        for (const auto i : nulls)
        {
            bitmap.set(i, false);
        }
        return bitmap.buffer();
    };
}
