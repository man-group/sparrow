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

#if defined(__cpp_lib_format)
#    include <format>
#    include <ostream>
#endif

#include "sparrow/utils/decimal.hpp"

namespace sparrow
{
    /**
     * @brief Reference proxy for decimal values in array layouts.
     *
     * This class provides a reference-like interface for accessing and modifying
     * decimal values stored in array layouts. It acts as a proxy that forwards
     * operations to the underlying layout while maintaining value semantics
     * similar to built-in references.
     *
     * The decimal_reference supports:
     * - Assignment operations that modify the underlying array
     * - Conversion operations to floating-point types
     * - Comparison operations with decimal values
     * - Access to decimal components (storage and scale)
     *
     * @tparam L The layout type that stores decimal values
     *
     * @pre L must provide assign(value_type, size_type) method
     * @pre L must provide value(size_type) const method returning decimal values
     * @pre L must define inner_value_type as a decimal type
     * @post Reference operations modify the underlying layout storage
     * @post Provides value semantics while maintaining reference behavior
     * @post Thread-safe for read operations, requires external synchronization for writes
     *
     * @example
     * ```cpp
     * decimal_array<int64_t> arr = ...;
     * auto ref = arr[0];                    // Get decimal reference
     * ref = decimal<int64_t>(12345, 2);     // Assign new value (123.45)
     * double val = static_cast<double>(ref); // Convert to double
     * int64_t storage = ref.storage();       // Get raw storage value
     * ```
     */
    template <class L>
    class decimal_reference
    {
    public:

        using self_type = decimal_reference<L>;
        using value_type = typename L::inner_value_type;
        using reference = typename L::inner_reference;
        using const_reference = typename L::inner_const_reference;
        using size_type = typename L::size_type;
        using difference_type = std::ptrdiff_t;

        /**
         * @brief Constructs a decimal reference for the given layout and index.
         *
         * @param layout Pointer to the layout containing the decimal data
         * @param index Index of the decimal element in the layout
         *
         * @pre layout must not be nullptr
         * @pre index must be valid within the layout bounds
         * @post Reference is bound to the specified element
         * @post Layout pointer and index are stored for future operations
         */
        constexpr decimal_reference(L* layout, size_type index) noexcept;

        constexpr decimal_reference(const decimal_reference&) = default;
        constexpr decimal_reference(decimal_reference&&) noexcept = default;

        /**
         * @brief Move assignment from another decimal reference.
         *
         * @param rhs Source decimal reference to move from
         * @return Reference to this object
         *
         * @pre rhs must be valid (pointing to valid layout and index)
         * @post This reference's target is assigned the value from rhs
         * @post rhs remains valid but may be in unspecified state
         */
        constexpr self_type& operator=(self_type&& rhs) noexcept;

        /**
         * @brief Copy assignment from another decimal reference.
         *
         * @param rhs Source decimal reference to copy from
         * @return Reference to this object
         *
         * @pre rhs must be valid (pointing to valid layout and index)
         * @post This reference's target is assigned the value from rhs
         * @post rhs remains unchanged
         */
        constexpr self_type& operator=(const self_type& rhs);

        /**
         * @brief Move assignment from a decimal value.
         *
         * @param rhs Decimal value to move assign
         * @return Reference to this object
         *
         * @pre Layout must remain valid during assignment
         * @pre Index must be within layout bounds
         * @post Underlying layout element is assigned the moved value
         * @post rhs is moved from and may be in unspecified state
         */
        constexpr self_type& operator=(value_type&& rhs);

        /**
         * @brief Copy assignment from a decimal value.
         *
         * @param rhs Decimal value to copy assign
         * @return Reference to this object
         *
         * @pre Layout must remain valid during assignment
         * @pre Index must be within layout bounds
         * @post Underlying layout element is assigned the copied value
         * @post rhs remains unchanged
         */
        constexpr self_type& operator=(const value_type& rhs);

        /**
         * @brief Conversion to float.
         *
         * Converts the referenced decimal value to a floating-point representation.
         * May lose precision for large values or high-precision decimals.
         *
         * @return Float approximation of the decimal value
         *
         * @pre value_type::integer_type must not be a placeholder type
         * @post Return value approximates the decimal's mathematical value
         * @post May lose precision due to float's limited precision
         */
        constexpr explicit operator float() const
            requires(!is_int_placeholder_v<typename value_type::integer_type>);

        /**
         * @brief Conversion to double.
         *
         * Converts the referenced decimal value to a double-precision floating-point
         * representation. Provides better precision than float conversion.
         *
         * @return Double approximation of the decimal value
         *
         * @pre value_type::integer_type must not be a placeholder type
         * @post Return value approximates the decimal's mathematical value
         * @post Better precision than float conversion
         */
        constexpr explicit operator double() const
            requires(!is_int_placeholder_v<typename value_type::integer_type>);

        /**
         * @brief Conversion to long double.
         *
         * Converts the referenced decimal value to a long double floating-point
         * representation. Provides the highest precision among floating-point conversions.
         *
         * @return Long double approximation of the decimal value
         *
         * @pre value_type::integer_type must not be a placeholder type
         * @post Return value approximates the decimal's mathematical value
         * @post Highest precision among floating-point conversions
         */
        constexpr explicit operator long double() const
            requires(!is_int_placeholder_v<typename value_type::integer_type>);

        /**
         * @brief Conversion to string representation.
         *
         * Converts the referenced decimal value to its human-readable string
         * representation with proper decimal point placement.
         *
         * @return String representation of the decimal value
         *
         * @pre value_type::integer_type must not be a placeholder type
         * @post Returns exact string representation of the decimal
         * @post Format matches decimal's string conversion rules
         */
        [[nodiscard]] explicit operator std::string() const
            requires(!is_int_placeholder_v<typename value_type::integer_type>);

        /**
         * @brief Equality comparison with decimal value.
         *
         * @param rhs Decimal value to compare with
         * @return true if the referenced decimal equals rhs, false otherwise
         *
         * @post Comparison includes both storage value and scale
         * @post Uses decimal's equality operator semantics
         */
        constexpr bool operator==(const value_type& rhs) const;

        /**
         * @brief Three-way comparison with decimal value.
         *
         * @param rhs Decimal value to compare with
         * @return Ordering result of the comparison
         *
         * @post Comparison includes both storage value and scale
         * @post Uses decimal's comparison operator semantics
         */
        constexpr auto operator<=>(const value_type& rhs) const;

        /**
         * @brief Gets the referenced decimal value.
         *
         * @return Const reference to the decimal value in the layout
         *
         * @pre Layout and index must still be valid
         * @post Returns reference to the actual decimal stored in layout
         * @post Reference remains valid while layout is unchanged
         */
        [[nodiscard]] constexpr const_reference value() const;

        /**
         * @brief Gets the raw storage value of the decimal.
         *
         * @return The integer storage value (coefficient) of the decimal
         *
         * @post Returns the raw integer value before scale application
         * @post Equivalent to value().storage()
         */
        [[nodiscard]] constexpr value_type::integer_type storage() const;

        /**
         * @brief Gets the scale of the decimal.
         *
         * @return The scale factor of the decimal
         *
         * @post Returns the decimal scale (power of 10 divisor)
         * @post Equivalent to value().scale()
         */
        [[nodiscard]] constexpr int scale() const;

    private:

        L* p_layout = nullptr;             ///< Pointer to the layout containing the data
        size_type m_index = size_type(0);  ///< Index of the element in the layout
    };

    /*************************************
     * decimal_reference implementation *
     *************************************/

    template <typename L>
    constexpr decimal_reference<L>::decimal_reference(L* layout, size_type index) noexcept
        : p_layout(layout)
        , m_index(index)
    {
    }

    template <typename L>
    constexpr auto decimal_reference<L>::operator=(value_type&& rhs) -> self_type&
    {
        p_layout->assign(std::forward<value_type>(rhs), m_index);
        return *this;
    }

    template <typename L>
    constexpr auto decimal_reference<L>::operator=(const value_type& rhs) -> self_type&
    {
        p_layout->assign(rhs, m_index);
        return *this;
    }

    template <typename L>
    constexpr auto decimal_reference<L>::operator=(self_type&& rhs) noexcept -> self_type&
    {
        this->operator=(rhs.value());
        return *this;
    }

    template <typename L>
    constexpr auto decimal_reference<L>::operator=(const self_type& rhs) -> self_type&
    {
        this->operator=(rhs.value());
        return *this;
    }

    template <typename L>
    constexpr bool decimal_reference<L>::operator==(const value_type& rhs) const
    {
        return value() == rhs;
    }

    template <typename L>
    constexpr auto decimal_reference<L>::operator<=>(const value_type& rhs) const
    {
        return value() <=> rhs;
    }

    template <typename L>
    constexpr auto decimal_reference<L>::value() const -> const_reference
    {
        return static_cast<const L*>(p_layout)->value(m_index);
    }

    template <typename L>
    constexpr auto decimal_reference<L>::storage() const -> value_type::integer_type
    {
        return value().storage();
    }

    template <typename L>
    constexpr int decimal_reference<L>::scale() const
    {
        return value().scale();
    }

    template <typename L>
    constexpr decimal_reference<L>::operator float() const
        requires(!is_int_placeholder_v<typename value_type::integer_type>)
    {
        return static_cast<float>(value());
    }

    template <typename L>
    constexpr decimal_reference<L>::operator double() const
        requires(!is_int_placeholder_v<typename value_type::integer_type>)
    {
        return static_cast<double>(value());
    }

    template <typename L>
    constexpr decimal_reference<L>::operator long double() const
        requires(!is_int_placeholder_v<typename value_type::integer_type>)
    {
        return static_cast<long double>(value());
    }
}

#if defined(__cpp_lib_format)


template <typename L>
struct std::formatter<sparrow::decimal_reference<L>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::decimal_reference<L>& ref, std::format_context& ctx) const
    {
        const auto& value = ref.value();
        return std::format_to(ctx.out(), "{}", value);
    }
};

namespace sparrow
{
    template <typename L>
    std::ostream& operator<<(std::ostream& os, const decimal_reference<L>& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

#endif
