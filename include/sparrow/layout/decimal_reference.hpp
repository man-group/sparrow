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

#if defined(__cpp_lib_format)
#    include <format>
#    include <ostream>
#endif

#include "sparrow/utils/decimal.hpp"

namespace sparrow
{
    /**
     * Implementation of reference to inner type used for layout L
     *
     * @tparam L the layout type
     */
    template <class L>
    class decimal_reference
    {
    public:

        using self_type = decimal_reference<L>;
        using value_type = typename L::inner_value_type;
        using reference = typename L::inner_reference;
        using const_reference = typename L::inner_const_reference;
        using size_type = typename L::size_type;
        using difference_type = std::ptrdiff_t;

        constexpr decimal_reference(L* layout, size_type index);
        constexpr decimal_reference(const decimal_reference&) = default;
        constexpr decimal_reference(decimal_reference&&) noexcept = default;

        constexpr self_type& operator=(self_type&& rhs);
        constexpr self_type& operator=(const self_type& rhs);

        constexpr self_type& operator=(value_type&& rhs);
        constexpr self_type& operator=(const value_type& rhs);

        constexpr explicit operator float() const
            requires(!is_int_placeholder_v<typename value_type::integer_type>);
        constexpr explicit operator double() const
            requires(!is_int_placeholder_v<typename value_type::integer_type>);
        constexpr explicit operator long double() const
            requires(!is_int_placeholder_v<typename value_type::integer_type>);

        [[nodiscard]] explicit operator std::string() const
            requires(!is_int_placeholder_v<typename value_type::integer_type>);

        constexpr bool operator==(const value_type& rhs) const;
        constexpr auto operator<=>(const value_type& rhs) const;

        [[nodiscard]] constexpr const_reference value() const;
        [[nodiscard]] constexpr value_type::integer_type storage() const;
        [[nodiscard]] constexpr int scale() const;

    private:

        L* p_layout = nullptr;
        size_type m_index = size_type(0);
    };

    /*************************************
     * decimal_reference implementation *
     *************************************/

    template <typename L>
    constexpr decimal_reference<L>::decimal_reference(L* layout, size_type index)
        : p_layout(layout)
        , m_index(index)
    {
    }

    template <typename L>
    constexpr auto decimal_reference<L>::operator=(value_type&& rhs) -> self_type&
    {
        p_layout->assign(std::forward<value_type>(rhs), m_index);
        return *this;
    }

    template <typename L>
    constexpr auto decimal_reference<L>::operator=(const value_type& rhs) -> self_type&
    {
        p_layout->assign(rhs, m_index);
        return *this;
    }

    template <typename L>
    constexpr auto decimal_reference<L>::operator=(self_type&& rhs) -> self_type&
    {
        this->operator=(rhs.value());
        return *this;
    }

    template <typename L>
    constexpr auto decimal_reference<L>::operator=(const self_type& rhs) -> self_type&
    {
        this->operator=(rhs.value());
        return *this;
    }

    template <typename L>
    constexpr bool decimal_reference<L>::operator==(const value_type& rhs) const
    {
        return value() == rhs;
    }

    template <typename L>
    constexpr auto decimal_reference<L>::operator<=>(const value_type& rhs) const
    {
        return value() <=> rhs;
    }

    template <typename L>
    constexpr auto decimal_reference<L>::value() const -> const_reference
    {
        return static_cast<const L*>(p_layout)->value(m_index);
    }

    template <typename L>
    constexpr auto decimal_reference<L>::storage() const -> value_type::integer_type
    {
        return value().storage();
    }

    template <typename L>
    constexpr int decimal_reference<L>::scale() const
    {
        return value().scale();
    }

    template <typename L>
    constexpr decimal_reference<L>::operator float() const
        requires(!is_int_placeholder_v<typename value_type::integer_type>)
    {
        return static_cast<float>(value());
    }

    template <typename L>
    constexpr decimal_reference<L>::operator double() const
        requires(!is_int_placeholder_v<typename value_type::integer_type>)
    {
        return static_cast<double>(value());
    }

    template <typename L>
    constexpr decimal_reference<L>::operator long double() const
        requires(!is_int_placeholder_v<typename value_type::integer_type>)
    {
        return static_cast<long double>(value());
    }
}

#if defined(__cpp_lib_format)

template <typename L>
struct std::formatter<sparrow::decimal_reference<L>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::decimal_reference<L>& ref, std::format_context& ctx) const
    {
        const auto& value = ref.value();
        return std::format_to(ctx.out(), "{}", value);
    }
};

template <typename L>
inline std::ostream& operator<<(std::ostream& os, const sparrow::decimal_reference<L>& value)
{
    os << std::format("{}", value);
    return os;
}

#endif