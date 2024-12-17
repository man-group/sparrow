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

#include <numeric>
#include <ranges>

#include "sparrow/buffer/buffer.hpp"
#include "sparrow/types/data_type.hpp"

#pragma once

namespace sparrow
{
    template <layout_offset OT, std::ranges::sized_range R>
        requires std::ranges::sized_range<std::ranges::range_value_t<R>>
    buffer<OT> make_offset_buffer(const R& range)
    {
        const size_t range_size = std::ranges::size(range);
        buffer<OT> offsets(range_size + 1, 0);
        std::transform(
            range.cbegin(),
            range.cend(),
            offsets.begin() + 1,
            [](const auto& elem)
            {
                return static_cast<OT>(elem.size());
            }
        );
        std::partial_sum(offsets.begin(), offsets.end(), offsets.begin());
        return offsets;
    }
}
