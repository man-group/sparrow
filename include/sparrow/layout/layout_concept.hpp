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

#include <concepts>

#include "sparrow/utils/nullable.hpp"

namespace sparrow
{
    namespace detail
    {
        template <class T>
        concept layout_const_reference = is_nullable_v<T> || is_nullable_variant_v<T>;
    }

    /**
     * @brief Concept for layouts
     *
     * A layout is an implementation of one of the Apache Arrow Layouts defined
     * at https://arrow.apache.org/docs/format/Columnar.html#
     *
     * It provides an API similar to that of a constant linear standard container,
     * with additional requirements on the access operator.
     *
     * @param T Layout type to check.
     */
    template <class T>
    concept layout = requires(const T& t) {
        typename T::inner_value_type;
        typename T::value_type;
        typename T::const_reference;
        typename T::size_type;
        typename T::const_iterator;
        typename T::const_reverse_iterator;

        requires detail::layout_const_reference<typename T::const_reference>;

        { t[std::size_t()] } -> std::same_as<typename T::const_reference>;
        { t.size() } -> std::same_as<typename T::size_type>;

        { t.begin() } -> std::same_as<typename T::const_iterator>;
        { t.end() } -> std::same_as<typename T::const_iterator>;
        { t.cbegin() } -> std::same_as<typename T::const_iterator>;
        { t.cend() } -> std::same_as<typename T::const_iterator>;
        { t.rbegin() } -> std::same_as<typename T::const_reverse_iterator>;
        { t.rend() } -> std::same_as<typename T::const_reverse_iterator>;
        { t.crbegin() } -> std::same_as<typename T::const_reverse_iterator>;
        { t.crend() } -> std::same_as<typename T::const_reverse_iterator>;
    };
}
