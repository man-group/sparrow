#pragma once
#include <algorithm>

#include "sparrow/array.hpp"
#include "sparrow/utils/nullable.hpp"

#include "doctest/doctest.h"

namespace sparrow::test
{

    template <class ARRAY_TYPE>
    void generic_consistency_test_impl(ARRAY_TYPE&& typed_arr)
    {
        using array_type = std::decay_t<ARRAY_TYPE>;
        const auto size = typed_arr.size();

        // check that iterators sizes are consistent
        SUBCASE("iterators")
        {
            auto it = typed_arr.begin();
            auto it_end = typed_arr.end();
            CHECK(std::distance(it, it_end) == size);
        }
        SUBCASE("const iterators")
        {
            auto it = typed_arr.cbegin();
            auto it_end = typed_arr.cend();
            CHECK(std::distance(it, it_end) == size);
        }

        SUBCASE("detype-visit-roundtrip")
        {
            // detyped copy
            array_type arr_copy(typed_arr);
            sparrow::array arr(std::move(arr_copy));

            // detyped visit
            arr.visit(
                [&](auto&& v)
                {
                    using typed_array_type = std::decay_t<decltype(v)>;
                    CHECK(std::is_same_v<typed_array_type, array_type>);
                    if constexpr (std::is_same_v<typed_array_type, ARRAY_TYPE>)
                    {
                        CHECK(v == typed_arr);
                    }
                }
            );
        }
    }

    template <class ARRAY_TYPE>
    void generic_consistency_test(ARRAY_TYPE& array)
    {
        // array as array
        generic_consistency_test_impl(array);

        // const array
        const auto& const_array = array;
        generic_consistency_test_impl(const_array);
    }

// ensure that the variant (of nullables) has a value
// and that the value is equal to the expected value
// (including the type)
#define CHECK_NULLABLE_VARIANT_EQ(variant, value) \
    sparrow::test::check_nullable_variant_eq(variant, value, __FILE__, __LINE__)

    template <class V, class T>
    static void check_nullable_variant_eq(const V& variant, const T& value, const char* file, int line)
    {
        return std::visit(
            [&](auto&& v)
            {
                // v is a nullable
                using nullable_type = std::decay_t<decltype(v)>;
                using inner_type = std::decay_t<typename nullable_type::value_type>;

                if constexpr (std::is_same_v<inner_type, T>)
                {
                    if (v.has_value())
                    {
                        if (v.value() != value)
                        {
                            ADD_FAIL_AT(
                                file,
                                line,
                                "value mismatch: expected " << value << " but got " << v.value()
                            );
                        }
                    }
                    else
                    {
                        ADD_FAIL_AT(file, line, "value is null");
                    }
                }
                else
                {
                    ADD_FAIL_AT(file, line, "type mismatch");
                }
            },

#if (!defined(__clang__) && defined(__GNUC__) && __GNUC__ < 11)
            static_cast<const typename V::base_type&>(variant)
#else
            variant
#endif
        );
    }


}
