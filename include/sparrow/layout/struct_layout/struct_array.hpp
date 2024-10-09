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
#include "sparrow/layout/array_base.hpp"
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

    class struct_array final : public array_crtp_base<struct_array>
    {
    public:

        using self_type = struct_array;
        using base_type = array_crtp_base<self_type>;
        using inner_types = array_inner_types<self_type>;
        using value_iterator = typename inner_types::value_iterator;
        using const_value_iterator = typename inner_types::const_value_iterator;
        using size_type = typename base_type::size_type;

        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_reference = typename base_type::bitmap_reference;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;

        using bitmap_range = base_type::bitmap_range;
        using const_bitmap_range = base_type::const_bitmap_range;

        using inner_value_type = struct_value;
        using inner_reference = struct_value;
        using inner_const_reference = struct_value;

        using value_type = nullable<inner_value_type>;
        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using iterator_tag = std::contiguous_iterator_tag;

        explicit struct_array(arrow_proxy proxy);

        const array_wrapper* raw_child(std::size_t i) const;
        array_wrapper* raw_child(std::size_t i);

    private:

        value_iterator value_begin();
        value_iterator value_end();
        const_value_iterator value_cbegin() const;
        const_value_iterator value_cend() const;
        inner_reference value(size_type i);
        inner_const_reference value(size_type i) const;

        bitmap_type::iterator bitmap_begin_impl();
        bitmap_type::const_iterator bitmap_begin_impl() const;

        // data members
        std::vector<cloning_ptr<array_wrapper>> m_children;
        bitmap_type m_bitmap;

        // friend classes
        friend class array_crtp_base<self_type>;

        // needs access to this->value(i)
        friend class detail::layout_value_functor<self_type, inner_value_type>;
        friend class detail::layout_value_functor<const self_type, inner_value_type>;
    };

    inline struct_array::struct_array(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , m_children(this->storage().children().size(), nullptr)
        , m_bitmap(make_simple_bitmap(storage()))
    {
        for (std::size_t i = 0; i < m_children.size(); ++i)
        {
            m_children[i] = array_factory(this->storage().children()[i].view());
        }
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

    auto struct_array::bitmap_begin_impl() -> bitmap_type::iterator
    {
        return next(m_bitmap.begin(), storage().offset());
    }

    auto struct_array::bitmap_begin_impl() const -> bitmap_type::const_iterator
    {
        return next(m_bitmap.begin(), storage().offset());
    }
}
