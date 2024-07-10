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
#include "sparrow/memory.hpp"

#include "doctest/doctest.h"

TEST_SUITE("any_data")
{
    TEST_CASE("constructors")
    {
        SUBCASE("std::vector<int>")
        {
            std::vector<int> vec{1, 2, 3, 4, 5};
            sparrow::any_data any_data(vec);
        }

        SUBCASE("raw pointer")
        {
            int i = 5;
            sparrow::any_data<int> any_data(&i);
        }

        SUBCASE("unique pointer")
        {
            auto ptr = std::make_unique<int>(5);
            sparrow::any_data<int> any_data(std::move(ptr));
        }

        SUBCASE("arrow_array_unique_ptr")
        {
            auto ptr = sparrow::default_arrow_array();
            ptr->length = 99;
            ptr->null_count = 42;
            sparrow::any_data<ArrowArray> any_data(std::move(ptr));
        }

        SUBCASE("std::nullptr_t")
        {
            sparrow::any_data<int> any_data(nullptr);
        }
    }

    TEST_CASE("get")
    {
        SUBCASE("mutable")
        {
            SUBCASE("raw pointer")
            {
                int i = 5;
                sparrow::any_data<int> data(&i);
                int* ptr = data.get();
                CHECK_EQ(*ptr, 5);
                *ptr = 2;
                CHECK_EQ(i, 2);
            }

            SUBCASE("std::vector<int>")
            {
                std::vector<int> vec{1, 2, 3, 4, 5};
                sparrow::any_data any_data(vec);
                std::vector<int>* ptr = any_data.get();
                for (size_t i = 0; i < vec.size(); ++i)
                {
                    CHECK_EQ(ptr->at(i), vec[i]);
                }
            }

            SUBCASE("unique_ptr")
            {
                auto unique_ptr = std::make_unique<int>(5);
                sparrow::any_data<int> data(std::move(unique_ptr));
                int* ptr = data.get();
                CHECK_EQ(*ptr, 5);
            }

            SUBCASE("std::nullptr_t")
            {
                sparrow::any_data<int> any_data(nullptr);
                CHECK_EQ(any_data.get(), nullptr);
            }
        }

        SUBCASE("const")
        {
            SUBCASE("raw pointer")
            {
                int i = 5;
                const sparrow::any_data<int> data(&i);
                const int* ptr = data.get();
                CHECK_EQ(*ptr, 5);
            }

            SUBCASE("std::vector<int>")
            {
                std::vector<int> vec{1, 2, 3, 4, 5};
                const sparrow::any_data any_data(vec);
                const std::vector<int>* ptr = any_data.get();
                for (size_t i = 0; i < vec.size(); ++i)
                {
                    CHECK_EQ(ptr->at(i), vec[i]);
                }
            }

            SUBCASE("unique_ptr")
            {
                auto unique_ptr = std::make_unique<int>(5);
                const sparrow::any_data<int> data(std::move(unique_ptr));
                const int* ptr = data.get();
                CHECK_EQ(*ptr, 5);
            }

            SUBCASE("std::nullptr_t")
            {
                const sparrow::any_data<int> any_data(nullptr);
                CHECK_EQ(any_data.get(), nullptr);
            }
        }
    }

    TEST_CASE("get_data")
    {
        SUBCASE("mutable")
        {
            SUBCASE("raw pointer")
            {
                int i = 5;
                sparrow::any_data<int> data(&i);
                REQUIRE_THROWS_AS(data.get_data<int&>(), std::bad_any_cast);
            }

            SUBCASE("std::vector<int>")
            {
                std::vector<int> vec{1, 2, 3, 4, 5};
                sparrow::any_data any_data(vec);
                REQUIRE_NOTHROW(any_data.get_data<std::vector<int>&>());
                auto& data = any_data.get_data<std::vector<int>&>();
                for (size_t i = 0; i < vec.size(); ++i)
                {
                    CHECK_EQ(data[i], vec[i]);
                }
            }

            SUBCASE("unique pointer")
            {
                auto ptr = std::make_unique<int>(5);
                sparrow::any_data<int> data(std::move(ptr));
                REQUIRE_NOTHROW(data.get_data<sparrow::value_ptr<int>&>());
                auto& value = data.get_data<sparrow::value_ptr<int>&>();
                CHECK_EQ(*value.get(), 5);
            }

            SUBCASE("std::nullptr_t")
            {
                sparrow::any_data<int> any_data(nullptr);
                REQUIRE_THROWS_AS(any_data.get_data<int&>(), std::bad_any_cast);
            }
        }

        SUBCASE("const")
        {
            SUBCASE("raw pointer")
            {
                int i = 5;
                const sparrow::any_data<int> data(&i);
                REQUIRE_THROWS_AS(data.get_data<const int&>(), std::bad_any_cast);
            }
        }
    }

    TEST_CASE("type_id")
    {
        SUBCASE("int pointer")
        {
            int i = 5;
            sparrow::any_data<int> data(&i);
            CHECK_EQ(data.type_id(), typeid(void));
        }

        SUBCASE("float pointer")
        {
            float f = 5.0f;
            sparrow::any_data<float> data(&f);
            CHECK_EQ(data.type_id(), typeid(void));
        }

        SUBCASE("unique pointer")
        {
            auto ptr = std::make_unique<int>(5);
            sparrow::any_data<int> data(std::move(ptr));
            std::string type_id = data.type_id().name();
            CHECK_EQ(data.type_id(), typeid(sparrow::value_ptr<int>));
        }
    }

    TEST_CASE("owns_data")
    {
        SUBCASE("int pointer")
        {
            int i = 5;
            sparrow::any_data<int> data(&i);
            CHECK_FALSE(data.owns_data());
        }

        SUBCASE("std::vector<int>")
        {
            std::vector<int> vec{1, 2, 3, 4, 5};
            sparrow::any_data any_data(vec);
            CHECK(any_data.owns_data());
        }

        SUBCASE("unique pointer")
        {
            auto ptr = std::make_unique<int>(5);
            sparrow::any_data<int> data(std::move(ptr));
            CHECK(data.owns_data());
        }

        SUBCASE("arrow_array_unique_ptr")
        {
            auto ptr = sparrow::default_arrow_array();
            sparrow::any_data<ArrowArray> data(std::move(ptr));
            CHECK(data.owns_data());
        }

        SUBCASE("ArrowArray")
        {
            auto ptr = sparrow::default_arrow_array();
            sparrow::any_data<ArrowArray> data(std::move(ptr));
            CHECK(data.owns_data());
        }

        SUBCASE("std::nullptr_t")
        {
            sparrow::any_data<int> any_data(nullptr);
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

std::tuple<sparrow::value_ptr<int>, std::shared_ptr<int>> create_tuple_of_value_shared_int()
{
    auto value_ptr = sparrow::value_ptr<int>{5};
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
            sparrow::any_data_container<int> data(create_vec_of_vec_int());
        }

        SUBCASE("std::vector<int*>")
        {
            std::vector<int*> vec;
            for (int i = 0; i < 5; ++i)
            {
                vec.push_back(new int(i));
            }
            sparrow::any_data_container<int> data(vec);
        }

        SUBCASE("std::vector<std::unique_ptr<sparrow::buffer<int>>>")
        {
            sparrow::any_data_container<int> data(create_vec_of_unique_buffer_int());
        }

        SUBCASE("std::vector<std::shared_ptr<sparrow::buffer<int>>>")
        {
            sparrow::any_data_container<int> data(create_vec_of_shared_buffer_int());
        }

        SUBCASE("std::tuple<std::vector<int32_t>, sparrow::buffer<uint8_t>, int64_t*")
        {
            std::vector<int64_t> vec{0, 1, 2, 3, 4};

            std::tuple<std::vector<int32_t>, sparrow::buffer<int64_t>, int64_t*> tuple{
                std::vector<int32_t>{0, 1, 2, 3, 4},
                sparrow::buffer<int64_t>(vec),
                vec.data()
            };

            sparrow::any_data_container<uint8_t> data(tuple);
        }

        SUBCASE("std:tuple<sparrow::value_ptr<int>, std::shared_ptr<int>>")
        {
            sparrow::any_data_container<int> data(create_tuple_of_value_shared_int());
        }

        SUBCASE("std:tuple<sparrow::unique_ptr<int>, std::shared_ptr<int>>")
        {
            sparrow::any_data_container<int> data(create_tuple_of_unique_shared_int());
        }
    }

    TEST_CASE("get_pointers_vec")
    {
        SUBCASE("std::vector<std::vector<int>>")
        {
            const auto vec = create_vec_of_vec_int();
            sparrow::any_data_container<int> data(vec);
            auto ptrs = data.get_pointers_vec();
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
            sparrow::any_data_container<int> data(vec);
            auto ptrs = data.get_pointers_vec();
            for (size_t i = 0; i < 5; ++i)
            {
                CHECK_EQ(*ptrs[i], i);
            }
        }

        SUBCASE("std::vector<std::unique_ptr<sparrow::buffer<int>>>")
        {
            sparrow::any_data_container<int> data(create_vec_of_unique_buffer_int());
            auto ptrs = data.get_pointers_vec();
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
            sparrow::any_data_container<int> data(create_vec_of_shared_buffer_int());
            auto ptrs = data.get_pointers_vec();
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

            sparrow::any_data_container<uint8_t> data(tuple);
            auto ptrs = data.get_pointers_vec();
            const auto vec_ptr = reinterpret_cast<int32_t*>(ptrs.at(0));
            const auto buffer_ptr = reinterpret_cast<int64_t*>(ptrs.at(1));
            const auto int64_ptr = reinterpret_cast<int64_t*>(ptrs.at(2));

            for (size_t i = 0; i < 5; ++i)
            {
                CHECK_EQ(vec_ptr[i], i);
                CHECK_EQ(buffer_ptr[i], i);
                CHECK_EQ(int64_ptr[i], i);
            }
        }

        SUBCASE("std:tuple<sparrow::value_ptr<int>, std::shared_ptr<int>>")
        {
            sparrow::any_data_container<int> data(create_tuple_of_value_shared_int());
            auto ptrs = data.get_pointers_vec();
            CHECK_EQ(*ptrs[0], 5);
            CHECK_EQ(*ptrs[1], 6);
        }

        SUBCASE("std:tuple<sparrow::unique_ptr<int>, std::shared_ptr<int>>")
        {
            sparrow::any_data_container<int> data(create_tuple_of_unique_shared_int());
            auto ptrs = data.get_pointers_vec();
            CHECK_EQ(*ptrs[0], 5);
            CHECK_EQ(*ptrs[1], 6);
        }
    }

    TEST_CASE("get_data")
    {
        SUBCASE("mutable")
        {
            SUBCASE("std::vector<std::vector<int>>")
            {
                const auto vec = create_vec_of_vec_int();
                sparrow::any_data_container<int> data(vec);
                auto& data_vec = data.get_data<std::vector<std::vector<int>>&>();
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
                sparrow::any_data_container<int> data(vec);
                CHECK_FALSE(data.owns_data());
            }

            SUBCASE("std::vector<std::unique_ptr<sparrow::buffer<int>>>")
            {
                sparrow::any_data_container<int> data(create_vec_of_unique_buffer_int());
                auto& data_vec = data.get_data<std::vector<sparrow::value_ptr<sparrow::buffer<int>>>&>();
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
                sparrow::any_data_container<int> data(create_vec_of_shared_buffer_int());
                auto& data_vec = data.get_data<std::vector<std::shared_ptr<sparrow::buffer<int>>>&>();
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

                sparrow::any_data_container<uint8_t> data(tuple);
                auto& data_tuple = data.get_data<
                    std::tuple<std::vector<int32_t>, sparrow::buffer<int64_t>, int64_t*>&>();
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

            SUBCASE("std:tuple<sparrow::value_ptr<int>, std::shared_ptr<int>>")
            {
                sparrow::any_data_container<int> data(create_tuple_of_value_shared_int());
                auto& data_tuple = data.get_data<std::tuple<sparrow::value_ptr<int>, std::shared_ptr<int>>&>();
                CHECK_EQ(*std::get<0>(data_tuple), 5);
                CHECK_EQ(*std::get<1>(data_tuple), 6);
            }

            SUBCASE("std:tuple<sparrow::unique_ptr<int>, std::shared_ptr<int>>")
            {
                sparrow::any_data_container<int> data(create_tuple_of_unique_shared_int());
                auto& data_tuple = data.get_data<std::tuple<sparrow::value_ptr<int>, std::shared_ptr<int>>&>();
                CHECK_EQ(*std::get<0>(data_tuple), 5);
                CHECK_EQ(*std::get<1>(data_tuple), 6);
            }
        }
    }

    TEST_CASE("owns_data")
    {
        SUBCASE("std::vector<std::vector<int>>")
        {
            const auto vec = create_vec_of_vec_int();
            sparrow::any_data_container<int> data(vec);
            CHECK(data.owns_data());
        }

        SUBCASE("raw pointer")
        {
            std::vector<int*> vec;
            for (int i = 0; i < 5; ++i)
            {
                vec.push_back(new int(i));
            }
            sparrow::any_data_container<int> data(vec.data());
            CHECK_FALSE(data.owns_data());
        }

        SUBCASE("std::vector<std::unique_ptr<sparrow::buffer<int>>>")
        {
            sparrow::any_data_container<int> data(create_vec_of_unique_buffer_int());
            CHECK(data.owns_data());
        }

        SUBCASE("std::vector<std::shared_ptr<sparrow::buffer<int>>>")
        {
            sparrow::any_data_container<int> data(create_vec_of_shared_buffer_int());
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

            sparrow::any_data_container<uint8_t> data(tuple);
            CHECK(data.owns_data());
        }

        SUBCASE("std:tuple<sparrow::value_ptr<int>, std::shared_ptr<int>>")
        {
            sparrow::any_data_container<int> data(create_tuple_of_value_shared_int());
            CHECK(data.owns_data());
        }

        SUBCASE("std:tuple<sparrow::unique_ptr<int>, std::shared_ptr<int>>")
        {
            sparrow::any_data_container<int> data(create_tuple_of_unique_shared_int());
            CHECK(data.owns_data());
        }
    }
}
