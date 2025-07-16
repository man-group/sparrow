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
                        CHECK(detail::array_access::get_arrow_proxy(array).flags().contains(ArrowFlag::NULLABLE)
                        );
                    }
                }

                SUBCASE("copy")
                {
                    const string_view_array array(words, where_nulls, "name", metadata_sample_opt);
                    const string_view_array array_copy(array);
                    CHECK_EQ(array, array_copy);
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
                            const std::string& value{array[i].value()};
                            CHECK_EQ(value, test_words[i]);
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
        }
    }
}
