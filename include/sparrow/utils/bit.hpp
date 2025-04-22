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

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>

#if (!defined(__clang__) && defined(__GNUC__) && __GNUC__ < 11)
#include <cstring>
#endif

namespace sparrow
{
    /**
     * Reverses the bytes in the given integer value.
     * \tparam T The type of the integer value.
     * \param value The integer value to reverse.
     * \return The integer value with the bytes reversed.
     * \note Implementation comes from https://en.cppreference.com/w/cpp/numeric/byteswap
     */
    template <std::integral T>
    [[nodiscard]] constexpr T byteswap(T value) noexcept
    {
        static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
#if (!defined(__clang__) && defined(__GNUC__) && __GNUC__ < 11)
        std::array<std::byte, sizeof(T)> value_representation;
        std::memcpy(&value_representation, &value, sizeof(T));
        std::ranges::reverse(value_representation);
        T res;
        std::memcpy(&res, &value_representation, sizeof(T));
        return res;
#else
        auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
        std::ranges::reverse(value_representation);
        return std::bit_cast<T>(value_representation);
#endif
    }

    template <std::endian input_value_endianess>
    [[nodiscard]] constexpr auto to_native_endian(std::integral auto value) noexcept
    {
        if constexpr (std::endian::native != input_value_endianess)
        {
            return byteswap(value);
        }
        else
        {
            return value;
        }
    }
}
