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
#include <cstddef>
#include <optional>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <vector>

#include "sparrow/c_interface.hpp"
#include "sparrow/memory.hpp"
#include "sparrow/mp_utils.hpp"

#include "mp_utils.hpp"


namespace sparrow
{
    template <class T>
    constexpr int64_t get_size(const T& value)
    {
        if constexpr (std::ranges::sized_range<T>)
        {
            return static_cast<int64_t>(std::ranges::size(value));
        }
        else if constexpr (mpl::is_type_instance_of_v<T, std::tuple>)
        {
            return std::tuple_size_v<T>;
        }
        else if constexpr (mpl::is_type_instance_of_v<T, std::optional>)
        {
            if (value.has_value())
            {
                return get_size(*value);
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    /**
     * Get a raw pointer from a smart pointer, a range, an object or a pointer.
     *
     * @tparam T The type of the pointer to obtain.
     * @tparam U The type of the variable.
     * @param var The variable.
     * @return A raw pointer.
     *          If the variable is a smart pointer, the pointer is obtained by calling get().
     *          If the variable is a range, the pointer is obtained by calling data().
     *          If the variable is a pointer, the pointer is returned as is.
     *          If the variable is an object, the pointer is returned by calling the address-of operator.
     */
    template <typename T, typename U>
    T* get_raw_ptr(U& var)
    {
        if constexpr (std::is_pointer_v<U>)
        {
            return var;
        }
        else if constexpr (mpl::has_element_type<U>)
        {
            if constexpr (mpl::smart_ptr<U> || std::derived_from<U, std::shared_ptr<typename U::element_type>>
                          || mpl::is_type_instance_of_v<U, value_ptr>)
            {
                if constexpr (std::ranges::contiguous_range<typename U::element_type>)
                {
                    return std::ranges::data(*var.get());
                }
                else if constexpr (std::same_as<typename U::element_type, T> || std::same_as<T, void>)
                {
                    return var.get();
                }
            }
        }
        else if constexpr (std::ranges::contiguous_range<U>)
        {
            return std::ranges::data(var);
        }
        else if constexpr (std::same_as<T, U> || std::same_as<T, void>)
        {
            return &var;
        }
        else
        {
            static_assert(mpl::dependent_false<T, U>::value, "get_raw_ptr: unsupported type.");
            mpl::unreachable();
        }
    }

    /**
     * Create a vector of pointers to elements from a range.
     * The range must be a non-view range.
     *
     * @tparam T The type of the pointers to obtain.
     * @tparam Range The range type.
     * @tparam Allocator The allocator type.
     * @param range The range.
     * @return A vector of pointers.
     */
    template <class T, std::ranges::input_range Range, template <typename> class Allocator = std::allocator>
        requires(!std::ranges::view<Range>)
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
     * Create a vector of pointers to elements from a std::optional<range>.
     * The range must be a non-view range.
     *
     * @tparam T The type of the pointers to obtain.
     * @tparam Optional The optional type.
     * @tparam Allocator The allocator type.
     * @param optional The optional range.
     * @return A vector of pointers.
     */
    template <class T, class Optional, template <typename> class Allocator = std::allocator>
        requires(mpl::is_type_instance_of_v<Optional, std::optional>)
    std::vector<T*, Allocator<T*>> to_raw_ptr_vec(Optional& optional)
    {
        if (!optional.has_value())
        {
            return {};
        }
        return to_raw_ptr_vec<T>(*optional);
    }

    /**
     * Create a vector of pointers to elements of a tuple.
     * Types of the tuple can be value_ptr, smart pointers, ranges, objects or pointers.
     * The type of the elements can be different. E.g: std::tuple<value_ptr<int>,
     * std::unique_ptr<char>, double>. Casting is used to convert the pointers to the desired type.
     *
     * @tparam T The type of the pointers to obtain.
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
