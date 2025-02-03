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

#include "sparrow/layout/array_trivial_copyable.hpp"
#include "sparrow/layout/temporal/interval_types.hpp"
#include "sparrow/types/data_traits.hpp"

// tiM : std::chrono::months
// tiD : sparrow::day_time_interval
// tin : sparrow::month_day_nanoseconds_interval

namespace sparrow
{
    using interval_types_t = mpl::typelist<chrono::months, days_time_interval, month_day_nanoseconds_interval>;

    static constexpr interval_types_t interval_types;
    template <typename T>
    concept interval_type = mpl::contains<T>(interval_types);

    /**
     * Array of interval values.
     *
     * As the other arrays in sparrow, \c interval_array<T> provides an API as if it was holding
     * \c nullable<T> values instead of \c T values.
     *
     * Internally, the array contains a validity bitmap and a contiguous memory buffer
     * holding the values.
     *
     * @tparam T the type of the values in the array.
     * @see https://arrow.apache.org/docs/dev/format/Columnar.html#fixed-size-primitive-layout
     */
    template <interval_type T>
    using interval_array = array_trivial_copyable<T>;

    using months_interval_array = interval_array<chrono::months>;
    using days_time_interval_array = interval_array<days_time_interval>;
    using month_day_nanoseconds_interval_array = interval_array<month_day_nanoseconds_interval>;

    template <class T>
    struct is_interval_array : std::false_type
    {
    };

    template <class T>
    struct is_interval_array<interval_array<T>> : std::true_type
    {
    };

    /**
     * Checks whether T is a interval_array type.
     */
    template <class T>
    constexpr bool is_interval_array_v = is_interval_array<T>::value;
}
