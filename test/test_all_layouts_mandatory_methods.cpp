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
#include <sparrow/null_array.hpp>
#include <sparrow/primitive_array.hpp>
#include <sparrow/run_end_encoded_array.hpp>
#include <sparrow/struct_array.hpp>
#include <sparrow/timestamp_array.hpp>
#include <sparrow/union_array.hpp>
#include <sparrow/variable_size_binary_array.hpp>
#include <sparrow/variable_size_binary_view_array.hpp>

template <typename T>
concept forward_iterators = requires(T& t, const T& ct) {
    t.begin();
    t.end();
    ct.cbegin();
    ct.cend();
};  // TODO: Replace by std::ranges::forward_range


template <typename T>
concept bidirectional_iterator = forward_iterators<T> && requires(T& t, const T& ct) {
    t.begin();
    t.end();
    ct.cbegin();
    ct.cend();
    t.rbegin();
    t.rend();
    ct.crbegin();
    ct.crend();
};  // TODO: Replace by std::ranges::bidirectional_range


static_assert(bidirectional_iterator<sparrow::big_list_array>);
static_assert(bidirectional_iterator<sparrow::big_list_view_array>);
static_assert(bidirectional_iterator<sparrow::binary_array>);
static_assert(bidirectional_iterator<sparrow::binary_view_array>);
static_assert(bidirectional_iterator<sparrow::date_days_array>);
static_assert(bidirectional_iterator<sparrow::decimal_128_array>);
static_assert(bidirectional_iterator<sparrow::decimal_256_array>);
static_assert(bidirectional_iterator<sparrow::decimal_32_array>);
static_assert(bidirectional_iterator<sparrow::decimal_64_array>);
static_assert(bidirectional_iterator<sparrow::dense_union_array>);
static_assert(bidirectional_iterator<sparrow::dictionary_encoded_array<int32_t>>);
static_assert(bidirectional_iterator<sparrow::duration_milliseconds_array>);
static_assert(bidirectional_iterator<sparrow::fixed_width_binary_array>);
static_assert(bidirectional_iterator<sparrow::list_array>);
static_assert(bidirectional_iterator<sparrow::list_view_array>);
static_assert(bidirectional_iterator<sparrow::null_array>);
static_assert(bidirectional_iterator<sparrow::primitive_array<bool>>);
static_assert(bidirectional_iterator<sparrow::primitive_array<float>>);
static_assert(bidirectional_iterator<sparrow::primitive_array<int32_t>>);
static_assert(bidirectional_iterator<sparrow::primitive_array<int64_t>>);
static_assert(forward_iterators<sparrow::run_end_encoded_array>);
static_assert(bidirectional_iterator<sparrow::string_array>);
static_assert(bidirectional_iterator<sparrow::struct_array>);
static_assert(bidirectional_iterator<sparrow::timestamp_microseconds_array>);
static_assert(bidirectional_iterator<sparrow::timestamp_nanoseconds_array>);
