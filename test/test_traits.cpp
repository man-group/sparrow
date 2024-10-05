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

#include <sparrow/array/data_traits.hpp>
#include <sparrow/layout/primitive_array.hpp>


/////////////////////////////////////////////////////////////////////////////////////////
// Opt-in support for custom C++ representations of arrow data types.

struct MyDataType
{
};

template <>
struct sparrow::arrow_traits<MyDataType>
{
    static constexpr data_type type_id = sparrow::data_type::INT32;
    using value_type = MyDataType;
    using default_layout = sparrow::primitive_array<MyDataType>;
};

static_assert(sparrow::is_arrow_traits<sparrow::arrow_traits<MyDataType>>);
static_assert(sparrow::any_arrow_type<MyDataType>);

//////////////////////////////////////////////////////////////////////////////////////////
// Base arrow types representations support tests and concept checking.
namespace sparrow
{
    static_assert(mpl::all_of(all_base_types_t{}, predicate::is_arrow_base_type));
    static_assert(mpl::all_of(all_base_types_t{}, predicate::has_arrow_traits));


// Native basic standard types support

    using basic_native_types = mpl::typelist<
        bool,
        char, unsigned char, signed char,
        short, unsigned short,
        int, unsigned int,
        long, unsigned long, // `long long` could be bigger than 64bits and is not supported
        float, double, // `long double` could be bigger than 64bit and is not supported
        std::uint8_t,
        std::int8_t,
        std::uint16_t,
        std::int16_t,
        std::uint32_t,
        std::int32_t,
        std::uint64_t,
        std::int64_t,
        float16_t,
        float32_t,
        float64_t
        >;

    template <std::integral T>
    consteval
    bool is_possible_arrow_data_type(data_type type_id)
    {
        // NOTE:
        // `char` is not specified by the C and C++ standard to be `signed` or `unsigned`.
        // The language also specifies `signed char` and `unsigned char` as distinct types from `char`.
        // Therefore, for `char`, sign-ness can vary depending on the C++ target platform and compiler.
        // The best we can check is that `sizeof(char)` matches the associated arrow data
        // integral type minimum size, whatever the sign-ness, which is why we don't treat
        // `char` as a special case below.

        if constexpr (std::same_as<T, bool>)
        {
            return type_id == data_type::BOOL;
        }
        // we need T to be able to store the data coming from an arrow data value
        // of the associated arrow data type
        else if constexpr (std::is_signed_v<T>)
        {
            switch (type_id)
            {
                case data_type::INT8:
                    return sizeof(T) == 1;
                case data_type::INT16:
                    return sizeof(T) <= 2;
                case data_type::INT32:
                    return sizeof(T) <= 4;
                case data_type::INT64:
                    return sizeof(T) <= 8;
                default:
                    return false;
            }
        }
        else
        {
            static_assert(std::is_unsigned_v<T>);

            switch (type_id)
            {
                case data_type::UINT8:
                    return sizeof(T) == 1;
                case data_type::UINT16:
                    return sizeof(T) <= 2;
                case data_type::UINT32:
                    return sizeof(T) <= 4;
                case data_type::UINT64:
                    return sizeof(T) <= 8;
                default:
                    return false;
            }
        }

        return false;
    }

    template <std::floating_point T>
    consteval
    bool is_possible_arrow_data_type(data_type type_id)
    {
        switch (type_id)
        {
            case data_type::HALF_FLOAT:
                return sizeof(T) <= 2;
            case data_type::FLOAT:
                return sizeof(T) <= 4;
            case data_type::DOUBLE:
                return sizeof(T) <= 8;
            default:
                return false;
        }
    }

    // Tests `data_type_from_size` and it's usage in `arrow_traits<T>::type_id`
    struct
    {
        template <class T>
            requires has_arrow_type_traits<T>
        consteval
        bool operator()(mpl::typelist<T>)
        {
            constexpr auto deduced_type_id = data_type_from_size<T>();
            static_assert(deduced_type_id == arrow_traits<T>::type_id);

            return is_possible_arrow_data_type<T>(arrow_traits<T>::type_id);
        }
    } constexpr has_possible_arrow_data_type;

    // Every basic native types must have an arrow trait, whatever the platform,
    // including when fixed-size standard library names are or not alias to basic types.
    // Only exceptions: types that could be bigger than 64bit (`long long`, `long double`, etc.)
    static_assert(mpl::all_of(basic_native_types{}, has_possible_arrow_data_type));

}
