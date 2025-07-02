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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or mplied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <cstdint>

#include "sparrow/layout/primitive_layout/primitive_array_impl.hpp"
#include "sparrow/types/data_type.hpp"

namespace sparrow
{
    template <typename T>
    concept primitive_type = std::is_arithmetic_v<T> || std::is_same_v<T, float16_t>
                             || std::is_same_v<T, bool> || std::is_same_v<T, std::byte>;

    /**
     * Array of values of whose type has fixed binary size.
     *
     * The type of the values in the array can be a primitive type, whose size is known at compile
     * time, or an arbitrary binary type whose fixed size is known at runtime only.
     * The current implementation supports types whose size is known at compile time only.
     *
     * As the other arrays in sparrow, \c primitive_array<T> provides an API as if it was holding
     * \c nullable<T> values instead of \c T values.
     *
     * Internally, the array contains a validity bitmap and a contiguous memory buffer
     * holding the values.
     *
     * @tparam T the type of the values in the array.
     * @see https://arrow.apache.org/docs/dev/format/Columnar.html#fixed-size-primitive-layout
     */
    template <primitive_type T>
    using primitive_array = primitive_array_impl<T>;

    template <class T>
    struct is_primitive_array : std::false_type
    {
    };

    template <class T>
    struct is_primitive_array<primitive_array<T>> : std::true_type
    {
    };

    /**
     * Checkes whether T is a primitive_array type.
     */
    template <class T>
    constexpr bool is_primitive_array_v = is_primitive_array<T>::value;

    namespace detail
    {
        template <class T>
        struct get_data_type_from_array;

        template <>
        struct get_data_type_from_array<primitive_array<bool>>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::BOOL;
            }
        };

        template <>
        struct get_data_type_from_array<primitive_array<std::int8_t>>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::INT8;
            }
        };

        template <>
        struct get_data_type_from_array<primitive_array<std::uint8_t>>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::UINT8;
            }
        };

        template <>
        struct get_data_type_from_array<primitive_array<std::int16_t>>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::INT16;
            }
        };

        template <>
        struct get_data_type_from_array<primitive_array<std::uint16_t>>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::UINT16;
            }
        };

        template <>
        struct get_data_type_from_array<primitive_array<std::int32_t>>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::INT32;
            }
        };

        template <>
        struct get_data_type_from_array<primitive_array<std::uint32_t>>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::UINT32;
            }
        };

        template <>
        struct get_data_type_from_array<primitive_array<std::int64_t>>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::INT64;
            }
        };

        template <>
        struct get_data_type_from_array<primitive_array<std::uint64_t>>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::UINT64;
            }
        };

        template <>
        struct get_data_type_from_array<primitive_array<float16_t>>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::HALF_FLOAT;
            }
        };

        template <>
        struct get_data_type_from_array<primitive_array<float32_t>>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::FLOAT;
            }
        };

        template <>
        struct get_data_type_from_array<primitive_array<float64_t>>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::DOUBLE;
            }
        };
    }
}
