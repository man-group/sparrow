#pragma once
#include "doctest/doctest.h"

#include "sparrow/utils/nullable.hpp"
#include <algorithm>

namespace sparrow::test
{

    template<class ARRAY_TYPE>
    void generic_consistency_test_impl(ARRAY_TYPE && array){
        
        const auto size = array.size();

        // check that iterators sizes are consistent
        SUBCASE("iterators"){
            auto it = array.begin();
            auto it_end = array.end();
            CHECK(std::distance(it, it_end) == size);
        }
        SUBCASE("const iterators"){
            auto it = array.cbegin();
            auto it_end = array.cend();
            CHECK(std::distance(it, it_end) == size);
        }
    }

    template<class ARRAY_TYPE>
    void generic_consistency_test(ARRAY_TYPE & array){
        
        // array as array 
        generic_consistency_test_impl(array);

        // const array 
        const auto & const_array = array;
        generic_consistency_test_impl(const_array);
    }


    // ensure that the variant (of nullables) has a value
    // and that the value is equal to the expected value
    // (including the type)
    #define CHECK_NULLABLE_VARIANT_EQ(variant, value) sparrow::test::check_nullable_variant_eq(variant, value, __FILE__, __LINE__)
    
    template <class V, class T>
    static void check_nullable_variant_eq(const V & variant, const T & value,const char* file, int line)
    {
        return std::visit([&](auto && v) { 

            // v is a nullable
            using nullable_type = std::decay_t<decltype(v)>;
            using inner_type = std::decay_t<typename nullable_type::value_type>;

            if constexpr(std::is_same_v<inner_type, T>)
            {
                if (v.has_value())
                {
                    if(v.value() != value)
                    {
                        ADD_FAIL_AT(file, line, "value mismatch: expected " << value << " but got " << v.value());
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
        }, variant);
    }

    
};