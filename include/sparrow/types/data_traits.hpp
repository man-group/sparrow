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

#include <chrono>
#include <concepts>

#include "sparrow/layout/temporal/date_array.hpp"
#include "sparrow/layout/temporal/interval_types.hpp"
#include "sparrow/types/data_type.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/utils/vector_view.hpp"

namespace sparrow
{

    template <class T>
    struct common_native_types_traits
    {
        using value_type = T;
        using const_reference = const T&;
    };

    template <>
    struct arrow_traits<null_type>
    {
        static constexpr data_type type_id = data_type::NA;
        using value_type = null_type;
        using const_reference = null_type;
    };

    // Define automatically all standard floating-point and integral types support, including `bool`.
    template <class T>
        requires std::integral<T> or std::floating_point<T>
    struct arrow_traits<T> : common_native_types_traits<T>
    {
        static constexpr data_type type_id = data_type_from_size<T>();
    };

    template <>
    struct arrow_traits<std::string>
    {
        static constexpr data_type type_id = data_type::STRING;
        using value_type = std::string;
        using const_reference = std::string_view;
    };

    template <>
    struct arrow_traits<std::vector<byte_t>>
    {
        static constexpr data_type type_id = data_type::BINARY;
        using value_type = std::vector<byte_t>;
        using const_reference = vector_view<const byte_t>;
    };

    template <>
    struct arrow_traits<list_value>
    {
        static constexpr data_type type_id = data_type::LIST;
        using value_type = list_value;
        using const_reference = list_value;
    };

    template <>
    struct arrow_traits<struct_value>
    {
        static constexpr data_type type_id = data_type::STRUCT;
        using value_type = struct_value;
        using const_reference = struct_value;
    };

    template <>
    struct arrow_traits<decimal<std::int32_t>>
    {
        static constexpr data_type type_id = data_type::DECIMAL32;
        using value_type = decimal<std::int32_t>;
        using const_reference = decimal<std::int32_t>;
    };

    template <>
    struct arrow_traits<decimal<std::int64_t>>
    {
        static constexpr data_type type_id = data_type::DECIMAL64;
        using value_type = decimal<std::int64_t>;
        using const_reference = decimal<std::int64_t>;
    };

    template <>
    struct arrow_traits<decimal<int128_t>>
    {
        static constexpr data_type type_id = data_type::DECIMAL128;
        using value_type = decimal<int128_t>;
        using const_reference = decimal<int128_t>;
    };

    template <>
    struct arrow_traits<decimal<int256_t>>
    {
        static constexpr data_type type_id = data_type::DECIMAL256;
        using value_type = decimal<int256_t>;
        using const_reference = decimal<int256_t>;
    };

    template <>
    struct arrow_traits<date_days> : common_native_types_traits<date_days>
    {
        static constexpr data_type type_id = data_type::DATE_DAYS;
    };

    template <>
    struct arrow_traits<date_milliseconds> : common_native_types_traits<date_milliseconds>
    {
        static constexpr data_type type_id = data_type::DATE_MILLISECONDS;
    };

    template <>
    struct arrow_traits<std::chrono::seconds> : common_native_types_traits<std::chrono::seconds>
    {
        static constexpr data_type type_id = data_type::DURATION_SECONDS;
    };

    template <>
    struct arrow_traits<std::chrono::milliseconds> : common_native_types_traits<std::chrono::milliseconds>
    {
        static constexpr data_type type_id = data_type::DURATION_MILLISECONDS;
    };

    template <>
    struct arrow_traits<std::chrono::microseconds> : common_native_types_traits<std::chrono::microseconds>
    {
        static constexpr data_type type_id = data_type::DURATION_MICROSECONDS;
    };

    template <>
    struct arrow_traits<std::chrono::nanoseconds> : common_native_types_traits<std::chrono::nanoseconds>
    {
        static constexpr data_type type_id = data_type::DURATION_NANOSECONDS;
    };

    template <>
    struct arrow_traits<timestamp<std::chrono::seconds>>
    {
        static constexpr data_type type_id = data_type::TIMESTAMP_SECONDS;
        using value_type = timestamp<std::chrono::seconds>;
        using const_reference = timestamp<std::chrono::seconds>;
    };

    template <>
    struct arrow_traits<timestamp<std::chrono::milliseconds>>
    {
        static constexpr data_type type_id = data_type::TIMESTAMP_MILLISECONDS;
        using value_type = timestamp<std::chrono::milliseconds>;
        using const_reference = timestamp<std::chrono::milliseconds>;
    };

    template <>
    struct arrow_traits<timestamp<std::chrono::microseconds>>
    {
        static constexpr data_type type_id = data_type::TIMESTAMP_MICROSECONDS;
        using value_type = timestamp<std::chrono::microseconds>;
        using const_reference = timestamp<std::chrono::microseconds>;
    };

    template <>
    struct arrow_traits<timestamp<std::chrono::nanoseconds>>
    {
        static constexpr data_type type_id = data_type::TIMESTAMP_NANOSECONDS;
        using value_type = timestamp<std::chrono::nanoseconds>;
        using const_reference = timestamp<std::chrono::nanoseconds>;
    };

    template <>
    struct arrow_traits<chrono::months> : common_native_types_traits<chrono::months>
    {
        static constexpr data_type type_id = data_type::INTERVAL_MONTHS;
    };

    template <>
    struct arrow_traits<days_time_interval> : common_native_types_traits<days_time_interval>
    {
        static constexpr data_type type_id = data_type::INTERVAL_DAYS_TIME;
    };

    template <>
    struct arrow_traits<month_day_nanoseconds_interval>
        : common_native_types_traits<month_day_nanoseconds_interval>
    {
        static constexpr data_type type_id = data_type::INTERVAL_MONTHS_DAYS_NANOSECONDS;
    };

    namespace detail
    {
        template <class T>
        using array_inner_value_type_t = typename arrow_traits<T>::value_type;

        template <class T>
        using array_inner_const_reference_t = typename arrow_traits<T>::const_reference;

        template <class T>
        using array_value_type_t = nullable<array_inner_value_type_t<T>>;

        template <class T>
        using array_const_reference_t = nullable<array_inner_const_reference_t<T>>;
    }

    struct array_traits
    {
        using inner_value_type = /* std::variant<null_type, bool, uint8_t, ...> */
            mpl::rename<all_base_types_t, std::variant>;
        // std::variant can not hold references, we need to write something based on variant and
        // reference_wrapper
        // using inner_const_reference = /* std::variant<null_type, const bool&, const suint8_t&, ....> */
        /*    mpl::rename<
                mpl::transform<
                    detail::array_inner_const_reference_t,
                    all_base_types_t>,
                std::variant>;*/
        using value_type =  // nullable_variant<nullable<null_type>, nullable<bool>, nullable<uint8_t>, ...>
            mpl::rename<mpl::transform<detail::array_value_type_t, all_base_types_t>, nullable_variant>;
        using const_reference =  // nullable_variant<nullable<null_type>, nullable<const bool&>,
                                 // nullable<const uint8_t&>, ...>
            mpl::rename<mpl::transform<detail::array_const_reference_t, all_base_types_t>, nullable_variant>;
    };

    namespace predicate
    {

        struct
        {
            template <class T>
            consteval bool operator()(mpl::typelist<T>)
            {
                return sparrow::is_arrow_base_type<T>;
            }
        } constexpr is_arrow_base_type;

        struct
        {
            template <class T>
            consteval bool operator()(mpl::typelist<T>)
            {
                return sparrow::is_arrow_traits<arrow_traits<T>>;
            }
        } constexpr has_arrow_traits;
    }
}
