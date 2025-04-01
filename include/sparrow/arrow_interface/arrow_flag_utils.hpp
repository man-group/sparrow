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

#include "sparrow/c_interface.hpp"
#include "sparrow/utils/constexpr.hpp"

namespace sparrow
{
    /// @returns `true` if the given value is a valid `ArrowFlag` value, `false` otherwise.
    constexpr bool is_valid_ArrowFlag_value(int64_t value) noexcept
    {
        constexpr std::array<ArrowFlag, 3> valid_values = {
            ArrowFlag::DICTIONARY_ORDERED,
            ArrowFlag::NULLABLE,
            ArrowFlag::MAP_KEYS_SORTED
        };
        return std::ranges::any_of(
            valid_values,
            [value](ArrowFlag flag)
            {
                return static_cast<std::underlying_type_t<ArrowFlag>>(flag) == value;
            }
        );
    }

    /// Converts a bitfield of ArrowFlag values to a vector of ArrowFlag values.
    SPARROW_CONSTEXPR_GCC_11 std::vector<ArrowFlag> to_vector_of_ArrowFlags(int64_t flag_values)
    {
        constexpr size_t n_bits = sizeof(flag_values) * 8;
        std::vector<ArrowFlag> flags;
        for (size_t i = 0; i < n_bits; ++i)
        {
            const int64_t flag_value = static_cast<int64_t>(1) << i;
            if ((flag_values & flag_value) != 0)
            {
                if (!is_valid_ArrowFlag_value(flag_value))
                {
                    // TODO: Replace with a more specific exception
                    throw std::runtime_error("Invalid ArrowFlag value");
                }
                flags.push_back(static_cast<ArrowFlag>(flag_value));
            }
        }
        return flags;
    }

    /// Converts a vector of ArrowFlag values to a bitfield of ArrowFlag values.
    SPARROW_CONSTEXPR_GCC_11 int64_t to_ArrowFlag_value(const std::vector<ArrowFlag>& flags)
    {
        int64_t flag_values = 0;
        for (const ArrowFlag flag : flags)
        {
            flag_values |= static_cast<int64_t>(flag);
        }
        return flag_values;
    }

}
