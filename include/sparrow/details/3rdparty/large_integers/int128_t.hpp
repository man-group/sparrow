////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SPARROW: THIS HAS BEEN COPY-PASTED FROM https://github.com/kimwalisch/primesum/blob/master/include/int128_t.hpp
// PLEASE UPDATE THIS COMMENT IF YOU REPLACE THIS
// SEE README.md for rational
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Modification from the original:
// * disabled io operators
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// @file   int128_t.hpp
/// @brief  Support for int128_t, uint128_t types.
///
/// Copyright (C) 2018 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#ifndef INT128_T_HPP
#define INT128_T_HPP

#include <sstream>
#include <stdint.h>
#include <limits>
#include <type_traits>

/// The __int128_t type (GCC/Clang) is not well supported by
/// the C++ standard library (in 2016) so we have to define
/// some functions ourselves. We also define typedefs so we
/// can use int128_t instead of __int128_t. Once this is done
/// int128_t can be used like a regular integer type.
///
#if !defined(INT128_MAX) && \
     defined(__SIZEOF_INT128__)

#include <ostream>
#include <string>

namespace primesum {

using int128_t = __int128_t;
using uint128_t = __uint128_t;

} // namespace

#endif

inline std::ostream& operator<<(std::ostream& stream, primesum::uint128_t n)
{
  std::string str;

  while (n > 0)
  {
    str += '0' + char(n % 10);
    n /= 10;
  }
  if (str.empty())
  {
    str = "0";
  }

  stream << std::string(str.rbegin(), str.rend());
  return stream;
}

inline std::ostream& operator<<(std::ostream& stream, primesum::int128_t n)
{
  if (n < 0)
  {
    stream << "-";
    n = -n;
  }
  stream << (primesum::uint128_t) n;
  return stream;
}

namespace primesum {

inline std::string to_string(const uint128_t& n)
{
  std::ostringstream oss;
  oss << n;
  return oss.str();
}

inline std::string to_string(const int128_t& n)
{
  std::ostringstream oss;
  oss << n;
  return oss.str();
}

/// Portable namespace, includes functions which (unlike the versions
/// form the C++ standard library) work with the int128_t and
/// uint128_t types (2014).
///
namespace prt {

template <typename T>
struct numeric_limits
{
  static constexpr T max()
  {
    return std::numeric_limits<T>::max();
  }
};

template <>
struct numeric_limits<int128_t>
{
  static constexpr int128_t min() { return ((int128_t) 1) << 127; }
  static constexpr int128_t max() { return ~min(); }
};

template <>
struct numeric_limits<uint128_t>
{
  static constexpr uint128_t min() { return 0; }
  static constexpr uint128_t max() { return ~min(); }
};

template <typename T>
struct is_integral
{
  enum
  {
    value = std::is_integral<T>::value ||
            std::is_same<T, int128_t>::value ||
            std::is_same<T, uint128_t>::value
  };
};

template <typename T>
struct is_signed
{
  enum
  {
    value = std::is_signed<T>::value ||
            std::is_same<T, int128_t>::value
  };
};

template <typename T>
struct is_unsigned
{
  enum
  {
    value = std::is_unsigned<T>::value ||
            std::is_same<T, uint128_t>::value
  };
};

} // namespace prt
} // namespace primesum

#endif