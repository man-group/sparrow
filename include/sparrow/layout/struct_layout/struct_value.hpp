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

#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#if defined(__cpp_lib_format)
#    include <format>
#endif

#include "sparrow/config/config.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/utils/memory.hpp"

namespace sparrow
{
    class SPARROW_API struct_value
    {
    public:

        using value_type = array_traits::value_type;
        using const_reference = array_traits::const_reference;
        using size_type = std::size_t;
        using child_ptr = cloning_ptr<array_wrapper>;
        using const_functor_type = detail::layout_bracket_functor<const struct_value, const_reference>;
        using const_iterator = functor_index_iterator<const_functor_type>;

        struct_value() = default;
        struct_value(const std::vector<child_ptr>& children, size_type index);

        [[nodiscard]] size_type size() const;
        [[nodiscard]] bool empty() const;

        [[nodiscard]] const_reference operator[](size_type i) const;

        [[nodiscard]] const_reference front() const;
        [[nodiscard]] const_reference back() const;

        [[nodiscard]] const_iterator begin() const;
        [[nodiscard]] const_iterator cbegin() const;

        [[nodiscard]] const_iterator end() const;
        [[nodiscard]] const_iterator cend() const;

    private:

        const std::vector<child_ptr>* p_children = nullptr;
        size_type m_index = 0u;
    };

    SPARROW_API
    bool operator==(const struct_value& lhs, const struct_value& rhs);
}

#if defined(__cpp_lib_format)

template <>
struct std::formatter<sparrow::struct_value>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    SPARROW_API auto format(const sparrow::struct_value& ar, std::format_context& ctx) const
        -> decltype(ctx.out());
};

namespace sparrow
{
    SPARROW_API std::ostream& operator<<(std::ostream& os, const sparrow::struct_value& value);
}

#endif
