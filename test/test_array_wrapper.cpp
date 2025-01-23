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

#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/null_array.hpp"
#include "sparrow/layout/primitive_array.hpp"

#include "doctest/doctest.h"
#include "external_array_data_creation.hpp"

namespace sparrow
{
    using test::make_arrow_proxy;
    using testing_types = std::tuple<
        null_array,
        primitive_array<std::int8_t>,
        primitive_array<std::uint8_t>,
        primitive_array<std::int16_t>,
        primitive_array<std::uint16_t>,
        primitive_array<std::int32_t>,
        primitive_array<std::uint32_t>,
        primitive_array<std::int64_t>,
        primitive_array<std::uint64_t>,
        primitive_array<float16_t>,
        primitive_array<float32_t>,
        primitive_array<float64_t>>;

    TEST_SUITE("array_wrapper")
    {
        constexpr std::size_t size = 10u;
        constexpr std::size_t offset = 0u;

        TEST_CASE_TEMPLATE_DEFINE("Constructor", AR, array_wrapper_ctor)
        {
            using array_type = AR;
            using scalar_value_type = typename AR::inner_value_type;
            using wrapper_type = array_wrapper_impl<AR>;

            SUBCASE("from rvalue")
            {
                array_type ar(make_arrow_proxy<scalar_value_type>(size, offset));
                array_type expected(ar);
                wrapper_type w(std::move(ar));
                CHECK_EQ(w.get_wrapped(), expected);
            }

            SUBCASE("from pointer")
            {
                array_type ar(make_arrow_proxy<scalar_value_type>(size, offset));
                wrapper_type w(&ar);
                CHECK_EQ(w.get_wrapped(), ar);
            }

            SUBCASE("from shared_ptr")
            {
                auto ptr = std::make_shared<array_type>(make_arrow_proxy<scalar_value_type>(size, offset));
                wrapper_type w(ptr);
                CHECK_EQ(w.get_wrapped(), *ptr);
            }
        }
        TEST_CASE_TEMPLATE_APPLY(array_wrapper_ctor, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("unwrap_array", AR, array_wrapper_unwrap)
        {
            using array_type = AR;
            using scalar_value_type = typename AR::inner_value_type;
            using wrapper_type = array_wrapper_impl<AR>;

            SUBCASE("from rvalue")
            {
                array_type ar(make_arrow_proxy<scalar_value_type>(size, offset));
                wrapper_type w(std::move(ar));
                CHECK_EQ(unwrap_array<AR>(w), w.get_wrapped());
            }

            SUBCASE("from pointer")
            {
                array_type ar(make_arrow_proxy<scalar_value_type>(size, offset));
                wrapper_type w(&ar);
                CHECK_EQ(unwrap_array<AR>(w), w.get_wrapped());
            }

            SUBCASE("from shared_ptr")
            {
                auto ptr = std::make_shared<array_type>(make_arrow_proxy<scalar_value_type>(size, offset));
                wrapper_type w(ptr);
                CHECK_EQ(unwrap_array<AR>(w), w.get_wrapped());
            }
        }
        TEST_CASE_TEMPLATE_APPLY(array_wrapper_unwrap, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("clone", AR, array_wrapper_clone)
        {
            using array_type = AR;
            using scalar_value_type = typename AR::inner_value_type;
            using wrapper_type = array_wrapper_impl<AR>;

            SUBCASE("from rvalue")
            {
                array_type ar(make_arrow_proxy<scalar_value_type>(size, offset));
                wrapper_type w(std::move(ar));
                auto cl = w.clone();

                CHECK_EQ(unwrap_array<AR>(*cl), w.get_wrapped());
                CHECK_NE(&(unwrap_array<AR>(*cl)), &(w.get_wrapped()));
            }

            SUBCASE("from pointer")
            {
                array_type ar(make_arrow_proxy<scalar_value_type>(size, offset));
                wrapper_type w(&ar);
                auto cl = w.clone();
                CHECK_EQ(unwrap_array<AR>(*cl), ar);
                CHECK_NE(&(unwrap_array<AR>(*cl)), &ar);
            }

            SUBCASE("from shared_ptr")
            {
                auto ptr = std::make_shared<array_type>(make_arrow_proxy<scalar_value_type>(size, offset));
                wrapper_type w(ptr);
                auto cl = w.clone();
                CHECK_EQ(unwrap_array<AR>(*cl), *ptr);
                CHECK_NE(&(unwrap_array<AR>(*cl)), ptr.get());
            }
        }
        TEST_CASE_TEMPLATE_APPLY(array_wrapper_clone, testing_types);
    }
}
