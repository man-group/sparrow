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

namespace sparrow
{
    constexpr int SPARROW_VERSION_MAJOR = 1;
    constexpr int SPARROW_VERSION_MINOR = 1;
    constexpr int SPARROW_VERSION_PATCH = 0;

    constexpr int SPARROW_BINARY_CURRENT = 10;
    constexpr int SPARROW_BINARY_REVISION = 0;
    constexpr int SPARROW_BINARY_AGE = 1;

    static_assert(
        SPARROW_BINARY_AGE <= SPARROW_BINARY_CURRENT,
        "SPARROW_BINARY_AGE cannot be greater than SPARROW_BINARY_CURRENT"
    );
}
