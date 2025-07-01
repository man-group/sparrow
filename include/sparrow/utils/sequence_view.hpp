// Man Group Operations Limited
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

#include <algorithm>
#include <ranges>
#include <span>
#include <vector>

#if defined(__cpp_lib_format)
#    include <format>
#endif

#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    /**
     * The class sequence_view describes an object that can refer to a constant contiguous
     * sequence of T with the first element of the sequence at position zero. It is similar
     * to string_view, but for arbitrary T. You can consider it as a span or range supporting
     * const operations only, and comparison operators.
     */
    template <class T, std::size_t Extent = std::dynamic_extent>
    class sequence_view : public std::span<T, Extent>
    {
    public:

        using base_type = std::span<T, Extent>;
        using value_type = typename base_type::value_type;

        using base_type::base_type;

        explicit operator std::vector<value_type>() const noexcept
        {
            return std::vector<value_type>(this->begin(), this->end());
        }
    };

    template <class T, std::size_t E>
    constexpr bool operator==(const sequence_view<T, E>& lhs, const sequence_view<T, E>& rhs);

    template <class T, std::size_t E, std::ranges::input_range R>
        requires mpl::weakly_equality_comparable_with<T, std::ranges::range_value_t<R>>
    constexpr bool operator==(const sequence_view<T, E>& lhs, const R& rhs);

    template <class T, std::size_t E>
    constexpr std::compare_three_way_result<T> operator<=>(const sequence_view<T, E>& lhs, const sequence_view<T, E>& rhs);

    template <class T, std::size_t E, std::ranges::input_range R>
    constexpr std::compare_three_way_result<T, std::ranges::range_value_t<R>>
    operator<=>(const sequence_view<T, E>& lhs, const R& rhs);
}

namespace std
{
    template <class T, std::size_t E>
    struct tuple_size<sparrow::sequence_view<T, E>>
        : tuple_size<std::span<T, E>>
    {
    };

    namespace ranges
    {
        template <class T, std::size_t E>
        constexpr bool enable_borrowed_range<sparrow::sequence_view<T, E>> = true;

        template <class T, std::size_t E>
        constexpr bool enable_view<sparrow::sequence_view<T, E>> = true;
    }
}

namespace sparrow
{
    // Concept for fixed-size std::sequence_view
    template <typename T>
    concept fixed_size_sequence_view = requires {
        typename std::remove_cvref_t<T>::element_type;
        requires std::tuple_size_v<T> != std::dynamic_extent;
        requires std::same_as<
            sequence_view<typename std::remove_cvref_t<T>::element_type, std::remove_cvref_t<T>::extent>,
            std::remove_cvref_t<T>>;
    };

    /********************************
     * sequence_view implementation *
     ********************************/

    template <class T, std::size_t E>
    constexpr bool operator==(const sequence_view<T, E>& lhs, const sequence_view<T, E>& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }

    template <class T, std::size_t E, std::ranges::input_range R>
        requires mpl::weakly_equality_comparable_with<T, std::ranges::range_value_t<R>>
    constexpr bool operator==(const sequence_view<T, E>& lhs, const R& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }

    template <class T, std::size_t E>
    constexpr std::compare_three_way_result<T> operator<=>(const sequence_view<T, E>& lhs, const sequence_view<T, E>& rhs)
    {
        return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    template <class T, std::size_t E, std::ranges::input_range R>
    constexpr std::compare_three_way_result<T, std::ranges::range_value_t<R>>
    operator<=>(const sequence_view<T, E>& lhs, const R& rhs)
    {
        return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
}

#if defined(__cpp_lib_format)
template <class T, std::size_t E>
struct std::formatter<sparrow::sequence_view<T, E>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::sequence_view<T, E>& vec, std::format_context& ctx) const
    {
        std::format_to(ctx.out(), "<");
        if (!vec.empty())
        {
            for (std::size_t i = 0; i < vec.size() - 1; ++i)
            {
                std::format_to(ctx.out(), "{}, ", vec[i]);
            }
        }
        return std::format_to(ctx.out(), "{}>", vec.back());
    }
};

template <typename T, std::size_t E>
std::ostream& operator<<(std::ostream& os, const sparrow::sequence_view<T, E>& value)
{
    os << std::format("{}", value);
    return os;
}

#endif
