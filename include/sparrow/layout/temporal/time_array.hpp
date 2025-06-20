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

#include "sparrow/layout/primitive_layout/primitive_array_impl.hpp"
#include "sparrow/layout/temporal/time_types.hpp"

// tts : std::chrono::seconds

namespace sparrow
{
    using time_types_t = mpl::
        typelist<chrono::time_seconds, chrono::time_milliseconds, chrono::time_microseconds, chrono::time_nanoseconds>;

    template <typename T>
    concept time_type = mpl::contains<time_types_t, T>();

    /**
     * Array of time values.
     *
     * As the other arrays in sparrow, \c time_array<T> provides an API as if it was holding
     * \c nullable<T> values instead of \c T values.
     *
     * Internally, the array contains a validity bitmap and a contiguous memory buffer
     * holding the values.
     *
     * @tparam T the type of the values in the array.
     * @see https://arrow.apache.org/docs/dev/format/Columnar.html#fixed-size-primitive-layout
     */
    template <time_type T>
    using time_array = primitive_array_impl<T>;

    /**
     * A time array for \c std::chrono::time_seconds values.
     * This is useful for representing times with second precision.
     */
    using time_seconds_array = time_array<chrono::time_seconds>;
    /**
     * A time array for \c std::chrono::time_milliseconds values.
     * This is useful for representing times with millisecond precision.
     */
    using time_milliseconds_array = time_array<chrono::time_milliseconds>;
    /**
     * A time array for \c std::chrono::time_microseconds values.
     * This is useful for representing times with microsecond precision.
     */
    using time_microseconds_array = time_array<chrono::time_microseconds>;
    /**
     * A time array for \c std::chrono::time_nanoseconds values.
     * This is useful for representing times with nanosecond precision.
     */
    using time_nanoseconds_array = time_array<chrono::time_nanoseconds>;

    template <class T>
    struct is_time_array : std::false_type
    {
    };

    template <class T>
    struct is_time_array<time_array<T>> : std::true_type
    {
    };

    /**
     * Checks whether T is a time_array type.
     */
    template <class T>
    constexpr bool is_time_array_v = is_time_array<T>::value;
}
