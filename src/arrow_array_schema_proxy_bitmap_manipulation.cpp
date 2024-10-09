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

#include "sparrow/arrow_array_schema_proxy_bitmap_manipulation.hpp"

namespace sparrow::proxy::bitmap_manipulation
{
    constexpr size_t bitmap_buffer_index = 0;

    [[nodiscard]] inline non_owning_dynamic_bitset<uint8_t>
    get_non_owning_dynamic_bitset(arrow_proxy& arrow_proxy)
    {
        SPARROW_ASSERT_TRUE(arrow_proxy.is_created_with_sparrow())
        auto private_data = static_cast<arrow_array_private_data*>(arrow_proxy.array().private_data);
        auto& bitmap_buffer = private_data->buffers()[bitmap_buffer_index];
        const size_t current_size = arrow_proxy.length() + arrow_proxy.offset();
        non_owning_dynamic_bitset<uint8_t> bitmap{&bitmap_buffer, current_size};
        return bitmap;
    }

    dynamic_bitset_view<uint8_t> resize_bitmap(arrow_proxy& arrow_proxy, size_t new_size)
    {
        SPARROW_ASSERT_TRUE(arrow_proxy.is_created_with_sparrow())
        auto bitmap = get_non_owning_dynamic_bitset(arrow_proxy);
        bitmap.resize(new_size, true);
        dynamic_bitset_view<uint8_t> new_bitmap = {arrow_proxy.buffers()[bitmap_buffer_index].data(), new_size};
        return new_bitmap;
    }

    size_t insert_bitmap(arrow_proxy& arrow_proxy, size_t index, bool value)
    {
        SPARROW_ASSERT_TRUE(arrow_proxy.is_created_with_sparrow())
        auto bitmap = get_non_owning_dynamic_bitset(arrow_proxy);
        auto it = bitmap.insert(sparrow::next(bitmap.cbegin(), index), value);
        return std::distance(bitmap.begin(), it);
    }

    size_t insert_bitmap(arrow_proxy& arrow_proxy, size_t index, bool value, size_t count)
    {
        SPARROW_ASSERT_TRUE(arrow_proxy.is_created_with_sparrow())
        auto bitmap = get_non_owning_dynamic_bitset(arrow_proxy);
        auto it = bitmap.insert(sparrow::next(bitmap.cbegin(), index), count, value);
        return std::distance(bitmap.begin(), it);
    }

    size_t insert_bitmap(arrow_proxy& arrow_proxy, size_t index, std::initializer_list<bool> values)
    {
        SPARROW_ASSERT_TRUE(arrow_proxy.is_created_with_sparrow())
        auto bitmap = get_non_owning_dynamic_bitset(arrow_proxy);
        auto it = bitmap.insert(sparrow::next(bitmap.cbegin(), index), values.begin(), values.end());
        return std::distance(bitmap.begin(), it);
    }

    size_t erase_bitmap(arrow_proxy& arrow_proxy, size_t index)
    {
        SPARROW_ASSERT_TRUE(arrow_proxy.is_created_with_sparrow())
        auto bitmap = get_non_owning_dynamic_bitset(arrow_proxy);
        auto it = bitmap.erase(sparrow::next(bitmap.cbegin(), index));
        return std::distance(bitmap.begin(), it);
    }

    size_t erase_bitmap(arrow_proxy& arrow_proxy, size_t index, size_t count)
    {
        SPARROW_ASSERT_TRUE(arrow_proxy.is_created_with_sparrow())
        auto bitmap = get_non_owning_dynamic_bitset(arrow_proxy);
        const auto it_first = sparrow::next(bitmap.cbegin(), index);
        const auto it_last = sparrow::next(it_first, count);
        auto it = bitmap.erase(it_first, it_last);
        return std::distance(bitmap.begin(), it);
    }

    void push_back_bitmap(arrow_proxy& arrow_proxy, bool value)
    {
        SPARROW_ASSERT_TRUE(arrow_proxy.is_created_with_sparrow())
        insert_bitmap(arrow_proxy, arrow_proxy.length(), value);
    }

    void pop_back_bitmap(arrow_proxy& arrow_proxy)
    {
        SPARROW_ASSERT_TRUE(arrow_proxy.is_created_with_sparrow())
        erase_bitmap(arrow_proxy, arrow_proxy.length() - 1);
    }

    size_t get_null_count(const arrow_proxy& arrow_proxy)
    {
        const size_t bitmap_size = arrow_proxy.length() + arrow_proxy.offset();
        const dynamic_bitset_view<const uint8_t> bitmap{arrow_proxy.buffers()[bitmap_buffer_index].data(), bitmap_size};
        return bitmap.null_count();
    }
}
