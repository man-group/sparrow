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

#include "sparrow/array_factory.hpp"
#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/memory.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/array_api.hpp"

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


        template <class ... Args>
        requires(mpl::excludes_copy_and_move_ctor_v<struct_array, Args...>)
        explicit struct_array(Args&& ... args)
            : struct_array(create_proxy(std::forward<Args>(args) ...))
        {}

        struct_array(const struct_array&);
        struct_array& operator=(const struct_array&);

        struct_array(struct_array&&) = default;
        struct_array& operator=(struct_array&&) = default;

        const array_wrapper* raw_child(std::size_t i) const;
        array_wrapper* raw_child(std::size_t i);

    private:

        template <validity_bitmap_input VB = validity_bitmap>
        static auto create_proxy(
            std::vector<array> && children,
            VB && bitmaps = validity_bitmap{}
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
        std::vector<array> && children,
        VB && validity_input
    ) -> arrow_proxy
    {
        const auto n_children = children.size();
        ArrowSchema** child_schemas = new ArrowSchema*[n_children];
        ArrowArray** child_arrays = new ArrowArray*[n_children];

        const auto size = children[0].size();
        
        for(std::size_t i=0; i<n_children; ++i)
        {
            auto & child = children[i];
            SPARROW_ASSERT_TRUE(child.size() == size);
            auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(child));
            child_arrays[i] = new ArrowArray(std::move(flat_arr));
            child_schemas[i] = new ArrowSchema(std::move(flat_schema));
        }

        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<VB>(validity_input));
        const auto null_count = vbitmap.null_count();
        
        ArrowSchema schema = make_arrow_schema(
            std::string("+s"), // format
            std::nullopt, // name
            std::nullopt, // metadata
            std::nullopt, // flags,
            static_cast<int64_t>(n_children),
            child_schemas, // children
            nullptr // dictionary
        );

        std::vector<buffer<std::uint8_t>> arr_buffs = {
            std::move(vbitmap).extract_storage()
        };

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size), // length
            static_cast<std::int64_t>(null_count),
            0, // offset
            std::move(arr_buffs),
            static_cast<std::size_t>(n_children), // n_children
            child_arrays, // children
            nullptr // dictionary
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    inline auto struct_array::raw_child(std::size_t i) const -> const array_wrapper*
    {
        return m_children[i].get();
    }

    inline auto struct_array::raw_child(std::size_t i) -> array_wrapper*
    {
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
