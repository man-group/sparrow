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

#if defined(__apple_build_version__)
#    define COMPILING_WITH_APPLE_CLANG 1
#else
#    define COMPILING_WITH_APPLE_CLANG 0
#endif

#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 170000
#    define USING_LIBCPP_PRE_17 1
#else
#    define USING_LIBCPP_PRE_17 0
#endif

#if defined(_WIN32)
#    if defined(SPARROW_STATIC_LIB)
#        define SPARROW_API
#    elif defined(SPARROW_EXPORTS)
#        define SPARROW_API __declspec(dllexport)
#    else
#        define SPARROW_API __declspec(dllimport)
#    endif
#else
#    define SPARROW_API __attribute__((visibility("default")))
#endif

[[nodiscard]] consteval bool is_apple_compiler()
{
    return static_cast<bool>(COMPILING_WITH_APPLE_CLANG);
}

// If using gcc versionn < 12, we define the constexpr keyword to be empty.
#if defined(__GNUC__) && __GNUC__ < 12
#    define SPARROW_CONSTEXPR_GCC_11 inline
#else
#    define SPARROW_CONSTEXPR_GCC_11 constexpr
#endif

#if (!defined(__clang__) && defined(__GNUC__))
#    if (__GNUC__ < 12 && __GNUC_MINOR__ < 3)
#        define SPARROW_GCC_11_2_WORKAROUND 1
#    endif
#endif

// If using clang or apple-clang version < 18, we define the constexpr keyword to be empty.
#if defined(__clang__) && __clang_major__ < 18
#    define SPARROW_CONSTEXPR_CLANG_17 inline
#else
#    define SPARROW_CONSTEXPR_CLANG_17 constexpr
#endif
