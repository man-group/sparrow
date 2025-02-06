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

#include <string_view>

#include "sparrow/layout/primitive_array.hpp"
#include "sparrow/record_batch.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    std::vector<array> make_array_list(const std::size_t data_size)
    {
        primitive_array<std::uint16_t> pr0(
            std::ranges::iota_view{std::size_t(0), std::size_t(data_size)}
                | std::views::transform(
                    [](auto i)
                    {
                        return static_cast<std::uint16_t>(i);
                    }
                ),
            "column0"
        );
        primitive_array<std::int32_t> pr1(
            std::ranges::iota_view{std::int32_t(4), 4 + std::int32_t(data_size)},
            "column1"
        );
        primitive_array<std::int32_t> pr2(
            std::ranges::iota_view{std::int32_t(2), 2 + std::int32_t(data_size)},
            "column2"
        );

        std::vector<array> arr_list = {array(std::move(pr0)), array(std::move(pr1)), array(std::move(pr2))};
        return arr_list;
    }

    std::vector<std::string> make_name_list()
    {
        std::vector<std::string> name_list = {"first", "second", "third"};
        return name_list;
    }

    record_batch make_record_batch(const std::size_t data_size)
    {
        return record_batch(make_name_list(), make_array_list(data_size));
    }

    TEST_SUITE("record_batch")
    {
        const std::size_t col_size = 10;

        TEST_CASE("constructor")
        {
            SUBCASE("from ranges")
            {
                auto record = make_record_batch(col_size);
                CHECK_EQ(record.nb_columns(), 3u);
                CHECK_EQ(record.nb_rows(), 10u);
            }

            SUBCASE("from initializer list")
            {
                auto col_list = make_array_list(col_size);

                record_batch record = {{"first", col_list[0]}, {"second", col_list[1]}, {"third", col_list[2]}};
                CHECK_EQ(record.nb_columns(), 3u);
                CHECK_EQ(record.nb_rows(), 10u);
            }

            SUBCASE("from column list")
            {
                record_batch record(make_array_list(col_size));
                CHECK_EQ(record.nb_columns(), 3u);
                CHECK_EQ(record.nb_rows(), 10u);
                CHECK_FALSE(std::ranges::equal(record.names(), make_name_list()));
            }

            SUBCASE("from struct array")
            {
                record_batch record0(struct_array(make_array_list(col_size)));
                record_batch record1(make_array_list(col_size));
                CHECK_EQ(record0, record1);
            }
        }

        TEST_CASE("operator==")
        {
            auto record1 = make_record_batch(col_size);
            auto record2 = make_record_batch(col_size);
            CHECK(record1 == record2);

            auto record3 = make_record_batch(col_size + 2u);
            CHECK(record1 != record3);
        }

        TEST_CASE("copy semantic")
        {
            auto record1 = make_record_batch(col_size);
            auto record2(record1);
            CHECK_EQ(record1, record2);

            auto record3 = make_record_batch(col_size + 2u);
            CHECK_NE(record1, record3);

            record3 = record2;
            CHECK_EQ(record1, record3);
        }

        TEST_CASE("move semantic")
        {
            auto record1 = make_record_batch(col_size);
            auto record_check(record1);
            auto record2(std::move(record1));
            CHECK_EQ(record2, record_check);

            auto record3 = make_record_batch(col_size + 2u);
            CHECK_NE(record3, record_check);

            record3 = std::move(record2);
            CHECK_EQ(record3, record_check);
        }

        TEST_CASE("contains column")
        {
            auto record = make_record_batch(col_size);
            auto name_list = make_name_list();
            for (const auto& name : name_list)
            {
                CHECK(record.contains_column(name));
            }
        }

        TEST_CASE("get_column_name")
        {
            auto record = make_record_batch(col_size);
            auto name_list = make_name_list();

            for (std::size_t i = 0; i < name_list.size(); ++i)
            {
                CHECK_EQ(record.get_column_name(i), name_list[i]);
            }
        }

        TEST_CASE("get_column")
        {
            auto record = make_record_batch(col_size);
            auto col_list = make_array_list(col_size);
            auto name_list = make_name_list();

            for (std::size_t i = 0; i < name_list.size(); ++i)
            {
                CHECK_EQ(col_list[i], record.get_column(i));
                CHECK_EQ(col_list[i], record.get_column(name_list[i]));
            }
        }

        TEST_CASE("names")
        {
            auto record = make_record_batch(col_size);
            auto name_list = make_name_list();
            auto names = record.names();

            bool res = std::ranges::equal(names, name_list);
            CHECK(res);
        }

        TEST_CASE("columns")
        {
            auto record = make_record_batch(col_size);
            auto col_list = make_array_list(col_size);
            auto columns = record.columns();

            bool res = std::ranges::equal(columns, col_list);
            CHECK(res);
        }

        TEST_CASE("extract_struct_array")
        {
            struct_array arr(make_array_list(col_size));
            struct_array control(arr);

            record_batch r(std::move(arr));
            auto extr = r.extract_struct_array();
            CHECK_EQ(extr, control);
        }

        TEST_CASE("add_column")
        {
            auto record = make_record_batch(col_size);
            primitive_array<std::int32_t> pr3(
                std::ranges::iota_view{std::int32_t(3), 3 + std::int32_t(col_size)},
                "column3"
            );

            auto ctrl = pr3;

            record.add_column(array(std::move(pr3)));
            std::vector<std::string> ctrl_name_list = make_name_list();
            ctrl_name_list.push_back("column3");
            std::vector<std::string> name_list(record.names().begin(), record.names().end());
            CHECK_EQ(name_list, ctrl_name_list);

            const auto& col3 = record.get_column(3);
            bool res = col3.visit(
                [&ctrl]<typename T>(const T& arg)
                {
                    if constexpr (std::same_as<primitive_array<std::int32_t>, T>)
                    {
                        return arg == ctrl;
                    }
                    else
                    {
                        return false;
                    }
                }
            );
            CHECK(res);
        }

#if defined(__cpp_lib_format)
        TEST_CASE("formatter")
        {
            const auto record = make_record_batch(col_size);
            const std::string formatted = std::format("{}", record);
            constexpr std::string_view expected = "|first|second|third|\n"
                                                  "--------------------\n"
                                                  "|    0|     4|    2|\n"
                                                  "|    1|     5|    3|\n"
                                                  "|    2|     6|    4|\n"
                                                  "|    3|     7|    5|\n"
                                                  "|    4|     8|    6|\n"
                                                  "|    5|     9|    7|\n"
                                                  "|    6|    10|    8|\n"
                                                  "|    7|    11|    9|\n"
                                                  "|    8|    12|   10|\n"
                                                  "|    9|    13|   11|\n"
                                                  "--------------------";
            CHECK_EQ(formatted, expected);
        }
#endif
    }
}
