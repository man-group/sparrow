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

#include "sparrow/config/config.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/types/data_traits.hpp"

namespace sparrow
{
    class SPARROW_API list_value
    {
    public:

        using value_type = array_traits::value_type;
        using const_reference = array_traits::const_reference;
        using size_type = std::size_t;

        list_value() = default;
        list_value(const array_wrapper* flat_array, size_type index_begin, size_type index_end);

        size_type size() const;
        bool empty() const;

        const_reference operator[](size_type i) const;

        const_reference front() const;
        const_reference back() const;

    private:

        const array_wrapper* p_flat_array = nullptr;
        size_type m_index_begin = 0u;
        size_type m_index_end = 0u;
    };

    SPARROW_API
    bool operator==(const list_value& lhs, const list_value& rhs);
}

#if defined(__cpp_lib_format)

template <>
struct std::formatter<sparrow::list_value>
{
    constexpr auto parse(std::format_parse_context& ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();  // Simple implementation
    }

    SPARROW_API auto format(const sparrow::list_value& list_value, std::format_context& ctx) const
        -> decltype(ctx.out());
};

inline std::ostream& operator<<(std::ostream& os, const sparrow::list_value& value)
{
    os << std::format("{}", value);
    return os;
}

#endif
