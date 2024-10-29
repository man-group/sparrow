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

#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/buffer/u8_buffer.hpp"

namespace sparrow
{
namespace detail
{
    // Functor to get the value of the layout at index i.
    //
    // This is usefull to create a iterator over the values of a layout.
    // This functor will be passed to the functor_index_iterator.
    template<class LAYOUT_TYPE, class VALUE_TYPE>
    class layout_value_functor
    {
    public:

        using value_type = VALUE_TYPE;
        using layout_type = LAYOUT_TYPE;

        constexpr explicit layout_value_functor(layout_type* layout = nullptr)
            : p_layout(layout)
        {
        }

        value_type operator()(std::size_t i) const
        {
            return this->p_layout->value(i);
        }

    private:

        layout_type* p_layout;
    };


    // Functor to get the optional-value of the layout at index i.
    //
    // This is usefull to create a iterator over the nullable-values of a layout.
    // This functor will be passed to the functor_index_iterator.
    template<class LAYOUT_TYPE, class VALUE_TYPE>
    class layout_bracket_functor
    {
    public:

        using value_type = VALUE_TYPE;
        using layout_type = LAYOUT_TYPE;

        constexpr explicit layout_bracket_functor(layout_type* layout = nullptr)
            : p_layout(layout)
        {
        }

        value_type operator()(std::size_t i) const
        {
            return this->p_layout->operator[](i);
        }

    private:

        layout_type* p_layout;
    };

} // namespace sparrow::detail


template<class OFFSET_TYPE>
concept offset_type = std::is_same<std::remove_const_t<OFFSET_TYPE>, std::uint32_t>::value || 
                        std::is_same<std::remove_const_t<OFFSET_TYPE>, std::uint64_t>::value;

template<offset_type OFFSET_TYPE, std::ranges::range SIZES_RANGE>
requires(std::unsigned_integral<std::ranges::range_value_t<SIZES_RANGE>>)
u8_buffer<OFFSET_TYPE> offset_buffer_from_sizes(SIZES_RANGE && sizes)
{
    u8_buffer<OFFSET_TYPE> buffer(std::ranges::size(sizes) + 1);
    
    OFFSET_TYPE offset = 0;
    auto it = buffer.begin();
    for(auto size : sizes)
    {
        *it = offset;
        offset += static_cast<OFFSET_TYPE>(size);
        ++it;
    }
    *it = offset;
    return buffer;
} 

} // namespace sparrow
