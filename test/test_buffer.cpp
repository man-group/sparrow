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

#include "doctest/doctest.h"

#include <numeric>

#include "sparrow/buffer.hpp"

namespace sparrow
{
    using buffer_test_type = buffer<uint8_t>;

    namespace
    {
        uint8_t* make_test_buffer(std::size_t size, uint8_t start_value = 0)
        {
            uint8_t* res = new uint8_t[size];
            std::iota(res, res + size, start_value);
            return res;
        }
    }

    TEST_SUITE("buffer")
    {
        TEST_CASE("constructors")
        {
            {
                [[maybe_unused]] buffer_test_type b(8u);
            }

            {
                const std::size_t size = 8u;
                [[maybe_unused]] buffer_test_type b(make_test_buffer(size), size);
            }

            buffer_test_type b0;
            CHECK_EQ(b0.data(), nullptr);
            CHECK_EQ(b0.size(), 0u);

            const std::size_t expected_size = 4u;
            buffer_test_type b1(expected_size);
            CHECK_NE(b1.data(), nullptr);
            CHECK_EQ(b1.size(), expected_size);

            uint8_t* mem = make_test_buffer(expected_size);
            buffer_test_type b2(mem, expected_size);
            CHECK_EQ(b2.data(), mem);
            CHECK_EQ(b2.size(), expected_size);
            CHECK_EQ(b2.data()[2], uint8_t(2));
        }

        TEST_CASE("copy semantic")
        {
            const std::size_t size = 4;
            buffer_test_type b1(make_test_buffer(size), size);
            buffer_test_type b2(b1);
            CHECK_EQ(b1, b2);

            const std::size_t size2 = 8;
            buffer_test_type b3(make_test_buffer(size2, 4), size2);
            b2 = b3;
            CHECK_EQ(b2, b3);
            CHECK_NE(b1, b2);
        }

        TEST_CASE("move semantic")
        {
            const std::size_t size = 4;
            buffer_test_type b1(make_test_buffer(size), size);
            buffer_test_type control(b1);
            buffer_test_type b2(std::move(b1));
            CHECK_EQ(b2, control);
            CHECK_EQ(b1.size(), 0);
            CHECK(b1.empty());
            CHECK_EQ(b1.data(), nullptr);

            const std::size_t size2 = 8;
            buffer_test_type b4(make_test_buffer(size2, 4), size2);
            buffer_test_type control2(b4);
            b2 = std::move(b4);
            CHECK_EQ(b2, control2);
            CHECK_EQ(b4, control);
        }

        TEST_CASE("data")
        {
            const std::size_t size = 4;
            buffer_test_type b1(make_test_buffer(size), size);
            
            const uint8_t expected_value = 101;
            const std::size_t idx = 3u;
            b1.data()[idx] = expected_value;
            buffer_test_type b2(b1);
            CHECK_EQ(b2.data()[idx], expected_value);

            buffer_test_type b3(std::move(b1));
            CHECK_EQ(b3.data()[idx], expected_value);
        }

        TEST_CASE("equality comparison")
        {
            const std::size_t size = 4;
            buffer_test_type b1(make_test_buffer(size), size);
            buffer_test_type b2(make_test_buffer(size), size);
            CHECK(b1 == b2);

            const std::size_t size2 = 8u;
            buffer_test_type b3(make_test_buffer(size2), size2);
            CHECK(b1 != b3);
        }

        TEST_CASE("swap")
        {
            const std::size_t size1 = 4u;
            const std::size_t size2 = 8u;

            buffer_test_type b1(make_test_buffer(size1), size1);
            buffer_test_type b2(make_test_buffer(size2), size2);
            auto* data1 = b1.data();
            auto* data2 = b2.data();
            b1.swap(b2);
            CHECK_EQ(b1.size(), size2);
            CHECK_EQ(b1.data(), data2);
            CHECK_EQ(b2.size(), size1);
            CHECK_EQ(b2.data(), data1);
        }

        TEST_CASE("resize")
        {
            const std::size_t size1 = 4u;
            const std::size_t size2 = 8u;
            buffer_test_type b(make_test_buffer(size1), size1); 
            b.resize(size2);
            CHECK_EQ(b.size(), size2);
            CHECK_EQ(b.data()[2], 2);
        }
    }
}
