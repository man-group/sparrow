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

#include "sparrow/array_api.hpp"
#include "sparrow/array_factory.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/layout/struct_layout/struct_array.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"

namespace sparrow
{
    class map_array;

    template <>
    struct array_inner_types<map_array> : array_inner_types_base
    {
        using array_type = map_array;
        using inner_value_type = map_value;
        using inner_reference = map_value;
        using inner_const_reference = map_value;
        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_value_type>>;
        using const_value_iterator = functor_index_iterator<
            detail::layout_value_functor<const array_type, inner_value_type>>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    template <class T>
    constexpr bool is_map_array_v = std::same_as<T, map_array>;

    class SPARROW_API map_array final : public array_bitmap_base<map_array>
    {
    public:

        using self_type = map_array;
        using base_type = array_bitmap_base<self_type>;
        using inner_types = array_inner_types<self_type>;
        using value_iterator = inner_types::value_iterator;
        using const_value_iterator = inner_types::const_value_iterator;
        using size_type = typename base_type::size_type;
        using offset_type = const std::int32_t;
        using offset_buffer_type = u8_buffer<std::remove_const_t<offset_type>>;

        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;

        using const_bitmap_range = typename base_type::const_bitmap_range;

        using inner_value_type = inner_types::inner_value_type;
        using inner_reference = inner_types::inner_reference;
        using inner_const_reference = inner_types::inner_const_reference;

        using value_type = nullable<inner_value_type>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using iterator_tag = typename base_type::iterator_tag;

        explicit map_array(arrow_proxy proxy);

        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<map_array, Args...>)
        explicit map_array(Args&&... args)
            : self_type(create_proxy(std::forward<Args>(args)...))
        {
        }

        map_array(const self_type&);
        map_array& operator=(const self_type&);

        map_array(map_array&&) noexcept = default;
        map_array& operator=(map_array&&) noexcept = default;

        [[nodiscard]] const array_wrapper* raw_keys_array() const;
        [[nodiscard]] array_wrapper* raw_keys_array();

        [[nodiscard]] const array_wrapper* raw_items_array() const;
        [[nodiscard]] array_wrapper* raw_items_array();

        template <std::ranges::range SIZES_RANGE>
        [[nodiscard]] static auto offset_from_sizes(SIZES_RANGE&& sizes) -> offset_buffer_type;

    private:

        [[nodiscard]] value_iterator value_begin();
        [[nodiscard]] value_iterator value_end();
        [[nodiscard]] const_value_iterator value_cbegin() const;
        [[nodiscard]] const_value_iterator value_cend() const;

        [[nodiscard]] inner_reference value(size_type i);
        [[nodiscard]] inner_const_reference value(size_type i) const;

        [[nodiscard]] offset_type* make_list_offsets() const;
        [[nodiscard]] cloning_ptr<array_wrapper> make_entries_array() const;
        [[nodiscard]] bool get_keys_sorted() const;

        [[nodiscard]] static bool check_keys_sorted(const array& flat_keys, const offset_buffer_type& offsets);

        template <
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_keys,
            array&& flat_items,
            offset_buffer_type&& list_offsets,
            VB&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        template <
            validity_bitmap_input VB = validity_bitmap,
            std::ranges::input_range OFFSET_BUFFER_RANGE,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<std::ranges::range_value_t<OFFSET_BUFFER_RANGE>, offset_type>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_keys,
            array&& flat_items,
            OFFSET_BUFFER_RANGE&& list_offsets_range,
            VB&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
        {
            offset_buffer_type list_offsets{std::move(list_offsets_range)};
            return map_array::create_proxy(
                std::move(flat_keys),
                std::move(flat_items),
                std::move(list_offsets),
                std::forward<VB>(validity_input),
                std::forward<std::optional<std::string_view>>(name),
                std::forward<std::optional<METADATA_RANGE>>(metadata)
            );
        }

        template <
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_keys,
            array&& flat_values,
            offset_buffer_type&& list_offsets,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        template <
            validity_bitmap_input VB = validity_bitmap,
            std::ranges::input_range OFFSET_BUFFER_RANGE,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<std::ranges::range_value_t<OFFSET_BUFFER_RANGE>, offset_type>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_keys,
            array&& flat_items,
            OFFSET_BUFFER_RANGE&& list_offsets_range,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
        {
            offset_buffer_type list_offsets{std::move(list_offsets_range)};
            return map_array::create_proxy(
                std::move(flat_keys),
                std::move(flat_items),
                std::move(list_offsets),
                nullable,
                std::forward<std::optional<std::string_view>>(name),
                std::forward<std::optional<METADATA_RANGE>>(metadata)
            );
        }

        static constexpr std::size_t OFFSET_BUFFER_INDEX = 1;
        offset_type* p_list_offsets;

        cloning_ptr<array_wrapper> p_entries_array;
        bool m_keys_sorted;

        // friend classes
        friend class array_crtp_base<map_array>;
        friend class detail::layout_value_functor<const map_array, map_value>;
    };

    template <std::ranges::range SIZES_RANGE>
    auto map_array::offset_from_sizes(SIZES_RANGE&& sizes) -> offset_buffer_type
    {
        return detail::offset_buffer_from_sizes<std::remove_const_t<offset_type>>(
            std::forward<SIZES_RANGE>(sizes)
        );
    }

    template <validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
    arrow_proxy map_array::create_proxy(
        array&& flat_keys,
        array&& flat_items,
        offset_buffer_type&& list_offsets,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto size = list_offsets.size() - 1;
        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<VB>(validity_input));

        std::optional<std::unordered_set<ArrowFlag>> flags{{ArrowFlag::NULLABLE}};
        bool keys_sorted = check_keys_sorted(flat_keys, list_offsets);
        if (keys_sorted)
        {
            flags.value().insert(ArrowFlag::MAP_KEYS_SORTED);
        }

        std::array<sparrow::array, 2> struct_children = {std::move(flat_keys), std::move(flat_items)};
        struct_array entries(std::move(struct_children), false, std::string("entries"));

        auto [entries_arr, entries_schema] = extract_arrow_structures(std::move(entries));

        const auto null_count = vbitmap.null_count();
        const repeat_view<bool> children_ownership{true, 1};

        ArrowSchema schema = make_arrow_schema(
            std::string("+m"),
            name,      // name
            metadata,  // metadata
            flags,     // flags,
            new ArrowSchema*[1]{new ArrowSchema(std::move(entries_schema))},
            children_ownership,  // children ownership
            nullptr,             // dictionary
            true                 // dictionary ownership

        );

        std::vector<buffer<std::uint8_t>> arr_buffs = {
            std::move(vbitmap).extract_storage(),
            std::move(list_offsets).extract_storage()
        };

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<std::int64_t>(null_count),
            0,  // offset
            std::move(arr_buffs),
            new ArrowArray*[1]{new ArrowArray(std::move(entries_arr))},
            children_ownership,  // children ownership
            nullptr,             // dictionary
            true                 // dictionary ownership
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
    arrow_proxy map_array::create_proxy(
        array&& flat_keys,
        array&& flat_items,
        offset_buffer_type&& list_offsets,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        if (nullable)
        {
            return map_array::create_proxy(
                std::move(flat_keys),
                std::move(flat_items),
                std::move(list_offsets),
                validity_bitmap{},
                name,
                metadata
            );
        }
        else
        {
            bool keys_sorted = check_keys_sorted(flat_keys, list_offsets);
            auto flags = keys_sorted
                             ? std::optional<std::unordered_set<ArrowFlag>>{{ArrowFlag::MAP_KEYS_SORTED}}
                             : std::nullopt;

            const auto size = list_offsets.size() - 1;

            std::array<sparrow::array, 2> struct_children = {std::move(flat_keys), std::move(flat_items)};
            struct_array entries(std::move(struct_children), false, std::string("entries"));

            auto [entries_arr, entries_schema] = extract_arrow_structures(std::move(entries));
            const repeat_view<bool> children_ownership{true, 1};

            ArrowSchema schema = make_arrow_schema(
                std::string_view("+m"),
                name,      // name
                metadata,  // metadata
                flags,     // flags,
                new ArrowSchema*[1]{new ArrowSchema(std::move(entries_schema))},
                children_ownership,  // children ownership
                nullptr,             // dictionary
                true                 // dictionary ownership

            );

            std::vector<buffer<std::uint8_t>> arr_buffs = {
                buffer<std::uint8_t>{nullptr, 0},  // no validity bitmap
                std::move(list_offsets).extract_storage()
            };

            ArrowArray arr = make_arrow_array(
                static_cast<std::int64_t>(size),  // length
                0,
                0,  // offset
                std::move(arr_buffs),
                new ArrowArray*[1]{new ArrowArray(std::move(entries_arr))},
                children_ownership,  // children ownership
                nullptr,             // dictionary
                true                 // dictionary ownership
            );
            return arrow_proxy{std::move(arr), std::move(schema)};
        }
    }
}
