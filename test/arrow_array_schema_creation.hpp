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

#include <utility>

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"

#include "external_array_data_creation.hpp"

inline std::pair<ArrowArray, ArrowSchema> make_external_arrow_schema_and_array()
{
    std::pair<ArrowArray, ArrowSchema> pair;
    constexpr size_t size = 10;
    constexpr size_t offset = 1;
    sparrow::test::fill_external_schema_and_array<uint32_t>(pair.second, pair.first, size, offset, {2, 3});
    return pair;
}

namespace detail
{
    inline void fill_sparrow_array_schema(ArrowArray& array, ArrowSchema& schema)
    {
        using namespace std::literals;
        sparrow::fill_arrow_schema(
            schema,
            sparrow::data_type_to_format(sparrow::data_type::UINT8),
            "test"sv,
            "test metadata"sv,
            std::nullopt,
            0,
            nullptr,
            nullptr
        );
        std::vector<sparrow::buffer<std::uint8_t>> buffers_dummy = {
            sparrow::buffer<std::uint8_t>({0xF3, 0xFF}),
            sparrow::buffer<std::uint8_t>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9})
        };
        sparrow::fill_arrow_array(array, 10, 2, 0, buffers_dummy, 0, nullptr, nullptr);
    }
}

inline sparrow::arrow_array_and_schema_pointers make_sparrow_arrow_schema_and_array_pointers()
{
    ArrowArray* array = new ArrowArray{};
    ArrowSchema* schema = new ArrowSchema{};
    detail::fill_sparrow_array_schema(*array, *schema);
    return {array, schema};
}

inline sparrow::arrow_array_and_schema make_sparrow_arrow_schema_and_array()
{
    ArrowArray array;
    ArrowSchema schema;
    detail::fill_sparrow_array_schema(array, schema);
    return {std::move(array), std::move(schema)};
}
