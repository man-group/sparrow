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

#include <sparrow/array.hpp>
#include <sparrow/layout/array_access.hpp>
#include <sparrow/layout/primitive_layout/primitive_array.hpp>

#include "sparrow/array_api.hpp"
#include "sparrow/utils/nullable.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    TEST_SUITE("flag_nullable")
    {
        const std::vector<int64_t> v{1, 2, 3, 4, 5};


        TEST_CASE("using the layout")
        {
            SUBCASE("using push_back")
            {
                primitive_array<int64_t> layout{v};
                const auto& proxy = detail::array_access::get_arrow_proxy(layout);
                CHECK(proxy.flags().empty());
                layout.push_back(make_nullable<int64_t>(99, true));
                CHECK(proxy.flags().empty());
                layout.push_back(make_nullable<int64_t>(99, false));
                CHECK(proxy.flags().contains(ArrowFlag::NULLABLE));
            }

            SUBCASE("using reference")
            {
                primitive_array<int64_t> layout{v};
                const auto& proxy = detail::array_access::get_arrow_proxy(layout);
                CHECK(proxy.flags().empty());
                layout[0] = make_nullable<int64_t>(99, true);
                CHECK(proxy.flags().empty());
                layout[1] = make_nullable<int64_t>(99, false);
                CHECK(proxy.flags().contains(ArrowFlag::NULLABLE));
            }

            SUBCASE("using at")
            {
                // layout.at(0) = make_nullable<int64_t>(99, true);
                // CHECK(proxy.flags().empty());
                // layout.at(1) = make_nullable<int64_t>(99, false);
                // CHECK(proxy.flags().contains(ArrowFlag::NULLABLE));
            }
        }
    }
}