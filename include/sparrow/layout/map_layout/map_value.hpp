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

#pragma once

#include <utility>

#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"

namespace sparrow
{
    class SPARROW_API map_value
    {
    public:

        using key_type = array_traits::value_type;
        using const_key_reference = array_traits::const_reference;
        using mapped_type = array_traits::value_type;
        using const_mapped_reference = array_traits::const_reference;

        using value_type = std::pair<key_type, mapped_type>;
        using const_reference = std::pair<const_key_reference, const_mapped_reference>;
        using size_type = std::size_t;

        using functor_type = detail::layout_value_functor<const map_value, const_reference>;
        using const_iterator = functor_index_iterator<functor_type>;

        map_value() = default;
        map_value(const array_wrapper* flat_keys, const array_wrapper* flat_items,
                size_type index_begin, size_type index_end, bool keys_sorted);

        [[nodiscard]] bool empty() const noexcept;
        [[nodiscard]] size_type size() const noexcept;

        [[nodiscard]] const_iterator begin() const;
        [[nodiscard]] const_iterator cbegin() const;

        [[nodiscard]] const_iterator end() const;
        [[nodiscard]] const_iterator cend() const;

    private:

        const_reference value(size_type i) const;

        const array_wrapper* p_flat_keys;
        const array_wrapper* p_flat_items;
        size_type m_index_begin;
        size_type m_index_end;
        bool m_keys_sorted;

        friend class detail::layout_value_functor<const map_value, const_reference>;
    };

    SPARROW_API
    bool operator==(const map_value& lhs, const map_value& rhs);
}

#if defined(__cpp_lib_format)

template <>
struct std::formatter<sparrow::map_value>
{
    constexpr auto parse(std::format_parse_context& ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();  // Simple implementation
    }

    SPARROW_API auto format(const sparrow::map_value& value, std::format_context& ctx) const
        -> decltype(ctx.out());
};

namespace sparrow
{
    SPARROW_API std::ostream& operator<<(std::ostream& os, const sparrow::map_value& value);
}

#endif
