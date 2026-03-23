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

#include <vector>

#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/list_array.hpp"
#include "sparrow/null_array.hpp"
#include "sparrow/primitive_array.hpp"

#include "doctest/doctest.h"
#include "external_array_data_creation.hpp"
#include "test_utils.hpp"

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

        const auto make_int_array = [](std::initializer_list<std::int32_t> values)
        {
            std::vector<nullable<std::int32_t>> nullable_values;
            nullable_values.reserve(values.size());
            for (const auto value : values)
            {
                nullable_values.push_back(make_nullable(value));
            }
            return primitive_array<std::int32_t>(nullable_values);
        };

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

        TEST_CASE("insert_elements_from")
        {
            using array_type = primitive_array<std::int32_t>;
            using wrapper_type = array_wrapper_impl<array_type>;

            SUBCASE("from another wrapper")
            {
                wrapper_type destination(make_int_array({1, 2, 3}));
                wrapper_type source(make_int_array({8, 9}));

                destination.insert_elements_from(1, source, 0, 2, 2);

                CHECK_EQ(destination.get_wrapped(), make_int_array({1, 8, 9, 8, 9, 2, 3}));
            }

            SUBCASE("self insertion copies the source slice first")
            {
                wrapper_type wrapper(make_int_array({1, 2, 3}));

                wrapper.insert_elements_from(1, wrapper, 0, 2, 2);

                CHECK_EQ(wrapper.get_wrapped(), make_int_array({1, 1, 2, 1, 2, 2, 3}));
            }

            SUBCASE("different wrappers sharing the same array still copy the source slice")
            {
                array_type array(make_int_array({1, 2, 3}));
                wrapper_type destination(&array);
                wrapper_type source(&array);

                destination.insert_elements_from(1, source, 0, 2, 2);

                CHECK_EQ(array, make_int_array({1, 1, 2, 1, 2, 2, 3}));
            }

            SUBCASE("different concrete wrapper types with the same data type throw")
            {
                using extended_array_type = primitive_array<
                    std::int32_t,
                    simple_extension<"sparrow.test.array_wrapper.insert_elements_from">>;
                using extended_wrapper_type = array_wrapper_impl<extended_array_type>;

                wrapper_type destination(array_type(make_arrow_proxy<std::int32_t>(3, offset)));
                extended_wrapper_type source(extended_array_type(make_arrow_proxy<std::int32_t>(2, offset)));

                CHECK_THROWS_AS(destination.insert_elements_from(1, source, 0, 2, 1), std::invalid_argument);
            }
        }

        TEST_CASE("erase_array_elements")
        {
            using array_type = primitive_array<std::int32_t>;
            using wrapper_type = array_wrapper_impl<array_type>;

            wrapper_type wrapper(make_int_array({1, 2, 3, 4, 5}));

            wrapper.erase_array_elements(1, 2);

            CHECK_EQ(wrapper.get_wrapped(), make_int_array({1, 4, 5}));
        }
    }
}
