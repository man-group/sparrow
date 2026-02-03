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

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>

// This header must be included AFTER including sparrow array headers
// It relies on types like sparrow::u8_buffer and sparrow::validity_bitmap being declared

namespace sparrow::test
{
    /**
     * @brief Helper to allocate and initialize a data buffer for zero-copy tests
     * Returns pair of (typed_ptr, u8_buffer)
     */
    template <typename T, typename Allocator>
    auto make_zero_copy_data_buffer(size_t num_elements, Allocator& allocator)
    {
        auto* data_ptr = allocator.allocate(sizeof(T) * num_elements);
        auto* typed_ptr = reinterpret_cast<T*>(data_ptr);
        for (size_t idx = 0; idx < num_elements; ++idx)
        {
            typed_ptr[idx] = static_cast<T>(idx);
        }
        return std::pair{typed_ptr, sparrow::u8_buffer<T>(typed_ptr, num_elements, allocator)};
    }

    /**
     * @brief Helper to allocate and initialize a validity bitmap for zero-copy tests
     * Returns pair of (original_ptr, validity_bitmap)
     */
    template <typename Allocator>
    auto make_zero_copy_validity_bitmap(size_t num_rows, Allocator& allocator)
    {
        size_t bitmap_size_bytes = (num_rows + 7) / 8;
        uint8_t* bitmap_ptr = allocator.allocate(bitmap_size_bytes);
        std::memset(bitmap_ptr, 0xFF, bitmap_size_bytes);
        sparrow::buffer<uint8_t> bitmap_buffer(bitmap_ptr, bitmap_size_bytes, allocator);
        const uint8_t* original_ptr = bitmap_buffer.data();
        return std::pair{original_ptr, sparrow::validity_bitmap(std::move(bitmap_buffer), num_rows, 0)};
    }
}
