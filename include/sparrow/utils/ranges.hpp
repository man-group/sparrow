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

#include <algorithm>
#include <bitset>
#include <ranges>
#include <vector>

#if defined(__cpp_lib_format)
#    include <format>
#    include "sparrow/utils/format.hpp"
#endif

#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/sequence_view.hpp"

namespace sparrow
{
    template <std::ranges::input_range R>
        requires(std::ranges::sized_range<R>)
    [[nodiscard]] constexpr std::size_t range_size(R&& r)
    {
        return static_cast<std::size_t>(std::ranges::size(r));
    }

    template <std::ranges::input_range R>
        requires(!std::ranges::sized_range<R>)
    [[nodiscard]] constexpr std::size_t range_size(R&& r)
    {
        return static_cast<std::size_t>(std::ranges::distance(r));
    }

    template <std::ranges::range Range>
        requires std::ranges::sized_range<std::ranges::range_value_t<Range>>
    [[nodiscard]] constexpr bool all_same_size(const Range& range)
    {
        // Optimization for std::array and fixed-size sequence_view
        if constexpr (mpl::std_array<std::ranges::range_value_t<Range>>
                      || fixed_size_sequence_view<std::ranges::range_value_t<Range>>)
        {
            return true;
        }
        else
        {
            if (std::ranges::empty(range))
            {
                return true;
            }

            const std::size_t first_size = range.front().size();
            return std::ranges::all_of(
                range,
                [first_size](const auto& element)
                {
                    return element.size() == first_size;
                }
            );
        }
    }

    namespace ranges
    {
        template <typename InputRange, typename OutputIterator>
        concept has_ranges_copy = requires(InputRange input, OutputIterator output) {
            {
                std::ranges::copy(input, output)
            } -> std::same_as<std::ranges::copy_result<std::ranges::iterator_t<InputRange>, OutputIterator>>;
        };

        /**
         * Copies the elements from the input range to the output iterator.
         * @details: Implementation from https://en.cppreference.com/w/cpp/algorithm/ranges/copy
         * This function is used because the implementation of std::ranges::copy is missing in GCC 12.
         */
        struct copy_fn
        {
            template <std::input_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O>
                requires std::indirectly_copyable<I, O>
            constexpr std::ranges::copy_result<I, O> operator()(I first, S last, O result) const
            {
                for (; first != last; ++first, (void) ++result)
                {
#if defined(__GNUC__) && __GNUC__ < 12
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wnull-dereference"
#endif
                    *result = *first;
#if defined(__GNUC__) && __GNUC__ < 12
#    pragma GCC diagnostic pop
#endif
                }
                return {std::move(first), std::move(result)};
            }

            template <std::ranges::input_range R, std::weakly_incrementable O>
                requires std::indirectly_copyable<std::ranges::iterator_t<R>, O>
            constexpr std::ranges::copy_result<std::ranges::borrowed_iterator_t<R>, O>
            operator()(R&& r, O result) const
            {
                return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result));
            }
        };

        template <std::ranges::input_range R, std::weakly_incrementable O>
            requires std::indirectly_copyable<std::ranges::iterator_t<R>, O>
        constexpr std::ranges::copy_result<std::ranges::borrowed_iterator_t<R>, O> copy(R&& r, O result)
        {
            if constexpr (has_ranges_copy<R, O>)
            {
                return std::ranges::copy(std::forward<R>(r), std::move(result));
            }
            else
            {
                return copy_fn{}(std::forward<R>(r), std::move(result));
            }
        }
    }
};

#if defined(__cpp_lib_format)

template <typename T, std::size_t N>
struct std::formatter<std::array<T, N>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
         return m_spec.parse(ctx.begin(), ctx.end());
    }

    auto format(const std::array<T, N>& array, std::format_context& ctx) const
    {
        std::string core = m_spec.build_core(array);
        std::string out_str = m_spec.apply_alignment(std::move(core));
        return std::ranges::copy(out_str, ctx.out()).out;
    }

       private:
        sparrow::detail::sequence_format_spec m_spec;

};

template <typename T>
struct std::formatter<std::vector<T>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
      return m_spec.parse(ctx.begin(), ctx.end());
    }

    auto format(const std::vector<T>& vector, std::format_context& ctx) const
    {
        std::string core = m_spec.build_core(vector);
        std::string out_str = m_spec.apply_alignment(std::move(core));
        return std::ranges::copy(out_str, ctx.out()).out;
    }

           private:
        sparrow::detail::sequence_format_spec m_spec;
};

template <std::size_t T>
struct std::formatter<std::bitset<T>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {return m_spec.parse(ctx.begin(), ctx.end());
    }

    auto format(const std::bitset<T>& vector, std::format_context& ctx) const
    {
        std::string core = m_spec.build_core(vector);
        std::string out_str = m_spec.apply_alignment(std::move(core));
        return std::ranges::copy(out_str, ctx.out()).out;
    }

          private:
        sparrow::detail::sequence_format_spec m_spec;
};

#endif
