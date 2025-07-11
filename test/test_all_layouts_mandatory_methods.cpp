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

#include <ranges>

#include <sparrow/date_array.hpp>
#include <sparrow/decimal_array.hpp>
#include <sparrow/dictionary_encoded_array.hpp>
#include <sparrow/duration_array.hpp>
#include <sparrow/fixed_width_binary_array.hpp>
#include <sparrow/interval_array.hpp>
#include <sparrow/list_array.hpp>
#include <sparrow/map_array.hpp>
#include <sparrow/null_array.hpp>
#include <sparrow/primitive_array.hpp>
#include <sparrow/run_end_encoded_array.hpp>
#include <sparrow/struct_array.hpp>
#include <sparrow/time_array.hpp>
#include <sparrow/timestamp_array.hpp>
#include <sparrow/timestamp_without_timezone_array.hpp>
#include <sparrow/union_array.hpp>
#include <sparrow/variable_size_binary_array.hpp>
#include <sparrow/variable_size_binary_view_array.hpp>

static_assert(sparrow::layout<sparrow::big_list_array>);
static_assert(sparrow::layout<sparrow::big_list_view_array>);
static_assert(sparrow::layout<sparrow::binary_array>);
static_assert(sparrow::layout<sparrow::binary_view_array>);
static_assert(sparrow::layout<sparrow::date_days_array>);
static_assert(sparrow::layout<sparrow::date_milliseconds_array>);
static_assert(sparrow::layout<sparrow::days_time_interval_array>);
static_assert(sparrow::layout<sparrow::decimal_128_array>);
static_assert(sparrow::layout<sparrow::decimal_256_array>);
static_assert(sparrow::layout<sparrow::decimal_32_array>);
static_assert(sparrow::layout<sparrow::decimal_64_array>);
static_assert(sparrow::layout<sparrow::dense_union_array>);
static_assert(sparrow::layout<sparrow::dictionary_encoded_array<int32_t>>);
static_assert(sparrow::layout<sparrow::duration_seconds_array>);
static_assert(sparrow::layout<sparrow::duration_milliseconds_array>);
static_assert(sparrow::layout<sparrow::duration_microseconds_array>);
static_assert(sparrow::layout<sparrow::duration_nanoseconds_array>);
static_assert(sparrow::layout<sparrow::fixed_sized_list_array>);
static_assert(sparrow::layout<sparrow::fixed_width_binary_array>);
static_assert(sparrow::layout<sparrow::list_array>);
static_assert(sparrow::layout<sparrow::list_view_array>);
static_assert(sparrow::layout<sparrow::month_day_nanoseconds_interval_array>);
static_assert(sparrow::layout<sparrow::months_interval_array>);
static_assert(sparrow::layout<sparrow::map_array>);
static_assert(sparrow::layout<sparrow::null_array>);
static_assert(sparrow::layout<sparrow::primitive_array<bool>>);
static_assert(sparrow::layout<sparrow::primitive_array<float>>);
static_assert(sparrow::layout<sparrow::primitive_array<int32_t>>);
static_assert(sparrow::layout<sparrow::primitive_array<int64_t>>);
static_assert(sparrow::layout<sparrow::run_end_encoded_array>);
static_assert(sparrow::layout<sparrow::sparse_union_array>);
static_assert(sparrow::layout<sparrow::string_array>);
static_assert(sparrow::layout<sparrow::string_view_array>);
static_assert(sparrow::layout<sparrow::struct_array>);
static_assert(sparrow::layout<sparrow::time_seconds_array>);
static_assert(sparrow::layout<sparrow::time_milliseconds_array>);
static_assert(sparrow::layout<sparrow::time_microseconds_array>);
static_assert(sparrow::layout<sparrow::time_nanoseconds_array>);
static_assert(sparrow::layout<sparrow::timestamp_seconds_array>);
static_assert(sparrow::layout<sparrow::timestamp_milliseconds_array>);
static_assert(sparrow::layout<sparrow::timestamp_microseconds_array>);
static_assert(sparrow::layout<sparrow::timestamp_nanoseconds_array>);

static_assert(!sparrow::layout<std::vector<int32_t>>);
static_assert(!sparrow::layout<std::array<int32_t, 10>>);

