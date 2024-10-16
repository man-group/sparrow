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

#include "sparrow/array.hpp"
#include "sparrow/array_factory.hpp"
#include "sparrow/layout/primitive_array.hpp"
#include "../test/external_array_data_creation.hpp"
#include "doctest/doctest.h"

namespace sparrow
{
    using testing_types = std::tuple<
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
        primitive_array<float64_t>
    >;

    namespace test
    {
        template <class T>
        array make_array(size_t n, size_t offset = 0)
        {
            ArrowSchema sc{};
            ArrowArray ar{};
            test::fill_schema_and_array<T>(sc, ar, n, offset, {});
            return array(std::move(ar), std::move(sc));
        }
    }

    TEST_SUITE("array")
    {
        TEST_CASE_TEMPLATE_DEFINE("constructor", AR, array_constructor_id)
        {
            constexpr size_t size = 10;
            array spar = test::make_array<typename AR::inner_value_type>(size);

            CHECK_EQ(spar.size(), size);
        }
        TEST_CASE_TEMPLATE_APPLY(array_constructor_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("operator[]", AR, access_operator_id)
        {
            using const_reference = typename AR::const_reference;
            using scalar_value_type = typename AR::inner_value_type;

            constexpr size_t size = 10;
            array ar = test::make_array<scalar_value_type>(size);
            auto pa = primitive_array<scalar_value_type>(test::make_arrow_proxy<scalar_value_type>(size));

            for (std::size_t i = 0; i < pa.size(); ++i)
            {
                CHECK_EQ(std::get<const_reference>(ar[i]), pa[i]);
            }
        }
        TEST_CASE_TEMPLATE_APPLY(access_operator_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("owns_arrow_strucure", AR, owns_arrow_structure_id)
        {
            SUBCASE("owning")
            {
                constexpr size_t size = 10;
                using scalar_value_type = typename AR::inner_value_type;
                array ar = test::make_array<scalar_value_type>(size);

                CHECK(ar.owns_arrow_array());
                CHECK(ar.owns_arrow_schema());
            }

            SUBCASE("not owning")
            {
                constexpr size_t offset = 0;
                constexpr size_t size = 10;
                using scalar_value_type = typename AR::inner_value_type;
                ArrowSchema sc{};
                ArrowArray ar{};
                test::fill_schema_and_array<scalar_value_type>(sc, ar, size, offset, {});
                array a(&ar, &sc);

                CHECK(!a.owns_arrow_array());
                CHECK(!a.owns_arrow_schema());
            }
        }
        TEST_CASE_TEMPLATE_APPLY(owns_arrow_structure_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("get_arrow_structure", AR, get_arrow_structure_id)
        {
            constexpr size_t offset = 0;
            constexpr size_t size = 10;
            using scalar_value_type = typename AR::inner_value_type;
            
            ArrowSchema sc_ctrl{};
            ArrowArray ar_ctrl{};
            test::fill_schema_and_array<scalar_value_type>(sc_ctrl, ar_ctrl, size, offset, {});
            auto pa_ctrl = primitive_array<scalar_value_type>(arrow_proxy(std::move(ar_ctrl), std::move(sc_ctrl)));

            SUBCASE("not owning")
            {
                ArrowSchema sc{};
                ArrowArray ar{};
                test::fill_schema_and_array<scalar_value_type>(sc, ar, size, offset, {});
                array a(&ar, &sc);

                ArrowSchema* sc_ptr = nullptr;
                ArrowArray* ar_ptr = nullptr;
                a.get_arrow_array(ar_ptr).get_arrow_schema(sc_ptr);

                auto pa = primitive_array<scalar_value_type>(arrow_proxy(ar_ptr, sc_ptr));

                CHECK_EQ(pa, pa_ctrl);
            }

            SUBCASE("owning")
            {
                ArrowSchema sc{};
                ArrowArray ar{};
                test::fill_schema_and_array<scalar_value_type>(sc, ar, size, offset, {});
                array a(std::move(ar), std::move(sc));

                ArrowSchema* sc_ptr = nullptr;
                ArrowArray* ar_ptr = nullptr;
                a.get_arrow_array(ar_ptr).get_arrow_schema(sc_ptr);

                auto pa = primitive_array<scalar_value_type>(arrow_proxy(ar_ptr, sc_ptr));

                CHECK_EQ(pa, pa_ctrl);
            }
        }
        TEST_CASE_TEMPLATE_APPLY(get_arrow_structure_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("extract_arrow_structure", AR, extract_arrow_structure_id)
        {
            constexpr size_t offset = 0;
            constexpr size_t size = 10;
            using scalar_value_type = typename AR::inner_value_type;

            ArrowSchema sc_ctrl{};
            ArrowArray ar_ctrl{};
            test::fill_schema_and_array<scalar_value_type>(sc_ctrl, ar_ctrl, size, offset, {});
            auto pa_ctrl = primitive_array<scalar_value_type>(arrow_proxy(std::move(ar_ctrl), std::move(sc_ctrl)));

            SUBCASE("not owning")
            {
                ArrowSchema sc{};
                ArrowArray ar{};
                test::fill_schema_and_array<scalar_value_type>(sc, ar, size, offset, {});
                array a(&ar, &sc);

                ArrowSchema sc_dst;
                ArrowArray ar_dst;
                CHECK_THROWS(std::move(a).extract_arrow_array(ar_dst));
                CHECK_THROWS(std::move(a).extract_arrow_schema(sc_dst));
            }

            SUBCASE("owning")
            {
                ArrowSchema sc{};
                ArrowArray ar{};
                test::fill_schema_and_array<scalar_value_type>(sc, ar, size, offset, {});
                array a(std::move(ar), std::move(sc));

                ArrowSchema sc_dst;
                ArrowArray ar_dst;
                std::move(a).extract_arrow_array(ar_dst).extract_arrow_schema(sc_dst);

                auto pa = primitive_array<scalar_value_type>(arrow_proxy(std::move(ar_dst), std::move(sc_dst)));

                CHECK_EQ(pa, pa_ctrl);
            }
        }
        TEST_CASE_TEMPLATE_APPLY(extract_arrow_structure_id, testing_types);
    }
}

