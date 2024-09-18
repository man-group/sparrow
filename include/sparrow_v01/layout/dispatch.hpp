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

#include <type_traits>

#include "sparrow/array/data_traits.hpp"
#include "sparrow_v01/layout/array_base.hpp"
#include "sparrow_v01/layout/null_array.hpp"
#include "sparrow_v01/layout/primitive_array.hpp"
#include "sparrow_v01/layout/list_layout/value_list.hpp"

namespace sparrow
{
    template <class F>
    using visit_result_t = std::invoke_result_t<F, null_array>;

    template <class F>
    visit_result_t<F> visit(F&& func, const array_base& ar);

    std::size_t array_size(const array_base& ar);
    array_traits::const_reference array_element(const array_base& ar, std::size_t index);

    /******************
     * Implementation *
     ******************/

    template <class F>
    visit_result_t<F> visit(F&& func, const array_base& ar)
    {
        const auto& format = ar.format();
        if (format.size() == 1)
        {
            switch(format[0])
            {
            case 'n':
                return func(static_cast<const null_array&>(ar));;
            case 'b':
                return func(static_cast<const primitive_array<bool>&>(ar));
            case 'c':
                return func(static_cast<const primitive_array<std::int8_t>&>(ar));
            case 'C':
                return func(static_cast<const primitive_array<std::uint8_t>&>(ar));
            case 's':
                return func(static_cast<const primitive_array<std::int16_t>&>(ar));
            case 'S':
                return func(static_cast<const primitive_array<std::uint16_t>&>(ar));
            case 'i':
                return func(static_cast<const primitive_array<std::int32_t>&>(ar));
            case 'I':
                return func(static_cast<const primitive_array<std::uint32_t>&>(ar));
            case 'l':
                return func(static_cast<const primitive_array<std::int64_t>&>(ar));
            case 'L':
                return func(static_cast<const primitive_array<std::uint64_t>&>(ar));
            case 'e':
                return func(static_cast<const primitive_array<float16_t>&>(ar));
            case 'f':
                return func(static_cast<const primitive_array<float32_t>&>(ar));
            case 'g':
                return func(static_cast<const primitive_array<float64_t>&>(ar));
            default:
                throw std::invalid_argument("array type not supported");
            }
        }
        else
        {
            throw std::invalid_argument("array type not supported");
        }
    }

    inline std::size_t array_size(const array_base& ar)
    {
        return visit([](const auto& impl) { return impl.size(); }, ar);
    }

    inline array_traits::const_reference array_element(const array_base& ar, std::size_t index)
    {
        using return_type = array_traits::const_reference;
        return visit([index](const auto& impl) -> return_type { return return_type(impl[index]); }, ar);
    }
}

