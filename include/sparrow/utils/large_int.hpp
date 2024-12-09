#pragma once

#ifndef SPARROW_DISABLE_LARGE_INTEGER_DECIMALS

#if SPARROW_USE_BOOST_MULTIPRECISION
#include <boost/multiprecision/cpp_int.hpp>
#else

// disabe warnings -Wold-style-cast sign-conversion for clang and gcc
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wshadow"
#endif
#include <sparrow/details/3rdparty/large_integers/int128_t.hpp>
#include <sparrow/details/3rdparty/large_integers/int256_t.hpp>


#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif


#endif



namespace sparrow
{
    #if SPARROW_USE_BOOST_MULTIPRECISION
    using int128_t = boost::multiprecision::int128_t;
    using int256_t = boost::multiprecision::int256_t;
    #else 
    using int128_t = primesum::int128_t;
    using int256_t = primesum::int256_t;

    template<class T>
    requires (std::is_same_v<T, int128_t> || std::is_same_v<T, int256_t>)
    inline std::ostream& operator<<(std::ostream& stream, T n)
    {
        std::string str;

        if (n < 0)
        {
            stream << "-";
            n = -n;
        }
        while (n > 0)
        {
            str += '0' + std::int8_t(n % 10);
            n /= 10;
        }

        if (str.empty())
            str = "0";

        stream << std::string(str.rbegin(), str.rend());

        return stream;
    }
    #endif
} // namespace sparrow

#endif // SPARROW_DISABLE_LARGE_INTEGER_DECIMALS