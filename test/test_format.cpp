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

#include <version>

#if defined(__cpp_lib_format)

#    include "sparrow/utils/format.hpp"

#    include "doctest/doctest.h"


using namespace sparrow;

TEST_SUITE("format")
{
    TEST_CASE("max_width")
    {
        SUBCASE("empty")
        {
            const std::vector<std::string> data;
            CHECK_EQ(max_width(data), 0);
        }

        SUBCASE("single")
        {
            const std::vector<std::string> data{"a"};
            CHECK_EQ(max_width(data), 1);
        }

        SUBCASE("multiple")
        {
            const std::vector<std::string> data{"a", "bb", "ccc"};
            CHECK_EQ(max_width(data), 3);
        }

        SUBCASE("floating points")
        {
            const std::vector<double> data{1.0, 2.0, 3.456};
            CHECK_EQ(max_width(data), 5);
        }

        SUBCASE("std::variant")
        {
            std::vector<std::variant<int, double, std::string>> data{1, 2.0, "three"};
            CHECK_EQ(max_width(data), 5);
        }
    }

    TEST_CASE("columns_widths")
    {
        SUBCASE("empty")
        {
            const std::vector<std::vector<std::string>> columns;
            const std::vector<size_t> widths = columns_widths(columns);
            CHECK(widths.empty());
        }

        SUBCASE("single")
        {
            const std::vector<std::vector<std::string>> columns{{"a"}};
            const std::vector<size_t> widths = columns_widths(columns);
            CHECK_EQ(widths, std::vector<size_t>{1});
        }

        SUBCASE("multiple, single columns")
        {
            const std::vector<std::vector<std::string>> columns{{"a", "bb", "ccc"}};
            CHECK_EQ(columns_widths(columns), std::vector<size_t>{3});
        }

        SUBCASE("multiple columns")
        {
            const std::vector<std::vector<std::string>> columns{{"a", "bb", "ccc"}, {"d", "ee", "ffff"}};
            CHECK_EQ(columns_widths(columns), std::vector<size_t>{3, 4});
        }
    }

    TEST_CASE("to_row")
    {
        SUBCASE("empty")
        {
            std::string out;
            const std::vector<size_t> widths{};
            const std::vector<std::string> values{};
            to_row(std::back_inserter(out), widths, values);
            CHECK_EQ(out, "");
        }

        SUBCASE("single")
        {
            std::string out;
            const std::vector<size_t> widths{1};
            const std::vector<std::string> values{"a"};
            to_row(std::back_inserter(out), widths, values);
            CHECK_EQ(out, "|a|");
        }

        SUBCASE("multiple")
        {
            std::string out;
            const std::vector<size_t> widths{1, 2, 3};
            const std::vector<std::string> values{"a", "bb", "ccc"};
            to_row(std::back_inserter(out), widths, values);
            CHECK_EQ(out, "|a|bb|ccc|");
        }

        SUBCASE("with formats")
        {
            std::string out;
            const std::vector<size_t> widths{3, 4, 3};
            const std::vector<std::string> values{"a", "bb", "ccc"};
            to_row(std::back_inserter(out), widths, values);
            CHECK_EQ(out, "|  a|  bb|ccc|");
        }

        SUBCASE("with variant")
        {
            std::string out;
            const std::vector<size_t> widths{3, 4, 8};
            const std::vector<std::variant<int, double, std::string>> values{1, 2.0, "three"};
            to_row(std::back_inserter(out), widths, values);
            CHECK_EQ(out, "|  1|   2|   three|");
        }
    }

    TEST_CASE("horizontal_separator")
    {
        constexpr std::string_view separator = "-";
        SUBCASE("empty")
        {
            std::string out;
            horizontal_separator(std::back_inserter(out), std::vector<size_t>{}, separator);
            CHECK_EQ(out, "");
        }

        SUBCASE("single")
        {
            std::string out;
            horizontal_separator(std::back_inserter(out), std::vector<size_t>{1}, separator);
            CHECK_EQ(out, "---");
        }

        SUBCASE("multiple")
        {
            std::string out;
            horizontal_separator(std::back_inserter(out), std::vector<size_t>{1, 2, 3}, separator);
            CHECK_EQ(out, "----------");
        }
    }

    TEST_CASE("to_table_with_columns")
    {
        SUBCASE("empty")
        {
            std::string out;
            const std::vector<std::string> names{};
            const std::vector<std::vector<std::string>> columns{};
            to_table_with_columns(std::back_inserter(out), names, columns);
            CHECK_EQ(out, "");
        }

        SUBCASE("single")
        {
            std::string out;
            const std::vector<std::string> names{"a"};
            const std::vector<std::vector<int>> columns{{1}};
            to_table_with_columns(std::back_inserter(out), names, columns);
            const std::string expected = "|a|\n"
                                         "---\n"
                                         "|1|\n"
                                         "---";
            CHECK_EQ(out, expected);
        }

        SUBCASE("multiple")
        {
            std::string out;
            const std::vector<std::string> names{"a", "bb"};
            const std::vector<std::vector<std::string>> columns{{"1", "2"}, {"long", "4"}};
            const std::string expected = "|a|  bb|\n"
                                         "--------\n"
                                         "|1|long|\n"
                                         "|2|   4|\n"
                                         "--------";
            to_table_with_columns(std::back_inserter(out), names, columns);
            CHECK_EQ(out, expected);
        }
    }
}

#endif
