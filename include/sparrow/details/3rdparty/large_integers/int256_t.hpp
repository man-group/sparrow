////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SPARROW: THIS HAS BEEN COPY-PASTED FROM https://github.com/kimwalisch/primesum/blob/master/include/int256_t.hpp
// PLEASE UPDATE THIS COMMENT IF YOU REPLACE THIS
// SEE README.md for rational
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Modification from the original:
// - disabled next_larger_type
// - disabled OpenMP support
// - disabled io operators (abigiousty...)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   int256_t.hpp
/// @brief  256-bit signed integer type based on Andrew G. Crowell's
///         128-bit signed integer type:
///         https://gist.github.com/Bananattack/6242ba7b8265c90ce6f3c2d84670c52d
///
/// Copyright (C) 2017 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#ifndef INT256_T_HPP
#define INT256_T_HPP

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <ostream>
#include <limits>
#include <type_traits>
#include <utility>

#include "int128_t.hpp"

namespace primesum {

class int256_t
{
public:
    int256_t()
        : low(0)
        , high(0)
    {
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    int256_t(T x)
        : low(x),
          high((x < 0) ? -1 : 0)
    {
    }

    bool operator==(const int256_t& other) const
    {
        return low == other.low &&
               high == other.high;
    }

    bool operator!=(const int256_t& other) const
    {
        return !(*this == other);
    }

    bool operator<(const int256_t& other) const
    {
        int128_t hi1 = high;
        int128_t hi2 = other.high;

        return hi1 < hi2 || (hi1 == hi2 && low < other.low);
    }

    bool operator<=(const int256_t& other) const
    {
        return !(other < *this);
    }

    bool operator>(const int256_t& other) const
    {
        return other < *this;
    }

    bool operator>=(const int256_t& other) const
    {
        return !(*this < other);
    }

    int256_t operator+() const
    {
        return *this;
    }

    int256_t operator-() const
    {
        int256_t res = ~*this;
        ++res;
        return res;
    }

    int256_t operator~() const
    {
        return int256_t(~low, ~high);
    }

    int256_t& operator--()
    {
        if (low == 0)
            high--;

        low--;
        return *this;
    }

    int256_t& operator++()
    {
        low++;
        if (low == 0)
            high++;

        return *this;
    }

    int256_t operator--(int)
    {
        int256_t res = *this;
        --*this;
        return res;
    }

    int256_t operator++(int)
    {
        int256_t res = *this;
        ++*this;
        return res;
    }

    int256_t operator+(const int256_t& other) const
    {
        int256_t res;

        res.high = high + other.high;
        res.low = low + other.low;

        if (res.low < other.low)
          res.high++;

        return res;
    }

    int256_t operator-(const int256_t& other) const
    {
        int256_t res;

        res.high = high - other.high;
        res.low = low - other.low;

        if (res.low > low)
          res.high--;

        return res;
    }

    int256_t operator*(const int256_t& other) const
    {
        auto max64 = prt::numeric_limits<std::uint64_t>::max();

        if (low <= max64 &&
            other.low <= max64 &&
            ((high == 0 || ~high == 0) &&
             (other.high == 0 || ~other.high == 0)))
        {
          return int256_t(low * other.low,
                          (high == other.high) ? 0 : -1);
        }
        else
        {
            auto al = low & max64;
            auto ah = low >> 64;
            auto bl = other.low & max64;
            auto bh = other.low >> 64;

            auto x = al * bl;
            auto y = al * bh;
            auto z = ah * bl;
            auto w = ah * bh;

            return int256_t(y << 64, y >> 64) +
                   int256_t(z << 64, z >> 64) +
                   int256_t(x, w + low * other.high + high * other.low);
        }
    }

    int256_t operator/(const int256_t& other) const
    {
        return div256(other).first;
    }

    int256_t operator%(const int256_t& other) const
    {
        return div256(other).second;
    }

    int256_t operator&(const int256_t& other) const
    {
        return int256_t(low & other.low,
                        high & other.high);
    }

    int256_t operator|(const int256_t& other) const
    {
        return int256_t(low | other.low,
                        high | other.high);
    }

    int256_t operator^(const int256_t& other) const
    {
        return int256_t(low ^ other.low,
                        high ^ other.high);
    }

    int256_t operator<<(std::size_t bits) const
    {
        if (bits >= 128)
            return int256_t(0, low << (bits - 128));
        else
            return int256_t(low << bits, (high << bits) | (low >> (128 - bits)));
    }

    int256_t operator>>(std::size_t bits) const
    {
        if (bits >= 128)
            return int256_t(high >> (bits - 128), 0);
        else
            return int256_t((low >> bits) | (high << (128 - bits)), high >> bits);
    }

    int256_t& operator+=(const int256_t& other)
    {
        *this = *this + other;
        return *this;
    }

    int256_t& operator-=(const int256_t& other)
    {
        *this = *this - other;
        return *this;
    }

    int256_t& operator*=(const int256_t& other)
    {
        *this = *this * other;
        return *this;
    }

    int256_t& operator/=(const int256_t& other)
    {
        *this = *this / other;
        return *this;
    }

    int256_t& operator%=(const int256_t& other)
    {
        *this = *this % other;
        return *this;
    }

    int256_t& operator&=(const int256_t& other)
    {
        *this = *this & other;
        return *this;
    }

    int256_t& operator|=(const int256_t& other)
    {
        *this = *this | other;
        return *this;
    }

    int256_t& operator^=(const int256_t& other)
    {
        *this = *this ^ other;
        return *this;
    }

    int256_t& operator<<=(std::size_t bits)
    {
        *this = *this << bits;
        return *this;
    }

    int256_t& operator>>=(std::size_t bits)
    {
        *this = *this >> bits;
        return *this;
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    bool operator==(T x) const
    {
        return *this == int256_t(x);
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    bool operator!=(T x) const
    {
        return *this != int256_t(x);
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    bool operator<(T x) const
    {
        return *this < int256_t(x);
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    bool operator<=(T x) const
    {
        return *this <= int256_t(x);
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    bool operator>(T x) const
    {
        return *this > int256_t(x);
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    bool operator>=(T x) const
    {
        return *this >= int256_t(x);
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    int256_t operator + (T x) const
    {
        return *this + int256_t(x);
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    int256_t operator - (T x) const
    {
        return *this - int256_t(x);
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    int256_t operator * (T x) const
    {
        return *this * int256_t(x);
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    int256_t operator / (T x) const
    {
        return *this / int256_t(x);
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    int256_t operator % (T x) const
    {
        return *this % int256_t(x);
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    int256_t operator & (T x) const
    {
        return *this & int256_t(x);
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    int256_t operator | (T x) const
    {
        return *this | int256_t(x);
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    int256_t operator ^ (T x) const
    {
        return *this ^ int256_t(x);
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    int256_t operator << (T x) const
    {
        return *this << static_cast<std::size_t>(x);
    }

    template <typename T,
              typename = typename std::enable_if<prt::is_integral<T>::value>::type>
    int256_t operator >> (T x) const
    {
        return *this >> static_cast<std::size_t>(x);
    }

    explicit operator std::int8_t() const
    {
        return (*this < 0)
            ? -static_cast<std::int8_t>(
                  (low - 1) ^ prt::numeric_limits<uint128_t>::max())
            : static_cast<std::int8_t>(low);
    }

    explicit operator std::int16_t() const
    {
        return (*this < 0)
            ? -static_cast<std::int16_t>(
                  (low - 1) ^ prt::numeric_limits<uint128_t>::max())
            : static_cast<std::int16_t>(low);
    }

    explicit operator std::int32_t() const
    {
        return (*this < 0)
            ? -static_cast<std::int32_t>(
                  (low - 1) ^ prt::numeric_limits<uint128_t>::max())
            : static_cast<std::int32_t>(low);
    }

    explicit operator std::int64_t() const
    {
        return (*this < 0)
            ? -static_cast<std::int64_t>(
                  (low - 1) ^ prt::numeric_limits<uint128_t>::max())
            : static_cast<std::int64_t>(low);
    }

    explicit operator int128_t() const
    {
        return (*this < 0)
            ? -static_cast<int128_t>(
                  (low - 1) ^ prt::numeric_limits<uint128_t>::max())
            : static_cast<int128_t>(low);
    }

    explicit operator std::uint8_t() const
    {
        return static_cast<std::uint8_t>(low);
    }

    explicit operator std::uint16_t() const
    {
        return static_cast<std::uint16_t>(low);
    }

    explicit operator std::uint32_t() const
    {
        return static_cast<std::uint32_t>(low);
    }

    explicit operator std::uint64_t() const
    {
        return static_cast<std::uint64_t>(low);
    }

    explicit operator uint128_t() const
    {
        return static_cast<uint128_t>(low);
    }

    //friend std::ostream& operator<<(std::ostream& out, int256_t n);

private:
    uint128_t low;
    uint128_t high;

    int256_t(uint128_t low, uint128_t high)
        : low(low)
        , high(high)
    {
    }

    static int256_t min_value()
    {
        return int256_t(0, prt::numeric_limits<int128_t>::min());
    }
  
    static int256_t max_value()
    {
        return int256_t(prt::numeric_limits<uint128_t>::max(),
                        prt::numeric_limits<int128_t>::max());
    }

    bool get_bit(std::size_t bit) const
    {
        if (bit >= 128)
            return (high >> (bit - 128)) & 1;
        else
            return (low >> bit) & 1;
    }

    void set_bit(std::size_t bit, bool value)
    {
        if (bit >= 256)
            return;

        if (bit >= 128)
        {
            uint128_t mask = static_cast<uint128_t>(1) << (bit - 128);

            if (value)
                high |= mask;
            else
                high &= ~mask;
        }
        else
        {
            uint128_t mask = static_cast<uint128_t>(1) << bit;

            if (value)
                low |= mask;
            else
                low &= ~mask;
        }
    }

    int find_most_significant_bit() const
    {
        auto x = *this;
        int pos = 0;

        while (x != 0)
        {
            pos++;
            x >>= 1;
        }

        return pos;
    }

    /// Unsigned division with remainder
    std::pair<int256_t, int256_t> udiv256(const int256_t& other) const
    {
        int256_t zero = 0;
        int256_t one = 1;

        if (other == 0)
        {
            assert(other != 0);
            std::abort();
            return { zero, zero };
        }
        else if (other == 1)
        {
            return { *this, zero };
        }
        else if (*this == other)
        {
            return { one, zero };
        }
        else if (*this == 0 || (*this != min_value() && *this < other))
        {
            return { zero, *this };
        }
        else if (high == 0 && other.high == 0)
        {
            return { int256_t(low / other.low, 0),
                     int256_t(low % other.low, 0) };
        }
        else
        {
            int256_t quotient = 0;
            int256_t remainder = 0;

            for (int i = find_most_significant_bit(); i >= 0 && i <= 256; i--)
            {
                remainder <<= 1;
                remainder.set_bit(0, get_bit(i));

                if (remainder >= other)
                {
                    remainder -= other;
                    quotient.set_bit(i, true);
                }
            }
            return { quotient, remainder };
        }
    }

    /// Signed division with remainder
    std::pair<int256_t, int256_t> div256(const int256_t& other) const
    {
        if (*this < 0)
        {
            auto x = -*this;

            if (other < 0)
            {
                auto res = x.udiv256(-other);
                return { res.first, -res.second };
            }
            else
            {
                auto res = x.udiv256(other);
                return { -res.first, -res.second };
            }
        }
        else
        {
            if (other < 0)
            {
                auto res = udiv256(-other);
                return { -res.first, res.second };
            }
            else
                return udiv256(other);
        }
    }
};

// inline std::ostream& operator<<(std::ostream& stream, int256_t n)
// {   
//     std::string str;

//     if (n < 0)
//     {
//         stream << "-";
//         n = -n;
//     }

//     while (n > 0)
//     {
//         str += '0' + std::int8_t(n % 10);
//         n /= 10;
//     }

//     if (str.empty())
//         str = "0";

//     stream << std::string(str.rbegin(), str.rend());

//     return stream;
// }

// template <typename T>
// struct next_larger_type
// {
//   typedef typename std::conditional<std::is_same<T, int64_t>::value, int128_t,
//           typename std::conditional<std::is_same<T, uint64_t>::value, uint128_t,
//           typename std::conditional<std::is_same<T, int128_t>::value, int256_t,
//           typename std::conditional<std::is_same<T, uint128_t>::value, int256_t,
//           T>::type>::type>::type>::type type;
// };

} // namespace

// #ifdef _OPENMP

// namespace primesum {

// /// Requires OpenMP >= 4.0
// #pragma omp declare reduction(+ : int256_t : omp_out += omp_in)

// } // namespace

// #endif

#endif