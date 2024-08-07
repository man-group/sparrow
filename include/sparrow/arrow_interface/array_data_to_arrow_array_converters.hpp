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
     * Convert an array_data vector to an ArrowArray vector.
     *
     * @param ads The array_data vector to convert.
     * @return The converted array_data elements
     */
    std::vector<arrow_array_shared_ptr> to_vector_of_arrow_array_shared_ptr(const std::vector<array_data>& ads);
    std::vector<arrow_array_shared_ptr> to_vector_of_arrow_array_shared_ptr(std::vector<array_data>&& ads);


    /**
     * Convert array_data buffers to ArrowArray buffers.
     *
     * @tparam T A const reference or rvalue reference to an array_data.
     * @param ad The array_data with the buffers convert.
     * @return The converted buffers.
     */
    template <class T>
        requires std::same_as<std::remove_cvref_t<T>, array_data>
    std::vector<sparrow::buffer<uint8_t>> arrow_array_buffer_from_array_data(T&& ad);

    /**
     * Convert an array_data to an ArrowArray.
     *
     * @tparam T A const reference or rvalue reference to an array_data.
     * @param ad The array_data to convert.
     * @return The converted ArrowArray.
     */
    template <class T>
        requires std::same_as<std::remove_cvref_t<T>, array_data>
    arrow_array_unique_ptr to_arrow_array_unique_ptr(T&& ad);

    template <class T>
        requires std::same_as<std::remove_cvref_t<T>, array_data>
    std::vector<sparrow::buffer<uint8_t>> arrow_array_buffer_from_array_data(T&& ad)
    {
        std::vector<sparrow::buffer<uint8_t>> buffers;
        buffers.reserve(ad.buffers.size() + 1);
        buffers.emplace_back(std::move(ad.bitmap.buffer()));
        if constexpr (std::is_rvalue_reference_v<T&&>)
        {
            std::ranges::move(ad.buffers, std::back_inserter(buffers));
        }
        else
        {
            std::ranges::copy(ad.buffers, std::back_inserter(buffers));
        }
        return buffers;
    }

    template <class T>
        requires std::same_as<std::remove_cvref_t<T>, array_data>
    arrow_array_unique_ptr to_arrow_array_unique_ptr(T&& ad)
    {
        arrow_array_shared_ptr dictionary = ad.dictionary.has_value()
                                                ? to_arrow_array_unique_ptr(std::move(*ad.dictionary))
                                                : nullptr;
        return make_arrow_array_unique_ptr(
            ad.length,
            static_cast<int64_t>(ad.bitmap.null_count()),
            ad.offset,
            arrow_array_buffer_from_array_data(ad),
            to_vector_of_arrow_array_shared_ptr(std::move(ad.child_data)),
            std::move(dictionary)
        );
    }

    inline std::vector<arrow_array_shared_ptr>
    to_vector_of_arrow_array_shared_ptr(std::vector<array_data>&& ads)
    {
        std::vector<arrow_array_shared_ptr> result;
        result.reserve(ads.size());
        std::transform(
            std::make_move_iterator(ads.begin()),
            std::make_move_iterator(ads.end()),
            std::back_inserter(result),
            [](array_data&& ad)
            {
                return to_arrow_array_unique_ptr(std::forward<array_data>(ad));
            }
        );
        return result;
    }

    inline std::vector<arrow_array_shared_ptr>
    to_vector_of_arrow_array_shared_ptr(const std::vector<array_data>& ads)
    {
        std::vector<arrow_array_shared_ptr> result;
        result.reserve(ads.size());
        std::transform(
            ads.begin(),
            ads.end(),
            std::back_inserter(result),
            [](const array_data& ad)
            {
                return to_arrow_array_unique_ptr(ad);
            }
        );
        return result;
    }
}
