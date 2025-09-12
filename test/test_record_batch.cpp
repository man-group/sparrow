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

#include "sparrow/fixed_width_binary_array.hpp"
#include "sparrow/primitive_array.hpp"
#include "sparrow/record_batch.hpp"
#include "sparrow/variable_size_binary_array.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    std::vector<array> make_array_list(const std::size_t data_size)
    {
        auto iota = std::ranges::iota_view{std::size_t(0), std::size_t(data_size)};
        primitive_array<std::uint16_t> pr0(
            iota
                | std::views::transform(
                    [](auto i)
                    {
                        return static_cast<std::uint16_t>(i);
                    }
                ),
            true,
            "column0"
        );
        auto iota2 = std::ranges::iota_view{std::int32_t(4), 4 + std::int32_t(data_size)};
        primitive_array<std::int32_t> pr1(iota2, true, "column1");
        auto iota3 = std::ranges::iota_view{std::int32_t(2), 2 + std::int32_t(data_size)};
        primitive_array<std::int32_t> pr2(iota3, true, "column2");

        std::vector<array> arr_list = {array(std::move(pr0)), array(std::move(pr1)), array(std::move(pr2))};
        return arr_list;
    }

    std::vector<std::string> make_name_list()
    {
        static const std::vector<std::string> name_list = {"first", "second", "third"};
        return name_list;
    }

    std::vector<array> make_named_array_list(const std::size_t data_size)
    {
        auto array_list = make_array_list(data_size);
        auto name_list = make_name_list();
        for (std::size_t i = 0; i < array_list.size(); ++i)
        {
            array_list[i].set_name(name_list[i]);
        }
        return array_list;
    }

    arrow_proxy make_rb_arrow_proxy(const std::size_t data_size)
    {
        auto arr_list = make_named_array_list(data_size);

        ArrowArray** arr_children = new ArrowArray*[data_size];
        ArrowSchema** sch_children = new ArrowSchema*[data_size];

        for (std::size_t i = 0; i < arr_list.size(); ++i)
        {
            auto [arr, sch] = extract_arrow_structures(std::move(arr_list[i]));
            arr_children[i] = new ArrowArray(std::move(arr));
            sch_children[i] = new ArrowSchema(std::move(sch));
        }

        std::vector<buffer<std::uint8_t>> arr_buffs(1, buffer<std::uint8_t>(nullptr, 0));

        auto rb_array = make_arrow_array(
            static_cast<int64_t>(data_size),
            0,
            0,
            std::move(arr_buffs),
            arr_children,
            repeat_view<bool>(true, arr_list.size()),
            nullptr,
            true
        );

        auto rb_schema = make_arrow_schema(
            std::string("+s"),
            std::optional<std::string_view>(),
            std::optional<std::vector<metadata_pair>>(),
            std::nullopt,
            sch_children,
            repeat_view<bool>(true, arr_list.size()),
            nullptr,
            true
        );

        return arrow_proxy(std::move(rb_array), std::move(rb_schema));
    }

    record_batch make_record_batch(const std::size_t data_size)
    {
        return record_batch(make_name_list(), make_array_list(data_size), "");
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
                record_batch record(make_array_list(col_size), "name");
                CHECK_EQ(record.nb_columns(), 3u);
                CHECK_EQ(record.nb_rows(), 10u);
                CHECK_EQ(record.name(), "name");
                CHECK_FALSE(std::ranges::equal(record.names(), make_name_list()));
            }

            SUBCASE("from struct array")
            {
                record_batch record0(struct_array(make_array_list(col_size), false, "name"));
                record_batch record1(make_array_list(col_size), "name");
                CHECK_EQ(record0, record1);
            }

            SUBCASE("from moved Arrow C structs")
            {
                auto record_exp = make_record_batch(col_size);
                auto proxy = make_rb_arrow_proxy(col_size);
                record_batch record(proxy.extract_array(), proxy.extract_schema());
                CHECK_EQ(record, record_exp);
            }

            SUBCASE("from pointers to Arrow C structs")
            {
                auto record_exp = make_record_batch(col_size);
                auto proxy = make_rb_arrow_proxy(col_size);
                record_batch record(&(proxy.array()), &(proxy.schema()));
                CHECK_EQ(record, record_exp);
            }

            SUBCASE("from const pointers to Arrow C structs")
            {
                auto record_exp = make_record_batch(col_size);
                const auto proxy = make_rb_arrow_proxy(col_size);
                record_batch record(&(proxy.array()), &(proxy.schema()));
                CHECK_EQ(record, record_exp);
            }

            SUBCASE("from ArrowArray&& and ArrowSchema*")
            {
                auto record_exp = make_record_batch(col_size);
                auto proxy = make_rb_arrow_proxy(col_size);
                record_batch record(proxy.extract_array(), &(proxy.schema()));
                CHECK_EQ(record, record_exp);
            }

            SUBCASE("from ArrowArray&& and const ArrowSchema*")
            {
                auto record_exp = make_record_batch(col_size);
                auto proxy = make_rb_arrow_proxy(col_size);
                const ArrowSchema* sch = &(proxy.schema());
                record_batch record(proxy.extract_array(), sch);
                CHECK_EQ(record, record_exp);
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

        TEST_CASE("const get_column")
        {
            const auto record = make_record_batch(col_size);
            const auto col_list = make_array_list(col_size);
            const auto name_list = make_name_list();

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
            auto iota = std::ranges::iota_view{std::int32_t(0), std::int32_t(col_size)};
            primitive_array<std::int32_t> pr3(iota, true, "column3");
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

        TEST_CASE("extract_arrow_structures")
        {
            auto record = make_record_batch(col_size);
            auto [arr, sch] = extract_arrow_structures(std::move(record));
            record_batch record2(std::move(arr), std::move(sch));
            auto record_check = make_record_batch(col_size);
            CHECK_EQ(record2, record_check);
        }

        TEST_CASE("add_column_reference")
        {
            SUBCASE("add single column by reference")
            {
                auto iota = std::ranges::iota_view{std::int32_t(0), std::int32_t(col_size)};
                primitive_array<std::int32_t> pr(iota, true, "ref_column");
                array ar(std::move(pr));

                record_batch record;
                record.add_column_reference("ref_column", ar);

                CHECK_EQ(record.nb_columns(), 1u);
                CHECK_EQ(record.nb_rows(), col_size);
                CHECK(record.contains_column("ref_column"));

                const auto& col = record.get_column("ref_column");
                CHECK_EQ(col, ar);
            }

            SUBCASE("add column by reference using array name")
            {
                auto iota = std::ranges::iota_view{std::int32_t(0), std::int32_t(col_size)};
                primitive_array<std::int32_t> pr(iota, true, "named_ref");
                array ar(std::move(pr));

                record_batch record;
                record.add_column_reference(ar);

                CHECK_EQ(record.nb_columns(), 1u);
                CHECK(record.contains_column("named_ref"));
            }

            SUBCASE("add multiple columns by reference")
            {
                auto iota1 = std::ranges::iota_view{std::int32_t(0), std::int32_t(col_size)};
                primitive_array<std::int32_t> pr1(iota1, true, "ref_col1");
                array ar1(std::move(pr1));

                auto iota2 = std::ranges::iota_view{std::int32_t(10), std::int32_t(10 + col_size)};
                primitive_array<std::int32_t> pr2(iota2, true, "ref_col2");
                array ar2(std::move(pr2));

                record_batch record;
                record.add_column_reference("ref_col1", ar1);
                record.add_column_reference("ref_col2", ar2);

                CHECK_EQ(record.nb_columns(), 2u);
                CHECK_EQ(record.nb_rows(), col_size);
                CHECK(record.contains_column("ref_col1"));
                CHECK(record.contains_column("ref_col2"));
            }
        }

        TEST_CASE("mixed_owned_and_referenced_columns")
        {
            SUBCASE("add owned then referenced")
            {
                auto iota1 = std::ranges::iota_view{std::int32_t(0), std::int32_t(col_size)};
                primitive_array<std::int32_t> owned(iota1, true, "owned_col");

                auto iota2 = std::ranges::iota_view{std::int32_t(10), std::int32_t(10 + col_size)};
                primitive_array<std::int32_t> pr_ref(iota2, true, "ref_col");
                array referenced(std::move(pr_ref));

                record_batch record;
                record.add_column(array(std::move(owned)));
                record.add_column_reference("ref_col", referenced);

                CHECK_EQ(record.nb_columns(), 2u);
                CHECK_EQ(record.nb_rows(), col_size);
                CHECK(record.contains_column("owned_col"));
                CHECK(record.contains_column("ref_col"));
            }

            SUBCASE("add referenced then owned")
            {
                auto iota1 = std::ranges::iota_view{std::int32_t(0), std::int32_t(col_size)};
                primitive_array<std::int32_t> pr_ref(iota1, true, "ref_col");
                array referenced(std::move(pr_ref));

                auto iota2 = std::ranges::iota_view{std::int32_t(10), std::int32_t(10 + col_size)};
                primitive_array<std::int32_t> owned(iota2, true, "owned_col");

                record_batch record;
                record.add_column_reference("ref_col", referenced);
                record.add_column(array(std::move(owned)));

                CHECK_EQ(record.nb_columns(), 2u);
                CHECK_EQ(record.nb_rows(), col_size);
                CHECK(record.contains_column("ref_col"));
                CHECK(record.contains_column("owned_col"));
            }

            SUBCASE("iterate over mixed columns")
            {
                auto iota1 = std::ranges::iota_view{std::int32_t(0), std::int32_t(col_size)};
                primitive_array<std::int32_t> owned(iota1, true, "owned");

                auto iota2 = std::ranges::iota_view{std::int32_t(10), std::int32_t(10 + col_size)};
                primitive_array<std::int32_t> pr_ref(iota2, true, "referenced");
                array referenced(std::move(pr_ref));

                record_batch record;
                record.add_column(array(std::move(owned)));
                record.add_column_reference("referenced", referenced);

                auto columns = record.columns();
                std::size_t count = 0;
                for (const auto& col : columns)
                {
                    CHECK(col.name().has_value());
                    ++count;
                }
                CHECK_EQ(count, 2u);
            }
        }

        TEST_CASE("extract_struct_array_with_references")
        {
            SUBCASE("extract from record with referenced columns")
            {
                auto iota1 = std::ranges::iota_view{std::int32_t(0), std::int32_t(col_size)};
                primitive_array<std::int32_t> pr1(iota1, true, "col1");
                array ar1(std::move(pr1));
                auto ar1_copy = ar1;

                auto iota2 = std::ranges::iota_view{std::int32_t(10), std::int32_t(10 + col_size)};
                primitive_array<std::int32_t> pr2(iota2, true, "col2");
                array ar2(std::move(pr2));
                auto ar2_copy = ar2;

                record_batch record;
                record.add_column_reference("col1", ar1);
                record.add_column_reference("col2", ar2);

                auto extracted = record.extract_struct_array();

                CHECK_EQ(extracted.size(), col_size);
                // Original arrays should still be valid
                CHECK_EQ(ar1, ar1_copy);
                CHECK_EQ(ar2, ar2_copy);
            }
        }

#if defined(__cpp_lib_format)
        TEST_CASE("formatter")
        {
            SUBCASE("simple")
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

            SUBCASE("complex")
            {
                sparrow::validity_bitmap vb(
                    std::vector<bool>{true, false, true, true, true, false, true, true, true, true}
                );
                sparrow::fixed_width_binary_array col(
                    std::vector<std::array<std::byte, 3>>{
                        {std::byte{1}, std::byte{2}, std::byte{3}},
                        {std::byte{4}, std::byte{5}, std::byte{6}},
                        {std::byte{7}, std::byte{8}, std::byte{9}},
                        {std::byte{10}, std::byte{11}, std::byte{12}},
                        {std::byte{13}, std::byte{14}, std::byte{15}},
                        {std::byte{16}, std::byte{17}, std::byte{18}},
                        {std::byte{19}, std::byte{20}, std::byte{21}},
                        {std::byte{22}, std::byte{23}, std::byte{24}},
                        {std::byte{25}, std::byte{26}, std::byte{27}},
                        {std::byte{28}, std::byte{29}, std::byte{30}}
                    },
                    vb,
                    "column fixed_width_binary_array"
                );

                sparrow::validity_bitmap vb2(
                    std::vector<bool>{true, true, true, false, true, false, true, true, true, true}
                );
                sparrow::string_array col2(
                    std::vector<std::string>{
                        "こんにちは",
                        "this",
                        "is",
                        "a",
                        "test",
                        "of",
                        "the",
                        "string",
                        "array",
                        "formatting"
                    },
                    vb2,
                    "column     string"
                );

                std::vector<array> arr_list;
                arr_list.emplace_back(std::move(col));
                arr_list.emplace_back(std::move(col2));


                sparrow::record_batch record_batch(std::move(arr_list));
                const std::string formatted = std::format("{}", record_batch);
                constexpr std::string_view expected = "|column fixed_width_binary_array|column     string|\n"
                                                      "---------------------------------------------------\n"
                                                      "|             <0x01, 0x02, 0x03>|       こんにちは|\n"
                                                      "|                           null|             this|\n"
                                                      "|             <0x07, 0x08, 0x09>|               is|\n"
                                                      "|             <0x0a, 0x0b, 0x0c>|             null|\n"
                                                      "|             <0x0d, 0x0e, 0x0f>|             test|\n"
                                                      "|                           null|             null|\n"
                                                      "|             <0x13, 0x14, 0x15>|              the|\n"
                                                      "|             <0x16, 0x17, 0x18>|           string|\n"
                                                      "|             <0x19, 0x1a, 0x1b>|            array|\n"
                                                      "|             <0x1c, 0x1d, 0x1e>|       formatting|\n"
                                                      "---------------------------------------------------";

                CHECK_EQ(formatted, expected);
            }
        }
#endif
    }
}
