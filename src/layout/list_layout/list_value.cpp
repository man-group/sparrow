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

#include "sparrow/layout/list_layout/list_value.hpp"
#include "sparrow/layout/dispatch.hpp"

namespace sparrow
{
    list_value::list_value(const array_wrapper* flat_array, size_type index_begin, size_type index_end)
        : p_flat_array(flat_array)
        , m_index_begin(index_begin)
        , m_index_end(index_end)
    {
        SPARROW_ASSERT_TRUE(index_begin <= index_end);
    }

    auto list_value::size() const -> size_type
    {
        return m_index_end - m_index_begin;
    }

    bool list_value::empty() const
    {
        return size() == 0;
    }

    auto list_value::operator[](size_type i) const -> const_reference
    {
        return array_element(*p_flat_array, m_index_begin + i);
    }

    auto list_value::front() const -> const_reference
    {
        return (*this)[0];
    }

    auto list_value::back() const -> const_reference
    {
        return (*this)[size() - 1];
    }

    bool operator==(const list_value& lhs, const list_value& rhs)
    {
        bool res = lhs.size() == rhs.size();
        for (std::size_t i = 0; res && i < lhs.size(); ++i)
        {
            res = lhs[i] == rhs[i];
        }
        return res;
        // TODO: refactor with the following when list_value is a range
        // return std::ranges::equal(lhs, rhs);
    }
}

#if defined(__cpp_lib_format)

auto std::formatter<sparrow::list_value>::format(const sparrow::list_value& list_value, std::format_context& ctx) const
    -> decltype(ctx.out())
{
    std::format_to(ctx.out(), "<");
    if (!list_value.empty())
    {
        for (std::size_t i = 0; i < list_value.size() - 1; ++i)
        {
            std::format_to(ctx.out(), "{}, ", list_value[i]);
        }
    }
    return std::format_to(ctx.out(), "{}>", list_value.back());
}

namespace sparrow
{
    std::ostream& operator<<(std::ostream& os, const sparrow::list_value& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

#endif
