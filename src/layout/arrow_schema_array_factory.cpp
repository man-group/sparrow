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

#include "sparrow/layout/arrow_schema_array_factory.hpp"

#include "sparrow/array.hpp"

namespace sparrow::detail
{
    // ─── Primitive array ───────────────────────────────────────────────────────

    ArrowArray
    make_primitive_arrow_array(std::int64_t size, std::optional<validity_bitmap>&& bitmap, buffer<uint8_t>&& data_buf)
    {
        auto [null_count, bitmap_buf] = extract_bitmap(std::move(bitmap));
        std::vector<buffer<uint8_t>> buffers;
        buffers.reserve(2);
        buffers.emplace_back(std::move(bitmap_buf));
        buffers.emplace_back(std::move(data_buf));
        return make_arrow_array(
            size,
            null_count,
            0,
            std::move(buffers),
            nullptr,
            repeat_view<bool>(true, 0),
            nullptr,
            true
        );
    }

    // ─── Fixed-width binary array ─────────────────────────────────────────────

    ArrowArray make_fixed_width_binary_arrow_array(
        std::int64_t element_count,
        std::optional<validity_bitmap>&& bitmap,
        buffer<uint8_t>&& data_buf
    )
    {
        auto [null_count, bitmap_buf] = extract_bitmap(std::move(bitmap));
        std::vector<buffer<uint8_t>> buffers;
        buffers.reserve(2);
        buffers.emplace_back(std::move(bitmap_buf));
        buffers.emplace_back(std::move(data_buf));
        return make_arrow_array(
            element_count,
            null_count,
            0,
            std::move(buffers),
            nullptr,
            repeat_view<bool>(true, 0),
            nullptr,
            true
        );
    }

    // ─── Variable-size binary array ───────────────────────────────────────────

    ArrowArray make_variable_size_binary_arrow_array(
        std::int64_t size,
        std::optional<validity_bitmap>&& bitmap,
        buffer<uint8_t>&& offset_buf,
        buffer<uint8_t>&& data_buf
    )
    {
        auto [null_count, bitmap_buf] = extract_bitmap(std::move(bitmap));
        std::vector<buffer<uint8_t>> buffers;
        buffers.reserve(3);
        buffers.emplace_back(std::move(bitmap_buf));
        buffers.emplace_back(std::move(offset_buf));
        buffers.emplace_back(std::move(data_buf));
        return make_arrow_array(
            size,
            null_count,
            0,
            std::move(buffers),
            nullptr,
            repeat_view<bool>(true, 0),
            nullptr,
            true
        );
    }

    // ─── Decimal array ────────────────────────────────────────────────────────

    ArrowArray
    make_decimal_arrow_array(std::int64_t size, std::optional<validity_bitmap>&& bitmap, buffer<uint8_t>&& data_buf)
    {
        auto [null_count, bitmap_buf] = extract_bitmap(std::move(bitmap));
        std::vector<buffer<uint8_t>> buffers;
        buffers.reserve(2);
        buffers.emplace_back(std::move(bitmap_buf));
        buffers.emplace_back(std::move(data_buf));
        return make_arrow_array(
            size,
            null_count,
            0,
            std::move(buffers),
            nullptr,
            repeat_view<bool>(true, 0),
            nullptr,
            true
        );
    }

    // ─── Timestamp array ──────────────────────────────────────────────────────

    ArrowArray
    make_timestamp_arrow_array(std::int64_t size, std::optional<validity_bitmap>&& bitmap, buffer<uint8_t>&& data_buf)
    {
        auto [null_count, bitmap_buf] = extract_bitmap(std::move(bitmap));
        std::vector<buffer<uint8_t>> buffers;
        buffers.reserve(2);
        buffers.emplace_back(std::move(bitmap_buf));
        buffers.emplace_back(std::move(data_buf));
        return make_arrow_array(
            size,
            null_count,
            0,
            std::move(buffers),
            nullptr,
            repeat_view<bool>(true, 0),
            nullptr,
            true
        );
    }

    // ─── Map array ────────────────────────────────────────────────────────────

    ArrowArray make_map_arrow_array(
        std::int64_t size,
        std::optional<validity_bitmap>&& bitmap,
        buffer<uint8_t>&& offset_buf,
        sparrow::array&& entries
    )
    {
        auto [null_count, validity_buf] = extract_bitmap(std::move(bitmap));
        ArrowArray entries_arr = extract_arrow_array(std::move(entries));
        const repeat_view<bool> children_ownership{true, 1};
        std::vector<buffer<uint8_t>> buffers;
        buffers.reserve(2);
        buffers.emplace_back(std::move(validity_buf));
        buffers.emplace_back(std::move(offset_buf));
        return make_arrow_array(
            size,
            null_count,
            0,
            std::move(buffers),
            new ArrowArray*[1]{new ArrowArray(std::move(entries_arr))},
            children_ownership,
            nullptr,
            true
        );
    }

    // ─── Dictionary-encoded array ─────────────────────────────────────────────

    ArrowArray make_dictionary_encoded_arrow_array(
        std::int64_t size,
        std::optional<validity_bitmap>&& bitmap,
        buffer<uint8_t>&& keys_buf,
        sparrow::array&& values
    )
    {
        auto [null_count, bitmap_buf] = extract_bitmap(std::move(bitmap));
        ArrowArray value_arr = extract_arrow_array(std::move(values));
        static const repeat_view<bool> children_ownership{true, 0};
        std::vector<buffer<uint8_t>> buffers;
        buffers.reserve(2);
        buffers.emplace_back(std::move(bitmap_buf));
        buffers.emplace_back(std::move(keys_buf));
        return make_arrow_array(
            size,
            null_count,
            0,
            std::move(buffers),
            nullptr,
            children_ownership,
            new ArrowArray(std::move(value_arr)),
            true
        );
    }

    // ─── Null array ───────────────────────────────────────────────────────────

    ArrowArray make_null_arrow_array(std::int64_t length)
    {
        std::vector<buffer<uint8_t>> empty_buffers;
        return make_arrow_array(
            length,
            length,  // all values are null
            0,
            std::move(empty_buffers),
            nullptr,
            repeat_view<bool>(false, 0),
            nullptr,
            false
        );
    }

    // ─── Variable-size binary view array ─────────────────────────────────────

    ArrowArray make_variable_size_binary_view_arrow_array(
        std::int64_t size,
        std::optional<validity_bitmap>&& bitmap,
        std::vector<buffer<uint8_t>>&& data_buffers
    )
    {
        auto [null_count, bitmap_buf] = extract_bitmap(std::move(bitmap));
        std::vector<buffer<uint8_t>> buffers;
        buffers.reserve(1 + data_buffers.size());
        buffers.emplace_back(std::move(bitmap_buf));
        for (auto& data_buffer : data_buffers)
        {
            buffers.emplace_back(std::move(data_buffer));
        }
        return make_arrow_array(
            size,
            null_count,
            0,
            std::move(buffers),
            nullptr,
            repeat_view<bool>(true, 0),
            nullptr,
            true
        );
    }

    // ─── Run-end encoded array ────────────────────────────────────────────────

    ArrowArray make_run_end_encoded_arrow_array(
        std::int64_t length,
        std::int64_t null_count,
        sparrow::array&& acc_lengths,
        sparrow::array&& encoded_values
    )
    {
        ArrowArray acc_arr = extract_arrow_array(std::move(acc_lengths));
        ArrowArray ev_arr = extract_arrow_array(std::move(encoded_values));
        ArrowArray** ptrs = new ArrowArray*[2];
        ptrs[0] = new ArrowArray(std::move(acc_arr));
        ptrs[1] = new ArrowArray(std::move(ev_arr));
        std::vector<buffer<uint8_t>> empty_buffers;

        return make_arrow_array(
            length,
            null_count,
            0,
            std::move(empty_buffers),
            ptrs,
            repeat_view<bool>(true, 2),
            nullptr,
            true
        );
    }

    // ─── List arrays ─────────────────────────────────────────────────────────

    ArrowArray make_list_arrow_array(
        std::int64_t size,
        std::optional<validity_bitmap>&& bitmap,
        std::vector<buffer<std::uint8_t>>&& extra_buffs,
        sparrow::array&& flat_values
    )
    {
        auto [null_count, bitmap_buf] = extract_bitmap(std::move(bitmap));
        ArrowArray flat_arr = extract_arrow_array(std::move(flat_values));
        // Build final buffer list: bitmap first, then extra_buffs.
        // Use move-construction to preserve each buffer's original data pointer
        // regardless of allocator type (move-assignment may copy when allocators differ).
        std::vector<buffer<uint8_t>> buffs;
        buffs.reserve(1 + extra_buffs.size());
        buffs.emplace_back(std::move(bitmap_buf));
        for (auto& buf : extra_buffs)
        {
            buffs.emplace_back(std::move(buf));
        }
        const repeat_view<bool> children_ownership{true, 1};
        return make_arrow_array(
            size,
            null_count,
            0,
            std::move(buffs),
            new ArrowArray*[1]{new ArrowArray(std::move(flat_arr))},
            children_ownership,
            nullptr,
            true
        );
    }

}  // namespace sparrow::detail
