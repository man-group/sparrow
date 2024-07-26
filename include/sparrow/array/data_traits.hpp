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

#pragma once

#include "sparrow/array/data_type.hpp"
#include "sparrow/layout/fixed_size_layout.hpp"
#include "sparrow/layout/null_layout.hpp"
#include "sparrow/layout/variable_size_binary_layout.hpp"

namespace sparrow
{

    template <class T>
    struct common_native_types_traits
    {
        using value_type = T;
        template <class DS>
        using default_layout = fixed_size_layout<T, DS>;
    };

    template <>
    struct arrow_traits<null_type>
    {
        static constexpr data_type type_id = data_type::NA;
        static constexpr const char* format = "n";
        using value_type = null_type;
        template <class DS>
        using default_layout = null_layout<DS>;
    };

    template <>
    struct arrow_traits<bool> : common_native_types_traits<bool>
    {
        static constexpr data_type type_id = data_type::BOOL;
        static constexpr const char* format = "b";
    };

    template <>
    struct arrow_traits<std::uint8_t> : common_native_types_traits<std::uint8_t>
    {
        static constexpr data_type type_id = data_type::UINT8;
        static constexpr const char* format = "C";
    };

    template <>
    struct arrow_traits<std::int8_t> : common_native_types_traits<std::int8_t>
    {
        static constexpr data_type type_id = data_type::INT8;
        static constexpr const char* format = "c";
    };

    template <>
    struct arrow_traits<char> : common_native_types_traits<char>
    {
        static constexpr data_type type_id = data_type::UINT8;
        static constexpr const char* format = "C";
    };

    template <>
    struct arrow_traits<std::uint16_t> : common_native_types_traits<std::uint16_t>
    {
        static constexpr data_type type_id = data_type::UINT16;
        static constexpr const char* format = "S";
    };

    template <>
    struct arrow_traits<std::int16_t> : common_native_types_traits<std::int16_t>
    {
        static constexpr data_type type_id = data_type::INT16;
        static constexpr const char* format = "s";
    };

    template <>
    struct arrow_traits<std::uint32_t> : common_native_types_traits<std::uint32_t>
    {
        static constexpr data_type type_id = data_type::UINT32;
        static constexpr const char* format = "I";
    };

    template <>
    struct arrow_traits<std::int32_t> : common_native_types_traits<std::int32_t>
    {
        static constexpr data_type type_id = data_type::INT32;
        static constexpr const char* format = "i";
    };

    template <>
    struct arrow_traits<std::uint64_t> : common_native_types_traits<std::uint64_t>
    {
        static constexpr data_type type_id = data_type::UINT64;
        static constexpr const char* format = "L";
    };

    template <>
    struct arrow_traits<std::int64_t> : common_native_types_traits<std::int64_t>
    {
        static constexpr data_type type_id = data_type::INT64;
        static constexpr const char* format = "l";
    };

    template <>
    struct arrow_traits<float16_t> : common_native_types_traits<float16_t>
    {
        static constexpr data_type type_id = data_type::HALF_FLOAT;
        static constexpr const char* format = "e";
    };

    template <>
    struct arrow_traits<float32_t> : common_native_types_traits<float32_t>
    {
        static constexpr data_type type_id = data_type::FLOAT;
        static constexpr const char* format = "f";
    };

    template <>
    struct arrow_traits<float64_t> : common_native_types_traits<float64_t>
    {
        static constexpr data_type type_id = data_type::DOUBLE;
        static constexpr const char* format = "g";
    };

    template <>
    struct arrow_traits<std::string>
    {
        static constexpr data_type type_id = data_type::STRING;
        static constexpr const char* format = "u";
        using value_type = std::string;
        template <class DS>
        using default_layout = variable_size_binary_layout<value_type, const std::string_view, DS>;
    };

    template <>
    struct arrow_traits<std::vector<byte_t>>
    {
        static constexpr data_type type_id = data_type::STRING;
        static constexpr const char* format = "z";
        using value_type = std::vector<byte_t>;
        template <class DS>
        using default_layout = variable_size_binary_layout<value_type, const std::span<byte_t>, DS>;
    };

    template <>
    struct arrow_traits<timestamp> : common_native_types_traits<timestamp>
    {
        static constexpr data_type type_id = data_type::TIMESTAMP;
        // By default duration in milliseconds, but see
        // https://arrow.apache.org/docs/dev/format/CDataInterface.html#data-type-description-format-strings
        // for other possibilities
        static constexpr const char* format = "tDm";
    };

    namespace predicate
    {

        struct
        {
            template <class T>
            consteval bool operator()(mpl::typelist<T>)
            {
                return sparrow::is_arrow_base_type<T>;
            }
        } constexpr is_arrow_base_type;

        struct
        {
            template <class T>
            consteval bool operator()(mpl::typelist<T>)
            {
                return sparrow::is_arrow_traits<arrow_traits<T>>;
            }
        } constexpr has_arrow_traits;
    }

}
