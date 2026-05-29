#pragma once
#include <algorithm>

#include "sparrow/array.hpp"
#include "sparrow/utils/nullable.hpp"

#include "doctest/doctest.h"

namespace sparrow::test
{

#if SPARROW_GCC_11_2_WORKAROUND
    template <class T, class = void>
    struct has_base_type : std::false_type
    {
    };

    template <class T>
    struct has_base_type<T, std::void_t<typename T::base_type>> : std::true_type
    {
    };

    template <class T>
    constexpr decltype(auto) unwrap_gcc11_variant_base(const T& value)
    {
        if constexpr (has_base_type<T>::value)
        {
            if constexpr (std::derived_from<T, typename T::base_type>)
            {
                return unwrap_gcc11_variant_base(static_cast<const typename T::base_type&>(value));
            }
            else
            {
                return (value);
            }
        }
        else
        {
            return (value);
        }
    }
#endif

    template <class T>
    struct is_nullable_variant_like : std::false_type
    {
    };

    template <class T>
        requires requires { typename std::remove_cvref_t<T>::base_type; }
    struct is_nullable_variant_like<T>
        : std::bool_constant<std::derived_from<std::remove_cvref_t<T>, typename std::remove_cvref_t<T>::base_type>>
    {
    };

    template <class V>
        requires is_nullable_variant_like<V>::value
    auto visitable_nullable_variant(const V& value) -> typename std::remove_cvref_t<V>::base_type
    {
        using variant_type = std::remove_cvref_t<V>;
#if SPARROW_GCC_11_2_WORKAROUND
        return typename variant_type::base_type(unwrap_gcc11_variant_base(value));
#else
        return typename variant_type::base_type(value);
#endif
    }

    template <class V>
        requires(!is_nullable_variant_like<V>::value && std::is_convertible_v<const V&, array_traits::const_reference>)
    auto visitable_nullable_variant(const V& value) -> array_traits::const_reference::base_type
    {
        const auto materialized = static_cast<array_traits::const_reference>(value);
#if SPARROW_GCC_11_2_WORKAROUND
        return array_traits::const_reference::base_type(unwrap_gcc11_variant_base(materialized));
#else
        return array_traits::const_reference::base_type(materialized);
#endif
    }

    template <class V>
        requires(
            !is_nullable_variant_like<V>::value
            && !std::is_convertible_v<const V&, array_traits::const_reference>
        )
    decltype(auto) visitable_nullable_variant(const V& value)
    {
        return (value);
    }

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
            visitable_nullable_variant(variant)
        );
    }


};
