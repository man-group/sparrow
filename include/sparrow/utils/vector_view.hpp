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
#include <vector>

namespace sparrow
{
    /**
     * The class vector_view describes an object that can refer to a constant contiguous
     * sequence of T with the first element of the sequence at position zero. It is similar
     * to string_view, but for arbitrary T. You can consider it as a span or range supporting
     * const operations only, and comparison operators.
     */
    template <class T>
    class vector_view : public std::span<T>
    {
    public:

        using base_type = std::span<T>;
        using value_type = typename base_type::value_type;

        using base_type::base_type;

        explicit operator std::vector<value_type>() const noexcept
        {
            return std::vector<value_type>(this->begin(), this->end());
        }
    };

    template <class T>
    constexpr bool operator==(const vector_view<T>& lhs, const vector_view<T>& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }

    template <class T>
    constexpr bool operator==(const vector_view<T>& lhs, const std::vector<std::decay_t<T>>& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }

    template <class T>
    constexpr std::compare_three_way_result<T>
    operator<=>(const vector_view<T>& lhs, const vector_view<T>& rhs)
    {
        return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    template <class T>
    constexpr std::compare_three_way_result<T>
    operator<=>(const vector_view<T>& lhs, const std::vector<std::decay_t<T>>& rhs)
    {
        return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
}
