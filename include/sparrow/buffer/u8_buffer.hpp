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

#include <cmath>
#include <ranges>
#include <type_traits>

#include "sparrow/buffer/buffer_adaptor.hpp"

namespace sparrow
{

    namespace detail
    {
        template<class T>
        class holder
        {
        public:
            template<class ...Args>
            holder(Args&&... args)
                : value(std::forward<Args>(args)...)
            {
            }
            T value;
        };
    }

    // like buffer<T> but for any type T, nut always use buffer<std::uint8_t> as storage
    // This internal storage can be extracted 
    template<class T>
    class u8_buffer : private detail::holder<buffer<std::uint8_t>>,
                           public buffer_adaptor<T, buffer<std::uint8_t>&>
    {
    public:
        using holder_type = detail::holder<buffer<std::uint8_t>>;
        using buffer_adaptor_type  = buffer_adaptor<T, buffer<std::uint8_t>&>;

        template<std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_value_t<R>, T>
        u8_buffer(R&& range)
            : holder_type{std::ranges::size(range) * sizeof(T)}
            ,buffer_adaptor_type(holder_type::value)
        {
            std::ranges::copy(range, this->begin());
        }

        u8_buffer(std::size_t n, T val = T{})
            : holder_type{n * sizeof(T)}
            , buffer_adaptor_type(holder_type::value)
        {
            std::fill(this->begin(), this->end(), val);
        }

        buffer<std::uint8_t> extract_storage() && 
        {
            return std::move(holder_type::value);
        }
    };

}
