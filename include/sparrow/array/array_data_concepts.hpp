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

#include <ranges>
#include <type_traits>

#include "sparrow/array/data_type.hpp"
#include "sparrow/utils/memory.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    /**
     * Matches types that are range of arrow base type extended.
     *
     * This concept checks if a type is a range and if its value type satisfies the
     * `is_arrow_base_type_extended` concept.
     *
     * @tparam R The range to check.
     */
    template <class R>
    concept range_of_arrow_base_type_extended = std::ranges::range<R>
                                                && is_arrow_base_type_extended<std::ranges::range_value_t<R>>;

    /**
     * Matches types that are range for array data.
     *
     * A type `R` is considered a range for array data if it satisfies either of the following conditions:
     * - It satisfies the `range_of_arrow_base_type_extended` concept.
     * - The unwrapped and decayed `range_value_t` of `R` satisfies the `range_of_arrow_base_type_extended`
     * concept.
     *
     * @tparam R The range to be checked.
     */
    template <class R>
    concept range_for_array_data = range_of_arrow_base_type_extended<R>
                                   || range_of_arrow_base_type_extended<
                                       std::unwrap_ref_decay_t<std::ranges::range_value_t<R>>>;

    /**
     * Matches a constant range that can be used with array data.
     *
     * @tparam R The type of the range.
     */
    template <class R>
    concept constant_range_for_array_data = mpl::constant_range<R> && range_for_array_data<R>;

    /**
     * Matches types that are valid data storage usable by layouts and the
     * `typed_array` type.
     */
    template <class T>
    concept data_storage_commons = requires(T t) {
        typename T::bitmap_type;
        typename T::buffer_type;
        { type_descriptor(t) } -> std::same_as<data_descriptor>;
        { length(t) } -> std::same_as<std::int64_t>;
        { offset(t) } -> std::same_as<std::int64_t>;
        { buffers_size(t) } -> std::same_as<std::size_t>;
        { child_data_size(t) } -> std::same_as<std::size_t>;
    };

    template <class T>
    concept data_storage_const_ref_for_buffer_bitmap = requires(const T& t, std::size_t i) {
        { bitmap(t) } -> std::same_as<const typename T::bitmap_type&>;
        { buffer_at(t, i) } -> std::same_as<const typename T::buffer_type&>;
    };

    template <class T>
    concept data_storage_copy_for_buffer_bitmap = requires(const T& t, std::size_t i) {
        { bitmap(t) } -> std::same_as<typename T::bitmap_type>;
        { buffer_at(t, i) } -> std::same_as<typename T::buffer_type>;
    };

    template <class T>
    concept immutable_data_storage_buffer_bitmap = data_storage_const_ref_for_buffer_bitmap<T>
                                                   || data_storage_copy_for_buffer_bitmap<T>;

    template <class T>
    concept mutable_data_storage = data_storage_commons<T> && immutable_data_storage_buffer_bitmap<T>
                                   && requires(T& t, std::size_t i) {
                                          { bitmap(t) } -> std::same_as<typename T::bitmap_type&>;
                                          { buffer_at(t, i) } -> std::same_as<typename T::buffer_type&>;
                                          { child_data_at(t, i) } -> std::same_as<T&>;
                                          { dictionary(t) } -> std::same_as<value_ptr<T>&>;
                                          buffers_clear(t);
                                          child_data_clear(t);
                                      };

    template <class T>
    concept immutable_data_storage_child_data_at_and_dictionnary = requires(const T& t, std::size_t i) {
        { child_data_at(t, i) } -> std::same_as<const T&>;
        { dictionary(t) } -> std::same_as<const value_ptr<T>&>;
    };

    template <class T>
    concept immutable_data_storage = data_storage_commons<T> && immutable_data_storage_buffer_bitmap<T>
                                     && immutable_data_storage_child_data_at_and_dictionnary<T>
                                     && (not mutable_data_storage<T>);


    template <class T>
    concept data_storage = immutable_data_storage<T> || mutable_data_storage<T>;

}  // namespace sparrow
