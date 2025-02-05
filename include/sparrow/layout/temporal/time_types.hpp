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
#include <cstdint>

#include "sparrow/layout/temporal/timestamp_array.hpp"
#include "sparrow/types/data_type.hpp"

namespace sparrow
{
    /**
     * A duration representing time elapsed since midnight.
     */
    struct time_seconds : public std::chrono::duration<int32_t>
    {
    };

    /**
     * A duration representing time elapsed since midnight, in milliseconds.
     */
    struct time_milliseconds : public std::chrono::duration<int32_t, std::milli>
    {
    };

    /**
     * A duration representing time elapsed since midnight, in microseconds.
     */
    struct time_microseconds : public std::chrono::duration<int64_t, std::micro>
    {
    };

    /**
     * A duration representing time elapsed since midnight, in nanoseconds.
     */
    struct time_nanoseconds : public std::chrono::duration<int64_t, std::nano>
    {
    };
}
