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

#include "sparrow/array.hpp"
#include "sparrow/layout/map_layout/map_array.hpp"
#include "sparrow/layout/primitive_layout/primitive_array.hpp"
#include "sparrow/layout/variable_size_binary_layout/variable_size_binary_array.hpp"

#include "doctest/doctest.h"
#include "test_utils.hpp"

namespace sparrow
{
    namespace test
    {
        static const std::vector<std::string> keys =
            {"Dark Knight", "Dark Knight", "Meet the Parents", "Superman", "Meet the Parents", "Superman"};
        static const std::vector<int> items = {10, 8, 4, 5, 10, 0};
        static const std::vector<std::size_t> sizes = {1, 3, 0, 2};  //{ 0, 1, 4, 4, 6};
        static const std::unordered_set<std::size_t> where_nulls{2};

        struct map_array_underlying
        {
            array flat_keys;
            array flat_items;
            map_array::offset_buffer_type offsets;

            map_array_underlying()
                : flat_keys(string_array(keys))
                , flat_items(primitive_array<int>(items, where_nulls))
                , offsets(map_array::offset_from_sizes(sizes))
            {
            }
        };

        void check_array(const map_array& map_arr)
        {
            // check the size
            REQUIRE_EQ(map_arr.size(), sizes.size());

            // check the sizes
            for (std::size_t i = 0; i < sizes.size(); ++i)
            {
                if (where_nulls.contains(i))
                {
                    continue;
                }
                CHECK_EQ(map_arr[i].value().size(), sizes[i]);
            }

            // Check the values
            std::size_t flat_index = 0;
            for (std::size_t i = 0; i < sizes.size(); ++i)
            {
                if (where_nulls.contains(i))
                {
                    continue;
                }
                auto m = map_arr[i];
                for (const auto& v : m.value())
                {
                    CHECK_NULLABLE_VARIANT_EQ(v.first, std::string_view(keys[flat_index]));
                    if (v.second.has_value())
                    {
                        CHECK_NULLABLE_VARIANT_EQ(v.second, items[flat_index]);
                    }
                    ++flat_index;
                }
            }

            CHECK(detail::array_access::get_arrow_proxy(map_arr).flags().contains(ArrowFlag::MAP_KEYS_SORTED));
        }

        map_array make_map_array()
        {
            map_array_underlying mau;
            return map_array(
                std::move(mau.flat_keys),
                std::move(mau.flat_items),
                std::move(mau.offsets),
                where_nulls
            );
        }
    }

    TEST_SUITE("map_array")
    {
        static_assert(is_map_array_v<map_array>);

        TEST_CASE("constructors")
        {
            test::map_array_underlying mau;

            SUBCASE("from children arrays, offset and ...")
            {
                SUBCASE("... validity bitmap")
                {
                    map_array arr(
                        std::move(mau.flat_keys),
                        std::move(mau.flat_items),
                        std::move(mau.offsets),
                        test::where_nulls
                    );
                    test::check_array(arr);
                }

                SUBCASE("... nullable == true")
                {
                    map_array arr(std::move(mau.flat_keys), std::move(mau.flat_items), std::move(mau.offsets), true);
                    test::check_array(arr);
                }

                SUBCASE("... nullable == false")
                {
                    map_array arr(std::move(mau.flat_keys), std::move(mau.flat_items), std::move(mau.offsets), false);
                    test::check_array(arr);
                }
            }
        }

        TEST_CASE("copy")
        {
            test::map_array_underlying mau;
            map_array arr(
                std::move(mau.flat_keys),
                std::move(mau.flat_items),
                std::move(mau.offsets),
                test::where_nulls
            );

            map_array arr2(arr);
            CHECK_EQ(arr, arr2);

            const std::vector<std::string> keys2 = {"John", "Peter", "Paul"};
            const std::vector<int> items2 = {3, 2, 5};
            const std::vector<std::size_t> sizes2 = {2, 1};

            map_array arr3(
                array(string_array(keys2)),
                array(primitive_array<int>(items2)),
                map_array::offset_from_sizes(sizes2)
            );
            arr = arr3;
            CHECK_EQ(arr, arr3);
            CHECK_NE(arr, arr2);

            CHECK(detail::array_access::get_arrow_proxy(arr).flags().contains(ArrowFlag::MAP_KEYS_SORTED));
        }

        TEST_CASE("move")
        {
            test::map_array_underlying mau;
            map_array arr(
                std::move(mau.flat_keys),
                std::move(mau.flat_items),
                std::move(mau.offsets),
                test::where_nulls
            );

            map_array arr2(arr);
            map_array arr3(std::move(arr2));
            CHECK_EQ(arr3, arr);

            const std::vector<std::string> keys2 = {"John", "Peter", "Paul"};
            const std::vector<int> items2 = {3, 2, 5};
            const std::vector<std::size_t> sizes2 = {2, 1};

            map_array arr4(
                array(string_array(keys2)),
                array(primitive_array<int>(items2)),
                map_array::offset_from_sizes(sizes2)
            );
            CHECK_NE(arr4, arr);
            arr4 = std::move(arr3);
            CHECK_EQ(arr4, arr);
        }

        TEST_CASE("consistency")
        {
            test::map_array_underlying mau;
            map_array arr(
                std::move(mau.flat_keys),
                std::move(mau.flat_items),
                std::move(mau.offsets),
                test::where_nulls
            );
            test::generic_consistency_test(arr);
        }

        TEST_CASE("map_value")
        {
            test::map_array_underlying mau;
            map_array arr(
                std::move(mau.flat_keys),
                std::move(mau.flat_items),
                std::move(mau.offsets),
                test::where_nulls
            );

            std::size_t flat_index = 0;
            for (std::size_t i = 0; i < arr.size(); ++i)
            {
                if (test::where_nulls.contains(i))
                {
                    continue;
                }
                auto m = arr[i].value();

                auto it = m.cbegin();
                for (std::size_t j = 0; j < m.size(); ++j)
                {
                    auto k = test::keys[flat_index];
                    CHECK_EQ(m[k], m.at(k));
                    CHECK_EQ(m[k], it->second);
                    ++flat_index;
                    ++it;
                }
            }

            auto m = arr[0].value();
            CHECK_THROWS_AS(m[std::string("Batman")], std::out_of_range);
        }

#if defined(__cpp_lib_format)
        TEST_CASE("formatting")
        {
            test::map_array_underlying mau;
            map_array arr(
                std::move(mau.flat_keys),
                std::move(mau.flat_items),
                std::move(mau.offsets),
                test::where_nulls
            );
            const std::string formatted = std::format("{}", arr);
            constexpr std::string_view
                expected = "Map [name=nullptr | size=4] <<Dark Knight: 10, >, <Dark Knight: 8, Meet the Parents: null, Superman: 5, >, null, <Meet the Parents: 10, Superman: 0, >>";
            CHECK_EQ(formatted, expected);
        }
#endif
    }
}
