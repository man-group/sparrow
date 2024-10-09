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

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/config/config.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset_view.hpp"
#include "sparrow/buffer/dynamic_bitset/non_owning_dynamic_bitset.hpp"

namespace sparrow::proxy::bitmap_manipulation
{
    [[nodiscard]] inline non_owning_dynamic_bitset<uint8_t>
    SPARROW_API get_non_owning_dynamic_bitset(arrow_proxy& arrow_proxy);

    /**
     * Resize a bitmap from an arrow proxy.
     */
    [[nodiscard]] dynamic_bitset_view<uint8_t>
    SPARROW_API resize_bitmap(arrow_proxy& arrow_proxy, size_t new_size);

    SPARROW_API size_t insert_bitmap(arrow_proxy& arrow_proxy, size_t index, bool value);
    SPARROW_API size_t insert_bitmap(arrow_proxy& arrow_proxy, size_t index, bool value, size_t count);
    SPARROW_API size_t insert_bitmap(arrow_proxy& arrow_proxy, size_t index, std::initializer_list<bool> values);

    template <std::input_iterator InputIt>
    size_t insert_bitmap(arrow_proxy& arrow_proxy, size_t index, InputIt first, InputIt last)
    {
        auto bitmap = get_non_owning_dynamic_bitset(arrow_proxy);
        const auto it = bitmap.insert(sparrow::next(bitmap.cbegin(), index), first, last);
        return static_cast<size_t>(std::distance(bitmap.begin(), it));
    }

    template <std::ranges::input_range R>
    size_t insert_bitmap(arrow_proxy& arrow_proxy, size_t index, const R& range)
    {
        return insert(arrow_proxy, index, std::ranges::begin(range), std::ranges::end(range));
    }

    SPARROW_API size_t erase_bitmap(arrow_proxy& arrow_proxy, size_t index);
    SPARROW_API size_t erase_bitmap(arrow_proxy& arrow_proxy, size_t index, size_t count);

    SPARROW_API void push_back_bitmap(arrow_proxy& arrow_proxy, bool value);
    SPARROW_API void pop_back_bitmap(arrow_proxy& arrow_proxy);

    SPARROW_API size_t get_null_count(const arrow_proxy& arrow_proxy);
}
