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

#include <ranges>
#include <string_view>
#include <version>

#include "sparrow/utils/mp_utils.hpp"

#if defined(__cpp_lib_format)
#    include "sparrow/utils/format.hpp"
#endif

#include "sparrow/array_api.hpp"
#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/memory.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{
    class struct_array;

    template <>
    struct array_inner_types<struct_array> : array_inner_types_base
    {
        using array_type = struct_array;
        using inner_value_type = struct_value;
        using inner_reference = struct_value;
        using inner_const_reference = struct_value;
        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_value_type>>;
        using const_value_iterator = functor_index_iterator<
            detail::layout_value_functor<const array_type, inner_value_type>>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    /**
     * Checks whether T is a struct_array type.
     */
    template <class T>
    constexpr bool is_struc_array_v = std::same_as<T, struct_array>;

    class struct_array final : public array_bitmap_base<struct_array>
    {
    public:

        using self_type = struct_array;
        using base_type = array_bitmap_base<self_type>;
        using inner_types = array_inner_types<self_type>;
        using value_iterator = typename inner_types::value_iterator;
        using const_value_iterator = typename inner_types::const_value_iterator;
        using size_type = typename base_type::size_type;

        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;

        using const_bitmap_range = base_type::const_bitmap_range;

        using inner_value_type = struct_value;
        using inner_reference = struct_value;
        using inner_const_reference = struct_value;

        using value_type = nullable<inner_value_type>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using iterator_tag = base_type::iterator_tag;

        SPARROW_API explicit struct_array(arrow_proxy proxy);

        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<struct_array, Args...>)
        explicit struct_array(Args&&... args)
            : struct_array(create_proxy(std::forward<Args>(args)...))
        {
        }

        SPARROW_API struct_array(const struct_array&);
        SPARROW_API struct_array& operator=(const struct_array&);

        struct_array(struct_array&&) = default;
        struct_array& operator=(struct_array&&) = default;

        [[nodiscard]] SPARROW_API size_type children_count() const;

        [[nodiscard]] SPARROW_API const array_wrapper* raw_child(std::size_t i) const;
        [[nodiscard]] SPARROW_API array_wrapper* raw_child(std::size_t i);

    private:

        template <
            std::ranges::input_range CHILDREN_RANGE,
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::same_as<std::ranges::range_value_t<CHILDREN_RANGE>, array>
        [[nodiscard]] static auto create_proxy(
            CHILDREN_RANGE&& children,
            VB&& bitmaps,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        template <std::ranges::input_range CHILDREN_RANGE, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::same_as<std::ranges::range_value_t<CHILDREN_RANGE>, array>
        [[nodiscard]] static auto create_proxy(
            CHILDREN_RANGE&& children,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        template <std::ranges::input_range CHILDREN_RANGE, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::same_as<std::ranges::range_value_t<CHILDREN_RANGE>, array>
        [[nodiscard]] static auto create_proxy_impl(
            CHILDREN_RANGE&& children,
            std::optional<validity_bitmap>&& bitmap,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        using children_type = std::vector<cloning_ptr<array_wrapper>>;

        [[nodiscard]] SPARROW_API value_iterator value_begin();
        [[nodiscard]] SPARROW_API value_iterator value_end();
        [[nodiscard]] SPARROW_API const_value_iterator value_cbegin() const;
        [[nodiscard]] SPARROW_API const_value_iterator value_cend() const;
        [[nodiscard]] SPARROW_API inner_reference value(size_type i);
        [[nodiscard]] SPARROW_API inner_const_reference value(size_type i) const;

        [[nodiscard]] SPARROW_API children_type make_children();

        // data members
        children_type m_children;

        // friend classes
        friend class array_crtp_base<self_type>;

        // needs access to this->value(i)
        friend class detail::layout_value_functor<self_type, inner_value_type>;
        friend class detail::layout_value_functor<const self_type, inner_value_type>;
    };

    template <std::ranges::input_range CHILDREN_RANGE, validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
        requires std::same_as<std::ranges::range_value_t<CHILDREN_RANGE>, array>
    auto struct_array::create_proxy(
        CHILDREN_RANGE&& children,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        const auto size = children.empty() ? 0 : children[0].size();
        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<VB>(validity_input));
        return create_proxy_impl(
            std::forward<std::vector<array>>(children),
            std::move(vbitmap),
            std::move(name),
            std::move(metadata)
        );
    }

    template <std::ranges::input_range CHILDREN_RANGE, input_metadata_container METADATA_RANGE>
        requires std::same_as<std::ranges::range_value_t<CHILDREN_RANGE>, array>
    auto struct_array::create_proxy(
        CHILDREN_RANGE&& children,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        const size_t size = children.empty() ? 0 : children[0].size();
        return create_proxy_impl(
            std::forward<std::vector<array>>(children),
            nullable ? std::make_optional<validity_bitmap>(nullptr, size) : std::nullopt,
            std::move(name),
            std::move(metadata)
        );
    }

    template <std::ranges::input_range CHILDREN_RANGE, input_metadata_container METADATA_RANGE>
        requires std::same_as<std::ranges::range_value_t<CHILDREN_RANGE>, array>
    auto struct_array::create_proxy_impl(
        CHILDREN_RANGE&& children,
        std::optional<validity_bitmap>&& bitmap,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        const auto n_children = children.size();
        ArrowSchema** child_schemas = new ArrowSchema*[n_children];
        ArrowArray** child_arrays = new ArrowArray*[n_children];

        const auto size = children.empty() ? 0 : children[0].size();

        for (std::size_t i = 0; i < n_children; ++i)
        {
            auto& child = children[i];
            SPARROW_ASSERT_TRUE(child.size() == size);
            auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(child));
            child_arrays[i] = new ArrowArray(std::move(flat_arr));
            child_schemas[i] = new ArrowSchema(std::move(flat_schema));
        }

        const bool bitmap_has_value = bitmap.has_value();
        const auto null_count = bitmap_has_value ? bitmap->null_count() : 0;
        const auto flags = bitmap_has_value
                               ? std::make_optional<std::unordered_set<sparrow::ArrowFlag>>({ArrowFlag::NULLABLE})
                               : std::nullopt;

        ArrowSchema schema = make_arrow_schema(
            std::string("+s"),                    // format
            std::move(name),                      // name
            std::move(metadata),                  // metadata
            flags,                                // flags,
            child_schemas,                        // children
            repeat_view<bool>(true, n_children),  // children_ownership
            nullptr,                              // dictionary
            true                                  // dictionary ownership
        );

        buffer<uint8_t> bitmap_buffer = bitmap_has_value ? std::move(*bitmap).extract_storage()
                                                         : buffer<uint8_t>{nullptr, 0};

        std::vector<buffer<std::uint8_t>> arr_buffs(1);
        arr_buffs[0] = std::move(bitmap_buffer);

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),        // length
            static_cast<std::int64_t>(null_count),  // null_count
            0,                                      // offset
            std::move(arr_buffs),
            child_arrays,                         // children
            repeat_view<bool>(true, n_children),  // children_ownership
            nullptr,                              // dictionary
            true                                  // dictionary ownership
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }
}

#if defined(__cpp_lib_format)

template <>
struct std::formatter<sparrow::struct_array>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    SPARROW_API auto format(const sparrow::struct_array& struct_array, std::format_context& ctx) const
        -> decltype(ctx.out());
};

namespace sparrow
{
    SPARROW_API std::ostream& operator<<(std::ostream& os, const sparrow::struct_array& value);
}

#endif
