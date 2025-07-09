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

#include <string_view>

#include "sparrow/config/config.hpp"

namespace sparrow
{
    /**
     * Get the number of bytes for a fixed width binary layout from the ArrowArray format string.
     * Example:
     *      w:42 -> 42 bytes
     *      w:1  -> 1 bytes
     * @param format the format string
     * @return the number of bytes
     * @throws std::invalid_argument if no conversion could be performed.
     * @throws std::out_of_range if the converted value would fall out of the range of the result type or if
     * the underlying function (std::strtoul or std::strtoull) sets errno to ERANGE.
     */
    [[nodiscard]] SPARROW_API std::size_t num_bytes_for_fixed_sized_binary(std::string_view format);
}
