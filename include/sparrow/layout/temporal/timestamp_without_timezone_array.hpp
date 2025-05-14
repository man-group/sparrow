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
#include "sparrow/layout/temporal/timestamp_without_timezone_types.hpp"
#include "sparrow/types/data_traits.hpp"

namespace sparrow
{
    using zoned_time_without_timezone_types_t = mpl::typelist<
        zoned_time_without_timezone_seconds,
        zoned_time_without_timezone_milliseconds,
        zoned_time_without_timezone_microseconds,
        zoned_time_without_timezone_nanoseconds>;

    static constexpr zoned_time_without_timezone_types_t zoned_time_without_timezone_types;
    template <typename T>
    concept zoned_time_without_timezone_type = mpl::contains<T>(zoned_time_without_timezone_types);

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
    template <zoned_time_without_timezone_type T>
    using timestamp_without_timezone_array = primitive_array_impl<T>;

    using timestamp_without_timezone_seconds_array = timestamp_without_timezone_array<zoned_time_without_timezone_seconds>;
    using timestamp_without_timezone_milliseconds_array = timestamp_without_timezone_array<
        zoned_time_without_timezone_milliseconds>;
    using timestamp_without_timezone_microseconds_array = timestamp_without_timezone_array<
        zoned_time_without_timezone_microseconds>;
    using timestamp_without_timezone_nanoseconds_array = timestamp_without_timezone_array<
        zoned_time_without_timezone_nanoseconds>;

    template <class T>
    struct is_timestamp_without_timezone_array : std::false_type
    {
    };

    template <class T>
    struct is_timestamp_without_timezone_array<timestamp_without_timezone_array<T>> : std::true_type
    {
    };

    /**
     * Checks whether T is a timestamp_without_timezone_array type.
     */
    template <class T>
    constexpr bool is_timestamp_without_timezone_array_v = is_timestamp_without_timezone_array<T>::value;
}
