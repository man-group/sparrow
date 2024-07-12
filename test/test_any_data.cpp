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

#include <any>
#include <cstdint>

#include "sparrow/any_data.hpp"
#include "sparrow/buffer.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/details/3rdparty/value_ptr_lite.hpp"

#include "doctest/doctest.h"

TEST_SUITE("any_data")
{
    TEST_CASE("constructors")
    {
        SUBCASE("std::vector<int>")
        {
            std::vector<int> vec{1, 2, 3, 4, 5};
            sparrow::any_data any_data{vec};
        }

        SUBCASE("raw pointer")
        {
            int i = 5;
            sparrow::any_data any_data{&i};
        }

        SUBCASE("unique pointer")
        {
            auto ptr = std::make_unique<int>(5);
            sparrow::any_data any_data{std::move(ptr)};
        }

        SUBCASE("arrow_array_unique_ptr")
        {
            auto ptr = sparrow::default_arrow_array();
            ptr->length = 99;
            ptr->null_count = 42;
            sparrow::any_data any_data{std::move(ptr)};
        }

        SUBCASE("std::nullptr_t")
        {
            sparrow::any_data any_data{nullptr};
        }

        struct CopyOnlyForTest
        {
            CopyOnlyForTest() = default;

            CopyOnlyForTest(const CopyOnlyForTest&)
                : copied(true)
            {
            }

            CopyOnlyForTest(CopyOnlyForTest&&) = delete;

            bool copied = false;
        };

        struct MoveAndCopyOnlyForTest
        {
            MoveAndCopyOnlyForTest() = default;

            MoveAndCopyOnlyForTest(const MoveAndCopyOnlyForTest&) noexcept
                : copied(true)
            {
            }

            MoveAndCopyOnlyForTest(MoveAndCopyOnlyForTest&&) noexcept
                : moved(true)
            {
            }

            bool copied = false;
            bool moved = false;
        };

        SUBCASE("check move")
        {
            MoveAndCopyOnlyForTest mo;
            std::any a = std::move(mo);
            MoveAndCopyOnlyForTest move_and_copy_only;
            CHECK_FALSE(move_and_copy_only.moved);
            sparrow::any_data any_data{std::move(move_and_copy_only)};
            auto& data = any_data.value<MoveAndCopyOnlyForTest&>();
            CHECK(data.moved);
            CHECK_FALSE(data.copied);
        }

        SUBCASE("check copy")
        {
            CopyOnlyForTest copy_only;
            CHECK_FALSE(copy_only.copied);
            sparrow::any_data any_data{copy_only};
            CHECK_FALSE(copy_only.copied);
            auto& data = any_data.value<CopyOnlyForTest&>();
            CHECK(data.copied);

            MoveAndCopyOnlyForTest move_and_copy_only;
            CHECK_FALSE(move_and_copy_only.copied);
            sparrow::any_data any_data_2{move_and_copy_only};
            CHECK_FALSE(move_and_copy_only.copied);
            auto& data_2 = any_data.value<CopyOnlyForTest&>();
            CHECK(data_2.copied);
        }
    }

    TEST_CASE("get")
    {
        SUBCASE("mutable")
        {
            SUBCASE("raw pointer")
            {
                int i = 5;
                sparrow::any_data data{&i};
                int* ptr = data.get<int>();
                CHECK_EQ(*ptr, 5);
                *ptr = 2;
                CHECK_EQ(i, 2);
            }

            SUBCASE("std::vector<int>")
            {
                std::vector<int> vec{1, 2, 3, 4, 5};
                sparrow::any_data any_data{vec};
                std::vector<int>* ptr = any_data.get<std::vector<int>>();
                for (size_t i = 0; i < vec.size(); ++i)
                {
                    CHECK_EQ(ptr->at(i), vec[i]);
                }
            }

            SUBCASE("unique_ptr")
            {
                auto unique_ptr = std::make_unique<int>(5);
                sparrow::any_data data{std::move(unique_ptr)};
                int* ptr = data.get<int>();
                CHECK_EQ(*ptr, 5);
            }

            SUBCASE("std::nullptr_t")
            {
                sparrow::any_data any_data{nullptr};
                CHECK_EQ(any_data.get<int>(), nullptr);
            }
        }

        SUBCASE("const")
        {
            SUBCASE("raw pointer")
            {
                int i = 5;
                const sparrow::any_data data{&i};
                const int* ptr = data.get<int>();
                CHECK_EQ(*ptr, 5);
            }

            SUBCASE("std::vector<int>")
            {
                std::vector<int> vec{1, 2, 3, 4, 5};
                const sparrow::any_data any_data{vec};
                const std::vector<int>* ptr = any_data.get<std::vector<int>>();
                for (size_t i = 0; i < vec.size(); ++i)
                {
                    CHECK_EQ(ptr->at(i), vec[i]);
                }
            }

            SUBCASE("unique_ptr")
            {
                auto unique_ptr = std::make_unique<int>(5);
                const sparrow::any_data data{std::move(unique_ptr)};
                const int* ptr = data.get<int>();
                CHECK_EQ(*ptr, 5);
            }

            SUBCASE("std::nullptr_t")
            {
                const sparrow::any_data any_data{nullptr};
                CHECK_EQ(any_data.get<int>(), nullptr);
            }
        }
    }

    TEST_CASE("value")
    {
        SUBCASE("mutable")
        {
            SUBCASE("raw pointer")
            {
                int i = 5;
                sparrow::any_data data{&i};
                REQUIRE_THROWS_AS(data.value<int&>(), std::bad_any_cast);
            }

            SUBCASE("std::vector<int>")
            {
                std::vector<int> vec{1, 2, 3, 4, 5};
                sparrow::any_data any_data{vec};
                REQUIRE_NOTHROW(any_data.value<std::vector<int>&>());
                auto& data = any_data.value<std::vector<int>&>();
                for (size_t i = 0; i < vec.size(); ++i)
                {
                    CHECK_EQ(data[i], vec[i]);
                }
            }

            SUBCASE("unique pointer")
            {
                auto ptr = std::make_unique<int>(5);
                sparrow::any_data data{std::move(ptr)};
                REQUIRE_NOTHROW(data.value<nonstd::value_ptr<int>&>());
                auto& value = data.value<nonstd::value_ptr<int>&>();
                CHECK_EQ(*value.get(), 5);
            }

            SUBCASE("std::nullptr_t")
            {
                sparrow::any_data any_data{nullptr};
                REQUIRE_THROWS_AS(any_data.value<int&>(), std::bad_any_cast);
            }
        }

        SUBCASE("const")
        {
            SUBCASE("raw pointer")
            {
                int i = 5;
                const sparrow::any_data data{&i};
                REQUIRE_THROWS_AS(data.value<const int&>(), std::bad_any_cast);
            }
        }
    }

    TEST_CASE("type_id")
    {
        SUBCASE("int pointer")
        {
            int i = 5;
            sparrow::any_data data{&i};
            CHECK_EQ(data.type_id(), typeid(void));
        }

        SUBCASE("float pointer")
        {
            float f = 5.0f;
            sparrow::any_data data{&f};
            CHECK_EQ(data.type_id(), typeid(void));
        }

        SUBCASE("unique pointer")
        {
            auto ptr = std::make_unique<int>(5);
            sparrow::any_data data{std::move(ptr)};
            std::string type_id = data.type_id().name();
            CHECK_EQ(data.type_id(), typeid(nonstd::value_ptr<int>));
        }
    }

    TEST_CASE("owns_data")
    {
        SUBCASE("int pointer")
        {
            int i = 5;
            sparrow::any_data data{&i};
            CHECK_FALSE(data.owns_data());
        }

        SUBCASE("std::vector<int>")
        {
            std::vector<int> vec{1, 2, 3, 4, 5};
            sparrow::any_data any_data{vec};
            CHECK(any_data.owns_data());
        }

        SUBCASE("unique pointer")
        {
            auto ptr = std::make_unique<int>(5);
            sparrow::any_data data{std::move(ptr)};
            CHECK(data.owns_data());
        }

        SUBCASE("arrow_array_unique_ptr")
        {
            auto ptr = sparrow::default_arrow_array();
            sparrow::any_data data{std::move(ptr)};
            CHECK(data.owns_data());
        }

        SUBCASE("ArrowArray")
        {
            auto ptr = sparrow::default_arrow_array();
            sparrow::any_data data{std::move(ptr)};
            CHECK(data.owns_data());
        }

        SUBCASE("std::nullptr_t")
        {
            sparrow::any_data any_data{nullptr};
            CHECK_FALSE(any_data.owns_data());
        }
    }
}

std::vector<std::vector<int>> create_vec_of_vec_int()
{
    std::vector<std::vector<int>> vec;
    int counter = 0;
    for (int i = 0; i < 5; ++i)
    {
        vec.push_back(std::vector<int>{counter++, counter++, counter++});
    }
    return vec;
}

std::vector<std::unique_ptr<sparrow::buffer<int>>> create_vec_of_unique_buffer_int()
{
    std::vector<std::unique_ptr<sparrow::buffer<int>>> vec;
    int counter = 0;
    for (size_t i = 0; i < 5; ++i)
    {
        vec.push_back(std::make_unique<sparrow::buffer<int>>(5));
        for (size_t j = 0; j < 5; ++j)
        {
            vec[i]->data()[j] = counter++;
        }
    }
    return vec;
}

std::vector<std::shared_ptr<sparrow::buffer<int>>> create_vec_of_shared_buffer_int()
{
    std::vector<std::shared_ptr<sparrow::buffer<int>>> vec;
    int counter = 0;
    for (size_t i = 0; i < 5; ++i)
    {
        vec.push_back(std::make_shared<sparrow::buffer<int>>(5));
        for (size_t j = 0; j < vec[i]->size(); ++j)
        {
            vec[i]->data()[j] = counter++;
        }
    }
    return vec;
}

std::tuple<nonstd::value_ptr<int>, std::shared_ptr<int>> create_tuple_of_value_shared_int()
{
    auto value_ptr = nonstd::value_ptr<int>{5};
    auto shared_ptr = std::make_shared<int>(6);
    return {std::move(value_ptr), shared_ptr};
}

std::tuple<std::unique_ptr<int>, std::shared_ptr<int>> create_tuple_of_unique_shared_int()
{
    auto unique_ptr = std::make_unique<int>(5);
    auto shared_ptr = std::make_shared<int>(6);
    return {std::move(unique_ptr), shared_ptr};
}

TEST_SUITE("any_data_container")
{
    TEST_CASE("constructors")
    {
        SUBCASE("std::vector<std::vector<int>>")
        {
            sparrow::any_data_container data{create_vec_of_vec_int()};
        }

        SUBCASE("std::vector<int*>")
        {
            std::vector<int*> vec;
            for (int i = 0; i < 5; ++i)
            {
                vec.push_back(new int(i));
            }
            sparrow::any_data_container data{vec};
        }

        SUBCASE("std::vector<int*> with nullptr")
        {
            std::vector<int*> vec;
            for (int i = 0; i < 5; ++i)
            {
                vec.push_back(nullptr);
            }
            sparrow::any_data_container data{vec};
        }

        SUBCASE("std::vector<std::unique_ptr<sparrow::buffer<int>>>")
        {
            sparrow::any_data_container data{create_vec_of_unique_buffer_int()};
        }

        SUBCASE("std::vector<std::shared_ptr<sparrow::buffer<int>>>")
        {
            sparrow::any_data_container data{create_vec_of_shared_buffer_int()};
        }

        SUBCASE("std::tuple<std::vector<int32_t>, sparrow::buffer<uint8_t>, int64_t*")
        {
            std::vector<int64_t> vec{0, 1, 2, 3, 4};

            std::tuple<std::vector<int32_t>, sparrow::buffer<int64_t>, int64_t*> tuple{
                std::vector<int32_t>{0, 1, 2, 3, 4},
                sparrow::buffer<int64_t>(vec),
                vec.data()
            };

            sparrow::any_data_container data{tuple};
        }

        SUBCASE("std:tuple<nonstd::value_ptr<int>, std::shared_ptr<int>>")
        {
            sparrow::any_data_container data{create_tuple_of_value_shared_int()};
        }

        SUBCASE("std:tuple<sparrow::unique_ptr<int>, std::shared_ptr<int>>")
        {
            sparrow::any_data_container data{create_tuple_of_unique_shared_int()};
        }
    }

    TEST_CASE("get_pointers_vec")
    {
        SUBCASE("std::vector<std::vector<int>>")
        {
            const auto vec = create_vec_of_vec_int();
            sparrow::any_data_container data{vec};
            auto ptrs = data.get_pointers_vec<int>();
            for (size_t i = 0; i < vec.size(); ++i)
            {
                for (size_t j = 0; j < vec[i].size(); ++j)
                {
                    CHECK_EQ(ptrs[i][j], vec[i][j]);
                }
            }
        }

        SUBCASE("std::vector<int*>")
        {
            std::vector<int*> vec;
            for (int i = 0; i < 5; ++i)
            {
                vec.push_back(new int(i));
            }
            sparrow::any_data_container data{vec};
            auto ptrs = data.get_pointers_vec<int>();
            for (size_t i = 0; i < 5; ++i)
            {
                CHECK_EQ(*ptrs[i], i);
            }
        }

        SUBCASE("std::vector<std::unique_ptr<sparrow::buffer<int>>>")
        {
            sparrow::any_data_container data{create_vec_of_unique_buffer_int()};
            auto ptrs = data.get_pointers_vec<int>();
            for (size_t i = 0; i < 5; ++i)
            {
                for (size_t j = 0; j < 5; ++j)
                {
                    CHECK_EQ(ptrs[i][j], j + i * 5);
                }
            }
        }

        SUBCASE("std::vector<std::shared_ptr<sparrow::buffer<int>>>")
        {
            sparrow::any_data_container data{create_vec_of_shared_buffer_int()};
            auto ptrs = data.get_pointers_vec<int>();
            for (size_t i = 0; i < 5; ++i)
            {
                for (size_t j = 0; j < 5; ++j)
                {
                    CHECK_EQ(ptrs[i][j], j + i * 5);
                }
            }
        }

        SUBCASE("std::tuple<std::vector<int32_t>, sparrow::buffer<uint8_t>, int64_t*")
        {
            std::vector<int64_t> vec{0, 1, 2, 3, 4};

            std::tuple<std::vector<int32_t>, sparrow::buffer<int64_t>, int64_t*> tuple{
                std::vector<int32_t>{0, 1, 2, 3, 4},
                sparrow::buffer<int64_t>(vec),
                vec.data()
            };

            sparrow::any_data_container data{tuple};
            auto ptrs = data.get_pointers_vec<int>();
            const auto vec_ptr = ptrs.at(0);
            const auto buffer_ptr = reinterpret_cast<int64_t*>(ptrs.at(1));
            const auto int64_ptr = reinterpret_cast<int64_t*>(ptrs.at(2));

            for (size_t i = 0; i < 5; ++i)
            {
                CHECK_EQ(vec_ptr[i], i);
                CHECK_EQ(buffer_ptr[i], i);
                CHECK_EQ(int64_ptr[i], i);
            }
        }

        SUBCASE("std:tuple<nonstd::value_ptr<int>, std::shared_ptr<int>>")
        {
            sparrow::any_data_container data{create_tuple_of_value_shared_int()};
            auto ptrs = data.get_pointers_vec<int>();
            CHECK_EQ(*ptrs[0], 5);
            CHECK_EQ(*ptrs[1], 6);
        }

        SUBCASE("std:tuple<sparrow::unique_ptr<int>, std::shared_ptr<int>>")
        {
            sparrow::any_data_container data{create_tuple_of_unique_shared_int()};
            auto ptrs = data.get_pointers_vec<int>();
            CHECK_EQ(*ptrs[0], 5);
            CHECK_EQ(*ptrs[1], 6);
        }
    }

    TEST_CASE("value")
    {
        SUBCASE("mutable")
        {
            SUBCASE("std::vector<std::vector<int>>")
            {
                const auto vec = create_vec_of_vec_int();
                sparrow::any_data_container data{vec};
                auto& data_vec = data.value<std::vector<std::vector<int>>&>();
                for (size_t i = 0; i < vec.size(); ++i)
                {
                    for (size_t j = 0; j < vec[i].size(); ++j)
                    {
                        CHECK_EQ(data_vec[i][j], vec[i][j]);
                    }
                }
            }

            SUBCASE("std::vector<int*>")
            {
                std::vector<int*> vec;
                for (int i = 0; i < 5; ++i)
                {
                    vec.push_back(new int(i));
                }
                sparrow::any_data_container data{vec};
                CHECK_FALSE(data.owns_data());
            }

            SUBCASE("std::vector<std::unique_ptr<sparrow::buffer<int>>>")
            {
                sparrow::any_data_container data{create_vec_of_unique_buffer_int()};
                auto& data_vec = data.value<std::vector<nonstd::value_ptr<sparrow::buffer<int>>>&>();
                for (size_t i = 0; i < 5; ++i)
                {
                    for (size_t j = 0; j < 5; ++j)
                    {
                        CHECK_EQ(data_vec[i]->data()[j], j + i * 5);
                    }
                }
            }

            SUBCASE("std::vector<std::shared_ptr<sparrow::buffer<int>>>")
            {
                sparrow::any_data_container data{create_vec_of_shared_buffer_int()};
                auto& data_vec = data.value<std::vector<std::shared_ptr<sparrow::buffer<int>>>&>();
                for (size_t i = 0; i < 5; ++i)
                {
                    for (size_t j = 0; j < 5; ++j)
                    {
                        CHECK_EQ(data_vec[i]->data()[j], j + i * 5);
                    }
                }
            }

            SUBCASE("std::tuple<std::vector<int32_t>, sparrow::buffer<uint8_t>, int64_t*")
            {
                std::vector<int64_t> vec{0, 1, 2, 3, 4};

                std::tuple<std::vector<int32_t>, sparrow::buffer<int64_t>, int64_t*> tuple{
                    std::vector<int32_t>{0, 1, 2, 3, 4},
                    sparrow::buffer<int64_t>(vec),
                    vec.data()
                };

                sparrow::any_data_container data{tuple};
                auto& data_tuple = data.value<std::tuple<std::vector<int32_t>, sparrow::buffer<int64_t>, int64_t*>&>(
                );
                auto& vec_data = std::get<0>(data_tuple);
                auto& buffer_data = std::get<1>(data_tuple);
                auto& int64_data = std::get<2>(data_tuple);

                for (size_t i = 0; i < 5; ++i)
                {
                    CHECK_EQ(vec_data[i], i);
                    CHECK_EQ(buffer_data[i], i);
                    CHECK_EQ(int64_data[i], i);
                }
            }

            SUBCASE("std:tuple<nonstd::value_ptr<int>, std::shared_ptr<int>>")
            {
                sparrow::any_data_container data{create_tuple_of_value_shared_int()};
                auto& data_tuple = data.value<std::tuple<nonstd::value_ptr<int>, std::shared_ptr<int>>&>();
                CHECK_EQ(*std::get<0>(data_tuple), 5);
                CHECK_EQ(*std::get<1>(data_tuple), 6);
            }

            SUBCASE("std:tuple<sparrow::unique_ptr<int>, std::shared_ptr<int>>")
            {
                sparrow::any_data_container data{create_tuple_of_unique_shared_int()};
                auto& data_tuple = data.value<std::tuple<nonstd::value_ptr<int>, std::shared_ptr<int>>&>();
                CHECK_EQ(*std::get<0>(data_tuple), 5);
                CHECK_EQ(*std::get<1>(data_tuple), 6);
            }
        }
    }

    TEST_CASE("get")
    {
        SUBCASE("mutable")
        {
            SUBCASE("std::vector<std::vector<int>>")
            {
                const auto vec = create_vec_of_vec_int();
                sparrow::any_data_container data{vec};
                auto ptr = data.get<int>();
                for (size_t i = 0; i < vec.size(); ++i)
                {
                    for (size_t j = 0; j < vec[i].size(); ++j)
                    {
                        CHECK_EQ(ptr[i][j], vec[i][j]);
                    }
                }
            }

            SUBCASE("std::vector<int*>")
            {
                std::vector<int*> vec;
                for (int i = 0; i < 5; ++i)
                {
                    vec.push_back(new int(i));
                }
                sparrow::any_data_container data{vec};
                auto ptr = data.get<int>();
                for (size_t i = 0; i < 5; ++i)
                {
                    CHECK_EQ(*ptr[i], i);
                }
            }

            SUBCASE("std::vector<int*> with nullptr")
            {
                std::vector<int*> vec;
                for (int i = 0; i < 5; ++i)
                {
                    vec.push_back(nullptr);
                }
                sparrow::any_data_container data{vec};
                auto ptr = data.get<int>();
                for (size_t i = 0; i < 5; ++i)
                {
                    CHECK_EQ(ptr[i], nullptr);
                }
            }

            SUBCASE("std::vector<std::unique_ptr<sparrow::buffer<int>>>")
            {
                sparrow::any_data_container data{create_vec_of_unique_buffer_int()};
                auto ptr = data.get<int>();
                for (size_t i = 0; i < 5; ++i)
                {
                    for (size_t j = 0; j < 5; ++j)
                    {
                        CHECK_EQ(ptr[i][j], j + i * 5);
                    }
                }
            }

            SUBCASE("std::vector<std::shared_ptr<sparrow::buffer<int>>>")
            {
                sparrow::any_data_container data{create_vec_of_shared_buffer_int()};
                auto ptr = data.get<int>();
                for (size_t i = 0; i < 5; ++i)
                {
                    for (size_t j = 0; j < 5; ++j)
                    {
                        CHECK_EQ(ptr[i][j], j + i * 5);
                    }
                }
            }

            SUBCASE("std::tuple<std::vector<int32_t>, sparrow::buffer<uint8_t>, int64_t*")
            {
                std::vector<int64_t> vec{0, 1, 2, 3, 4};

                std::tuple<std::vector<int32_t>, sparrow::buffer<int64_t>, int64_t*> tuple{
                    std::vector<int32_t>{0, 1, 2, 3, 4},
                    sparrow::buffer<int64_t>(vec),
                    vec.data()
                };

                sparrow::any_data_container data{tuple};
                auto ptrs = data.get<void>();
                auto ptr_0 = reinterpret_cast<int32_t*>(ptrs[0]);
                auto ptr_1 = reinterpret_cast<int64_t*>(ptrs[1]);
                auto ptr_2 = reinterpret_cast<int64_t*>(ptrs[2]);

                for (size_t i = 0; i < 5; ++i)
                {
                    CHECK_EQ(ptr_0[i], i);
                    CHECK_EQ(ptr_1[i], i);
                    CHECK_EQ(ptr_2[i], i);
                }
            }

            SUBCASE("std:tuple<nonstd::value_ptr<int>, std::shared_ptr<int>>")
            {
                sparrow::any_data_container data{create_tuple_of_value_shared_int()};
                auto ptr = data.get<int>();
                CHECK_EQ(*ptr[0], 5);
                CHECK_EQ(*ptr[1], 6);
            }

            SUBCASE("std:tuple<sparrow::unique_ptr<int>, std::shared_ptr<int>>")
            {
                sparrow::any_data_container data{create_tuple_of_unique_shared_int()};
                auto ptr = data.get<int>();
                CHECK_EQ(*ptr[0], 5);
                CHECK_EQ(*ptr[1], 6);
            }
        }
    }

    TEST_CASE("owns_data")
    {
        SUBCASE("std::vector<std::vector<int>>")
        {
            const auto vec = create_vec_of_vec_int();
            sparrow::any_data_container data{vec};
            CHECK(data.owns_data());
        }

        SUBCASE("raw pointer")
        {
            std::vector<int*> vec;
            for (int i = 0; i < 5; ++i)
            {
                vec.push_back(new int(i));
            }
            sparrow::any_data_container data(vec.data());
            CHECK_FALSE(data.owns_data());
        }

        SUBCASE("std::vector<std::unique_ptr<sparrow::buffer<int>>>")
        {
            sparrow::any_data_container data{create_vec_of_unique_buffer_int()};
            CHECK(data.owns_data());
        }

        SUBCASE("std::vector<std::shared_ptr<sparrow::buffer<int>>>")
        {
            sparrow::any_data_container data{create_vec_of_shared_buffer_int()};
            CHECK(data.owns_data());
        }

        SUBCASE("std::tuple<std::vector<int32_t>, sparrow::buffer<uint8_t>, int64_t*")
        {
            std::vector<int64_t> vec{0, 1, 2, 3, 4};

            std::tuple<std::vector<int32_t>, sparrow::buffer<int64_t>, int64_t*> tuple{
                std::vector<int32_t>{0, 1, 2, 3, 4},
                sparrow::buffer<int64_t>(vec),
                vec.data()
            };

            sparrow::any_data_container data{tuple};
            CHECK(data.owns_data());
        }

        SUBCASE("std:tuple<nonstd::value_ptr<int>, std::shared_ptr<int>>")
        {
            sparrow::any_data_container data{create_tuple_of_value_shared_int()};
            CHECK(data.owns_data());
        }

        SUBCASE("std:tuple<sparrow::unique_ptr<int>, std::shared_ptr<int>>")
        {
            sparrow::any_data_container data{create_tuple_of_unique_shared_int()};
            CHECK(data.owns_data());
        }
    }

    TEST_CASE("type_id")
    {
        SUBCASE("std::vector<std::vector<int>>")
        {
            const auto vec = create_vec_of_vec_int();
            sparrow::any_data_container data{vec};
            CHECK_EQ(data.type_id(), typeid(std::vector<std::vector<int>>));
        }

        SUBCASE("std::vector<int*>")
        {
            std::vector<int*> vec;
            for (int i = 0; i < 5; ++i)
            {
                vec.push_back(new int(i));
            }
            sparrow::any_data_container data(vec.data());
            CHECK_EQ(data.type_id(), typeid(void));
        }

        SUBCASE("std::vector<std::unique_ptr<sparrow::buffer<int>>>")
        {
            sparrow::any_data_container data{create_vec_of_unique_buffer_int()};
            CHECK_EQ(data.type_id(), typeid(std::vector<nonstd::value_ptr<sparrow::buffer<int>>>));
        }

        SUBCASE("std::vector<std::shared_ptr<sparrow::buffer<int>>>")
        {
            sparrow::any_data_container data{create_vec_of_shared_buffer_int()};
            CHECK_EQ(data.type_id(), typeid(std::vector<std::shared_ptr<sparrow::buffer<int>>>));
        }

        SUBCASE("std::tuple<std::vector<int32_t>, sparrow::buffer<uint8_t>, int64_t*")
        {
            std::vector<int64_t> vec{0, 1, 2, 3, 4};

            std::tuple<std::vector<int32_t>, sparrow::buffer<int64_t>, int64_t*> tuple{
                std::vector<int32_t>{0, 1, 2, 3, 4},
                sparrow::buffer<int64_t>(vec),
                vec.data()
            };

            sparrow::any_data_container data{tuple};
            CHECK_EQ(data.type_id(), typeid(std::tuple<std::vector<int32_t>, sparrow::buffer<int64_t>, int64_t*>));
        }

        SUBCASE("std:tuple<nonstd::value_ptr<int>, std::shared_ptr<int>>")
        {
            sparrow::any_data_container data{create_tuple_of_value_shared_int()};
            CHECK_EQ(data.type_id(), typeid(std::tuple<nonstd::value_ptr<int>, std::shared_ptr<int>>));
        }

        SUBCASE("std:tuple<sparrow::unique_ptr<int>, std::shared_ptr<int>>")
        {
            sparrow::any_data_container data{create_tuple_of_unique_shared_int()};
            CHECK_EQ(data.type_id(), typeid(std::tuple<nonstd::value_ptr<int>, std::shared_ptr<int>>));
        }
    }
}
