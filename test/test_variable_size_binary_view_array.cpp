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
#include <cstring>
#include <string>
#include <vector>

#include "sparrow/layout/array_access.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/variable_size_binary_view_array.hpp"

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
#ifdef SPARROW_TRACK_COPIES
                    copy_tracker::reset("variable_size_binary_view_array");
#endif
                    const string_view_array array(words, where_nulls, "name", metadata_sample_opt);
                    const string_view_array array_copy(array);
                    CHECK_EQ(array, array_copy);
#ifdef SPARROW_TRACK_COPIES
                    CHECK_EQ(copy_tracker::count("variable_size_binary_view_array"), 1);
#endif
                }

                SUBCASE("u8_buffers constructor")
                {
                    // Create buffer view (16 bytes per element - each element contains length + inline data
                    // or pointers)
                    const std::size_t element_count = 3;
                    const std::size_t view_structure_size = 16;
                    sparrow::u8_buffer<uint8_t> buffer_view(element_count * view_structure_size);

                    // Clear buffer
                    std::memset(buffer_view.data(), 0, buffer_view.size());

                    // Test data: mix of short and long strings
                    const std::vector<std::string> test_words = {
                        "hi",
                        "short_string",
                        "this_is_a_very_long_string_that_exceeds_twelve_bytes"
                    };

                    // Create value buffers for long strings
                    std::vector<sparrow::u8_buffer<uint8_t>> value_buffers;

                    // Buffer for long strings (only the third string is > 12 bytes)
                    const std::string& long_string = test_words[2];
                    sparrow::u8_buffer<uint8_t> long_string_buffer(long_string.size());
                    std::memcpy(long_string_buffer.data(), long_string.data(), long_string.size());
                    value_buffers.push_back(std::move(long_string_buffer));

                    // Build view structures manually
                    for (std::size_t i = 0; i < element_count; ++i)
                    {
                        uint8_t* view_ptr = buffer_view.data() + (i * view_structure_size);
                        const std::string& word = test_words[i];

                        // Write length (first 4 bytes)
                        std::uint32_t length = static_cast<std::uint32_t>(word.size());
                        std::memcpy(view_ptr, &length, sizeof(std::uint32_t));

                        if (word.size() <= 12)
                        {
                            // Short string: store inline in bytes 4-15
                            std::memcpy(view_ptr + 4, word.data(), word.size());
                        }
                        else
                        {
                            // Long string: store prefix + buffer index + offset
                            std::memcpy(view_ptr + 4, word.data(), 4);  // prefix (4 bytes)
                            std::uint32_t buffer_index = 0;  // Relative index in variadic buffers (0-based)
                            std::memcpy(view_ptr + 8, &buffer_index, sizeof(std::uint32_t));
                            std::uint32_t offset = 0;  // offset in the buffer
                            std::memcpy(view_ptr + 12, &offset, sizeof(std::uint32_t));
                        }
                    }

                    // Create array with validity bitmap (no nulls)
                    std::vector<std::size_t> no_nulls{};
                    string_view_array array(
                        element_count,
                        std::move(buffer_view),
                        std::move(value_buffers),
                        no_nulls,
                        "u8_test",
                        metadata_sample_opt
                    );

                    CHECK_EQ(array.name(), "u8_test");
                    test_metadata(metadata_sample, array.metadata().value());
                    CHECK_EQ(array.size(), element_count);
                    CHECK(detail::array_access::get_arrow_proxy(array).flags().contains(ArrowFlag::NULLABLE));

                    // Verify the values
                    for (std::size_t i = 0; i < element_count; ++i)
                    {
                        if (array[i].has_value())
                        {
                            CHECK_EQ(array[i].value(), test_words[i]);
                        }
                    }
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
                        CHECK_EQ(array[i].value(), words[i]);
                    }
                }
                CHECK_EQ(detail::array_access::get_arrow_proxy(array).format(), "vu");

                const std::vector<std::vector<byte_t>> input{
                    {static_cast<byte_t>(1), static_cast<byte_t>(2), static_cast<byte_t>(3)},
                    {static_cast<byte_t>(4), static_cast<byte_t>(5), static_cast<byte_t>(6)},
                    {static_cast<byte_t>(7), static_cast<byte_t>(8), static_cast<byte_t>(9)}
                };

                binary_view_array binary_array(input, where_nulls, "name", metadata_sample_opt);
                CHECK_EQ(detail::array_access::get_arrow_proxy(binary_array).format(), "vz");
            }

            SUBCASE("consistency")
            {
                string_view_array array(words, where_nulls, "name", metadata_sample_opt);
                test::generic_consistency_test(array);
            }

            SUBCASE("mutating methods")
            {
                SUBCASE("resize_values")
                {
                    SUBCASE("shrink array")
                    {
                        string_view_array array(words, true);
                        const size_t original_size = array.size();
                        const size_t new_size = original_size - 2;

                        array.resize(new_size, sparrow::make_nullable<std::string>("fill"));

                        CHECK_EQ(array.size(), new_size);
                        for (std::size_t i = 0; i < new_size; ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i]);
                        }
                    }

                    SUBCASE("grow array with short string")
                    {
                        string_view_array array(words, true);
                        const auto original_size = array.size();
                        const auto new_size = original_size + 3;
                        const std::string fill_value = "new";

                        array.resize(new_size, sparrow::make_nullable(fill_value));

                        CHECK_EQ(array.size(), new_size);

                        // Check original elements
                        for (std::size_t i = 0; i < original_size; ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i]);
                        }

                        // Check new elements
                        for (std::size_t i = original_size; i < new_size; ++i)
                        {
                            CHECK_EQ(array[i].value(), fill_value);
                        }
                    }

                    SUBCASE("grow array with long string")
                    {
                        string_view_array array(words, true);
                        const auto original_size = array.size();
                        const auto new_size = original_size + 2;
                        const std::string fill_value = "this is a long string that exceeds 12 bytes";

                        array.resize(new_size, sparrow::make_nullable(fill_value));

                        CHECK_EQ(array.size(), new_size);

                        // Check original elements
                        for (std::size_t i = 0; i < original_size; ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i]);
                        }

                        // Check new elements
                        for (std::size_t i = original_size; i < new_size; ++i)
                        {
                            CHECK_EQ(array[i].value(), fill_value);
                        }
                    }

                    SUBCASE("resize to same size")
                    {
                        string_view_array array(words, true);
                        const auto original_size = array.size();

                        array.resize(original_size, sparrow::make_nullable<std::string>("unchanged"));

                        CHECK_EQ(array.size(), original_size);
                        for (std::size_t i = 0; i < original_size; ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i]);
                        }
                    }

                    SUBCASE("resize to zero")
                    {
                        string_view_array array(words, true);

                        array.resize(0, sparrow::make_nullable<std::string>("empty"));

                        CHECK_EQ(array.size(), 0);
                    }
                }

                SUBCASE("insert_value")
                {
                    SUBCASE("insert at beginning")
                    {
                        string_view_array array(words, true);
                        const sparrow::nullable<std::string> new_value = "prefix";
                        const auto original_size = array.size();

                        auto it = array.insert(array.cbegin(), new_value, 1);

                        REQUIRE_EQ(array.size(), original_size + 1);
                        CHECK_EQ(std::distance(array.begin(), it), 0);
                        CHECK_EQ(array[0].value(), new_value);

                        // Check shifted elements
                        for (std::size_t i = 1; i < array.size(); ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i - 1]);
                        }
                    }

                    SUBCASE("insert at middle")
                    {
                        string_view_array array(words, true);
                        const auto new_value = sparrow::make_nullable<std::string>("middle");
                        const auto original_size = array.size();
                        const std::size_t insert_pos = 2;

                        auto it = array.insert(array.cbegin() + insert_pos, new_value, 1);

                        REQUIRE_EQ(array.size(), original_size + 1);
                        CHECK_EQ(std::distance(array.begin(), it), insert_pos);
                        CHECK_EQ(array[insert_pos].value(), new_value);

                        // Check elements before insertion
                        for (std::size_t i = 0; i < insert_pos; ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i]);
                        }

                        // Check elements after insertion
                        for (std::size_t i = insert_pos + 1; i < array.size(); ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i - 1]);
                        }
                    }

                    SUBCASE("insert at end")
                    {
                        string_view_array array(words, true);
                        const auto new_value = sparrow::make_nullable<std::string>("suffix");
                        const auto original_size = array.size();

                        auto it = array.insert(array.cend(), new_value, 1);

                        REQUIRE_EQ(array.size(), original_size + 1);
                        CHECK_EQ(std::distance(array.begin(), it), original_size);
                        CHECK_EQ(array[original_size].value(), new_value);

                        // Check original elements
                        for (std::size_t i = 0; i < original_size; ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i]);
                        }
                    }

                    SUBCASE("insert multiple copies")
                    {
                        string_view_array array(words, true);
                        const auto new_value = sparrow::make_nullable<std::string>("repeated");
                        const auto original_size = array.size();
                        const std::size_t count = 3;
                        const std::size_t insert_pos = 1;

                        auto it = array.insert(array.cbegin() + insert_pos, new_value, count);

                        REQUIRE_EQ(array.size(), original_size + count);
                        CHECK_EQ(std::distance(array.begin(), it), insert_pos);

                        // Check elements before insertion
                        for (std::size_t i = 0; i < insert_pos; ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i]);
                        }

                        // Check inserted elements
                        for (std::size_t i = insert_pos; i < insert_pos + count; ++i)
                        {
                            CHECK_EQ(array[i].value(), new_value);
                        }

                        // Check elements after insertion
                        for (std::size_t i = insert_pos + count; i < array.size(); ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i - count]);
                        }
                    }

                    SUBCASE("insert long string")
                    {
                        string_view_array array(words, true);
                        const auto new_value = sparrow::make_nullable<std::string>(
                            "this is a very long string that definitely exceeds 12 bytes"
                        );
                        const auto original_size = array.size();

                        array.insert(array.cbegin() + 1, new_value, 1);

                        REQUIRE_EQ(array.size(), original_size + 1);
                        CHECK_EQ(array[1].value(), new_value);
                    }

                    SUBCASE("insert zero count")
                    {
                        string_view_array array(words, true);
                        const auto original_size = array.size();

                        auto it = array.insert(array.cbegin() + 1, sparrow::make_nullable<std::string>("test"), 0);

                        REQUIRE_EQ(array.size(), original_size);
                        CHECK_EQ(std::distance(array.begin(), it), 1);

                        // Check all elements unchanged
                        for (std::size_t i = 0; i < array.size(); ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i]);
                        }
                    }
                }

                SUBCASE("insert")
                {
                    SUBCASE("insert range at beginning")
                    {
                        string_view_array array(words, true);
                        const std::vector<sparrow::nullable<std::string>> to_insert = {
                            sparrow::make_nullable<std::string>("new1"),
                            sparrow::make_nullable<std::string>("new2"),
                            sparrow::make_nullable<std::string>("new3")
                        };
                        const auto original_size = array.size();

                        auto it = array.insert(array.cbegin(), to_insert.begin(), to_insert.end());

                        REQUIRE_EQ(array.size(), original_size + to_insert.size());
                        CHECK_EQ(std::distance(array.begin(), it), 0);

                        // Check inserted elements
                        for (std::size_t i = 0; i < to_insert.size(); ++i)
                        {
                            CHECK_EQ(array[i].value(), to_insert[i]);
                        }

                        // Check shifted elements
                        for (std::size_t i = to_insert.size(); i < array.size(); ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i - to_insert.size()]);
                        }
                    }

                    SUBCASE("insert range at middle")
                    {
                        string_view_array array(words, true);
                        const std::vector<sparrow::nullable<std::string>> to_insert = {
                            sparrow::make_nullable<std::string>("mid1"),
                            sparrow::make_nullable<std::string>("mid2")
                        };
                        const auto original_size = array.size();
                        const std::size_t insert_pos = 2;

                        auto it = array.insert(array.cbegin() + insert_pos, to_insert.begin(), to_insert.end());

                        REQUIRE_EQ(array.size(), original_size + to_insert.size());
                        CHECK_EQ(std::distance(array.begin(), it), insert_pos);

                        // Check elements before insertion
                        for (std::size_t i = 0; i < insert_pos; ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i]);
                        }

                        // Check inserted elements
                        for (std::size_t i = 0; i < to_insert.size(); ++i)
                        {
                            CHECK_EQ(array[insert_pos + i].value(), to_insert[i]);
                        }

                        // Check elements after insertion
                        for (std::size_t i = insert_pos + to_insert.size(); i < array.size(); ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i - to_insert.size()]);
                        }
                    }

                    SUBCASE("insert range at end")
                    {
                        string_view_array array(words, true);
                        const std::vector<sparrow::nullable<std::string>> to_insert = {
                            sparrow::make_nullable<std::string>("end1"),
                            sparrow::make_nullable<std::string>("end2")
                        };
                        const auto original_size = array.size();

                        auto it = array.insert(array.cend(), to_insert.begin(), to_insert.end());

                        REQUIRE_EQ(array.size(), original_size + to_insert.size());
                        CHECK_EQ(std::distance(array.begin(), it), original_size);

                        // Check original elements
                        for (std::size_t i = 0; i < original_size; ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i]);
                        }

                        // Check inserted elements
                        for (std::size_t i = 0; i < to_insert.size(); ++i)
                        {
                            CHECK_EQ(array[original_size + i].value(), to_insert[i]);
                        }
                    }

                    SUBCASE("insert empty range")
                    {
                        string_view_array array(words, true);
                        const std::vector<sparrow::nullable<std::string>> to_insert;
                        const auto original_size = array.size();

                        auto it = array.insert(array.cbegin() + 1, to_insert.begin(), to_insert.end());

                        REQUIRE_EQ(array.size(), original_size);
                        CHECK_EQ(std::distance(array.begin(), it), 1);

                        // Check all elements unchanged
                        for (std::size_t i = 0; i < array.size(); ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i]);
                        }
                    }

                    SUBCASE("insert range with mixed string lengths")
                    {
                        string_view_array array(words, true);
                        const std::vector<sparrow::nullable<std::string>> to_insert = {
                            sparrow::make_nullable<std::string>("short"),
                            sparrow::make_nullable<std::string>(
                                "this is a very long string that exceeds 12 bytes limit"
                            ),
                            sparrow::make_nullable<std::string>("mid")
                        };
                        const auto original_size = array.size();
                        const std::size_t insert_pos = 1;

                        array.insert(array.cbegin() + insert_pos, to_insert.begin(), to_insert.end());

                        REQUIRE_EQ(array.size(), original_size + to_insert.size());

                        // Check inserted elements
                        for (std::size_t i = 0; i < to_insert.size(); ++i)
                        {
                            CHECK_EQ(array[insert_pos + i].value(), to_insert[i]);
                        }
                    }
                }

                SUBCASE("erase")
                {
                    SUBCASE("erase from beginning")
                    {
                        string_view_array array(words, true);
                        const auto original_size = array.size();
                        const std::size_t erase_count = 2;

                        auto it = array.erase(array.cbegin(), array.cbegin() + erase_count);

                        REQUIRE_EQ(array.size(), original_size - erase_count);
                        CHECK_EQ(std::distance(array.begin(), it), 0);

                        // Check remaining elements
                        for (std::size_t i = 0; i < array.size(); ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i + erase_count]);
                        }
                    }

                    SUBCASE("erase from middle")
                    {
                        string_view_array array(words, true);
                        const auto original_size = array.size();
                        const std::size_t erase_pos = 2;
                        const std::size_t erase_count = 2;

                        auto it = array.erase(array.cbegin() + erase_pos, array.cbegin() + erase_pos + erase_count);

                        REQUIRE_EQ(array.size(), original_size - erase_count);
                        CHECK_EQ(std::distance(array.begin(), it), erase_pos);

                        // Check elements before erase position
                        for (std::size_t i = 0; i < erase_pos; ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i]);
                        }

                        // Check elements after erase position
                        for (std::size_t i = erase_pos; i < array.size(); ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i + erase_count]);
                        }
                    }

                    SUBCASE("erase from end")
                    {
                        string_view_array array(words, true);
                        const auto original_size = array.size();
                        const std::size_t erase_count = 2;
                        const std::size_t erase_pos = original_size - erase_count;

                        auto it = array.erase(
                            sparrow::next(array.cbegin(), erase_pos),
                            sparrow::next(array.cbegin(), erase_pos + erase_count)
                        );

                        REQUIRE_EQ(array.size(), original_size - erase_count);
                        CHECK_EQ(it, array.end());

                        // Check remaining elements
                        for (std::size_t i = 0; i < array.size(); ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i]);
                        }
                    }

                    SUBCASE("erase all elements")
                    {
                        string_view_array array(words, true);
                        const auto original_size = array.size();

                        auto it = array.erase(array.cbegin(), sparrow::next(array.cbegin(), original_size));

                        REQUIRE_EQ(array.size(), 0);
                        CHECK_EQ(it, array.begin());
                        CHECK_EQ(it, array.end());
                    }

                    SUBCASE("erase zero count")
                    {
                        string_view_array array(words, true);
                        const auto original_size = array.size();
                        const std::size_t erase_pos = 2;

                        auto it = array.erase(
                            sparrow::next(array.cbegin(), erase_pos),
                            sparrow::next(array.cbegin(), erase_pos)
                        );

                        REQUIRE_EQ(array.size(), original_size);
                        CHECK_EQ(std::distance(array.begin(), it), erase_pos);

                        // Check all elements unchanged
                        for (std::size_t i = 0; i < array.size(); ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i]);
                        }
                    }
                }

                SUBCASE("combined operations")
                {
                    SUBCASE("resize then insert")
                    {
                        string_view_array array(words, true);
                        const auto original_size = array.size();

                        // First resize
                        array.resize(original_size + 2, sparrow::make_nullable<std::string>("extra"));
                        REQUIRE_EQ(array.size(), original_size + 2);

                        // Then insert
                        array.insert(array.cbegin() + 1, sparrow::make_nullable<std::string>("inserted"), 1);
                        CHECK_EQ(array.size(), original_size + 3);
                        CHECK_EQ(array[1].value(), "inserted");
                    }

                    SUBCASE("insert then erase")
                    {
                        string_view_array array(words, true);
                        const auto original_size = array.size();

                        // First insert
                        array.insert(array.cbegin() + 2, sparrow::make_nullable<std::string>("temp"), 2);
                        REQUIRE_EQ(array.size(), original_size + 2);

                        // Then erase what we inserted
                        array.erase(array.cbegin() + 2, array.cbegin() + 2 + 2);
                        CHECK_EQ(array.size(), original_size);

                        // Should be back to original
                        for (std::size_t i = 0; i < array.size(); ++i)
                        {
                            CHECK_EQ(array[i].value(), words[i]);
                        }
                    }

                    SUBCASE("erase then resize")
                    {
                        string_view_array array(words, true);
                        const auto original_size = array.size();

                        // First erase
                        array.erase(array.cbegin() + 1, array.cbegin() + 1 + 2);
                        REQUIRE_EQ(array.size(), original_size - 2);

                        // Then resize back up
                        array.resize(original_size, sparrow::make_nullable<std::string>("refill"));
                        CHECK_EQ(array.size(), original_size);
                        CHECK_EQ(array[original_size - 1].value(), "refill");
                        CHECK_EQ(array[original_size - 2].value(), "refill");
                    }
                }
            }
        }
    }
}
