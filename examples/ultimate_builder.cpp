#include <vector>
#include <string>
#include <array>
#include <tuple>
#include <set>

#include "printer.hpp"
#include <sparrow/builder/builder.hpp>

// namespace alias
namespace sp = sparrow;

template<class T>
using nt = sp::nullable<T>;

int main()
{   


    // arr[float]
    {   
        std::vector<float> v{1.0, 2.0, 3.0, 4.0, 5.0};
        print_arr(sp::build(v));
    }
    // arr[double] (with nulls)
    {   
        std::vector<nt<double>> v{1.0, 2.0, 3.0, sp::nullval, 5.0};
        print_arr(sp::build(v));
    }
    // list[float]
    {   
        std::vector<std::vector<float>> v{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
        print_arr(sp::build(v));
    }
    // list[list[float]]
    {   
        std::vector<std::vector<std::vector<float>>> v{
            {{1.2f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}},
            {{7.0f, 8.0f, 9.0f}, {10.0f, 11.0f, 12.0f}}
        };
        print_arr(sp::build(v));
    }
    // struct<float, float>
    {   
        std::vector<std::tuple<float, int>> v{
            {1.5f, 2},
            {3.5f, 4},
            {5.5f, 6}
        };
        print_arr(sp::build(v));
    }
    // struct<float, float> (with nulls)
    {   
        std::vector<nt<std::tuple<float, int>>> v{
            std::tuple<float, int>{1.5f, 2},
            sp::nullval,
            std::tuple<float, int>{5.5f, 6}
        };
        print_arr(sp::build(v));
    }
    // struct<list[float], uint16>
    {   
        std::vector<std::tuple<std::vector<float>,std::uint16_t>> v{
            {{1.0f, 2.0f, 3.0f}, 1},
            {{4.0f, 5.0f, 6.0f}, 2},
            {{7.0f, 8.0f, 9.0f}, 3}
        };
        print_arr(sp::build(v));
    }
    // fixed_sized_list<float, 3>
    {   
        std::vector<std::array<float, 3>> v{
            {1.0f, 2.0f, 3.0f},
            {4.0f, 5.0f, 6.0f},
            {7.0f, 8.0f, 9.0f}
        };
        print_arr(sp::build(v));
    }
    // fixed_sized_list<float, 3>  with nulls
    {   
        std::vector<nt<std::array<nt<float>, 3>>> v{
            std::array<nt<float>, 3>{1.0f, sp::nullval, 3.0f},
            sp::nullval,
            std::array<nt<float>, 3>{7.0f, 8.0f, sp::nullval}
        };
        print_arr(sp::build(v));
    }


    return 0;

}