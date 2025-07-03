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

    template <typename T>
    concept zoned_time_without_timezone_type = mpl::contains<zoned_time_without_timezone_types_t, T>();

    namespace detail
    {
        template <>
        struct primitive_data_traits<zoned_time_without_timezone_seconds>
        {
            static constexpr sparrow::data_type type_id = sparrow::data_type::TIMESTAMP_SECONDS;
        };

        template <>
        struct primitive_data_traits<zoned_time_without_timezone_milliseconds>
        {
            static constexpr sparrow::data_type type_id = sparrow::data_type::TIMESTAMP_MILLISECONDS;
        };

        template <>
        struct primitive_data_traits<zoned_time_without_timezone_microseconds>
        {
            static constexpr sparrow::data_type type_id = sparrow::data_type::TIMESTAMP_MICROSECONDS;
        };

        template <>
        struct primitive_data_traits<zoned_time_without_timezone_nanoseconds>
        {
            static constexpr sparrow::data_type type_id = sparrow::data_type::TIMESTAMP_NANOSECONDS;
        };
    }

    /**
     * Array of timestamps without timezone.
     */
    template <zoned_time_without_timezone_type T>
    using timestamp_without_timezone_array = primitive_array_impl<T>;

    /**
     * A timestamp without timezone array for \c zoned_time_without_timezone_seconds values.
     * This is useful for representing timestamps with second precision.
     */
    using timestamp_without_timezone_seconds_array = timestamp_without_timezone_array<zoned_time_without_timezone_seconds>;

    /**
     * A timestamp without timezone array for \c zoned_time_without_timezone_milliseconds values.
     * This is useful for representing timestamps with millisecond precision.
     */
    using timestamp_without_timezone_milliseconds_array = timestamp_without_timezone_array<
        zoned_time_without_timezone_milliseconds>;

    /**
     * A timestamp without timezone array for \c zoned_time_without_timezone_microseconds values.
     * This is useful for representing timestamps with microsecond precision.
     */
    using timestamp_without_timezone_microseconds_array = timestamp_without_timezone_array<
        zoned_time_without_timezone_microseconds>;
    /**
     * A timestamp without timezone array for \c zoned_time_without_timezone_nanoseconds values.
     * This is useful for representing timestamps with nanosecond precision.
     */
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
