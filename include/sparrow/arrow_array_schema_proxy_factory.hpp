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

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_factory.hpp"
#include "sparrow/types/data_traits.hpp"

namespace sparrow
{
    template <std::ranges::sized_range Values, std::ranges::sized_range Nulls>
        requires std::is_arithmetic_v<std::ranges::range_value_t<Values>>
                 && std::integral<std::ranges::range_value_t<Nulls>>
    arrow_proxy make_primitive_arrow_proxy(
        Values&& values,
        Nulls&& nulls,
        int64_t offset,
        std::string_view name,
        std::optional<std::string_view> metadata
    )
    {
        using ValueType = std::ranges::range_value_t<Values>;
        return arrow_proxy{
            make_primitive_arrow_array(std::forward<Values>(values), std::forward<Nulls>(nulls), offset),
            make_primitive_arrow_schema(arrow_traits<ValueType>::type_id, name, metadata, std::nullopt)
        };
    }
}
