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
#include <cstdint>
#include <type_traits>

#include "sparrow/array/array_data.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"

namespace sparrow
{
    /**
     * Convert array_data buffers to ArrowArray buffers.
     *
     * @tparam T A const reference or rvalue reference to an array_data.
     * @param ad The array_data with the buffers convert.
     * @return The converted buffers.
     */
    template <class T>
        requires std::same_as<std::remove_cvref_t<T>, array_data>
    std::vector<sparrow::buffer<uint8_t>> to_vector_of_buffer(T&& ad);


    template <class T>
        requires std::same_as<std::remove_cvref_t<T>, array_data>
    std::vector<sparrow::buffer<uint8_t>> to_vector_of_buffer(T&& ad)
    {
        std::vector<sparrow::buffer<uint8_t>> buffers;
        buffers.reserve(ad.buffers.size() + 1);
        buffers.emplace_back(std::move(ad.bitmap.buffer()));
        if constexpr (std::is_rvalue_reference_v<decltype(ad)>)
        {
            std::ranges::move(ad.buffers, std::back_inserter(buffers));
        }
        else
        {
            std::ranges::copy(ad.buffers, std::back_inserter(buffers));
        }
        return buffers;
    }
}
