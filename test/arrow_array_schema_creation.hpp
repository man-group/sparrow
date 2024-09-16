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

#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"

#include "external_array_data_creation.hpp"

#include <utility>


constexpr std::pair<ArrowSchema, ArrowArray> make_external_arrow_schema_and_array()
{
    std::pair<ArrowSchema, ArrowArray> pair;
    constexpr size_t size = 10;
    constexpr size_t offset = 1;
    sparrow::test::fill_schema_and_array<uint32_t>(pair.first, pair.second, size, offset, {2, 3});
    return pair;
}

inline std::pair<ArrowSchema, ArrowArray> make_sparrow_arrow_schema_and_array()
{
    using namespace std::literals;
    ArrowSchema schema = *sparrow::make_arrow_schema_unique_ptr(
                              sparrow::data_type_to_format(sparrow::data_type::UINT8),
                              "test"sv,
                              "test metadata"sv,
                              std::nullopt,
                              0,
                              nullptr,
                              nullptr
    )
                              .release();

    std::vector<sparrow::buffer<std::uint8_t>> buffers_dummy = {
        sparrow::buffer<std::uint8_t>({0xF3, 0xFF}),
        sparrow::buffer<std::uint8_t>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9})
    };
    ArrowArray array = *sparrow::make_arrow_array_unique_ptr(10, 2, 0, 2, buffers_dummy, 0, nullptr, nullptr)
                            .release();
    return {schema, array};
}