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
#   if defined(SPARROW_STATIC_LIB)
#       define SPARROW_API
#   elif defined(SPARROW_EXPORTS)
#       define SPARROW_API __declspec(dllexport)
#   else
#       define SPARROW_API __declspec(dllimport)
#   endif
#else
#   define SPARROW_API __attribute__((visibility("default")))
#endif

namespace sparrow
{
    consteval bool is_apple_compiler()
    {
        return static_cast<bool>(COMPILING_WITH_APPLE_CLANG);
    }

    namespace config
    {
        inline constexpr bool enable_size_limit_runtime_check =
#if defined(SPAROW_ENABLE_SIZE_CHECKS)
#   if SPAROW_ENABLE_SIZE_CHECKS == 1
            true
#   else
            false
#   endif
#else
#   ifndef NDEBUG // if not specified, by default in debug builds, we enable these checks
            true
#   else
            false
#   endif
#endif
            ;

        inline constexpr bool enable_32bit_size_limit =
#if defined(SPAROW_ENABLE_32BIT_SIZE_LIMIT)
            true
#else
            false
#endif
            ;
    }
}
