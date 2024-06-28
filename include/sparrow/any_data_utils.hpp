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
#include <ranges>
#include <vector>

#include "sparrow/mp_utils.hpp"
#include "sparrow/memory.hpp"

namespace sparrow
{
    /**
     * Get a raw pointer from a smart pointer, a range, an object or a pointer.
     *
     * @tparam T The type of the pointer.
     * @tparam U The type of the element.
     * @param elem The element.
     * @return A raw pointer.
     */
    template <typename T, typename U>
    T* get_raw_ptr(U& elem)
    {
        if constexpr (std::is_pointer_v<U>)
        {
            return reinterpret_cast<T*>(elem);
        }
        else if constexpr (mpl::has_element_type<U>)
        {
            if constexpr (mpl::smart_ptr<U> || std::derived_from<U, std::shared_ptr<typename U::element_type>>
                          || mpl::is_type_instance_of_v<U, value_ptr>)
            {
                if constexpr (std::is_same_v<typename U::element_type, T>)
                {
                    return reinterpret_cast<T*>(elem.get());
                }
                else if constexpr (mpl::has_data_function<typename U::element_type, T>)
                {
                    return reinterpret_cast<T*>(std::ranges::data(*elem.get()));
                }
            }
        }
        else if constexpr (std::ranges::input_range<U>)
        {
            return reinterpret_cast<T*>(std::ranges::data(elem));
        }
        else if constexpr (mpl::has_data_function<U, T>)
        {
            return reinterpret_cast<T*>(elem.data());
        }
        else if constexpr (std::is_same_v<T, U>)
        {
            return reinterpret_cast<T*>(&elem);
        }
        else
        {
            static_assert(mpl::dependent_false<U>::value, "get_raw_ptr: unsupported type.");
            mpl::unreachable();
        }
    }

    /**
     * Create a vector of pointers from a range.
     *
     * @tparam T The type of the pointers.
     * @tparam Range The range type.
     * @tparam Allocator The allocator type.
     * @param range The range.
     * @return A vector of pointers.
     */
    template <class T, std::ranges::input_range Range, template <typename> class Allocator = std::allocator>
    std::vector<T*, Allocator<T*>> to_raw_ptr_vec(Range& range)
    {
        std::vector<T*, Allocator<T*>> raw_ptr_vec;
        raw_ptr_vec.reserve(range.size());
        std::ranges::transform(
            range,
            std::back_inserter(raw_ptr_vec),
            [](auto& elem) -> T*
            {
                return get_raw_ptr<T>(elem);
            }
        );
        return raw_ptr_vec;
    }

    /**
     * Create a vector of pointers from a tuple.
     *
     * @tparam T The type of the pointers.
     * @tparam Tuple The tuple type.
     * @tparam Allocator The allocator type.
     * @param tuple The tuple.
     * @return A vector of pointers.
     */
    template <class T, class Tuple, template <typename> class Allocator = std::allocator>
        requires mpl::is_type_instance_of_v<Tuple, std::tuple>
    std::vector<T*, Allocator<T*>> to_raw_ptr_vec(Tuple& tuple)
    {
        std::vector<T*, Allocator<T*>> raw_ptr_vec;
        raw_ptr_vec.reserve(std::tuple_size_v<Tuple>);
        std::apply(
            [&raw_ptr_vec](auto&&... args)
            {
                (raw_ptr_vec.push_back(get_raw_ptr<T>(args)), ...);
            },
            tuple
        );
        return raw_ptr_vec;
    }
}
