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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or mplied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <utility>

#include "sparrow/buffer/u8_buffer.hpp"
namespace sparrow
{
    template<class OFFSET_TYPE>
    concept offset_type = std::is_same<OFFSET_TYPE, std::uint32_t>::value || std::is_same<OFFSET_TYPE, std::uint64_t>::value;

    template<offset_type OFFSET_TYPE>
    class offset_buffer : public u8_buffer<OFFSET_TYPE>
    {   
    public:
        template<std::ranges::input_range R>
        requires(std::unsigned_integral<std::ranges::range_value_t<R>>)
        static auto from_sizes(R && sizes) -> offset_buffer<OFFSET_TYPE>;

    };

    template<offset_type OFFSET_TYPE>
    template<std::ranges::input_range R>
    requires(std::unsigned_integral<std::ranges::range_value_t<R>>)
    auto offset_buffer<OFFSET_TYPE>::from_sizes(R && sizes) -> offset_buffer<OFFSET_TYPE>
    {
        // do the accumulation
        const auto size  = std::ranges::size(sizes);
        offset_buffer<OFFSET_TYPE> buffer(size+1);

        auto it = buffer.begin();
        OFFSET_TYPE offset = 0;
        for(auto s : sizes)
        {
            *it = offset;
            offset += s;
            ++it;
        }
        *it = offset;
        return buffer;
    }

}