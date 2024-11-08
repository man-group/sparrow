#include <vector>
#include <string>
#include <array>
#include <tuple>

#include "builder/printer.hpp"
#include "builder/builder.hpp"


int main()
{
    // arr[float]
    {   
        std::vector<float> v{1.0, 2.0, 3.0, 4.0, 5.0};
        std::cout<<"arr[float]:"<<std::endl;
        print_arr(sparrow::build(v));
    }
    // arr[float] (with nulls)
    {   
        std::vector<sparrow::nullable<float>> v{1.0, 2.0, 3.0, sparrow::nullval, 5.0};
        std::cout<<"arr[float]:"<<std::endl;
        print_arr(sparrow::build(v));
    }
    // list[float]
    {   
        std::vector<std::vector<float>> v{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
        std::cout<<std::endl<<"list[float]:"<<std::endl;
        print_arr(sparrow::build(v));
    }
    // list[list[float]]
    {   
        std::vector<std::vector<std::vector<float>>> v{
            {{1.2f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}},
            {{7.0f, 8.0f, 9.0f}, {10.0f, 11.0f, 12.0f}}
        };
        std::cout<<std::endl<<"list[list[float]]:"<<std::endl;
        print_arr(sparrow::build(v));
    }
    // struct<float, float>
    {   
        std::vector<std::tuple<float, float>> v{
            {1.0f, 2.0f},
            {3.0f, 4.0f},
            {5.0f, 6.0f}
        };
        std::cout<<std::endl<<"struct<float, float>:"<<std::endl;
        print_arr(sparrow::build(v));
    }
    // struct<list[float], uint16>
    {   
        std::vector<std::tuple<std::vector<float>,std::uint16_t>> v{
            {{1.0f, 2.0f, 3.0f}, 1},
            {{4.0f, 5.0f, 6.0f}, 2},
            {{7.0f, 8.0f, 9.0f}, 3}
        };
        std::cout<<std::endl<<"struct<list[float], uint16>:"<<std::endl;
        print_arr(sparrow::build(v));
    }

    return 0;

}