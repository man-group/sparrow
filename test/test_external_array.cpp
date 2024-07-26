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

#include "sparrow/external_array.hpp"

#include "external_array_data_creation.hpp"
#include "doctest/doctest.h"

using namespace sparrow;
//using sparrow::test::to_value_type;

namespace
{
    constexpr size_t array_size = 10;
    constexpr size_t array_offset = 0;

    template <class T>
    external_typed_array<T> make_test_external_typed_array(std::size_t size = array_size, std::size_t off = array_offset)
    {
        auto ar_data = sparrow::test::make_test_external_array_data<T>(size, off);
        return external_typed_array<T>(std::move(ar_data));
    }

    template <class T>
    external_array make_test_external_array(std::size_t size = array_size, std::size_t off = array_offset)
    {
        ArrowSchema schema;
        ArrowArray arr;
        sparrow::test::fill_schema_and_array<T>(schema, arr, size, off, {});
        return external_array(std::move(schema), std::move(arr));
    }

    /*template <class T>
    array make_test_external_array(std::size_t size = array_size, std::size_t off = array_offset)
    {
        auto ar_data = sparrow::test::make_test_external_array_data<T>(size, off);
        return array(std::move(ar_data));
    }*/
}

TEST_SUITE("const_external_array_iterator")
{
    using const_iter_type = external_array_iterator<true>;

    TEST_CASE("default constructor")
    {
        [[maybe_unused]] const_iter_type iter;
    }

    TEST_CASE_TEMPLATE_DEFINE("all", T, all)
    {
        SUBCASE("constructor")
        {
            auto tarray = make_test_external_typed_array<T>();
            [[maybe_unused]] const_iter_type iter(tarray.cbegin());
        }

        SUBCASE("equality")
        {
            auto tarray = make_test_external_typed_array<T>();
            const_iter_type iter(tarray.cbegin());
            const_iter_type iter2(tarray.cbegin());
            CHECK_EQ(iter, iter2);

            const_iter_type iter3;
            if constexpr (std::same_as<T, double>)
            {
                auto tarray2 = make_test_external_typed_array<int>();
                iter3 = tarray2.cbegin();
            }
            else
            {
                auto tarray2 = make_test_external_typed_array<double>();
                iter3 = tarray2.cbegin();
            }
            CHECK_NE(iter2, iter3);
        }

        SUBCASE("copy semantic")
        {
            auto tarray = make_test_external_typed_array<T>();
            const_iter_type iter(tarray.cbegin());
            const_iter_type iter2(iter);
            CHECK_EQ(iter, iter2);

            const_iter_type iter3;
            if constexpr (std::same_as<T, double>)
            {
                auto tarray2 = make_test_external_typed_array<int>();
                iter3 = tarray2.cbegin();
            }
            else
            {
                auto tarray2 = make_test_external_typed_array<double>();
                iter3 = tarray2.cbegin();
            }
            iter3 = iter;
            CHECK_EQ(iter, iter3);
        }

        SUBCASE("increment")
        {
            auto tarray = make_test_external_typed_array<T>();
            const_iter_type iter(tarray.cbegin());
            const_iter_type iter2(tarray.cbegin());
            ++iter;
            iter2++;
            CHECK_EQ(iter, iter2);

            iter2 += 2;
            const_iter_type iter3 = tarray.cbegin() + 3;
            CHECK_EQ(iter2, iter3);
        }

        SUBCASE("decrement")
        {
            auto tarray = make_test_external_typed_array<T>();
            const_iter_type iter(tarray.cbegin() + 3);
            const_iter_type iter2(tarray.cbegin() + 3);

            iter--;
            --iter2;
            CHECK_EQ(iter, iter2);

            iter2 -= 2;
            const_iter_type iter3 = tarray.cbegin();
            CHECK_EQ(iter2, iter3);
        }

        SUBCASE("distance")
        {
            auto tarray = make_test_external_typed_array<T>();
            const_iter_type iter(tarray.cbegin());
            const_iter_type iter2(tarray.cbegin() + 3);

            auto diff = iter2 - iter;
            CHECK_EQ(diff, 3);
        }

        SUBCASE("dereference")
        {
            using cref_t = external_typed_array<T>::const_reference;
            const auto tarray = make_test_external_typed_array<T>();
            const_iter_type iter = tarray.cbegin();

            auto val = std::get<cref_t>(*iter);
            CHECK_EQ(val, tarray[0]);

            ++iter;
            auto val2 = std::get<cref_t>(*iter);
            CHECK_EQ(val2, tarray[1]);
        }
    }

    TEST_CASE_TEMPLATE_INVOKE(
        all,
        bool,
        std::uint8_t,
        std::int8_t,
        std::uint16_t,
        std::int16_t,
        std::uint32_t,
        std::int32_t,
        std::uint64_t,
        std::int64_t,
        std::string,
        float16_t,
        float32_t,
        float64_t
    );
}

TEST_SUITE("external_array")
{
    TEST_CASE_TEMPLATE_DEFINE("all", T, all)
    {
        SUBCASE("constructor")
        {
            [[maybe_unused]] auto ar = make_test_external_array<T>();
        }

        /*SUBCASE("empty")
        {
            auto ar = make_test_external_array<T>();
            CHECK_FALSE(ar.empty());

            auto ar2 = make_test_external_array<T>(0, 0);
            CHECK(ar2.empty());
        }

        SUBCASE("size")
        {
            auto ar = make_test_external_array<T>();
            CHECK_EQ(ar.size(), array_size);
        }

        SUBCASE("const at")
        {
            using const_ref = typed_array<T>::const_reference;
            const auto ar = make_test_external_array<T>();
            for (std::size_t i = 0; i < ar.size(); ++i)
            {
                CHECK_EQ(std::get<const_ref>(ar.at(i)).value(), to_value_type<T>(i + array_offset));
            }
        }

        SUBCASE("const operator[]")
        {
            using const_ref = typed_array<T>::const_reference;
            const auto ar = make_test_external_array<T>();
            for (std::size_t i = 0; i < ar.size(); ++i)
            {
                CHECK_EQ(std::get<const_ref>(ar[i]).value(), to_value_type<T>(i + array_offset));
            }
        }

        SUBCASE("const iterators")
        {
            const auto ar = make_test_external_array<T>();
            using const_ref = typed_array<T>::const_reference;

            auto iter = ar.cbegin();
            auto iter2 = ar.begin();
            CHECK_EQ(iter, iter2);

            auto iter_end = ar.cend();
            auto iter_end2 = ar.end();
            CHECK_EQ(iter_end, iter_end2);

            for (std::size_t i = 0; i < ar.size(); ++iter, ++i)
            {
                CHECK_EQ(std::get<const_ref>(*iter), to_value_type<T>(i + array_offset));
            }

            CHECK_EQ(iter, iter_end);
        }

        SUBCASE("get")
        {
            auto ar = make_test_external_array<T>();
            const auto& car = ar;
            for (std::size_t i = 0; i < ar.size(); ++i)
            {
                if constexpr (std::same_as<T , bool>)
                {
                    ar.template get<T>(i).value() = false;
                    CHECK_EQ(car.template get<T>(i).value(), false);
                }
                else if constexpr (std::same_as<T, std::string>)
                {
                    ar.template get<T>(i).value() = "chopsuey";
                    CHECK_EQ(car.template get<T>(i).value(), "chopsuey");
                }
                else
                {
                    ar.template get<T>(i).value() *= 100;
                    CHECK_EQ(car.template get<T>(i).value(), to_value_type<T>(100 * (i + array_offset)));
                }
            }
        }*/
    }

    TEST_CASE_TEMPLATE_INVOKE(
        all,
        bool,
        std::uint8_t,
        std::int8_t,
        std::uint16_t,
        std::int16_t,
        std::uint32_t,
        std::int32_t,
        std::uint64_t,
        std::int64_t,
        std::string,
        float16_t,
        float32_t,
        float64_t
    );
}
