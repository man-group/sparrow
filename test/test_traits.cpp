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

#include <sparrow/fixed_width_binary_array.hpp>
#include <sparrow/types/data_traits.hpp>

/////////////////////////////////////////////////////////////////////////////////////////
// Opt-in support for custom C++ representations of arrow data types.

struct MyDataType
{
};

template <>
struct sparrow::arrow_traits<MyDataType>
{
    static constexpr data_type type_id = sparrow::data_type::FIXED_WIDTH_BINARY;
    using value_type = MyDataType;
    using default_layout = sparrow::fixed_width_binary_array;
};

static_assert(sparrow::is_arrow_traits<sparrow::arrow_traits<MyDataType>>);
static_assert(sparrow::any_arrow_type<MyDataType>);

//////////////////////////////////////////////////////////////////////////////////////////
// Base arrow types representations support tests and concept checking.
namespace sparrow
{
    static_assert(mpl::all_of(all_base_types_t{}, predicate::is_arrow_base_type));
    static_assert(mpl::all_of(all_base_types_t{}, predicate::has_arrow_traits));

    template <std::integral T>
    consteval bool is_possible_arrow_data_type(data_type type_id)
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
    consteval bool is_possible_arrow_data_type(data_type type_id)
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

}
