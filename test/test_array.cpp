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
#include "sparrow/layout/array_factory.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/primitive_array.hpp"
#include "sparrow/struct_array.hpp"
#include "sparrow/utils/nullable.hpp"

#include "../test/external_array_data_creation.hpp"
#include "doctest/doctest.h"

namespace sparrow
{
    using testing_types = std::tuple<
        primitive_array<bool>,
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

            CHECK_EQ(dt, detail::get_data_type_from_array<AR>::get());
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
                sc.release(&sc);
                ar.release(&ar);
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

        TEST_CASE_TEMPLATE_DEFINE("get_arrow_array and get_arrow_schema", AR, get_arrow_array_id)
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

            ArrowSchema sc{};
            ArrowArray ar{};
            test::fill_schema_and_array<scalar_value_type>(sc, ar, size, offset, {});
            array a(std::move(ar), std::move(sc));

            SUBCASE("non const")
            {
                ArrowArray* ar_ptr = get_arrow_array(a);
                ArrowSchema* sc_ptr = get_arrow_schema(a);
                auto pa = primitive_array<scalar_value_type>(arrow_proxy(ar_ptr, sc_ptr));
                CHECK_EQ(pa, pa_ctrl);
                ar_ptr = nullptr;
            }

            SUBCASE("const")
            {
                const ArrowArray* ar_ptr = get_arrow_array(a);
                const ArrowSchema* sc_ptr = get_arrow_schema(a);
                auto pa = primitive_array<scalar_value_type>(
                    arrow_proxy(const_cast<ArrowArray*>(ar_ptr), const_cast<ArrowSchema*>(sc_ptr))
                );
                CHECK_EQ(pa, pa_ctrl);
                ar_ptr = nullptr;
            }
        }
        TEST_CASE_TEMPLATE_APPLY(get_arrow_array_id, testing_types);

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

        namespace detail
        {
            template <class T>
            T next_test_value(T val)
            {
                return ++val;
            }

            template <>
            inline bool next_test_value(bool val)
            {
                return !val;
            }
        }

        TEST_CASE_TEMPLATE_DEFINE("view", AR, view_id)
        {
            using scalar_value_type = typename AR::inner_value_type;
            constexpr size_t size = 10;
            array ar = test::make_array<scalar_value_type>(size);
            CHECK_FALSE(ar.is_view());
            auto ar_view = ar.view();
            CHECK(ar_view.is_view());
        }
        TEST_CASE_TEMPLATE_APPLY(view_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("slice", AR, slice_id)
        {
            using const_reference = typename AR::const_reference;
            using scalar_value_type = typename AR::inner_value_type;

            constexpr size_t size = 10;
            array ar = test::make_array<scalar_value_type>(size);

            REQUIRE_EQ(ar.size(), size);
            scalar_value_type scalar_value = 0;
            for (size_t i = 0; i < size; ++i)
            {
                CHECK_EQ(std::get<const_reference>(ar[i]), make_nullable(scalar_value));
                scalar_value = detail::next_test_value(scalar_value);
            }

            const auto slice_1_5 = ar.slice(1, 5);
            REQUIRE_EQ(slice_1_5.size(), 4);
            scalar_value = static_cast<scalar_value_type>(1);
            for (size_t i = 0; i < slice_1_5.size(); ++i)
            {
                CHECK_EQ(std::get<const_reference>(slice_1_5[i]).get(), scalar_value);
                scalar_value = detail::next_test_value(scalar_value);
            }

            const auto slice_2_8 = ar.slice(2, 8);
            REQUIRE_EQ(slice_2_8.size(), 6);
            if constexpr (std::same_as<scalar_value_type, bool>)
            {
                scalar_value = false;
            }
            else
            {
                scalar_value = static_cast<scalar_value_type>(2);
            }
            for (size_t i = 0; i < slice_2_8.size(); ++i)
            {
                CHECK_EQ(std::get<const_reference>(slice_2_8[i]).get(), scalar_value);
                scalar_value = detail::next_test_value(scalar_value);
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
            for (size_t i = 0; i < size; ++i)
            {
                CHECK_EQ(std::get<const_reference>(ar[i]).get(), scalar_value);
                scalar_value = detail::next_test_value(scalar_value);
            }

            const auto slice_1_5 = ar.slice_view(1, 5);
            REQUIRE_EQ(slice_1_5.size(), 4);
            scalar_value = static_cast<scalar_value_type>(1);
            for (size_t i = 0; i < slice_1_5.size(); ++i)
            {
                CHECK_EQ(std::get<const_reference>(slice_1_5[i]).get(), scalar_value);
                scalar_value = detail::next_test_value(scalar_value);
            }

            const auto slice_2_8 = ar.slice_view(2, 8);
            REQUIRE_EQ(slice_2_8.size(), 6);
            if constexpr (std::same_as<scalar_value_type, bool>)
            {
                scalar_value = false;
            }
            else
            {
                scalar_value = static_cast<scalar_value_type>(2);
            }
            for (size_t i = 0; i < slice_2_8.size(); ++i)
            {
                CHECK_EQ(std::get<const_reference>(slice_2_8[i]).get(), scalar_value);
                scalar_value = detail::next_test_value(scalar_value);
            }
        }
        TEST_CASE_TEMPLATE_APPLY(slice_view_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("name", AR, name_id)
        {
            constexpr size_t size = 10;
            using T = typename AR::inner_value_type;

            array spar = test::make_array<T>(size);
            CHECK_EQ(spar.name(), "test");
        }
        TEST_CASE_TEMPLATE_APPLY(name_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("set_name", AR, set_name_id)
        {
            constexpr size_t size = 10;
            using T = typename AR::inner_value_type;

            array spar = test::make_array<T>(size);
            CHECK_EQ(spar.name(), "test");
            spar.set_name(std::nullopt);
            CHECK_EQ(spar.name(), std::nullopt);
            spar.set_name("new_name");
            CHECK_EQ(spar.name(), "new_name");
        }
        TEST_CASE_TEMPLATE_APPLY(set_name_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("metadata", AR, metadata_id)
        {
            constexpr size_t size = 10;
            using T = typename AR::inner_value_type;

            array spar = test::make_array<T>(size);
            test_metadata(metadata_sample, *(spar.metadata()));
            spar.set_metadata<std::vector<metadata_pair>>(std::nullopt);
            CHECK_FALSE(spar.metadata().has_value());
            spar.set_metadata(metadata_sample_opt);
            test_metadata(metadata_sample, *(spar.metadata()));
        }
        TEST_CASE_TEMPLATE_APPLY(metadata_id, testing_types);

        TEST_CASE_TEMPLATE_DEFINE("offset_and_null_count", AR, offset_null_count_id)
        {
            constexpr size_t size = 10;
            constexpr size_t offset = 3;
            using T = typename AR::inner_value_type;

            SUBCASE("initial offset is 0")
            {
                array ar = test::make_array<T>(size);
                CHECK_EQ(ar.offset(), 0);
                CHECK_EQ(ar.null_count(), 0);
            }

            SUBCASE("offset after slicing")
            {
                array ar = test::make_array<T>(size);
                auto sliced = ar.slice(offset, size);
                CHECK_EQ(sliced.offset(), offset);
                CHECK_EQ(sliced.size(), size - offset);
            }

            SUBCASE("offset with initial offset")
            {
                array ar = test::make_array<T>(size, offset);
                CHECK_EQ(ar.offset(), offset);
                CHECK_EQ(ar.size(), size - offset);
            }
        }
        TEST_CASE_TEMPLATE_APPLY(offset_null_count_id, testing_types);


        TEST_CASE("with a dictionary")
        {
            using array_type = dictionary_encoded_array<int32_t>;

            // the words in the dictionary
            const std::vector<std::string> words{"zero", "one", "two", "three"};

            auto make_dictionary = [&]() -> array_type
            {
                using keys_buffer_type = typename array_type::keys_buffer_type;

                // the value array
                string_array values{words};

                // detyped array
                array values_arr(std::move(values));

                // the keys **data**
                keys_buffer_type keys{0, 1, 2, 3, 0, 1, 2, 3, 0, 1};

                // where nulls are
                std::vector<std::size_t> where_null{0};

                // create the array
                return array_type(
                    std::move(keys),
                    std::move(values_arr),
                    std::move(where_null),
                    "name",
                    metadata_sample_opt
                );
            };

            array_type dict = make_dictionary();
            array dict_arr{std::move(dict)};
            REQUIRE_EQ(dict_arr.size(), 10);

            CHECK_EQ(dict_arr.name(), "name");
            test_metadata(metadata_sample, *(dict_arr.metadata()));
            CHECK_EQ(dict_arr.data_type(), data_type::INT32);
            CHECK(dict_arr.dictionary().has_value());
            std::optional<array> dict_array_from_array = dict_arr.dictionary();
            CHECK_EQ(dict_array_from_array->data_type(), data_type::STRING);
        }

        TEST_CASE("without a dictionary")
        {
            sparrow::primitive_array<int32_t> primitives{0, 1, 2, 3, 0, 1, 2, 3, 0, 1};

            sparrow::array arr{std::move(primitives)};

            CHECK_EQ(arr.data_type(), data_type::INT32);
            CHECK_FALSE(arr.dictionary().has_value());
        }

        TEST_CASE("children")
        {
            SUBCASE("array with no children")
            {
                // Primitive arrays have no children
                sparrow::primitive_array<int32_t> primitives{0, 1, 2, 3};
                sparrow::array arr{std::move(primitives)};
                
                auto children = arr.children();
                CHECK_EQ(std::ranges::distance(children), 0);
            }

            SUBCASE("array with children - struct array")
            {
                // Create child arrays
                primitive_array<std::int16_t> flat_arr1(
                    {{std::int16_t(0), std::int16_t(1), std::int16_t(2)}, true, "child1"}
                );
                primitive_array<float32_t> flat_arr2(
                    {{3.0f, 4.0f, 5.0f}, true, "child2"}
                );

                // Convert to detyped arrays
                array arr1(std::move(flat_arr1));
                array arr2(std::move(flat_arr2));
                std::vector<array> child_arrays{std::move(arr1), std::move(arr2)};

                // Create struct array
                struct_array struct_arr(child_arrays, false, "struct_name", std::optional<std::vector<metadata_pair>>{});
                array arr(std::move(struct_arr));

                // Get children
                auto children = arr.children();
                
                // Check number of children
                CHECK_EQ(std::ranges::distance(children), 2);
                
                // Verify children are accessible
                std::vector<array> child_vec;
                for (const auto& child : children)
                {
                    child_vec.push_back(child);
                }
                
                CHECK_EQ(child_vec.size(), 2);
                CHECK_EQ(child_vec[0].size(), 3);
                CHECK_EQ(child_vec[1].size(), 3);
                CHECK_EQ(child_vec[0].name(), "child1");
                CHECK_EQ(child_vec[1].name(), "child2");
            }
        }
    }
}
