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

#include <chrono>
#include <version>


#if defined(SPARROW_USE_DATE_POLYFILL)
#    include <date/tz.h>
#else
namespace date = std::chrono;
#endif

#include <climits>
#include <cstdint>
#include <cstring>
#include <concepts>
#include <string>

#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/mp_utils.hpp"


#if __cplusplus > 202002L and defined(__STDCPP_FLOAT16_T__) and defined(__STDCPP_FLOAT32_T__) \
    and defined(__STDCPP_FLOAT64_T__)
#    define SPARROW_STD_FIXED_FLOAT_SUPPORT
#endif

// TODO: use exclusively `std::float16_t etc. once we switch to c++23, see
// https://en.cppreference.com/w/cpp/types/floating-point
#if defined(SPARROW_STD_FIXED_FLOAT_SUPPORT)
#    include <stdfloat>
#else
// We disable some warnings for the 3rd party float16_t library
#    if defined(__clang__)
#        pragma clang diagnostic push
#        pragma clang diagnostic ignored "-Wconversion"
#        pragma clang diagnostic ignored "-Wsign-conversion"
#        pragma clang diagnostic ignored "-Wold-style-cast"
#        pragma clang diagnostic ignored "-Wdeprecated-declarations"
#    elif defined(__GNUC__)
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wconversion"
#        pragma GCC diagnostic ignored "-Wsign-conversion"
#        pragma GCC diagnostic ignored "-Wold-style-cast"
#    elif defined(_MSC_VER)
#        pragma warning(push)
#        pragma warning(disable : 4365)  // 'action' : conversion from 'type_1' to 'type_2', signed/unsigned
                                         // mismatch
#        pragma warning(disable : 4514)  // 'function' : unreferenced inline function has been removed
#        pragma warning(disable : 4668)  // 'symbol' is not defined as a preprocessor macro, replacing with
                                         // '0' for 'directives'
#    endif
#    include "sparrow/details/3rdparty/float16_t.hpp"
#    if defined(__GNUC__)
#        pragma GCC diagnostic pop
#    elif defined(__clang__)
#        pragma clang diagnostic pop
#    elif defined(_MSC_VER)
#        pragma warning(pop)
#    endif
#endif


namespace sparrow
{

// TODO: use exclusively `std::float16_t etc. once we switch to c++23, see
// https://en.cppreference.com/w/cpp/types/floating-point
#if defined(SPARROW_STD_FIXED_FLOAT_SUPPORT)
    using float16_t = std::float16_t;
    using float32_t = std::float32_t;
    using float64_t = std::float64_t;
#else
    using float16_t = numeric::float16_t;
    using float32_t = float;
    using float64_t = double;
#endif

    // P0355R7 (Extending chrono to Calendars and Time Zones) has not been entirely implemented in libc++ yet.
    // See: https://libcxx.llvm.org/Status/Cxx20.html#note-p0355
    // For now, we use HowardHinnant/date as a replacement if we are compiling with libc++.
    // TODO: use the following once libc++ has full support for P0355R7.
    // using timestamp = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;
    using timestamp = date::zoned_time<std::chrono::nanoseconds>;

    // We need to be sure the current target platform is setup to support correctly these types.
    static_assert(sizeof(float16_t) == 2);
    static_assert(sizeof(float32_t) == 4);
    static_assert(sizeof(float64_t) == 8);
    static_assert(std::is_floating_point_v<float16_t>);
    static_assert(std::is_floating_point_v<float32_t>);
    static_assert(std::is_floating_point_v<float64_t>);
    static_assert(CHAR_BIT == 8);

    using byte_t = std::byte;  // For now we will use this to represent raw data TODO: evaluate later if it's
                               // the right choice, switch to char if not

    struct null_type
    {
    };

    inline bool operator==(const null_type&, const null_type&)
    {
        return true;
    }

    /// Runtime identifier of arrow data types, usually associated with raw bytes with the associated value.
    // TODO: does not support all types specified by the Arrow specification
    // yet
    enum class data_type : uint8_t
    {
        NA = 0,
        BOOL = 1,
        UINT8 = 2,
        INT8 = 3,
        UINT16 = 4,
        INT16 = 5,
        UINT32 = 6,
        INT32 = 7,
        UINT64 = 8,
        INT64 = 9,
        HALF_FLOAT = 10,
        FLOAT = 11,
        DOUBLE = 12,
        // UTF8 variable-length string
        STRING = 13,
        // Variable-length bytes (no guarantee of UTF8-ness)
        BINARY = 14,
        // Fixed-size binary. Each value occupies the same number of bytes
        FIXED_SIZE_BINARY = 15,
        // Number of nanoseconds since the UNIX epoch with an optional timezone.
        // See: https://arrow.apache.org/docs/python/timestamps.html#timestamps
        TIMESTAMP = 18,
        LIST = 19,
        LARGE_LIST = 20,
        LIST_VIEW = 21,
        LARGE_LIST_VIEW = 22,
        FIXED_SIZED_LIST = 23,
        STRUCT = 24,
        MAP = 25,
        DENSE_UNION,
        SPARSE_UNION,
        RUN_ENCODED,
        DECIMAL,
        FIXED_WIDTH_BINARY
    };

    /// @returns The data_type value matching the provided format string or `data_type::NA`
    ///          if we couldnt find a matching data_type.
    // TODO: consider returning an optional instead
    constexpr data_type format_to_data_type(std::string_view format)
    {
        // TODO: add missing conversions from
        // https://arrow.apache.org/docs/dev/format/CDataInterface.html#data-type-description-format-strings
        if (format.size() == 1)
        {
            switch (format[0])
            {
                case 'n':
                    return data_type::NA;
                case 'b':
                    return data_type::BOOL;
                case 'C':
                    return data_type::UINT8;
                case 'c':
                    return data_type::INT8;
                case 'S':
                    return data_type::UINT16;
                case 's':
                    return data_type::INT16;
                case 'I':
                    return data_type::UINT32;
                case 'i':
                    return data_type::INT32;
                case 'L':
                    return data_type::UINT64;
                case 'l':
                    return data_type::INT64;
                case 'e':
                    return data_type::HALF_FLOAT;
                case 'f':
                    return data_type::FLOAT;
                case 'g':
                    return data_type::DOUBLE;
                case 'u':
                case 'U':  // large string
                    return data_type::STRING;
                case 'z':  // binary
                case 'Z':  // large binary
                    return data_type::BINARY;
                default:
                    return data_type::NA;
            }
        }
        else if (format == "vu")  // string view
        {
            return data_type::STRING;
        }
        else if (format == "vz")  // binary view
        {
            return data_type::FIXED_SIZE_BINARY;
        }
        // TODO: add propper timestamp support below
        else if (format.starts_with("t"))
        {
            return data_type::TIMESTAMP;
        }
        else if (format == "+l")
        {
            return data_type::LIST;
        }
        else if (format == "+L")
        {
            return data_type::LARGE_LIST;
        }
        else if (format == "+vl")
        {
            return data_type::LIST_VIEW;
        }
        else if (format == "+vL")
        {
            return data_type::LARGE_LIST_VIEW;
        }
        else if (format.starts_with("+w:"))
        {
            return data_type::FIXED_SIZED_LIST;
        }
        else if (format == "+s")
        {
            return data_type::STRUCT;
        }
        else if (format == "+m")
        {
            return data_type::MAP;
        }
        else if (format.starts_with("+ud:"))
        {
            return data_type::DENSE_UNION;
        }
        else if (format.starts_with("+us:"))
        {
            return data_type::SPARSE_UNION;
        }
        else if (format.starts_with("+r"))
        {
            return data_type::RUN_ENCODED;
        }
        else if (format.starts_with("d:"))
        {
            return data_type::DECIMAL;
        }
        else if (format.starts_with("w:"))
        {
            return data_type::FIXED_WIDTH_BINARY;
        }

        return data_type::NA;
    }

    /// @returns The number of buffers expected for the provided data_type.
    ///         The returned value is guaranteed to be in the range [0, 3].
    constexpr std::size_t get_expected_buffer_count(data_type type)
    {
        switch (type)
        {
            case data_type::NA:
            case data_type::RUN_ENCODED:
                return 0;
            case data_type::STRUCT:
            case data_type::SPARSE_UNION:
                return 1;
            case data_type::BOOL:
            case data_type::UINT8:
            case data_type::INT8:
            case data_type::UINT16:
            case data_type::INT16:
            case data_type::UINT32:
            case data_type::INT32:
            case data_type::FLOAT:
            case data_type::UINT64:
            case data_type::INT64:
            case data_type::DOUBLE:
            case data_type::HALF_FLOAT:
            case data_type::FIXED_WIDTH_BINARY:
            case data_type::DECIMAL:
            case data_type::LIST:
            case data_type::LARGE_LIST:
            case data_type::MAP:
            case data_type::FIXED_SIZED_LIST:
            case data_type::DENSE_UNION:
                return 2;
            case data_type::STRING:
            case data_type::BINARY:
            case data_type::FIXED_SIZE_BINARY:
            case data_type::TIMESTAMP:
            case data_type::LIST_VIEW:
            case data_type::LARGE_LIST_VIEW:
                return 3;
        }
        mpl::unreachable();
    }
    
    /// @returns The default floating-point `data_type`  that should be associated with the provided type.
    ///          The deduction will be based on the size of the type. Calling this function with unsupported sizes
    ///          will not compile.
    template<std::floating_point T>
        requires (sizeof(T) >= 2 && sizeof(T) <= 8)
    constexpr data_type data_type_from_size(T = {})
    {
        // TODO: consider rewriting this to benefit from if constexpr? might not be necessary
        switch(sizeof(T))
        {
            case 2: return data_type::HALF_FLOAT;
            case 4: return data_type::FLOAT;
            case 8: return data_type::DOUBLE;
        }

        mpl::unreachable();
    }

    /// @returns The default integral `data_type`  that should be associated with the provided type.
    ///          The deduction will be based on the size of the type. Calling this function with unsupported
    ///          sizes will not compile.
    template <std::integral T>
        requires(sizeof(T) >= 1 && sizeof(T) <= 8)
    constexpr data_type data_type_from_size(T = {})
    {
        if constexpr (std::same_as<bool, T>)
        {
            return data_type::BOOL;
        }
        else if constexpr (std::signed_integral<T>)
        {
            // TODO: consider rewriting this to benefit from if constexpr? might not be necessary
            switch (sizeof(T))
            {
                case 1: return data_type::INT8;
                case 2: return data_type::INT16;
                case 4: return data_type::INT32;
                case 8: return data_type::INT64;
            }
        }
        else
        {
            static_assert(std::unsigned_integral<T>);

            // TODO: consider rewriting this to benefit from if constexpr? might not be necessary
            switch (sizeof(T))
            {
                case 1: return data_type::UINT8;
                case 2: return data_type::UINT16;
                case 4: return data_type::UINT32;
                case 8: return data_type::UINT64;
            }
        }

        mpl::unreachable();
    }

    /// @returns Format string matching the provided data_type.
    ///          The returned string is guaranteed to be null-terminated and to have static storage
    ///          lifetime. (this means you can do data_type_to_format(mytype).data() to get a C pointer.
    constexpr std::string_view data_type_to_format(data_type type)
    {
        switch (type)
        {
            case data_type::NA:
                return "n";
            case data_type::BOOL:
                return "b";
            case data_type::UINT8:
                return "C";
            case data_type::INT8:
                return "c";
            case data_type::UINT16:
                return "S";
            case data_type::INT16:
                return "s";
            case data_type::UINT32:
                return "I";
            case data_type::INT32:
                return "i";
            case data_type::UINT64:
                return "L";
            case data_type::INT64:
                return "l";
            case data_type::HALF_FLOAT:
                return "e";
            case data_type::FLOAT:
                return "f";
            case data_type::DOUBLE:
                return "g";
            case data_type::STRING:
                return "u";
            case data_type::BINARY:
                return "z";
            case data_type::TIMESTAMP:
                return "tDm";
            case data_type::LIST:
                return "+l";
            case data_type::LARGE_LIST:
                return "+L";
            default:
                // TODO: add missing types
                throw std::runtime_error("Unsupported data type");
        }
    }

    /// @returns True if the provided data_type is a primitive type, false otherwise.
    constexpr bool data_type_is_primitive(data_type dt)
    {
        switch (dt)
        {
            case data_type::BOOL:
            case data_type::UINT8:
            case data_type::INT8:
            case data_type::UINT16:
            case data_type::INT16:
            case data_type::UINT32:
            case data_type::INT32:
            case data_type::UINT64:
            case data_type::INT64:
            case data_type::HALF_FLOAT:
            case data_type::FLOAT:
            case data_type::DOUBLE:
                return true;
            default:
                return false;
        }
    }

    /// @returns True if the provided data_type is an integer type, false otherwise.
    constexpr bool data_type_is_integer(data_type dt)
    {
        switch (dt)
        {
            case data_type::UINT8:
            case data_type::INT8:
            case data_type::UINT16:
            case data_type::INT16:
            case data_type::UINT32:
            case data_type::INT32:
            case data_type::UINT64:
            case data_type::INT64:
                return true;
            default:
                return false;
        }
    }

    /// @returns The number of bytes required to store the provided primitive data type.
    template<std::integral T>
    constexpr size_t primitive_bytes_count(data_type data_type, T size)
    {
        SPARROW_ASSERT_TRUE(data_type_is_primitive(data_type));
        constexpr double bit_per_byte = 8.;
        switch (data_type)
        {
            case data_type::BOOL:
                return static_cast<std::size_t>(std::ceil(static_cast<double>(size) / bit_per_byte));
            case data_type::UINT8:
            // TODO: Replace static_cast<std::size_t> by the 32 bit fix check function
            case data_type::INT8:
                return static_cast<std::size_t>(size);
            case data_type::UINT16:
                return (sizeof(std::uint16_t) / sizeof(std::uint8_t)) * static_cast<std::size_t>(size);
            case data_type::INT16:
                return (sizeof(std::int16_t) / sizeof(std::uint8_t)) * static_cast<std::size_t>(size);
            case data_type::UINT32:
                return (sizeof(std::uint32_t) / sizeof(std::uint8_t)) * static_cast<std::size_t>(size);
            case data_type::INT32:
                return (sizeof(std::int32_t) / sizeof(std::uint8_t)) * static_cast<std::size_t>(size);
            case data_type::UINT64:
                return (sizeof(std::uint64_t) / sizeof(std::uint8_t)) * static_cast<std::size_t>(size);
            case data_type::INT64:
                return (sizeof(std::int64_t) / sizeof(std::uint8_t)) * static_cast<std::size_t>(size);
            case data_type::HALF_FLOAT:
                return (sizeof(float16_t) / sizeof(std::uint8_t)) * static_cast<std::size_t>(size);
            case data_type::FLOAT:
                return (sizeof(float32_t) / sizeof(std::uint8_t)) * static_cast<std::size_t>(size);
            case data_type::DOUBLE:
                return (sizeof(float64_t) / sizeof(std::uint8_t)) * static_cast<std::size_t>(size);
            default:
                throw std::runtime_error("Unsupported data type");
        }
    }

    class list_value;
    class struct_value;

    /// C++ types value representation types matching Arrow types.
    // NOTE: this needs to be in sync-order with `data_type`
    using all_base_types_t = mpl::typelist<
        null_type,
        bool,
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
        float64_t,
        std::string,
        // std::vector<byte_t>,
        sparrow::timestamp,
        // TODO: add missing fundamental types here
        list_value,
        struct_value
        >;

    /// Type list of every C++ representation types supported by default, in order matching `data_type`
    /// related values.
    static constexpr all_base_types_t all_base_types;

    /// Matches C++ representation types which are supported by default.
    template <class T>
    concept is_arrow_base_type = mpl::contains<T>(all_base_types);


    /// is arrow base type or arrow compound type (list<T>, struct<T> etc.)
    //template <class T>
    //concept is_arrow_base_type_or_compound = is_arrow_base_type<T> || is_list_value_v<T>;


    using all_base_types_extended_t = mpl::append_t<all_base_types_t, char, std::string_view>;

    /// Type list of every C++ representation types supported by default, in order matching `data_type`
    /// related values.
    static constexpr all_base_types_extended_t all_base_types_extended;

    /// Checks if a type is an extended base type for Arrow.
    ///
    /// This concept checks if a given type `T` is an extended base type for Arrow.
    /// It uses the `mpl::contains` function to check if `T` is present in the `all_base_types_extended`
    /// list.
    ///
    /// @tparam T The type to check.
    /// @return `true` if `T` is an extended base type for Arrow, `false` otherwise.
    template <class T>
    concept is_arrow_base_type_extended = mpl::contains<T>(all_base_types_extended);

    /// Template alias to get the corresponding Arrow type for a given type.
    ///
    /// This template alias is used to determine the corresponding Arrow type for a given type.
    /// For example, the given type is std::string_view, the corresponding Arrow type is std::string.
    /// Otherwise, the corresponding Arrow type is the same as the given type.
    ///
    /// @tparam T The type for which to determine the corresponding Arrow type.
    /// @return The corresponding Arrow type for the given type.
    template <class T>
    using get_corresponding_arrow_type_t = std::conditional_t<std::same_as<T, std::string_view>, std::string, T>;

    /// Provides compile-time information about Arrow data types.
    /// Custom types can be made compatible by implementing this traits type.
    ///
    /// The following information must be provided if the type is an arrow type:
    /// - type_id : the runtime identifier value for that type, see `data_type`
    /// - value_type : the value representation type to use in C++ (usually T)
    /// - default_layout: the layout to use for that type FIXME: be more precise than that
    /// - MORE TO COME SOON, SEE TODOs BELOW
    ///
    /// @tparam T Type for C++ value-representation that this type describes the traits of.
    ///
    /// @note: See ./arrow_traits.hpp for implementations for default base types.
    /// @see `is_arrow_traits`, `has_arrow_type_traits`
    template <class T>
    struct arrow_traits;

    namespace detail
    {
        template <template <class> class>
        struct accepts_template
        {
        };
    }
    /// Matches valid and complete `arrow_traits` specializations for type T.
    /// Every type that needs to be compatible with this library's interface must
    /// provide a specialization of `arrow_traits`
    /// @see `arrow_traits`, `has_arrow_type_traits`
    template <class T>
    concept is_arrow_traits = mpl::is_type_instance_of_v<T, arrow_traits> and requires {
        /// Must provide a compile-time value of type `data_type`.
        /// This is used to identify which arrow data type is represented in `value_type`
        requires std::same_as<std::remove_cvref_t<decltype(T::type_id)>, ::sparrow::data_type>;

        /// The C++ representation of the arrow value. For `arrow_traits<X>`, this is usually `X`.
        typename T::value_type;

        /// The arrow (binary) layout to use by default for representing a set of data for that type.
        //typename detail::accepts_template<T::template default_layout>;

        // TODO: add more interface requirements on the traits here
        // TODO: add conversion operations between bytes and the value type
    };


    /// Matches types providing valid and complete `arrow_traits` specialization.
    /// @see `is_arrow_traits`, `arrow_traits`
    template <class T>
    concept has_arrow_type_traits = requires { typename ::sparrow::arrow_traits<T>; }
                                    and is_arrow_traits<::sparrow::arrow_traits<T>>;

    /// Matches any type which is one of the base C++ types supported or at least that provides an
    /// `arrow_traits` specialization.
    template <class T>
    concept any_arrow_type = is_arrow_base_type<T> or has_arrow_type_traits<T>;

    /// @returns Arrow type id to use for a given C++ representation of that type.
    ///          @see `arrow_traits`
    template <has_arrow_type_traits T>
    constexpr auto arrow_type_id() -> data_type
    {
        return arrow_traits<T>::type_id;
    }

    /// @returns Arrow type id to use for the type of a given object.
    ///          @see `arrow_traits`
    template <has_arrow_type_traits T>
    constexpr auto arrow_type_id(const T&) -> data_type
    {
        return arrow_type_id<T>();
    }

    /// @returns Format string matching the arrow data-type mathcing the provided
    ///          arrow type.
    template <has_arrow_type_traits T>
    constexpr std::string_view data_type_format_of()
    {
        return data_type_to_format(arrow_type_id<T>());
    }

    /// Binary layout type to use by default for the given C++ representation T of an arrow value.
    template <has_arrow_type_traits T>
    using default_layout_t = typename arrow_traits<T>::default_layout;

    // For now, a tiny wrapper around data_type
    // TODO: More data and functions to come
    class data_descriptor
    {
    public:

        constexpr data_descriptor()
            : data_descriptor(data_type::UINT8)
        {
        }

        data_descriptor(std::string_view format)
            : data_descriptor(format_to_data_type(format))
        {
        }

        constexpr explicit data_descriptor(data_type id)
            : m_id(id)
        {
        }

        constexpr data_type id() const
        {
            return m_id;
        }

    private:

        data_type m_id;
    };

    namespace impl
    {
        template <class C, bool is_const>
        struct get_inner_reference
            : std::conditional<is_const, typename C::inner_const_reference, typename C::inner_reference>
        {
        };

        template <class C, bool is_const>
        using get_inner_reference_t = typename get_inner_reference<C, is_const>::type;
    }  // namespace impl

    template <class T>
    concept layout_offset = std::same_as<T, std::int32_t> || std::same_as<T, std::int64_t>;
}
