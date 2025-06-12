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

#include "sparrow/layout/array_helper.hpp"
#include "sparrow/layout/nested_value_types.hpp"

namespace sparrow
{
    struct_value_iterator::struct_value_iterator(const struct_value* struct_value_ptr, size_type index)
        : m_struct_value_ptr(struct_value_ptr)
        , m_index(index)
    {
    }

    auto struct_value_iterator::dereference() const -> reference
    {
        return m_struct_value_ptr->operator[](m_index);
    }

    void struct_value_iterator::increment()
    {
        ++m_index;
    }

    void struct_value_iterator::decrement()
    {
        --m_index;
    }

    void struct_value_iterator::advance(difference_type n)
    {
        m_index += n;
    }

    auto struct_value_iterator::distance_to(const self_type& rhs) const -> difference_type
    {
        return rhs.m_index - m_index;
    }

    bool struct_value_iterator::equal(const self_type& rhs) const
    {
        return (m_struct_value_ptr == rhs.m_struct_value_ptr) && (m_index == rhs.m_index);
    }

    bool struct_value_iterator::less_than(const self_type& rhs) const
    {
        return m_index < rhs.m_index;
    }

    struct_value::struct_value(const std::vector<child_ptr>& children, size_type index)
        : p_children(&children)
        , m_index(index)
    {
    }

    auto struct_value::size() const -> size_type
    {
        return p_children->size();
    }

    bool struct_value::empty() const
    {
        return size() == 0;
    }

    auto struct_value::operator[](size_type i) const -> const_reference
    {
        return array_element(*(((*p_children)[i]).get()), m_index);
    }

    auto struct_value::front() const -> const_reference
    {
        return (*this)[0];
    }

    auto struct_value::back() const -> const_reference
    {
        return (*this)[size() - 1];
    }

    bool operator==(const struct_value& lhs, const struct_value& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }

    struct_value_iterator struct_value::begin()
    {
        return {this, 0};
    }

    struct_value_iterator struct_value::begin() const
    {
        return {this, 0};
    }

    struct_value_iterator struct_value::cbegin() const
    {
        return {this, 0};
    }

    struct_value_iterator struct_value::end()
    {
        return {this, size()};
    }

    struct_value_iterator struct_value::end() const
    {
        return {this, size()};
    }

    struct_value_iterator struct_value::cend() const
    {
        return {this, size()};
    }
}

#if defined(__cpp_lib_format)

auto std::formatter<sparrow::struct_value>::format(const sparrow::struct_value& ar, std::format_context& ctx) const
    -> decltype(ctx.out())
{
    std::format_to(ctx.out(), "<");
    if (!ar.empty())
    {
        for (std::size_t i = 0; i < ar.size() - 1; ++i)
        {
            std::format_to(ctx.out(), "{}, ", ar[i]);
        }
        std::format_to(ctx.out(), "{}", ar[ar.size() - 1]);
    }
    return std::format_to(ctx.out(), ">");
}

namespace sparrow
{
    std::ostream& operator<<(std::ostream& os, const sparrow::struct_value& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

#endif
