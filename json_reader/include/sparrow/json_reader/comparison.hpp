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

#include <optional>
#include <string>

#include <sparrow/c_interface.hpp>

namespace sparrow::json_reader
{
    std::optional<std::string>
    compare_schemas(const std::string& prefix, const ArrowSchema* schema, const ArrowSchema* schema_from_json);

    std::optional<std::string> compare_arrays(
        const std::string& prefix,
        ArrowArray* array,
        ArrowArray* array_from_json,
        ArrowSchema* schema_from_json
    );
}