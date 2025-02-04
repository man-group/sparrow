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

#include "sparrow/layout/array_trivial_copyable.hpp"
#include "sparrow/layout/temporal/date_types.hpp"

// tdD : std::chrono::seconds
// tdm : std::chrono::milliseconds

namespace sparrow
{
    using date_types_t = mpl::typelist<date_days, date_milliseconds>;

    static constexpr date_types_t date_types;
    template <typename T>
    concept date_type = mpl::contains<T>(date_types);

    /**
     * Array of std::chrono::duration values.
     *
     * As the other arrays in sparrow, \c date_array<T> provides an API as if it was holding
     * \c nullable<T> values instead of \c T values.
     *
     * Internally, the array contains a validity bitmap and a contiguous memory buffer
     * holding the values.
     *
     * @tparam T the type of the values in the array.
     * @see https://arrow.apache.org/docs/dev/format/Columnar.html#fixed-size-primitive-layout
     */
    template <date_type T>
    using date_array = array_trivial_copyable<T>;

    using date_days_array = date_array<date_days>;
    using date_milliseconds_array = date_array<date_milliseconds>;

    template <class T>
    struct is_date_array : std::false_type
    {
    };

    template <class T>
    struct is_date_array<date_array<T>> : std::true_type
    {
    };

    /**
     * Checks whether T is a date_array type.
     */
    template <class T>
    constexpr bool is_date_array_v = is_date_array<T>::value;
}
