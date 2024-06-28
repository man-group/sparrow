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
#include <concepts>
#include <cstddef>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <vector>

#include "sparrow/c_interface.hpp"
#include "sparrow/memory.hpp"
#include "sparrow/mp_utils.hpp"

namespace sparrow
{
    /// A concept that checks if a type is an ArrowArray or an ArrowSchema.
    template <typename T>
    concept any_arrow_array = std::is_same_v<T, ArrowArray> || std::is_same_v<T, ArrowSchema>;

    /// A concept that checks if a type is a valid Arrow Array buffers or children variable.
    template <class T>
    concept arrow_array_buffers_or_children = std::ranges::sized_range<T>
                                              || mpl::is_type_instance_of_v<T, std::tuple>
                                              || std::is_same_v<T, std::nullptr_t> || std::is_pointer_v<T>;

    

    /**
     * Transforms a range of unique pointers to a vector of shared pointers.
     *
     * @tparam Input A range of unique_ptr.
     * @param input The input range.
     * @return A vector of shared pointers.
     */
    template <std::ranges::input_range Input>
        requires mpl::unique_ptr<std::ranges::range_value_t<Input>>
    std::vector<std::shared_ptr<typename std::ranges::range_value_t<Input>::element_type>>
    range_of_unique_ptr_to_vec_of_shared_ptr(Input& input)
    {
        using T = std::ranges::range_value_t<Input>::element_type;
        std::vector<std::shared_ptr<T>> shared_ptrs;
        shared_ptrs.reserve(std::ranges::size(input));
        std::ranges::transform(
            input,
            std::back_inserter(shared_ptrs),
            [](auto& child)
            {
                return std::shared_ptr<T>(child.release(), child.get_deleter());
            }
        );
        return shared_ptrs;
    }

    /**
     * Transforms a range of unique pointers to a vector of value_ptr.
     *
     * @tparam Input A range of unique_ptr.
     * @param input The input range.
     * @return A vector of value_ptr.
     */
    template <std::ranges::input_range Input>
        requires mpl::unique_ptr<std::ranges::range_value_t<Input>>
    std::vector<sparrow::value_ptr<
        typename std::ranges::range_value_t<Input>::element_type,
        typename std::ranges::range_value_t<Input>::deleter_type>>
    range_of_unique_ptr_to_vec_of_value_ptr(Input& input)
    {
        using UniquePtr = std::ranges::range_value_t<Input>;
        using T = UniquePtr::element_type;
        using D = UniquePtr::deleter_type;
        std::vector<value_ptr<T, D>> values_ptrs;
        values_ptrs.reserve(std::ranges::size(input));
        std::ranges::transform(
            input,
            std::back_inserter(values_ptrs),
            [](auto& child)
            {
                return value_ptr<T, D>(std::move(child));
            }
        );
        return values_ptrs;
    }

    template <class T>
    constexpr int64_t get_size(const T& value)
        requires arrow_array_buffers_or_children<T>
    {
        if constexpr (std::ranges::sized_range<T>)
        {
            return static_cast<int64_t>(std::ranges::size(value));
        }
        else if constexpr (mpl::is_type_instance_of_v<T, std::tuple>)
        {
            return std::tuple_size_v<T>;
        }
        else
        {
            return 0;
        }
    }
}
