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

#include "sparrow/types/data_type.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    using timestamp_types_t = mpl::typelist<
        timestamp<std::chrono::seconds>,
        timestamp<std::chrono::milliseconds>,
        timestamp<std::chrono::microseconds>,
        timestamp<std::chrono::nanoseconds>>;

    static constexpr timestamp_types_t timestamp_types;
    template <typename T>
    concept timestamp_type = mpl::contains<T>(timestamp_types);
}  // namespace sparrow
