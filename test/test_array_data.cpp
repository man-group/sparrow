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

#include <numeric>

#include "sparrow/array/array_data.hpp"

#include "doctest/doctest.h"


namespace sparrow
{
    namespace test
    {
        template <class T>
        class cast_vector : private std::vector<T>
        {
        public:

            using base_type = std::vector<T>;
            using difference_type = base_type::difference_type;

            using base_type::base_type;
            using base_type::resize;
            using base_type::size;
            using base_type::operator[];
            using base_type::begin;
            using base_type::cbegin;
            using base_type::cend;
            using base_type::end;

            template <class U>
            U* data()
            {
#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-align"
#endif
                return reinterpret_cast<U*>(base_type::data());
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif
            }

            template <class U>
            const U* data() const
            {
#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-align"
#endif
                return reinterpret_cast<const U*>(base_type::data());
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif
            }
        };
    };

    struct test_array_data
    {
        static constexpr bool is_mutable = false;
        using block_type = std::uint8_t;
        using bitmap_type = dynamic_bitset_view<const block_type>;
        using buffer_type = test::cast_vector<block_type>;
        using length_type = std::int64_t;

        data_descriptor type;
        length_type length = 0;
        std::int64_t offset = 0;
        // bitmap buffer and null_count
        std::vector<block_type> bitmap;
        // Other buffers
        std::vector<buffer_type> buffers;
        std::vector<test_array_data> child_data;
        value_ptr<test_array_data> dictionary;
    };

    data_descriptor type_descriptor(const test_array_data& data)
    {
        return data.type;
    }

    test_array_data::length_type length(const test_array_data& data)
    {
        return data.length;
    }

    std::int64_t offset(const test_array_data& data)
    {
        return data.offset;
    }

    const test_array_data::bitmap_type bitmap(const test_array_data& data)
    {
        using return_type = const test_array_data::bitmap_type;
        return return_type(data.bitmap.data(), std::size_t(data.length));
    }

    std::size_t buffers_size(const test_array_data& data)
    {
        return data.buffers.size();
    }

    const test_array_data::buffer_type& buffer_at(const test_array_data& data, std::size_t i)
    {
        SPARROW_ASSERT_TRUE(i < buffers_size(data));
        return data.buffers[i];
    }

    std::size_t child_data_size(const test_array_data& data)
    {
        return data.child_data.size();
    }

    const test_array_data& child_data_at(const test_array_data& data, std::size_t i)
    {
        SPARROW_ASSERT_TRUE(i < child_data_size(data));
        return data.child_data[i];
    }

    const value_ptr<test_array_data>& dictionary(const test_array_data& data)
    {
        return data.dictionary;
    }
}

// test_array_data free functions must be defined before including the layout
// headers, otherwise they are not found.

#include "sparrow/layout/primitive_array.hpp"
#include "sparrow/layout/null_array.hpp"
#include "sparrow/layout/variable_size_binary_array.hpp"

namespace sparrow
{
    TEST_SUITE("array_data")
    {
        TEST_CASE("primitive_array")
        {
            using buffer_type = test_array_data::buffer_type;
            test_array_data td;
            {
                td.type = data_descriptor(data_type::INT32);
                td.length = 16;
                td.offset = 0;
                td.bitmap = {std::uint8_t(255), std::uint8_t(255)};
                td.buffers = std::vector<buffer_type>(1u, buffer_type(64u));
                for (std::int32_t i = 0; i < 16; ++i)
                {
                    td.buffers[0].data<std::int32_t>()[i] = i;
                }
            }

            const primitive_array<std::int32_t> layout(td);
            CHECK_EQ(layout.size(), std::size_t(td.length));
            for (std::size_t i = 0; i < 16u; ++i)
            {
                CHECK_EQ(layout[i], static_cast<std::int32_t>(i));
            }

            auto iter = layout.cbegin();
            for (std::int32_t i = 0; i < 16; ++i, ++iter)
            {
                CHECK_EQ(*iter, i);
            }
            CHECK_EQ(iter, layout.cend());
        }

        TEST_CASE("variable_size_binary_array")
        {
            std::vector<std::string> words = {
                "once",
                "upon",
                "a",
                "time",
                "I",
                "was",
                "writing",
                "clean",
                "code",
                "now",
                "I'm",
                "only",
                "drawing",
                "flowcharts",
                "Bonnie",
                "Compyler"
            };
            test_array_data td;
            {
                td.type = data_descriptor(data_type::STRING);
                td.length = 16;
                td.offset = 0;
                td.bitmap = {std::uint8_t(255), std::uint8_t(255)};
                td.buffers.resize(2);
                // Chars buffer
                td.buffers[0].resize(sizeof(std::int64_t) * (16 + 1));
                // Offsets buffer
                td.buffers[1].resize(std::accumulate(
                    words.begin(),
                    words.end(),
                    size_t(0),
                    [](std::size_t res, const auto& s)
                    {
                        return res + s.size();
                    }
                ));

                td.buffers[0].data<std::int64_t>()[0] = 0u;
                auto iter = td.buffers[1].begin();
                const auto offset_func = [&td]()
                {
                    return td.buffers[0].data<std::int64_t>();
                };
                for (size_t i = 0; i < words.size(); ++i)
                {
                    offset_func()[i + 1] = offset_func()[i]
                                           + static_cast<sparrow::test_array_data::buffer_type::difference_type>(
                                               words[i].size()
                                           );
                    std::ranges::copy(words[i], iter);
                    iter += static_cast<sparrow::test_array_data::buffer_type::difference_type>(words[i].size());
                }
            }

            using layout_type = variable_size_binary_array<std::string, const std::string_view, test_array_data>;
            const layout_type layout(td);

            CHECK_EQ(layout.size(), words.size());

            for (std::size_t i = 0; i < 16u; ++i)
            {
                CHECK_EQ(layout[i], words[i]);
            }

            auto iter = layout.cbegin();
            auto words_iter = words.cbegin();
            auto words_end = words.cend();
            while (words_iter != words_end)
            {
                CHECK_EQ(*words_iter++, *iter++);
            }
        }
    }
}
