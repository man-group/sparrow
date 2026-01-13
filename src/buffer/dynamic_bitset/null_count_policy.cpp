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

#include "sparrow/buffer/dynamic_bitset/null_count_policy.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>

#include <libpopcnt.h>

namespace sparrow
{
    std::size_t count_non_null(const std::uint8_t* data, std::size_t bit_size, std::size_t byte_size) noexcept
    {
        if (data == nullptr || byte_size == 0)
        {
            return bit_size;
        }

        constexpr std::size_t bits_per_byte = 8;
        const std::size_t full_bytes = bit_size / bits_per_byte;

        const std::size_t bytes_to_count = std::min(full_bytes, byte_size);
        uint64_t res = popcnt(data, bytes_to_count);

        // Handle remaining bits in the last partial byte
        const std::size_t remaining_bits = bit_size % bits_per_byte;
        if (remaining_bits != 0 && full_bytes < byte_size)
        {
            const std::uint8_t mask = static_cast<std::uint8_t>((1u << remaining_bits) - 1u);
            const std::uint8_t last_byte = data[full_bytes] & mask;
            res += static_cast<uint64_t>(std::popcount(last_byte));
        }

        return static_cast<std::size_t>(res);
    }

}  // namespace sparrow
