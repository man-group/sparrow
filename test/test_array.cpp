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
#include "sparrow/utils/nullable.hpp"

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
        primitive_array<float64_t>>;

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
            using T = typename AR::inner_value_type;
            SUBCASE("from Arrow C Structures")
            {
                array spar = test::make_array<T>(size);

                CHECK_EQ(spar.size(), size);
            }
            SUBCASE("from moved typed array")
            {
                ArrowSchema sc{};
                ArrowArray ar{};
                test::fill_schema_and_array<T>(sc, ar, size, 0, {});
                auto pr = primitive_array<T>(arrow_proxy(std::move(ar), std::move(sc)));
                auto spar = array(std::move(pr));
                CHECK_EQ(spar.size(), size);
            }
            SUBCASE("from typed array ref")
            {
                ArrowSchema sc{};
                ArrowArray ar{};
                test::fill_schema_and_array<T>(sc, ar, size, 0, {});
                auto pr = primitive_array<T>(arrow_proxy(std::move(ar), std::move(sc)));
                auto spar = array(&pr);
                CHECK_EQ(spar.size(), size);
            }
        }
        TEST_CASE_TEMPLATE_APPLY(array_constructor_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("operator==", AR, equal_operator_id)
        {
            constexpr size_t size = 10;
            using T = typename AR::inner_value_type;

            array spar = test::make_array<T>(size);
            array spar2 = test::make_array<T>(size);
            CHECK(spar == spar2);

            array spar3 = test::make_array<T>(size + 2u);
            CHECK(spar != spar3);
        }
        TEST_CASE_TEMPLATE_APPLY(equal_operator_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("data_type", AR, data_type_id)
        {
            using scalar_value_type = typename AR::inner_value_type;
            constexpr size_t size = 10;
            array ar = test::make_array<scalar_value_type>(size);
            data_type dt = ar.data_type();

            CHECK_EQ(dt, arrow_traits<scalar_value_type>::type_id);
        }
        TEST_CASE_TEMPLATE_APPLY(data_type_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("at", AR, at_id)
        {
            using const_reference = typename AR::const_reference;
            using scalar_value_type = typename AR::inner_value_type;

            constexpr size_t size = 10;
            array ar = test::make_array<scalar_value_type>(size);
            auto pa = primitive_array<scalar_value_type>(test::make_arrow_proxy<scalar_value_type>(size));

            for (std::size_t i = 0; i < pa.size(); ++i)
            {
                CHECK_EQ(std::get<const_reference>(ar.at(i)), pa[i]);
            }
            CHECK_THROWS_AS(ar.at(size), std::out_of_range);
        }
        TEST_CASE_TEMPLATE_APPLY(at_id, testing_types);

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

        TEST_CASE_TEMPLATE_DEFINE("front", AR, front_id)
        {
            using const_reference = typename AR::const_reference;
            using scalar_value_type = typename AR::inner_value_type;

            constexpr size_t size = 10;
            array ar = test::make_array<scalar_value_type>(size);
            auto pa = primitive_array<scalar_value_type>(test::make_arrow_proxy<scalar_value_type>(size));
            CHECK_EQ(std::get<const_reference>(ar.front()), pa.front());
        }
        TEST_CASE_TEMPLATE_APPLY(front_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("back", AR, back_id)
        {
            using const_reference = typename AR::const_reference;
            using scalar_value_type = typename AR::inner_value_type;

            constexpr size_t size = 10;
            array ar = test::make_array<scalar_value_type>(size);
            auto pa = primitive_array<scalar_value_type>(test::make_arrow_proxy<scalar_value_type>(size));
            CHECK_EQ(std::get<const_reference>(ar.back()), pa.back());
        }
        TEST_CASE_TEMPLATE_APPLY(back_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("owns_arrow_strucure", AR, owns_arrow_structure_id)
        {
            SUBCASE("owning")
            {
                constexpr size_t size = 10;
                using scalar_value_type = typename AR::inner_value_type;
                array ar = test::make_array<scalar_value_type>(size);

                CHECK(owns_arrow_array(ar));
                CHECK(owns_arrow_schema(ar));
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

                CHECK(!owns_arrow_array(a));
                CHECK(!owns_arrow_schema(a));
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
            auto pa_ctrl = primitive_array<scalar_value_type>(
                arrow_proxy(std::move(ar_ctrl), std::move(sc_ctrl))
            );

            SUBCASE("not owning")
            {
                ArrowSchema sc{};
                ArrowArray ar{};
                test::fill_schema_and_array<scalar_value_type>(sc, ar, size, offset, {});
                array a(&ar, &sc);

                auto [ar_ptr, sc_ptr] = get_arrow_structures(a);
                // Now sc_ptr and ar_ptr points to &sc and &ar

                auto pa = primitive_array<scalar_value_type>(arrow_proxy(ar_ptr, sc_ptr));

                CHECK_EQ(pa, pa_ctrl);

                sc.release(&sc);
                ar.release(&ar);
                sc_ptr = nullptr;
                ar_ptr = nullptr;
            }

            SUBCASE("owning")
            {
                ArrowSchema sc{};
                ArrowArray ar{};
                test::fill_schema_and_array<scalar_value_type>(sc, ar, size, offset, {});
                array a(std::move(ar), std::move(sc));

                auto [ar_ptr, sc_ptr] = get_arrow_structures(a);
                // Now sc_ptr and ar_ptr points to &sc and &ar

                auto pa = primitive_array<scalar_value_type>(arrow_proxy(ar_ptr, sc_ptr));

                CHECK_EQ(pa, pa_ctrl);

                sc_ptr = nullptr;
                ar_ptr = nullptr;
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
            auto pa_ctrl = primitive_array<scalar_value_type>(
                arrow_proxy(std::move(ar_ctrl), std::move(sc_ctrl))
            );

            SUBCASE("not owning")
            {
                ArrowSchema sc{};
                ArrowArray ar{};
                test::fill_schema_and_array<scalar_value_type>(sc, ar, size, offset, {});
                array a(&ar, &sc);

                CHECK_THROWS(extract_arrow_array(std::move(a)));
                CHECK_THROWS(extract_arrow_schema(std::move(a)));

                sc.release(&sc);
                ar.release(&ar);
            }

            SUBCASE("owning")
            {
                ArrowSchema sc{};
                ArrowArray ar{};
                test::fill_schema_and_array<scalar_value_type>(sc, ar, size, offset, {});
                array a(std::move(ar), std::move(sc));

                auto [ar_dst, sc_dst] = extract_arrow_structures(std::move(a));

                auto pa = primitive_array<scalar_value_type>(arrow_proxy(std::move(ar_dst), std::move(sc_dst)));

                CHECK_EQ(pa, pa_ctrl);
            }
        }
        TEST_CASE_TEMPLATE_APPLY(extract_arrow_structure_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("visit", AR, visit_id)
        {
            constexpr size_t offset = 0;
            constexpr size_t size = 10;
            using scalar_value_type = typename AR::inner_value_type;

            ArrowSchema sc{};
            ArrowArray ar{};
            test::fill_schema_and_array<scalar_value_type>(sc, ar, size, offset, {});
            array arr(std::move(ar), std::move(sc));

            size_t res = arr.visit(
                [](const auto& impl)
                {
                    return impl.size();
                }
            );
            CHECK_EQ(res, size);
        }
        TEST_CASE_TEMPLATE_APPLY(visit_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("slice", AR, slice_id)
        {
            using const_reference = typename AR::const_reference;
            using scalar_value_type = typename AR::inner_value_type;

            constexpr size_t size = 10;
            array ar = test::make_array<scalar_value_type>(size);

            REQUIRE_EQ(ar.size(), size);
            scalar_value_type scalar_value = 0;
            for (size_t i = 0; i < size; ++i, ++scalar_value)
            {
                CHECK_EQ(std::get<const_reference>(ar[i]), make_nullable(scalar_value));
            }

            const auto slice_1_5 = ar.slice(1, 5);
            REQUIRE_EQ(slice_1_5.size(), 4);
            scalar_value = static_cast<scalar_value_type>(1);
            for (size_t i = 0; i < slice_1_5.size(); ++i, ++scalar_value)
            {
                CHECK_EQ(std::get<const_reference>(slice_1_5[i]).get(), scalar_value);
            }

            ar.slice(2, 8);
            const auto slice_2_8 = ar.slice(2, 8);
            REQUIRE_EQ(slice_2_8.size(), 6);
            scalar_value = static_cast<scalar_value_type>(2);
            for (size_t i = 0; i < slice_2_8.size(); ++i, ++scalar_value)
            {
                CHECK_EQ(std::get<const_reference>(slice_2_8[i]).get(), scalar_value);
            }
        }
        TEST_CASE_TEMPLATE_APPLY(slice_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("slice_view", AR, slice_view_id)
        {
            using const_reference = typename AR::const_reference;
            using scalar_value_type = typename AR::inner_value_type;

            constexpr size_t size = 10;
            array ar = test::make_array<scalar_value_type>(size);

            REQUIRE_EQ(ar.size(), size);
            scalar_value_type scalar_value = 0;
            for (size_t i = 0; i < size; ++i, ++scalar_value)
            {
                CHECK_EQ(std::get<const_reference>(ar[i]).get(), scalar_value);
            }

            const auto slice_1_5 = ar.slice_view(1, 5);
            REQUIRE_EQ(slice_1_5.size(), 4);
            scalar_value = static_cast<scalar_value_type>(1);
            for (size_t i = 0; i < slice_1_5.size(); ++i, ++scalar_value)
            {
                CHECK_EQ(std::get<const_reference>(slice_1_5[i]).get(), scalar_value);
            }

            ar.slice_view(2, 8);
            const auto slice_2_8 = ar.slice_view(2, 8);
            REQUIRE_EQ(slice_2_8.size(), 6);
            scalar_value = static_cast<scalar_value_type>(2);
            for (size_t i = 0; i < slice_2_8.size(); ++i, ++scalar_value)
            {
                CHECK_EQ(std::get<const_reference>(slice_2_8[i]).get(), scalar_value);
            }
        }
        TEST_CASE_TEMPLATE_APPLY(slice_view_id, testing_types);
    }
}
