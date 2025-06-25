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

#include <sparrow/layout/decimal_array.hpp>
#include <sparrow/layout/dictionary_encoded_array.hpp>
#include <sparrow/layout/fixed_width_binary_array.hpp>
#include <sparrow/layout/list_layout/list_array.hpp>
#include <sparrow/layout/null_array.hpp>
#include <sparrow/layout/primitive_layout/primitive_array.hpp>
#include <sparrow/layout/run_end_encoded_layout/run_end_encoded_array.hpp>
#include <sparrow/layout/struct_layout/struct_array.hpp>
#include <sparrow/layout/temporal/date_array.hpp>
#include <sparrow/layout/temporal/duration_array.hpp>
#include <sparrow/layout/temporal/interval_array.hpp>
#include <sparrow/layout/temporal/timestamp_array.hpp>
#include <sparrow/layout/union_array.hpp>
#include <sparrow/layout/variable_size_binary_layout/variable_size_binary_array.hpp>
#include <sparrow/layout/variable_size_binary_view_array.hpp>

template <typename T>
concept HasForwardIteratorMethods = requires(T& t, const T& ct) {
    t.begin();
    t.end();
    ct.cbegin();
    ct.cend();
};


template <typename T>
concept HasAllBidirectionalIteratorMethods = std::ranges::range<T> && requires(T& t, const T& ct) {
    t.begin();
    t.end();
    ct.cbegin();
    ct.cend();
    t.rbegin();
    t.rend();
    ct.crbegin();
    ct.crend();
};


static_assert(HasAllBidirectionalIteratorMethods<sparrow::big_list_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::big_list_view_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::binary_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::binary_view_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::date_days_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::decimal_128_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::decimal_256_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::decimal_32_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::decimal_64_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::dense_union_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::dictionary_encoded_array<int32_t>>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::duration_milliseconds_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::fixed_width_binary_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::list_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::list_view_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::null_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::primitive_array<bool>>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::primitive_array<float>>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::primitive_array<int32_t>>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::primitive_array<int64_t>>);
static_assert(HasForwardIteratorMethods<sparrow::run_end_encoded_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::string_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::struct_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::timestamp_microseconds_array>);
static_assert(HasAllBidirectionalIteratorMethods<sparrow::timestamp_nanoseconds_array>);
