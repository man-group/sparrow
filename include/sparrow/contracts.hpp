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

// NOTE: This is based upon v0.3.0 of NoContracts library https://github.com/Klaim/nocontracts/tree/0ffc183f7527213e3f0a57b8dae9befa7c0ca02c
// Modifications: renamed macros

#pragma once
#include <cstdio>
#include <csignal>


///////////////////////////////////////////////////////////////////
// Possible bits used to compose the behavior:

// if not specified by the user but available, use std::print
#if not defined(SPARROW_CONTRACTS_USE_STD_PRINT) and not defined(SPARROW_CONTRACTS_USE_STD_FORMAT)
# if __cplusplus >= 202002L
#  include <version>
#  ifdef __cpp_lib_print
#   define SPARROW_CONTRACTS_USE_STD_PRINT 1
#  endif
# endif
#else
// The option is defined, but is it supported? If not we fail.
#  if defined(SPARROW_CONTRACTS_USE_STD_PRINT) and not defined(__cpp_lib_print)
#    error "std::print usage is requested but not available"
#  endif
#endif

// if not specified by the user but available and std::print is not already forced, use std::format
#if not defined(SPARROW_CONTRACTS_USE_STD_FORMAT) and not defined(SPARROW_CONTRACTS_USE_STD_PRINT)
# if __cplusplus >= 202002L
#  include <version>
#  ifdef __cpp_lib_format
#   define SPARROW_CONTRACTS_USE_STD_FORMAT 1
#  endif
# endif
// The option is defined, but is it supported? If not we fail.
#  if defined(SPARROW_CONTRACTS_USE_STD_FORMAT) and not defined(__cpp_lib_format)
#    error "std::format usage is requested but not available"
#  endif
#endif

// user specified to use neither std::format nor std::print, but C's formatting
#if defined(SPARROW_CONTRACTS_USE_CFORMAT) && SPARROW_CONTRACTS_USE_CFORMAT == 1
# ifdef SPARROW_CONTRACTS_USE_STD_FORMAT
#   undef SPARROW_CONTRACTS_USE_STD_FORMAT
# endif
# ifdef SPARROW_CONTRACTS_USE_STD_PRINT
#   undef SPARROW_CONTRACTS_USE_STD_PRINT
# endif
#endif

#if defined(__GNUC__)
#define SPARROW_CONTRACTS_IGNORE_WARNINGS \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wall\"")
    _Pragma("GCC diagnostic ignored \"-Wformat-security\"")
#define SPARROW_CONTRACTS_RESTORE_WARNINGS \
    _Pragma("GCC diagnostic pop")
#elif defined(__clang__)
#define SPARROW_CONTRACTS_IGNORE_WARNINGS \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Weverything\"")
#define SPARROW_CONTRACTS_RESTORE_WARNINGS \
    _Pragma("clang diagnostic pop")
#else
#define SPARROW_CONTRACTS_IGNORE_WARNINGS
#define SPARROW_CONTRACTS_RESTORE_WARNINGS
#endif

#ifndef SPARROW_CONTRACTS_LOG_FAILURE
# if defined(SPARROW_CONTRACTS_USE_STD_PRINT) && SPARROW_CONTRACTS_USE_STD_PRINT == 1
#   include <print>
#   include <cstdio>
#   define SPARROW_CONTRACTS_LOG_FAILURE( expr__, message__ ) \
      ::std::print(stderr, "Assertion Failed ({}:{}): {} - ({} is wrong)\n", __FILE__, __LINE__, message__, #expr__ );
# elif defined(SPARROW_CONTRACTS_USE_STD_FORMAT) && SPARROW_CONTRACTS_USE_STD_FORMAT == 1
#   include <format>
#   include <cstdio>
#   define SPARROW_CONTRACTS_LOG_FAILURE( expr__, message__ ) \
      do { \
            SPARROW_CONTRACTS_IGNORE_WARNINGS; \
            ::fprintf(stderr, ::std::format("Assertion Failed ({}:{}): {} - ({} is wrong)\n", __FILE__, __LINE__, message__, #expr__ ).c_str()); \
            SPARROW_CONTRACTS_RESTORE_WARNINGS; \
      } while (0);
# else
#   include <cstdlib>
#   include <cstdio>
#   define SPARROW_CONTRACTS_LOG_FAILURE( expr__, message__ ) \
      ::fprintf(stderr, "Assertion Failed (%s:%i): %s - (%s is wrong)\n", __FILE__, __LINE__, message__, #expr__ );
# endif
#endif

#ifndef SPARROW_CONTRACTS_ABORT
# define SPARROW_CONTRACTS_ABORT() \
    std::abort()
#endif

// User specifies to just continue instead of abort on failure.
#if defined(SPARROW_CONTRACTS_CONTINUE_ON_FAILURE) and SPARROW_CONTRACTS_CONTINUE_ON_FAILURE == 1
# undef SPARROW_CONTRACTS_ABORT
# define SPARROW_CONTRACTS_ABORT
#endif

#ifndef SPARROW_CONTRACTS_DEBUGBREAK
# ifdef _WIN32
#   define SPARROW_CONTRACTS_DEBUGBREAK() __debugbreak();
# else
#   define SPARROW_CONTRACTS_DEBUGBREAK() std::raise(SIGTRAP);
# endif
#endif

///////////////////////////////////////////////////////////////////
// Default behavior:

#define SPARROW_CONTRACTS_DEFAULT_CHECKS_ENABLED 1
#define SPARROW_CONTRACTS_DEFAULT_ABORT_ON_FAILURE 1

#define SPARROW_CONTRACTS_DEFAULT_ON_FAILURE( expr__, message__ ) \
  SPARROW_CONTRACTS_LOG_FAILURE( expr__, message__ ); \
  SPARROW_CONTRACTS_DEBUGBREAK(); \
  SPARROW_CONTRACTS_ABORT();

///////////////////////////////////////////////////////////////////
// Apply Configuration:

// If not override, use the default behavior.
#ifndef SPARROW_CONTRACTS_CHECKS_ENABLED
# define SPARROW_CONTRACTS_CHECKS_ENABLED SPARROW_CONTRACTS_DEFAULT_CHECKS_ENABLED
#endif

#if SPARROW_CONTRACTS_CHECKS_ENABLED == 1 // Behavior when contracts are enabled.

# ifndef SPARROW_CONTRACTS_ON_FAILURE
#   define SPARROW_CONTRACTS_ON_FAILURE( expr__, message__ ) \
      SPARROW_CONTRACTS_DEFAULT_ON_FAILURE( expr__, message__ )
# endif

# ifndef SPARROW_ASSERT
#   define SPARROW_ASSERT( expr__, message__ ) \
      if(!( expr__ )) \
      { SPARROW_CONTRACTS_ON_FAILURE( expr__, message__ ); }
# endif

# define SPARROW_ASSERT_TRUE( expr__ ) SPARROW_ASSERT( expr__, #expr__ )
# define SPARROW_ASSERT_FALSE( expr__ ) SPARROW_ASSERT( !(expr__), "!("#expr__")" )

# else // Do nothing otherwise.
# define SPARROW_CONTRACTS_ON_FAILURE( expr__ )
# define SPARROW_ASSERT( expr__, message__ )
# define SPARROW_ASSERT_TRUE( expr__ )
# define SPARROW_ASSERT_FALSE( expr__ )
#endif

