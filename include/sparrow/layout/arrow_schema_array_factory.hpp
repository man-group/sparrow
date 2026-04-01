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

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/arrow_interface/arrow_flag_utils.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/buffer.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/layout/layout_concept.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/config/config.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/metadata.hpp"
#include "sparrow/utils/repeat_container.hpp"

// Forward declaration: the concrete sparrow::array type may not be fully visible here
// (to avoid circular includes with array_registry.hpp).
// schema functions use a layout_or_array template; array functions use forward-declared array&&.
namespace sparrow
{
    class array;
}

namespace sparrow::detail
{
    // ─── Helpers ───────────────────────────────────────────────────────────────

    inline std::optional<std::unordered_set<ArrowFlag>> nullable_to_flags(bool nullable)
    {
        return nullable ? std::make_optional<std::unordered_set<ArrowFlag>>({ArrowFlag::NULLABLE})
                       : std::nullopt;
    }

    // Extracts null_count and bitmap buffer from an optional validity_bitmap.
    // The returned buffer<uint8_t> is valid (possibly zero-length) for use in make_arrow_array.
    inline std::pair<std::int64_t, buffer<uint8_t>> extract_bitmap(std::optional<validity_bitmap>&& bitmap)
    {
        if (bitmap.has_value())
        {
            const auto null_count = static_cast<std::int64_t>(bitmap->null_count());
            return {null_count, std::move(*bitmap).extract_storage()};
        }
        return {0, buffer<uint8_t>{nullptr, 0, buffer<uint8_t>::default_allocator()}};
    }

    // ─── Primitive array ───────────────────────────────────────────────────────

    template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
    [[nodiscard]] ArrowSchema make_primitive_arrow_schema(
        std::string_view format,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        return make_arrow_schema(
            format,
            std::move(name),
            std::move(metadata),
            nullable_to_flags(nullable),
            nullptr,
            repeat_view<bool>(true, 0),
            nullptr,
            true
        );
    }

    SPARROW_API ArrowArray make_primitive_arrow_array(
        std::int64_t size,
        std::optional<validity_bitmap>&& bitmap,
        buffer<uint8_t>&& data_buf
    );

    // ─── Fixed-width binary array ─────────────────────────────────────────────

    template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
    [[nodiscard]] ArrowSchema make_fixed_width_binary_arrow_schema(
        std::string_view format,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        return make_arrow_schema(
            std::move(format),
            std::move(name),
            std::move(metadata),
            nullable_to_flags(nullable),
            nullptr,
            repeat_view<bool>(true, 0),
            nullptr,
            true
        );
    }

    SPARROW_API ArrowArray make_fixed_width_binary_arrow_array(
        std::int64_t element_count,
        std::optional<validity_bitmap>&& bitmap,
        buffer<uint8_t>&& data_buf
    );

    // ─── Variable-size binary array ───────────────────────────────────────────

    template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
    [[nodiscard]] ArrowSchema make_variable_size_binary_arrow_schema(
        std::string_view format,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        return make_arrow_schema(
            std::move(format),
            std::move(name),
            std::move(metadata),
            nullable_to_flags(nullable),
            nullptr,
            repeat_view<bool>(true, 0),
            nullptr,
            true
        );
    }

    SPARROW_API ArrowArray make_variable_size_binary_arrow_array(
        std::int64_t size,
        std::optional<validity_bitmap>&& bitmap,
        buffer<uint8_t>&& offset_buf,
        buffer<uint8_t>&& data_buf
    );

    // ─── Variable-size binary view array ─────────────────────────────────────

    template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
    [[nodiscard]] ArrowSchema make_variable_size_binary_view_arrow_schema(
        std::string_view format,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        return make_arrow_schema(
            std::string(format),
            std::move(name),
            std::move(metadata),
            nullable_to_flags(nullable),
            nullptr,
            repeat_view<bool>(true, 0),
            nullptr,
            true
        );
    }

    SPARROW_API ArrowArray make_variable_size_binary_view_arrow_array(
        std::int64_t size,
        std::optional<validity_bitmap>&& bitmap,
        std::vector<buffer<uint8_t>>&& data_buffers
    );

    // ─── Decimal array ────────────────────────────────────────────────────────

    template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
    [[nodiscard]] ArrowSchema make_decimal_arrow_schema(
        std::string_view format,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        return make_arrow_schema(
            std::move(format),
            std::move(name),
            std::move(metadata),
            nullable_to_flags(nullable),
            nullptr,
            repeat_view<bool>(true, 0),
            nullptr,
            true
        );
    }

    SPARROW_API ArrowArray make_decimal_arrow_array(
        std::int64_t size,
        std::optional<validity_bitmap>&& bitmap,
        buffer<uint8_t>&& data_buf
    );

    // ─── Timestamp array ──────────────────────────────────────────────────────

    template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
    [[nodiscard]] ArrowSchema make_timestamp_arrow_schema(
        std::string_view format,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        return make_arrow_schema(
            std::move(format),
            std::move(name),
            std::move(metadata),
            nullable_to_flags(nullable),
            nullptr,
            repeat_view<bool>(true, 0),
            nullptr,
            true
        );
    }

    SPARROW_API ArrowArray make_timestamp_arrow_array(
        std::int64_t size,
        std::optional<validity_bitmap>&& bitmap,
        buffer<uint8_t>&& data_buf
    );

    // ─── Struct array ─────────────────────────────────────────────────────────
    // children is taken by lvalue reference: extract_arrow_schema is called on each child,
    // leaving each child's ArrowArray intact for a subsequent make_struct_arrow_array call.

    template <
        std::ranges::input_range CHILDREN_RANGE,
        input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        requires std::same_as<std::ranges::range_value_t<CHILDREN_RANGE>, sparrow::array>
    [[nodiscard]] ArrowSchema make_struct_arrow_schema(
        CHILDREN_RANGE& children,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        std::vector<ArrowSchema> child_schemas;
        if constexpr (std::ranges::sized_range<CHILDREN_RANGE>)
        {
            child_schemas.reserve(std::ranges::size(children));
        }
        for (auto& child : children)
        {
            child_schemas.push_back(array_access::get_arrow_proxy(child).extract_schema());
        }
        const std::size_t n = child_schemas.size();
        ArrowSchema** schemas_ptr = new ArrowSchema*[n];
        for (std::size_t i = 0; i < n; ++i)
        {
            schemas_ptr[i] = new ArrowSchema(std::move(child_schemas[i]));
        }
        return make_arrow_schema(
            std::string("+s"),
            std::move(name),
            std::move(metadata),
            nullable_to_flags(nullable),
            schemas_ptr,
            repeat_view<bool>(true, n),
            nullptr,
            true
        );
    }

    // children is consumed: extract_arrow_array is called on each child (after schemas have been extracted).
    template <std::ranges::input_range CHILDREN_RANGE>
        requires std::same_as<std::ranges::range_value_t<CHILDREN_RANGE>, sparrow::array>
    [[nodiscard]] ArrowArray make_struct_arrow_array(
        std::int64_t size,
        std::optional<validity_bitmap>&& bitmap,
        CHILDREN_RANGE&& children
    )
    {
        auto [null_count, bitmap_buf] = extract_bitmap(std::move(bitmap));
        std::vector<ArrowArray> child_arrays;
        if constexpr (std::ranges::sized_range<CHILDREN_RANGE>)
        {
            child_arrays.reserve(std::ranges::size(children));
        }
        for (auto&& child : children)
        {
            child_arrays.push_back(array_access::get_arrow_proxy(child).extract_array());
        }
        const std::size_t n = child_arrays.size();
        ArrowArray** ptrs = new ArrowArray*[n];
        for (std::size_t i = 0; i < n; ++i)
        {
            ptrs[i] = new ArrowArray(std::move(child_arrays[i]));
        }
        std::vector<buffer<uint8_t>> buffers;
        buffers.reserve(1);
        buffers.emplace_back(std::move(bitmap_buf));
        return make_arrow_array(
            size,
            null_count,
            0,
            std::move(buffers),
            ptrs,
            repeat_view<bool>(true, n),
            nullptr,
            true
        );
    }

    // ─── Map array ────────────────────────────────────────────────────────────
    // entries is taken by lvalue reference: extract_arrow_schema is called to build the schema,
    // leaving entries' ArrowArray intact for make_map_arrow_array.

    template <typename ENTRY_ARRAY, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
    [[nodiscard]] ArrowSchema make_map_arrow_schema(
        ENTRY_ARRAY& entries,
        bool nullable,
        bool keys_sorted,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        ArrowSchema entries_schema = array_access::get_arrow_proxy(entries).extract_schema();
        std::optional<std::unordered_set<ArrowFlag>> flags = nullable_to_flags(nullable);
        if (keys_sorted)
        {
            if (!flags.has_value())
            {
                flags.emplace();
            }
            flags->insert(ArrowFlag::MAP_KEYS_SORTED);
        }
        const repeat_view<bool> children_ownership{true, 1};
        return make_arrow_schema(
            std::string("+m"),
            std::move(name),
            std::move(metadata),
            std::move(flags),
            new ArrowSchema*[1]{new ArrowSchema(std::move(entries_schema))},
            children_ownership,
            nullptr,
            true
        );
    }

    // entries is consumed: extract_arrow_array is called (after schema was extracted).
    SPARROW_API ArrowArray make_map_arrow_array(
        std::int64_t size,
        std::optional<validity_bitmap>&& bitmap,
        buffer<uint8_t>&& offset_buf,
        sparrow::array&& entries
    );

    // ─── Union arrays (dense and sparse) ─────────────────────────────────────
    // children is taken by lvalue reference (schema extraction only).

    template <
        std::ranges::input_range CHILDREN_RANGE,
        input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        requires std::same_as<std::ranges::range_value_t<CHILDREN_RANGE>, sparrow::array>
    [[nodiscard]] ArrowSchema make_union_arrow_schema(
        std::string_view format,
        CHILDREN_RANGE& children,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        std::vector<ArrowSchema> child_schemas;
        if constexpr (std::ranges::sized_range<CHILDREN_RANGE>)
        {
            child_schemas.reserve(std::ranges::size(children));
        }
        for (auto& child : children)
        {
            child_schemas.push_back(array_access::get_arrow_proxy(child).extract_schema());
        }
        const bool is_nullable = !child_schemas.empty()
                                 && std::all_of(
                                     child_schemas.begin(),
                                     child_schemas.end(),
                                     [](const ArrowSchema& s)
                                     {
                                         return to_set_of_ArrowFlags(s.flags).contains(ArrowFlag::NULLABLE);
                                     }
                                 );
        const std::size_t n = child_schemas.size();
        ArrowSchema** schemas_ptr = new ArrowSchema*[n];
        for (std::size_t i = 0; i < n; ++i)
        {
            schemas_ptr[i] = new ArrowSchema(std::move(child_schemas[i]));
        }
        return make_arrow_schema(
            format,
            std::move(name),
            std::move(metadata),
            nullable_to_flags(is_nullable),
            schemas_ptr,
            repeat_view<bool>(true, n),
            nullptr,
            true
        );
    }

    // children is consumed: extract_arrow_array is called on each child.
    template <std::ranges::input_range CHILDREN_RANGE>
        requires std::same_as<std::ranges::range_value_t<CHILDREN_RANGE>, sparrow::array>
    [[nodiscard]] ArrowArray make_union_arrow_array(
        std::int64_t size,
        std::vector<buffer<uint8_t>>&& buffers,
        CHILDREN_RANGE&& children
    )
    {
        std::vector<ArrowArray> child_arrays;
        if constexpr (std::ranges::sized_range<CHILDREN_RANGE>)
        {
            child_arrays.reserve(std::ranges::size(children));
        }
        for (auto&& child : children)
        {
            child_arrays.push_back(array_access::get_arrow_proxy(child).extract_array());
        }
        const std::size_t n = child_arrays.size();
        ArrowArray** ptrs = new ArrowArray*[n];
        for (std::size_t i = 0; i < n; ++i)
        {
            ptrs[i] = new ArrowArray(std::move(child_arrays[i]));
        }
        return make_arrow_array(
            size,
            0,  // unions have no validity bitmap; nullability lives in individual children
            0,
            std::move(buffers),
            ptrs,
            repeat_view<bool>(true, n),
            nullptr,
            true
        );
    }

    // ─── Dictionary-encoded array ─────────────────────────────────────────────
    // values is taken by lvalue reference: extract_arrow_schema is called to build the schema,
    // leaving values' ArrowArray intact for make_dictionary_encoded_arrow_array.

    template <typename VALUE_ARRAY, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
    [[nodiscard]] ArrowSchema make_dictionary_encoded_arrow_schema(
        std::string_view format,
        bool nullable,
        VALUE_ARRAY& values,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        ArrowSchema value_schema = array_access::get_arrow_proxy(values).extract_schema();
        static const repeat_view<bool> children_ownership{true, 0};
        return make_arrow_schema(
            format,
            std::move(name),
            std::move(metadata),
            nullable_to_flags(nullable),
            nullptr,
            children_ownership,
            new ArrowSchema(std::move(value_schema)),
            true
        );
    }

    // values is consumed: extract_arrow_array is called (after schema was extracted).
    SPARROW_API ArrowArray make_dictionary_encoded_arrow_array(
        std::int64_t size,
        std::optional<validity_bitmap>&& bitmap,
        buffer<uint8_t>&& keys_buf,
        sparrow::array&& values
    );

    // ─── Null array ───────────────────────────────────────────────────────────

    template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
    [[nodiscard]] ArrowSchema make_null_arrow_schema(
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        using namespace std::literals;
        static const std::optional<std::unordered_set<sparrow::ArrowFlag>> flags{{ArrowFlag::NULLABLE}};
        return make_arrow_schema(
            "n"sv,
            std::move(name),
            std::move(metadata),
            flags,
            0,
            repeat_view<bool>(false, 0),
            nullptr,
            false
        );
    }

    SPARROW_API ArrowArray make_null_arrow_array(std::int64_t length);

    // ─── Run-end encoded array ────────────────────────────────────────────────
    // acc_lengths and encoded_values are taken by lvalue reference: extract_arrow_schema is called
    // on each, leaving their ArrowArrays intact for make_run_end_encoded_arrow_array.

    template <
        typename ACC_ARRAY,
        typename EV_ARRAY,
        input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
    [[nodiscard]] ArrowSchema make_run_end_encoded_arrow_schema(
        ACC_ARRAY& acc_lengths,
        EV_ARRAY& encoded_values,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        ArrowSchema acc_sch = array_access::get_arrow_proxy(acc_lengths).extract_schema();
        ArrowSchema ev_sch  = array_access::get_arrow_proxy(encoded_values).extract_schema();
        // Nullable flag is derived from the encoded-values child schema.
        std::optional<std::unordered_set<ArrowFlag>> flags =
            std::make_optional(to_set_of_ArrowFlags(ev_sch.flags));
        ArrowSchema** schemas_ptr = new ArrowSchema*[2];
        schemas_ptr[0] = new ArrowSchema(std::move(acc_sch));
        schemas_ptr[1] = new ArrowSchema(std::move(ev_sch));
        return make_arrow_schema(
            std::string("+r"),
            std::move(name),
            std::move(metadata),
            std::move(flags),
            schemas_ptr,
            repeat_view<bool>(true, 2),
            nullptr,
            true
        );
    }

    // Both arrays are consumed: extract_arrow_array is called on each.
    // `null_count` and `length` must be pre-computed (e.g. via extract_length_and_null_count)
    // before any extraction is performed on acc_lengths / encoded_values.
    SPARROW_API ArrowArray make_run_end_encoded_arrow_array(
        std::int64_t length,
        std::int64_t null_count,
        sparrow::array&& acc_lengths,
        sparrow::array&& encoded_values
    );

    // ─── List arrays (list, list-view, fixed-sized-list) ─────────────────────
    // flat_values is taken by lvalue reference: extract_arrow_schema is called,
    // leaving flat_values' ArrowArray intact for make_list_arrow_array.

    template <typename FLAT_ARRAY, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
    [[nodiscard]] ArrowSchema make_list_arrow_schema(
        std::string_view format,
        FLAT_ARRAY& flat_values,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata,
        bool nullable
    )
    {
        ArrowSchema flat_schema = array_access::get_arrow_proxy(flat_values).extract_schema();
        const repeat_view<bool> children_ownership{true, 1};
        std::optional<std::unordered_set<ArrowFlag>> flags =
            nullable ? std::optional<std::unordered_set<ArrowFlag>>{{ArrowFlag::NULLABLE}} : std::nullopt;
        return make_arrow_schema(
            format,
            std::move(name),
            std::move(metadata),
            std::move(flags),
            new ArrowSchema*[1]{new ArrowSchema(std::move(flat_schema))},
            children_ownership,
            nullptr,
            true
        );
    }

    // flat_values is consumed: extract_arrow_array is called (after schema was extracted).
    // `extra_buffs` must NOT include the validity bitmap; it is prepended by this function.
    SPARROW_API ArrowArray make_list_arrow_array(
        std::int64_t size,
        std::optional<validity_bitmap>&& bitmap,
        std::vector<buffer<std::uint8_t>>&& extra_buffs,
        sparrow::array&& flat_values
    );

}  // namespace sparrow::detail
