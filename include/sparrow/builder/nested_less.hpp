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

#include <sparrow/builder/builder_utils.hpp>
#include <sparrow/utils/ranges.hpp>

namespace sparrow
{
    namespace detail
    {
        // nested eq / nested hash
        template <class T>
        struct nested_less;

        // scalars
        template <class T>
            requires std::is_scalar_v<T>
        struct nested_less<T>
        {
            [[nodiscard]] constexpr bool operator()(const T& a, const T& b) const noexcept
            {
                return a < b;
            }
        };

        template <is_express_layout_desire T>
        struct nested_less<T>
        {
            [[nodiscard]] constexpr bool operator()(const T& a, const T& b) const noexcept
            {
                return nested_less<typename T::value_type>{}(a.get(), b.get());
            }
        };

        // nullables
        template <class T>
            requires is_nullable_like<T>
        struct nested_less<T>
        {
            [[nodiscard]] constexpr bool operator()(const T& a, const T& b) const
            {
                // if one is null and the other is not then the null is less
                // both are null:
                if (!a.has_value() && !b.has_value())
                {
                    return false;
                }
                // a is null, b is not
                else if (!a.has_value())
                {
                    return true;
                }
                // a is not null, b is
                else if (!b.has_value())
                {
                    return false;
                }
                else
                {
                    // both are not null
                    return nested_less<typename T::value_type>{}(a.value(), b.value());
                }
            }
        };

        // tuple like
        template <class T>
            requires tuple_like<T>
        struct nested_less<T>
        {
            [[nodiscard]] constexpr bool operator()(const T& a, const T& b) const
            {
                constexpr std::size_t N = std::tuple_size_v<T>;
                bool is_less = false;
                exitable_for_each_index<N>(
                    [&](auto i)
                    {
                        constexpr std::size_t index = decltype(i)::value;
                        using tuple_element_type = std::decay_t<std::tuple_element_t<decltype(i)::value, T>>;

                        const auto& a_val = std::get<index>(a);
                        const auto& b_val = std::get<index>(b);

                        // a < b
                        if (nested_less<tuple_element_type>{}(a_val, b_val))
                        {
                            is_less = true;
                            return false;  // break
                        }
                        // a >= b
                        else if (nested_less<tuple_element_type>{}(b_val, a_val))
                        {
                            is_less = false;
                            return false;  // break
                        }
                        // a == b
                        else
                        {
                            is_less = false;
                            return true;  // continue
                        }
                    }
                );
                return is_less;
            }
        };

        // ranges (and not tuple like)
        template <class T>
            requires(std::ranges::input_range<T> && !tuple_like<T>)
        struct nested_less<T>
        {
            [[nodiscard]] constexpr bool operator()(const T& a, const T& b) const
            {
                using value_type = std::decay_t<std::ranges::range_value_t<T>>;
                return std::ranges::lexicographical_compare(a, b, nested_less<value_type>{});
            }
        };

        // variants
        template <class T>
            requires variant_like<T>
        struct nested_less<T>
        {
            [[nodiscard]] constexpr bool operator()(const T& a, const T& b) const
            {
                if (a.index() != b.index())
                {
                    return a.index() < b.index();
                }
                return std::visit(
                    [&](const auto& a_val)
                    {
                        using value_type = std::decay_t<decltype(a_val)>;
                        const auto& b_val = std::get<value_type>(b);
                        return nested_less<value_type>{}(a_val, b_val);
                    },
                    a
                );
            }
        };

    }  // namespace detail
}  // namespace sparrow
