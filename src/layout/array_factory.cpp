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

#include "sparrow/layout/array_factory.hpp"

#include "sparrow/date_array.hpp"
#include "sparrow/decimal_array.hpp"
#include "sparrow/dictionary_encoded_array.hpp"
#include "sparrow/duration_array.hpp"
#include "sparrow/fixed_width_binary_array.hpp"
#include "sparrow/interval_array.hpp"
#include "sparrow/list_array.hpp"
#include "sparrow/map_array.hpp"
#include "sparrow/null_array.hpp"
#include "sparrow/primitive_array.hpp"
#include "sparrow/run_end_encoded_array.hpp"
#include "sparrow/struct_array.hpp"
#include "sparrow/time_array.hpp"
#include "sparrow/timestamp_array.hpp"
#include "sparrow/timestamp_without_timezone_array.hpp"
#include "sparrow/union_array.hpp"
#include "sparrow/utils/temporal.hpp"
#include "sparrow/variable_size_binary_array.hpp"
#include "sparrow/variable_size_binary_view_array.hpp"

namespace sparrow
{
    namespace detail
    {
        template <class T>
        cloning_ptr<array_wrapper> make_wrapper_ptr(arrow_proxy proxy)
        {
            return cloning_ptr<array_wrapper>{new array_wrapper_impl<T>(T(std::move(proxy)))};
        }
    }

    cloning_ptr<array_wrapper> array_factory(arrow_proxy proxy)
    {
        const auto dt = proxy.data_type();

        if (proxy.dictionary())
        {
            switch (dt)
            {
                case data_type::INT8:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::int8_t>>(std::move(proxy));
                case data_type::UINT8:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::uint8_t>>(std::move(proxy));
                case data_type::INT16:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::int16_t>>(std::move(proxy));
                case data_type::UINT16:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::uint16_t>>(std::move(proxy));
                case data_type::INT32:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::int32_t>>(std::move(proxy));
                case data_type::UINT32:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::uint32_t>>(std::move(proxy));
                case data_type::INT64:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::int64_t>>(std::move(proxy));
                case data_type::UINT64:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::uint64_t>>(std::move(proxy));
                default:
                    throw std::runtime_error("data datype of dictionary encoded array must be an integer");
            }
        }
        else
        {
            switch (dt)
            {
                case data_type::NA:
                    return detail::make_wrapper_ptr<null_array>(std::move(proxy));
                case data_type::BOOL:
                    return detail::make_wrapper_ptr<primitive_array<bool>>(std::move(proxy));
                case data_type::INT8:
                    return detail::make_wrapper_ptr<primitive_array<std::int8_t>>(std::move(proxy));
                case data_type::UINT8:
                    return detail::make_wrapper_ptr<primitive_array<std::uint8_t>>(std::move(proxy));
                case data_type::INT16:
                    return detail::make_wrapper_ptr<primitive_array<std::int16_t>>(std::move(proxy));
                case data_type::UINT16:
                    return detail::make_wrapper_ptr<primitive_array<std::uint16_t>>(std::move(proxy));
                case data_type::INT32:
                    return detail::make_wrapper_ptr<primitive_array<std::int32_t>>(std::move(proxy));
                case data_type::UINT32:
                    return detail::make_wrapper_ptr<primitive_array<std::uint32_t>>(std::move(proxy));
                case data_type::INT64:
                    return detail::make_wrapper_ptr<primitive_array<std::int64_t>>(std::move(proxy));
                case data_type::UINT64:
                    return detail::make_wrapper_ptr<primitive_array<std::uint64_t>>(std::move(proxy));
                case data_type::HALF_FLOAT:
                    return detail::make_wrapper_ptr<primitive_array<float16_t>>(std::move(proxy));
                case data_type::FLOAT:
                    return detail::make_wrapper_ptr<primitive_array<float32_t>>(std::move(proxy));
                case data_type::DOUBLE:
                    return detail::make_wrapper_ptr<primitive_array<float64_t>>(std::move(proxy));
                case data_type::LIST:
                    return detail::make_wrapper_ptr<list_array>(std::move(proxy));
                case data_type::LARGE_LIST:
                    return detail::make_wrapper_ptr<big_list_array>(std::move(proxy));
                case data_type::LIST_VIEW:
                    return detail::make_wrapper_ptr<list_view_array>(std::move(proxy));
                case data_type::LARGE_LIST_VIEW:
                    return detail::make_wrapper_ptr<big_list_view_array>(std::move(proxy));
                case data_type::FIXED_SIZED_LIST:
                    return detail::make_wrapper_ptr<fixed_sized_list_array>(std::move(proxy));
                case data_type::STRUCT:
                    return detail::make_wrapper_ptr<struct_array>(std::move(proxy));
                case data_type::STRING:
                    return detail::make_wrapper_ptr<string_array>(std::move(proxy));
                case data_type::STRING_VIEW:
                    return detail::make_wrapper_ptr<string_view_array>(std::move(proxy));
                case data_type::LARGE_STRING:
                    return detail::make_wrapper_ptr<big_string_array>(std::move(proxy));
                case data_type::BINARY:
                    return detail::make_wrapper_ptr<binary_array>(std::move(proxy));
                case data_type::LARGE_BINARY:
                    return detail::make_wrapper_ptr<big_binary_array>(std::move(proxy));
                case data_type::RUN_ENCODED:
                    return detail::make_wrapper_ptr<run_end_encoded_array>(std::move(proxy));
                case data_type::DENSE_UNION:
                    return detail::make_wrapper_ptr<dense_union_array>(std::move(proxy));
                case data_type::SPARSE_UNION:
                    return detail::make_wrapper_ptr<sparse_union_array>(std::move(proxy));
                case data_type::DATE_DAYS:
                    return detail::make_wrapper_ptr<date_days_array>(std::move(proxy));
                case data_type::DATE_MILLISECONDS:
                    return detail::make_wrapper_ptr<date_milliseconds_array>(std::move(proxy));
                case data_type::TIMESTAMP_SECONDS:
                    if (get_timezone(proxy) == nullptr)
                    {
                        return detail::make_wrapper_ptr<timestamp_without_timezone_seconds_array>(
                            std::move(proxy)
                        );
                    }
                    else
                    {
                        return detail::make_wrapper_ptr<timestamp_seconds_array>(std::move(proxy));
                    }
                case data_type::TIMESTAMP_MILLISECONDS:
                    if (get_timezone(proxy) == nullptr)
                    {
                        return detail::make_wrapper_ptr<timestamp_without_timezone_milliseconds_array>(
                            std::move(proxy)
                        );
                    }
                    else
                    {
                        return detail::make_wrapper_ptr<timestamp_milliseconds_array>(std::move(proxy));
                    }
                case data_type::TIMESTAMP_MICROSECONDS:
                    if (get_timezone(proxy) == nullptr)
                    {
                        return detail::make_wrapper_ptr<timestamp_without_timezone_microseconds_array>(
                            std::move(proxy)
                        );
                    }
                    else
                    {
                        return detail::make_wrapper_ptr<timestamp_microseconds_array>(std::move(proxy));
                    }
                case data_type::TIMESTAMP_NANOSECONDS:
                    if (get_timezone(proxy) == nullptr)
                    {
                        return detail::make_wrapper_ptr<timestamp_without_timezone_nanoseconds_array>(
                            std::move(proxy)
                        );
                    }
                    else
                    {
                        return detail::make_wrapper_ptr<timestamp_nanoseconds_array>(std::move(proxy));
                    }
                case data_type::DURATION_SECONDS:
                    return detail::make_wrapper_ptr<duration_seconds_array>(std::move(proxy));
                case data_type::DURATION_MILLISECONDS:
                    return detail::make_wrapper_ptr<duration_milliseconds_array>(std::move(proxy));
                case data_type::DURATION_MICROSECONDS:
                    return detail::make_wrapper_ptr<duration_microseconds_array>(std::move(proxy));
                case data_type::DURATION_NANOSECONDS:
                    return detail::make_wrapper_ptr<duration_nanoseconds_array>(std::move(proxy));
                case data_type::INTERVAL_MONTHS:
                    return detail::make_wrapper_ptr<months_interval_array>(std::move(proxy));
                case data_type::INTERVAL_DAYS_TIME:
                    return detail::make_wrapper_ptr<days_time_interval_array>(std::move(proxy));
                case data_type::INTERVAL_MONTHS_DAYS_NANOSECONDS:
                    return detail::make_wrapper_ptr<month_day_nanoseconds_interval_array>(std::move(proxy));
                case data_type::TIME_SECONDS:
                    return detail::make_wrapper_ptr<time_seconds_array>(std::move(proxy));
                case data_type::TIME_MILLISECONDS:
                    return detail::make_wrapper_ptr<time_milliseconds_array>(std::move(proxy));
                case data_type::TIME_MICROSECONDS:
                    return detail::make_wrapper_ptr<time_microseconds_array>(std::move(proxy));
                case data_type::TIME_NANOSECONDS:
                    return detail::make_wrapper_ptr<time_nanoseconds_array>(std::move(proxy));
                case data_type::MAP:
                    return detail::make_wrapper_ptr<map_array>(std::move(proxy));
                case data_type::DECIMAL32:
                    return detail::make_wrapper_ptr<decimal_32_array>(std::move(proxy));
                case data_type::DECIMAL64:
                    return detail::make_wrapper_ptr<decimal_64_array>(std::move(proxy));
                case data_type::DECIMAL128:
                    return detail::make_wrapper_ptr<decimal_128_array>(std::move(proxy));
                case data_type::DECIMAL256:
                    return detail::make_wrapper_ptr<decimal_256_array>(std::move(proxy));
                case data_type::FIXED_WIDTH_BINARY:
                    return detail::make_wrapper_ptr<fixed_width_binary_array>(std::move(proxy));
                case data_type::BINARY_VIEW:
                    return detail::make_wrapper_ptr<binary_view_array>(std::move(proxy));
                default:
                    throw std::runtime_error("not supported data type");
            }
        }
    }
}
