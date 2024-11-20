#pragma once

#include <iostream>
#include <sparrow/layout/list_layout/list_value.hpp>
#include <sparrow/layout/struct_layout/struct_value.hpp>
#include "sparrow/types/data_traits.hpp"

namespace sparrow
{   
    template<class T>
    void print_value(const T& v)
    {
        std::cout << "unkown" << std::endl;
    }

    void print_value(const numeric::float16_t & v)
    {
        std::cout << v;
    }

    template<class T>
    requires(std::is_scalar_v<T>)
    void print_value(const T& v)
    {
        std::cout << v;
    }

    template<class T>
    void print_value(const nullable<T>& v)
    {
        if(v.has_value())
        {
            print_value(v.value());
        }
        else
        {
            std::cout << "null";
        }
    }
    void print_value(const array_traits::const_reference & v)
    {
        std::visit([]( auto&& vv){ print_value(vv); }, v);
    }
    void print_value(const array_traits::value_type & v)
    {
        std::visit([]( auto&& vv){ print_value(vv); }, v);
    }
    void print_value(const list_value& v)
    {   
        const auto size = v.size();
        std::cout<<"[";
        for(std::size_t i = 0; i < size; ++i)
        {
            auto value = v[i];
            print_value(value);
            if(i != size - 1)
            {
                std::cout << ", ";
            }
        }
        std::cout << "]";
    }

    void print_value(const struct_value & v)
    {   
        const auto size = v.size();
        std::cout<<"<";
        for(std::size_t i = 0; i < size; ++i)
        {
            auto value = v[i];
            print_value(value);
            if(i != size - 1)
            {
                std::cout << ", ";
            }
        }
        std::cout << ">";
    }

   



    template<class ARR>
    void print_arr(const ARR& arr)
    {
        std::cout<<"{";
        std::size_t i = 0;
        for(auto&& v : arr)
        {
            print_value(v);
            if(i != arr.size() - 1)
            {
                std::cout<<", ";
            }
            ++i;
        }
        std::cout<<"}"<<std::endl;
    }
}// namespace sparrow