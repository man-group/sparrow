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

#include <chrono>

#include "sparrow/layout/temporal/temporal_types.hpp"

namespace sparrow
{
    using date_days = std::chrono::time_point<std::chrono::system_clock, chrono::days>;
    using date_milliseconds = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>;
}
