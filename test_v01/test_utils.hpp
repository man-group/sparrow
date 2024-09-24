#pragma once
#include "doctest/doctest.h"


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
};