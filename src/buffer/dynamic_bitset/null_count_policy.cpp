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

#include "sparrow/details/3rdparty/libpopcnt/libpopcnt.h"

namespace sparrow
{
    std::size_t
    count_non_null(const std::uint8_t* data, std::size_t bit_size, std::size_t byte_size, std::size_t offset) noexcept
    {
        if (data == nullptr || byte_size == 0)
        {
            return bit_size;
        }

        if (bit_size == 0)
        {
            return 0;
        }

        constexpr std::size_t bits_per_byte = 8;

        // Calculate the starting byte and bit position
        const std::size_t start_byte = offset / bits_per_byte;
        const std::size_t start_bit = offset % bits_per_byte;

        // Check if offset is beyond the buffer
        if (start_byte >= byte_size)
        {
            return 0;
        }

        uint64_t res = 0;
        std::size_t bits_counted = 0;
        std::size_t current_byte = start_byte;

        // Handle the first partial byte (if offset is not byte-aligned)
        if (start_bit != 0)
        {
            const std::size_t bits_in_first_byte = std::min(bits_per_byte - start_bit, bit_size);
            const std::uint8_t first_byte_mask = static_cast<std::uint8_t>(
                ((1u << bits_in_first_byte) - 1u) << start_bit
            );
            const std::uint8_t masked_byte = data[current_byte] & first_byte_mask;
            res += static_cast<uint64_t>(std::popcount(masked_byte));
            bits_counted += bits_in_first_byte;
            ++current_byte;
        }

        // Count full bytes in the middle
        if (bits_counted < bit_size && current_byte < byte_size)
        {
            const std::size_t remaining_bits = bit_size - bits_counted;
            const std::size_t full_bytes = remaining_bits / bits_per_byte;
            const std::size_t bytes_to_count = std::min(full_bytes, byte_size - current_byte);

            if (bytes_to_count > 0)
            {
                res += popcnt(data + current_byte, bytes_to_count);
                bits_counted += bytes_to_count * bits_per_byte;
                current_byte += bytes_to_count;
            }
        }

        // Handle the last partial byte
        if (bits_counted < bit_size && current_byte < byte_size)
        {
            const std::size_t remaining_bits = bit_size - bits_counted;
            const std::uint8_t last_byte_mask = static_cast<std::uint8_t>((1u << remaining_bits) - 1u);
            const std::uint8_t masked_byte = data[current_byte] & last_byte_mask;
            res += static_cast<uint64_t>(std::popcount(masked_byte));
        }

        return static_cast<std::size_t>(res);
    }

}  // namespace sparrow
