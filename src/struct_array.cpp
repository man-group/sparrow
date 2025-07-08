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

#include "sparrow/struct_array.hpp"

#include "sparrow/layout/array_factory.hpp"

namespace sparrow
{
    struct_array::struct_array(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , m_children(make_children())
    {
    }

    struct_array::struct_array(const struct_array& rhs)
        : base_type(rhs)
        , m_children(make_children())
    {
    }

    struct_array& struct_array::operator=(const struct_array& rhs)
    {
        if (this != &rhs)
        {
            base_type::operator=(rhs);
            m_children = make_children();
        }
        return *this;
    }

    auto struct_array::children_count() const -> size_type
    {
        return m_children.size();
    }

    auto struct_array::raw_child(std::size_t i) const -> const array_wrapper*
    {
        SPARROW_ASSERT_TRUE(i < m_children.size());
        return m_children[i].get();
    }

    auto struct_array::raw_child(std::size_t i) -> array_wrapper*
    {
        SPARROW_ASSERT_TRUE(i < m_children.size());
        return m_children[i].get();
    }

    auto struct_array::value_begin() -> value_iterator
    {
        return value_iterator{detail::layout_value_functor<self_type, inner_value_type>{this}, 0};
    }

    auto struct_array::value_end() -> value_iterator
    {
        return value_iterator(detail::layout_value_functor<self_type, inner_value_type>(this), this->size());
    }

    auto struct_array::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator(detail::layout_value_functor<const self_type, inner_value_type>(this), 0);
    }

    auto struct_array::value_cend() const -> const_value_iterator
    {
        return const_value_iterator(
            detail::layout_value_functor<const self_type, inner_value_type>(this),
            this->size()
        );
    }

    auto struct_array::value(size_type i) -> inner_reference
    {
        return struct_value{m_children, i};
    }

    auto struct_array::value(size_type i) const -> inner_const_reference
    {
        return struct_value{m_children, i};
    }

    auto struct_array::make_children() -> children_type
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

auto std::formatter<sparrow::struct_array>::format(
    const sparrow::struct_array& struct_array,
    std::format_context& ctx
) const -> decltype(ctx.out())
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

namespace sparrow
{
    std::ostream& operator<<(std::ostream& os, const sparrow::struct_array& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

#endif
