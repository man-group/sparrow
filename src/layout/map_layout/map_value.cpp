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

#include <variant>
#include "sparrow/layout/map_layout/map_value.hpp"

#include "sparrow/layout/dispatch.hpp"
#include "sparrow/layout/nested_value_types.hpp"

namespace sparrow
{
    map_value::map_value(const array_wrapper* flat_keys, const array_wrapper* flat_items,
        size_type index_begin, size_type index_end, bool keys_sorted)
        : p_flat_keys(flat_keys)
        , p_flat_items(flat_items)
        , m_index_begin(index_begin)
        , m_index_end(index_end)
        , m_keys_sorted(keys_sorted)
    {
    }

    bool map_value::empty() const noexcept
    {
        return m_index_begin == m_index_end;
    }

    auto map_value::size() const noexcept -> size_type
    {
        return m_index_end - m_index_begin;
    }

    auto map_value::begin() const -> const_iterator
    {
        return cbegin();
    }

    auto map_value::cbegin() const -> const_iterator
    {
        return const_iterator(functor_type(this), 0);
    }

    auto map_value::end() const -> const_iterator
    {
        return cend();
    }

    auto map_value::cend() const -> const_iterator
    {
        return const_iterator(functor_type(this), size());
    }

    auto map_value::value(size_type i) const -> const_reference
    {
        return std::make_pair(array_element(*p_flat_keys, i + m_index_begin), array_element(*p_flat_items, i + m_index_begin));
    }

    bool operator==(const map_value& lhs, const map_value& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }
}


#if defined(__cpp_lib_format)

auto std::formatter<sparrow::map_value>::format(const sparrow::map_value& map_value, std::format_context& ctx) const
    -> decltype(ctx.out())
{
    std::format_to(ctx.out(), "<");
    if (!map_value.empty())
    {
        for (auto iter = map_value.cbegin(); iter != map_value.cend(); ++iter)
        {
            std::format_to(ctx.out(), "{}: {}, ", iter->first, iter->second);
        }
    }
    return std::format_to(ctx.out(), ">");
}

namespace sparrow
{
    std::ostream& operator<<(std::ostream& os, const sparrow::map_value& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

#endif
