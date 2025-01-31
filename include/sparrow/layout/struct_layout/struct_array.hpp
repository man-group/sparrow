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

#include <string_view>
#if defined(__cpp_lib_format)
#    include "sparrow/utils/format.hpp"
#endif
#include <ranges>

#include "sparrow/array_api.hpp"
#include "sparrow/array_factory.hpp"
#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/iterator.hpp"
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

        explicit struct_array(arrow_proxy proxy);

        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<struct_array, Args...>)
        explicit struct_array(Args&&... args)
            : struct_array(create_proxy(std::forward<Args>(args)...))
        {
        }

        struct_array(const struct_array&);
        struct_array& operator=(const struct_array&);

        struct_array(struct_array&&) = default;
        struct_array& operator=(struct_array&&) = default;

        [[nodiscard]] size_type children_count() const;

        const array_wrapper* raw_child(std::size_t i) const;
        array_wrapper* raw_child(std::size_t i);

    private:

        template <validity_bitmap_input VB = validity_bitmap>
        static auto create_proxy(
            std::vector<array>&& children,
            VB&& bitmaps = validity_bitmap{},
            std::optional<std::string_view> name = std::nullopt,
            std::optional<std::string_view> metadata = std::nullopt
        ) -> arrow_proxy;

        using children_type = std::vector<cloning_ptr<array_wrapper>>;

        value_iterator value_begin();
        value_iterator value_end();
        const_value_iterator value_cbegin() const;
        const_value_iterator value_cend() const;
        inner_reference value(size_type i);
        inner_const_reference value(size_type i) const;

        children_type make_children();

        // data members
        children_type m_children;

        // friend classes
        friend class array_crtp_base<self_type>;

        // needs access to this->value(i)
        friend class detail::layout_value_functor<self_type, inner_value_type>;
        friend class detail::layout_value_functor<const self_type, inner_value_type>;
    };

    inline struct_array::struct_array(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , m_children(make_children())
    {
    }

    inline struct_array::struct_array(const struct_array& rhs)
        : base_type(rhs)
        , m_children(make_children())
    {
    }

    inline struct_array& struct_array::operator=(const struct_array& rhs)
    {
        if (this != &rhs)
        {
            base_type::operator=(rhs);
            m_children = make_children();
        }
        return *this;
    }

    template <validity_bitmap_input VB>
    auto struct_array::create_proxy(
        std::vector<array>&& children,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<std::string_view> metadata
    ) -> arrow_proxy
    {
        const auto n_children = children.size();
        ArrowSchema** child_schemas = new ArrowSchema*[n_children];
        ArrowArray** child_arrays = new ArrowArray*[n_children];

        const auto size = children[0].size();

        for (std::size_t i = 0; i < n_children; ++i)
        {
            auto& child = children[i];
            SPARROW_ASSERT_TRUE(child.size() == size);
            auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(child));
            child_arrays[i] = new ArrowArray(std::move(flat_arr));
            child_schemas[i] = new ArrowSchema(std::move(flat_schema));
        }

        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<VB>(validity_input));
        const auto null_count = vbitmap.null_count();

        ArrowSchema schema = make_arrow_schema(
            std::string("+s"),    // format
            std::move(name),      // name
            std::move(metadata),  // metadata
            std::nullopt,         // flags,
            static_cast<int64_t>(n_children),
            child_schemas,  // children
            nullptr         // dictionary
        );

        std::vector<buffer<std::uint8_t>> arr_buffs = {std::move(vbitmap).extract_storage()};

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<std::int64_t>(null_count),
            0,  // offset
            std::move(arr_buffs),
            static_cast<std::size_t>(n_children),  // n_children
            child_arrays,                          // children
            nullptr                                // dictionary
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    inline auto struct_array::children_count() const -> size_type
    {
        return m_children.size();
    }

    inline auto struct_array::raw_child(std::size_t i) const -> const array_wrapper*
    {
        SPARROW_ASSERT_TRUE(i < m_children.size());
        return m_children[i].get();
    }

    inline auto struct_array::raw_child(std::size_t i) -> array_wrapper*
    {
        SPARROW_ASSERT_TRUE(i < m_children.size());
        return m_children[i].get();
    }

    inline auto struct_array::value_begin() -> value_iterator
    {
        return value_iterator{detail::layout_value_functor<self_type, inner_value_type>{this}, 0};
    }

    inline auto struct_array::value_end() -> value_iterator
    {
        return value_iterator(detail::layout_value_functor<self_type, inner_value_type>(this), this->size());
    }

    inline auto struct_array::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator(detail::layout_value_functor<const self_type, inner_value_type>(this), 0);
    }

    inline auto struct_array::value_cend() const -> const_value_iterator
    {
        return const_value_iterator(
            detail::layout_value_functor<const self_type, inner_value_type>(this),
            this->size()
        );
    }

    inline auto struct_array::value(size_type i) -> inner_reference
    {
        return struct_value{m_children, i};
    }

    inline auto struct_array::value(size_type i) const -> inner_const_reference
    {
        return struct_value{m_children, i};
    }

    inline auto struct_array::make_children() -> children_type
    {
        arrow_proxy& proxy = this->get_arrow_proxy();
        children_type children(proxy.children().size(), nullptr);
        for (std::size_t i = 0; i < children.size(); ++i)
        {
            children[i] = array_factory(proxy.children()[i].view());
        }
        return children;
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

    auto format(const sparrow::struct_array& struct_array, std::format_context& ctx) const
    {
        const auto get_names = [](const sparrow::struct_array& sa) -> std::vector<std::string>
        {
            std::vector<std::string> names;
            names.reserve(sa.children_count());
            for (std::size_t i = 0; i < sa.children_count(); ++i)
            {
                names.emplace_back(sa.raw_child(i)->get_arrow_proxy().name().value_or("N/A"));
            }
            return names;
        };

        const size_t member_count = struct_array.at(0).get().size();
        const auto result = std::views::iota(0u, member_count)
                            | std::ranges::views::transform(
                                [&struct_array](const auto index)
                                {
                                    return std::ranges::views::transform(
                                        struct_array,
                                        [index](const auto& ref) -> sparrow::array_traits::const_reference
                                        {
                                            if (ref.has_value())
                                            {
                                                return ref.value()[index];
                                            }
                                            return {};
                                        }
                                    );
                                }
                            );

        sparrow::to_table_with_columns(ctx.out(), get_names(struct_array), result);
        return ctx.out();
    }
};

inline std::ostream& operator<<(std::ostream& os, const sparrow::struct_array& value)
{
    os << std::format("{}", value);
    return os;
}

#endif
