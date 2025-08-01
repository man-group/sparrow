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
    /**
     * @brief Concept for valid decimal integer backing types.
     *
     * Defines the integer types that can be used as the underlying storage
     * for decimal values. Supports both standard integer types and extended
     * precision integer types.
     */
    template <typename T>
    concept decimal_integer_type = std::is_same_v<T, std::int32_t> || std::is_same_v<T, std::int64_t>
                                   || std::is_same_v<T, int128_t> || std::is_same_v<T, int256_t>;

    /**
     * @brief Fixed-point decimal number representation with arbitrary precision.
     *
     * The decimal class provides a fixed-point representation of decimal numbers
     * using an integer value and a scale factor. The actual decimal value is
     * computed as: value * 10^(-scale).
     *
     * This representation is particularly useful for financial calculations and
     * data science applications where exact decimal representation is required
     * to avoid floating-point precision errors.
     *
     * @tparam T The underlying integer type for storing the decimal value
     *
     * @pre T must satisfy decimal_integer_type concept
     * @post Maintains exact decimal representation without floating-point errors
     * @post Thread-safe for read operations, requires external synchronization for writes
     * @post Supports conversion to floating-point types with appropriate precision loss warnings
     *
     * @example
     * ```cpp
     * // Represent 123.45 as decimal with scale 2
     * decimal<int64_t> d1(12345, 2);  // 12345 * 10^(-2) = 123.45
     *
     * // Represent 1000 as decimal with negative scale
     * decimal<int64_t> d2(1, -3);     // 1 * 10^(-(-3)) = 1000
     *
     * // Convert to floating-point
     * double value = static_cast<double>(d1);  // 123.45
     *
     * // Convert to string representation
     * std::string str = static_cast<std::string>(d1);  // "123.45"
     * ```
     */
    template <decimal_integer_type T>
    class decimal
    {
    public:

        using integer_type = T;

        /**
         * @brief Default constructor for non-placeholder integer types.
         *
         * Creates a decimal representing zero with scale 0.
         *
         * @pre T must not be an integer placeholder type
         * @post storage() returns 0
         * @post scale() returns 0
         * @post Decimal represents the value 0.0
         */
        constexpr decimal()
            requires(!is_int_placeholder_v<T>);

        /**
         * @brief Default constructor for placeholder integer types.
         *
         * Creates a decimal with default-initialized storage and scale.
         * Used for placeholder types that don't have well-defined zero values.
         *
         * @pre T must be an integer placeholder type
         * @post storage() returns default-constructed T
         * @post scale() returns default-constructed int (typically 0)
         */
        constexpr decimal()
            requires(is_int_placeholder_v<T>);

        /**
         * @brief Constructor from value and scale.
         *
         * Creates a decimal representing the value: value * 10^(-scale).
         *
         * @param value The integer coefficient of the decimal number
         * @param scale The decimal scale (number of digits after decimal point)
         *
         * @post storage() returns value
         * @post scale() returns scale
         * @post Decimal represents the value: value * 10^(-scale)
         */
        constexpr decimal(T value, int scale);

        /**
         * @brief Equality comparison operator.
         *
         * Two decimals are equal if they have the same storage value and scale.
         * Note: This does not perform mathematical equality (e.g., 1.0 vs 10.00
         * with different scales would not be considered equal).
         *
         * @param other Decimal to compare with
         * @return true if both storage and scale are equal, false otherwise
         *
         * @post Return value is true iff (storage() == other.storage() && scale() == other.scale())
         * @post Does not normalize values before comparison
         */
        constexpr bool operator==(const decimal& other) const;

        /**
         * @brief Conversion to float.
         *
         * Converts the decimal to a floating-point value by computing
         * storage() / 10^scale(). May lose precision for large values or
         * high precision decimals.
         *
         * @return Floating-point approximation of the decimal value
         *
         * @pre T must not be an integer placeholder type
         * @post Return value approximates storage() * 10^(-scale())
         * @post May lose precision due to float's limited precision
         *
         * @note For int256_t, value is first cast to int128_t to avoid overflow
         */
        constexpr explicit operator float() const
            requires(!is_int_placeholder_v<T>);

        /**
         * @brief Conversion to double.
         *
         * Converts the decimal to a double-precision floating-point value.
         * Provides better precision than float conversion but may still lose
         * precision for very large or high-precision decimals.
         *
         * @return Double-precision approximation of the decimal value
         *
         * @pre T must not be an integer placeholder type
         * @post Return value approximates storage() * 10^(-scale())
         * @post Better precision than float conversion
         *
         * @note For int256_t, value is first cast to int128_t to avoid overflow
         */
        constexpr explicit operator double() const
            requires(!is_int_placeholder_v<T>);

        /**
         * @brief Conversion to long double.
         *
         * Converts the decimal to a long double floating-point value.
         * Provides the highest precision among floating-point conversions
         * but may still lose precision for extremely large or high-precision decimals.
         *
         * @return Long double approximation of the decimal value
         *
         * @pre T must not be an integer placeholder type
         * @post Return value approximates storage() * 10^(-scale())
         * @post Highest precision among floating-point conversions
         *
         * @note For int256_t, value is first cast to int128_t to avoid overflow
         */
        constexpr explicit operator long double() const
            requires(!is_int_placeholder_v<T>);

        /**
         * @brief Conversion to string representation.
         *
         * Converts the decimal to its human-readable string representation
         * with proper decimal point placement and sign handling.
         *
         * @return String representation of the decimal value
         *
         * @pre T must not be an integer placeholder type
         * @post Returns exact string representation of the decimal value
         * @post Positive scale adds decimal point and fractional digits
         * @post Negative scale appends trailing zeros
         * @post Negative values are prefixed with '-'
         * @post Zero values return "0" regardless of scale
         */
        [[nodiscard]] explicit operator std::string() const
            requires(!is_int_placeholder_v<T>);

        /**
         * @brief Gets the raw storage value.
         *
         * @return The underlying integer value used for storage
         *
         * @post Returns the exact value passed to constructor or default value
         */
        [[nodiscard]] constexpr T storage() const;

        /**
         * @brief Gets the decimal scale.
         *
         * @return The scale factor used to interpret the storage value
         *
         * @post Returns the exact scale passed to constructor or default scale
         * @post Positive scale indicates fractional digits
         * @post Negative scale indicates trailing zeros
         */
        [[nodiscard]] constexpr int scale() const;

    private:

        /**
         * @brief Template helper for floating-point conversions.
         *
         * Implements the common logic for converting decimal to floating-point
         * types by dividing the storage value by 10^scale.
         *
         * @tparam FLOAT_TYPE Target floating-point type
         * @return Converted floating-point value
         *
         * @pre T must not be an integer placeholder type
         * @pre FLOAT_TYPE must be a floating-point type
         * @post Return value approximates storage() * 10^(-scale())
         *
         * @note Special handling for int256_t to prevent overflow
         */
        template <class FLOAT_TYPE>
        [[nodiscard]] constexpr FLOAT_TYPE convert_to_floating_point() const
            requires(!is_int_placeholder_v<T>);

        T m_value;    ///< The integer coefficient of the decimal number
        int m_scale;  ///< The decimal scale (10^(-scale) multiplier)
    };

    /**
     * @brief Type trait to check if a type is a decimal instantiation.
     *
     * @tparam T Type to check
     */
    template <typename T>
    constexpr bool is_decimal_v = mpl::is_type_instance_of_v<T, decimal>;

    /**
     * @brief Concept for valid decimal types.
     *
     * A type satisfies decimal_type if it's a decimal instantiation with
     * a valid decimal integer backing type.
     *
     * @tparam T Type to check
     */
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

namespace sparrow
{
    template <typename T>
    std::ostream& operator<<(std::ostream& os, const decimal<T>& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

#endif
