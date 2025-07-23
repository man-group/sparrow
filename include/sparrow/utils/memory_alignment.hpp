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

namespace sparrow
{
    // 64-byte alignment constant
    constexpr size_t SPARROW_BUFFER_ALIGNMENT = 64;

    [[nodiscard]] constexpr size_t align_to_64_bytes(size_t size) noexcept
    {
        constexpr size_t mask = SPARROW_BUFFER_ALIGNMENT - 1;
        return (size + mask) & ~mask;
    }

    template <class T>
    [[nodiscard]] size_t calculate_aligned_size(size_t n) noexcept
    {
        const size_t byte_size = n * sizeof(T);
        const size_t aligned_byte_size = align_to_64_bytes(byte_size);
        return aligned_byte_size;
    }
}