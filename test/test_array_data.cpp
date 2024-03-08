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

#include <ios>
#include <iostream>
#include <numeric>

#include "sparrow/array_data.hpp"

namespace sparrow
{
    struct array_data_fixture
    {
        array_data_fixture()
        {
            data_size = 16u;
            data.buffers.resize(2);
            data.buffers[0].resize(data_size / 8, 255);
            data.buffers[1].resize(data_size);
            std::iota(data.buffers[1].data(), data.buffers[1].data() + data_size, 0u);
        }

        array_data data;
        std::size_t data_size;

        using reference_type = reference_proxy<std::uint8_t>;
    };

    TEST_SUITE("bitset_view")
    {
        TEST_CASE_FIXTURE(array_data_fixture, "constructor")
        {
            [[maybe_unused]] bitset_view v(data.buffers[0]);
            [[maybe_unused]] auto v2 = data.bitmap();
        }

        TEST_CASE_FIXTURE(array_data_fixture, "size")
        {
            auto bm = data.bitmap();
            CHECK_EQ(bm.size(), data_size);
        }

        TEST_CASE_FIXTURE(array_data_fixture, "test")
        {
            auto bm = data.bitmap();
            for (size_t i = 0; i < bm.size(); ++i)
            {
                CHECK(bm.test(i));
            }
        }

        TEST_CASE_FIXTURE(array_data_fixture, "set")
        {
            auto bm = data.bitmap();
            bm.set(3, false);
            bm.set(11, false);
            for (size_t i = 0; i < bm.size(); ++i)
            {
                if (i == 3 || i == 11)
                {
                    CHECK(!bm.test(i));
                }
                else
                {
                    CHECK(bm.test(i));
                }
            }

            bm.set(3, true);
            for (size_t i = 0; i < bm.size(); ++i)
            {
                if (i == 11)
                {
                    CHECK(!bm.test(i));
                }
                else
                {
                    CHECK(bm.test(i));
                }
            }
        }
    }

    TEST_SUITE("reference_proxy")
    {
        TEST_CASE_FIXTURE(array_data_fixture, "value_semantic")
        {
            std::size_t index1 = 4u;

            SUBCASE("Constructor")
            {
                reference_type r(data.buffers[1][index1], index1, data);
                CHECK_EQ(r, data.buffers[1][index1]);
            }

            SUBCASE("Copy semantic")
            {
                std::size_t index2 = 12u;
                reference_type r(data.buffers[1][index1], index1, data);
                reference_type r2(r);
                CHECK_EQ(r, r2);

                reference_type r3(data.buffers[1][index2], index2, data);
                r2 = r3;
                CHECK_EQ(r2, r3);
                CHECK_EQ(data.buffers[1][index1], data.buffers[1][index2]);
            }

            SUBCASE("Move semantic")
            {
                std::size_t index2 = 10u;
                reference_type r(data.buffers[1][index1], index1, data);
                reference_type r2(std::move(r));
                CHECK_EQ(r, r2);

                reference_type r3(data.buffers[1][index1], index1, data);
                reference_type r4(data.buffers[1][index2], index2, data);
                r3 = std::move(r4);
                CHECK_EQ(r3, r4);
                CHECK_EQ(data.buffers[1][index1], data.buffers[1][index2]);
            }
        }

        TEST_CASE_FIXTURE(array_data_fixture, "assignment")
        {
            std::size_t index = 4u;
            reference_type r(data.buffers[1][index], index, data);
            r = 2;
            CHECK_EQ(data.buffers[1][index], 2);

            r += 3;
            CHECK_EQ(data.buffers[1][index], 5);

            r -= 1;
            CHECK_EQ(data.buffers[1][index], 4);

            r *= 3;
            CHECK_EQ(data.buffers[1][index], 12);

            r /= 2;
            CHECK_EQ(data.buffers[1][index], 6);
        }
    }
}

