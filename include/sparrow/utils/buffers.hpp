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

#include <algorithm>
#include <numeric>
#include <ranges>

#include "sparrow/buffer/buffer.hpp"
#include "sparrow/buffer/buffer_adaptor.hpp"
#include "sparrow/utils/ranges.hpp"

namespace sparrow
{
    template <std::ranges::range R>
        requires std::ranges::sized_range<std::ranges::range_value_t<R>>
    [[nodiscard]] size_t number_of_bytes(const R& ranges)
    {
        using value_type = std::ranges::range_value_t<std::ranges::range_value_t<R>>;
        return std::transform_reduce(
            ranges.begin(),
            ranges.end(),
            std::size_t{0},
            std::plus<std::size_t>(),
            [](const auto& range)
            {
                return range.size() * sizeof(value_type);
            }
        );
    }

    template <std::ranges::range R>
        requires(std::ranges::sized_range<std::ranges::range_value_t<R>>)
    [[nodiscard]] buffer<uint8_t> strings_to_buffer(R&& strings)
    {
        using range_value_type = std::ranges::range_value_t<R>;
        using atomic_element_type = std::ranges::range_value_t<range_value_type>;
        const size_t values_byte_count = number_of_bytes(strings);
        buffer<uint8_t> values_buffer(values_byte_count, buffer<uint8_t>::default_allocator());
        if (!strings.empty())
        {
            auto buffer_adaptor = make_buffer_adaptor<atomic_element_type>(values_buffer);
            auto iter = buffer_adaptor.begin();
            for (auto&& string : strings)
            {
                if constexpr (std::is_rvalue_reference_v<R>)
                {
                    std::ranges::move(string, iter);
                }
                else
                {
                    sparrow::ranges::copy(string, iter);
                }
                std::advance(iter, string.size());
            }
        }
        return values_buffer;
    }

    template <std::ranges::range R>
        requires(std::is_arithmetic_v<std::ranges::range_value_t<R>>)
    [[nodiscard]] buffer<uint8_t> range_to_buffer(R&& range)
    {
        const size_t values_byte_count = std::ranges::size(range) * sizeof(std::ranges::range_value_t<R>);
        buffer<uint8_t> values_buffer(values_byte_count, buffer<uint8_t>::default_allocator());
        auto buffer_adaptor = make_buffer_adaptor<std::ranges::range_value_t<R>>(values_buffer);
        if constexpr (std::is_rvalue_reference_v<R>)
        {
            std::ranges::move(range, buffer_adaptor.begin());
        }
        else
        {
            sparrow::ranges::copy(range, buffer_adaptor.begin());
        }

        return values_buffer;
    }
}
