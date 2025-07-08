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

#include "sparrow/array.hpp"
#include "sparrow/list_array.hpp"
#include "sparrow/primitive_array.hpp"

#include "doctest/doctest.h"
#include "test_utils.hpp"

namespace sparrow
{

    TEST_SUITE("high_level_constructors")
    {
        TEST_CASE("list")
        {
            // a primite array
            std::size_t flat_size = 10;
            auto iota = std::ranges::iota_view{std::size_t(0), std::size_t(10)};
            primitive_array<std::uint16_t> primitive_arr(
                iota
                | std::views::transform(
                    [](auto i)
                    {
                        return static_cast<std::uint16_t>(i);
                    }
                )
            );

            // wrap into an detyped array
            array arr(std::move(primitive_arr));

            // create a list array (fixed size)
            std::uint64_t list_size = 2;
            fixed_sized_list_array list_arr(list_size, std::move(arr));
            CHECK(list_arr.size() == flat_size / list_size);

            std::size_t n = flat_size / static_cast<std::size_t>(list_size);

            const auto size = list_arr.size();

            REQUIRE_EQ(size, n);

            auto flat_i = 0;
            for (std::size_t i = 0; i < size; ++i)
            {
                auto list = list_arr[i].value();
                CHECK_EQ(list.size(), list_size);

                for (std::size_t j = 0; j < list.size(); ++j)
                {
                    auto opt_val_variant = list[j];
#if SPARROW_GCC_11_2_WORKAROUND
                    using variant_type = std::decay_t<decltype(opt_val_variant)>;
                    using base_type = typename variant_type::base_type;
#endif
                    std::visit(
                        [&](auto&& opt_val)
                        {
                            using nullable_type = std::decay_t<decltype(opt_val)>;
                            using inner_type = std::decay_t<typename nullable_type::value_type>;
                            if constexpr (std::is_same_v<inner_type, std::uint16_t>)
                            {
                                REQUIRE(opt_val.has_value());
                                CHECK_EQ(opt_val.value(), static_cast<std::uint16_t>(flat_i));
                            }
                            else
                            {
                                REQUIRE(false);
                            }
                        },
#if SPARROW_GCC_11_2_WORKAROUND
                        *static_cast<base_type*>(&opt_val_variant)
#else
                        opt_val_variant
#endif
                    );
                    ++flat_i;
                }
            }
        }
    }


}
