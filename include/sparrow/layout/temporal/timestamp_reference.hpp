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

#if defined(__cpp_lib_format)
#    include <format>
#    include <ostream>
#endif

#include "sparrow/types/data_type.hpp"

namespace sparrow
{
    /**
     * Implementation of reference to inner type used for layout L
     *
     * @tparam L the layout type
     */
    template <class L>
    class timestamp_reference
    {
    public:

        using self_type = timestamp_reference<L>;
        using value_type = typename L::inner_value_type;
        using reference = typename L::inner_reference;
        using const_reference = typename L::inner_const_reference;
        using size_type = typename L::size_type;
        using difference_type = std::ptrdiff_t;

        timestamp_reference(L* layout, size_type index);
        timestamp_reference(const timestamp_reference&) = default;
        timestamp_reference(timestamp_reference&&) = default;

        self_type& operator=(value_type&& rhs);
        self_type& operator=(const value_type& rhs);

        bool operator==(const value_type& rhs) const;
        auto operator<=>(const value_type& rhs) const;

    private:

        L::inner_value_type value() const
        {
            return static_cast<const L*>(p_layout)->value(m_index);
        }

        L* p_layout = nullptr;
        size_type m_index = size_type(0);
#if defined(__cpp_lib_format)
        friend std::formatter<timestamp_reference<L>>;
#endif
    };
}

namespace std

{
    template <typename Layout, typename T, template <typename> typename TQual, template <typename> typename UQual>
    struct basic_common_reference<sparrow::timestamp_reference<Layout>, sparrow::timestamp<T>, TQual, UQual>
    {
        using type = T;
    };

    template <typename Layout, typename T, template <typename> typename TQual, template <class> class UQual>
    struct basic_common_reference<sparrow::timestamp<T>, sparrow::timestamp_reference<Layout>, TQual, UQual>
    {
        using type = T;
    };
}

namespace sparrow
{
    /*************************************
     * timestamp_reference implementation *
     *************************************/

    template <typename L>
    timestamp_reference<L>::timestamp_reference(L* layout, size_type index)
        : p_layout(layout)
        , m_index(index)
    {
    }

    template <typename L>
    auto timestamp_reference<L>::operator=(value_type&& rhs) -> self_type&
    {
        p_layout->assign(std::forward<value_type>(rhs), m_index);
        return *this;
    }

    template <typename L>
    auto timestamp_reference<L>::operator=(const value_type& rhs) -> self_type&
    {
        p_layout->assign(rhs, m_index);
        return *this;
    }

    template <typename L>
    bool timestamp_reference<L>::operator==(const value_type& rhs) const
    {
        const auto& value = static_cast<const L*>(p_layout)->value(m_index);
        return value == rhs;
    }

    template <typename L>
    auto timestamp_reference<L>::operator<=>(const value_type& rhs) const
    {
        return lexicographical_compare_three_way(*this, rhs);
    }
}

#if defined(__cpp_lib_format)

template <typename L>
struct std::formatter<sparrow::timestamp_reference<L>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::timestamp_reference<L>& ref, std::format_context& ctx) const
    {
        const auto& value = ref.value();
        return std::format_to(ctx.out(), "{}", value);
    }
};

template <typename L>
inline std::ostream& operator<<(std::ostream& os, const sparrow::timestamp_reference<L>& value)
{
    os << std::format("{}", value);
    return os;
}

#endif
