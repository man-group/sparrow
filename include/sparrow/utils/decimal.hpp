#pragma once

#include <cmath>
#include <iostream>
#include <sstream>

#if defined(__cpp_lib_format)
#    include <format>
#endif

#include "sparrow/utils/large_int.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    template <typename T>
    concept decimal_integer_type = std::is_same_v<T, std::int32_t> || std::is_same_v<T, std::int64_t>
                                   || std::is_same_v<T, int128_t> || std::is_same_v<T, int256_t>;

    template <decimal_integer_type T>
    class decimal
    {
    public:

        using integer_type = T;

        constexpr decimal()
            requires(!is_int_placeholder_v<T>);
        constexpr decimal()
            requires(is_int_placeholder_v<T>);
        constexpr decimal(T value, int scale);

        constexpr bool operator==(const decimal& other) const;

        constexpr explicit operator float() const
            requires(!is_int_placeholder_v<T>);
        constexpr explicit operator double() const
            requires(!is_int_placeholder_v<T>);
        constexpr explicit operator long double() const
            requires(!is_int_placeholder_v<T>);

        // convert to string
        [[nodiscard]] explicit operator std::string() const
            requires(!is_int_placeholder_v<T>);

        [[nodiscard]] constexpr T storage() const;

        [[nodiscard]] constexpr int scale() const;

    private:

        template <class FLOAT_TYPE>
        [[nodiscard]] constexpr FLOAT_TYPE convert_to_floating_point() const
            requires(!is_int_placeholder_v<T>);


        T m_value;
        int m_scale;
    };

    template <typename T>
    constexpr bool is_decimal_v = mpl::is_type_instance_of_v<T, decimal>;

    template <typename T>
    concept decimal_type = is_decimal_v<T> && decimal_integer_type<typename T::integer_type>;

    template <decimal_integer_type T>
    constexpr decimal<T>::decimal()
        requires(!is_int_placeholder_v<T>)
        : m_value(0)
        , m_scale()
    {
    }

    template <decimal_integer_type T>
    constexpr decimal<T>::decimal()
        requires(is_int_placeholder_v<T>)
        : m_value()
        , m_scale()
    {
    }

    template <decimal_integer_type T>
    constexpr decimal<T>::decimal(T value, int scale)
        : m_value(value)
        , m_scale(scale)
    {
    }

    template <decimal_integer_type T>
    constexpr bool decimal<T>::operator==(const decimal& other) const
    {
        return m_value == other.m_value && m_scale == other.m_scale;
    }

    template <decimal_integer_type T>
    constexpr decimal<T>::operator float() const
        requires(!is_int_placeholder_v<T>)
    {
        return convert_to_floating_point<float>();
    }

    template <decimal_integer_type T>
    constexpr decimal<T>::operator double() const
        requires(!is_int_placeholder_v<T>)
    {
        return convert_to_floating_point<double>();
    }

    template <decimal_integer_type T>
    constexpr decimal<T>::operator long double() const
        requires(!is_int_placeholder_v<T>)
    {
        return convert_to_floating_point<long double>();
    }

    template <decimal_integer_type T>
    decimal<T>::operator std::string() const
        requires(!is_int_placeholder_v<T>)
    {
        std::stringstream ss;
        ss << m_value;
        std::string result = ss.str();
        if (m_scale == 0)
        {
            return result;
        }
        if (result[0] == '0')
        {
            return "0";
        }
        // remove -  (we handle it later)
        if (result[0] == '-')
        {
            result = result.substr(1);
        }

        if (m_scale > 0)
        {
            if (result.length() <= static_cast<std::size_t>(m_scale))
            {
                result.insert(
                    0,
                    std::string(static_cast<std::size_t>(m_scale) + 1 - result.length(), '0')
                );  // Pad with leading zeros
            }
            std::size_t int_part_len = result.length() - static_cast<std::size_t>(m_scale);
            std::string int_part = result.substr(0, int_part_len);
            std::string frac_part = result.substr(int_part_len);
            result = int_part + "." + frac_part;
        }
        else
        {
            result += std::string(static_cast<std::size_t>(-m_scale), '0');  // Append zeros
        }
        // handle negative values
        if (m_value < 0)
        {
            result.insert(0, 1, '-');
        }
        return result;
    }

    template <decimal_integer_type T>
    constexpr T decimal<T>::storage() const
    {
        return m_value;
    }

    template <decimal_integer_type T>
    constexpr int decimal<T>::scale() const
    {
        return m_scale;
    }

    template <decimal_integer_type T>
    template <class FLOAT_TYPE>
    constexpr FLOAT_TYPE decimal<T>::convert_to_floating_point() const
        requires(!is_int_placeholder_v<T>)
    {
        using to_type = FLOAT_TYPE;
        if constexpr (std::is_same_v<T, int256_t>)
        {
            // danger zone
            auto val = static_cast<int128_t>(m_value);
            return static_cast<to_type>(val) / static_cast<to_type>(std::pow(10, m_scale));
        }
        else
        {
            return static_cast<to_type>(m_value) / static_cast<to_type>(std::pow(10, m_scale));
        }
    }

}  // namespace sparrow

#if defined(__cpp_lib_format)

template <typename T>
struct std::formatter<sparrow::decimal<T>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const sparrow::decimal<T>& d, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "Decimal({}, {})", d.storage(), d.scale());
    }
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const sparrow::decimal<T>& value)
{
    os << std::format("{}", value);
    return os;
}

#endif
