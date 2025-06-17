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

#include <string_view>

namespace sparrow::c_data_integration
{
    constexpr std::string_view VALIDITY = "VALIDITY";
    constexpr std::string_view DATA = "DATA";
    constexpr std::string_view OFFSET = "OFFSET";
    constexpr std::string_view SIZE = "SIZE";
    constexpr std::string_view TYPE_ID = "TYPE_ID";
    constexpr std::string_view VIEWS = "VIEWS";
    constexpr std::string_view VARIADIC_DATA_BUFFERS = "VARIADIC_DATA_BUFFERS";
    constexpr std::string_view INLINED = "INLINED";
    constexpr std::string_view PREFIX_HEX = "PREFIX_HEX";
    constexpr std::string_view BUFFER_INDEX = "BUFFER_INDEX";
}
