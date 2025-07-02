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

#include "sparrow/layout/primitive_layout/primitive_array_impl.hpp"

// tDs : std::chrono::seconds
// tDm : std::chrono::milliseconds
// tDu : std::chrono::microseconds
// tDn : std::chrono::nanoseconds

namespace sparrow
{
    using duration_types_t = mpl::
        typelist<std::chrono::seconds, std::chrono::milliseconds, std::chrono::microseconds, std::chrono::nanoseconds>;

    template <typename T>
    concept duration_type = mpl::contains<duration_types_t, T>();

    /**
     * Array of std::chrono::duration values.
     *
     * As the other arrays in sparrow, \c duration_array<T> provides an API as if it was holding
     * \c nullable<T> values instead of \c T values.
     *
     * Internally, the array contains a validity bitmap and a contiguous memory buffer
     * holding the values.
     *
     * @tparam T the type of the values in the array.
     * @see https://arrow.apache.org/docs/dev/format/Columnar.html#fixed-size-primitive-layout
     */
    template <duration_type T>
    using duration_array = primitive_array_impl<T>;

    /**
     * A duration array for \c std::chrono::seconds values.
     * This is useful for representing durations in seconds.
     */
    using duration_seconds_array = duration_array<std::chrono::seconds>;
    /**
     * A duration array for \c std::chrono::milliseconds values.
     * This is useful for representing durations in milliseconds.
     */
    using duration_milliseconds_array = duration_array<std::chrono::milliseconds>;
    /**
     * A duration array for \c std::chrono::microseconds values.
     * This is useful for representing durations in microseconds.
     */
    using duration_microseconds_array = duration_array<std::chrono::microseconds>;
    /**
     * A duration array for \c std::chrono::nanoseconds values.
     * This is useful for representing durations in nanoseconds.
     */
    using duration_nanoseconds_array = duration_array<std::chrono::nanoseconds>;

    template <class T>
    struct is_duration_array : std::false_type
    {
    };

    template <class T>
    struct is_duration_array<duration_array<T>> : std::true_type
    {
    };

    /**
     * Checks whether T is a duration_array type.
     */
    template <class T>
    constexpr bool is_duration_array_v = is_duration_array<T>::value;

    namespace detail
    {
        template <class T>
        struct get_data_type_from_array;

        template <>
        struct get_data_type_from_array<duration_seconds_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::DURATION_SECONDS;
            }
        };

        template <>
        struct get_data_type_from_array<duration_milliseconds_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::DURATION_MILLISECONDS;
            }
        };

        template <>
        struct get_data_type_from_array<duration_microseconds_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::DURATION_MICROSECONDS;
            }
        };

        template <>
        struct get_data_type_from_array<duration_nanoseconds_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::DURATION_NANOSECONDS;
            }
        };

    }
}
