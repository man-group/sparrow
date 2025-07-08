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
#include <string>
#include <vector>

#include "sparrow/layout/array_access.hpp"
#include "sparrow/layout/variable_size_binary_view_array.hpp"
#include "sparrow/utils/nullable.hpp"

#include "metadata_sample.hpp"
#include "test_utils.hpp"


using namespace std::literals;

namespace sparrow
{
    TEST_SUITE("variable_size_binary_view_array")
    {
        static const std::vector<std::string> words{
            "short",
            "longer",
            "abcdefghijk",      // exactly 11
            "abcdefghijkl",     // exactly 12
            "123456789101112",  // longer than 12,
            "hello world this is a long string"
        };

        static const std::vector<std::size_t> where_nulls{1};

        TEST_CASE("string_view_array")
        {
            SUBCASE("constructors")
            {
                SUBCASE("range, validity, name and metadata")
                {
                    string_view_array array(words, where_nulls, "name", metadata_sample_opt);
                    CHECK_EQ(array.name(), "name");
                    test_metadata(metadata_sample, array.metadata().value());
                    CHECK_EQ(array.size(), words.size());
                    CHECK(detail::array_access::get_arrow_proxy(array).flags().contains(ArrowFlag::NULLABLE));
                }

                SUBCASE("nullable range, name and metadata")
                {
                    std::vector<sparrow::nullable<std::string>> nullable_words;
                    for (const auto& word : words)
                    {
                        nullable_words.emplace_back(word);
                    }
                    string_view_array array(nullable_words, "name", metadata_sample_opt);
                    CHECK_EQ(array.name(), "name");
                    test_metadata(metadata_sample, array.metadata().value());
                    CHECK_EQ(array.size(), words.size());
                    CHECK(detail::array_access::get_arrow_proxy(array).flags().contains(ArrowFlag::NULLABLE));
                }

                SUBCASE("range, nullable, name and metadata")
                {
                    SUBCASE("nullable == false")
                    {
                        string_view_array array(words, false, "name", metadata_sample_opt);
                        CHECK_EQ(array.name(), "name");
                        test_metadata(metadata_sample, array.metadata().value());
                        CHECK_EQ(array.size(), words.size());
                        CHECK(detail::array_access::get_arrow_proxy(array).flags().empty());
                    }

                    SUBCASE("nullable == true")
                    {
                        string_view_array array(words, true, "name", metadata_sample_opt);
                        CHECK_EQ(array.name(), "name");
                        test_metadata(metadata_sample, array.metadata().value());
                        CHECK_EQ(array.size(), words.size());
                        CHECK(detail::array_access::get_arrow_proxy(array).flags().contains(ArrowFlag::NULLABLE));
                    }
                }

                SUBCASE("copy")
                {
                    string_view_array array(words, where_nulls, "name", metadata_sample_opt);
                    string_view_array array_copy(array);
                    CHECK_EQ(array, array_copy);
                }
            }

            SUBCASE("general")
            {
                string_view_array array(words, where_nulls, "name", metadata_sample_opt);
                CHECK_EQ(array.name(), "name");
                test_metadata(metadata_sample, array.metadata().value());

                for (std::size_t i = 0; i < words.size(); ++i)
                {
                    if (i == 1)
                    {
                        CHECK_FALSE(array[i].has_value());
                    }
                    else
                    {
                        CHECK(array[i].has_value());
                        CHECK(array[i].value() == words[i]);
                    }
                }
            }

            SUBCASE("consistency")
            {
                string_view_array array(words, where_nulls, "name", metadata_sample_opt);
                test::generic_consistency_test(array);
            }
        }
    }
}
