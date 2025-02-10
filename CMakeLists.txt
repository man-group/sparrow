# Copyright 2024 Man Group Operations Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.28)

# This is better specified per target, but cmake keeps ignoring these language version
# specification when building this project by itself, in particular the gnu extensions,
# so here we go.
# This will affects all following targets being defined.
set(CMAKE_CXX_EXTENSIONS OFF)
cmake_policy(SET CMP0091 NEW)

# MSVC debug information format flags are selected via
# CMAKE_MSVC_DEBUG_INFORMATION_FORMAT, instead of
# embedding flags in e.g. CMAKE_CXX_FLAGS_RELEASE.
# New in CMake 3.25.
#
# Supports debug info with SCCache
# (https://github.com/mozilla/sccache?tab=readme-ov-file#usage)
# avoiding “fatal error C1041: cannot open program database; if
# multiple CL.EXE write to the same .PDB file, please use /FS"
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
cmake_policy(SET CMP0141 NEW)

project(sparrow CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CMakeDependentOption)

set(SPARROW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SPARROW_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(CMAKE_CXX_COMPILER_LAUNCHER)
    message(STATUS "Using C++ compiler launcher: ${CMAKE_CXX_COMPILER_LAUNCHER}")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
message(DEBUG "CMake module path: ${CMAKE_MODULE_PATH}")

# Versionning
# ===========

file(STRINGS "${SPARROW_INCLUDE_DIR}/sparrow/config/sparrow_version.hpp" sparrow_version_defines
     REGEX "constexpr int SPARROW_VERSION_(MAJOR|MINOR|PATCH)")
foreach(ver ${sparrow_version_defines})
    if(ver MATCHES "constexpr int SPARROW_VERSION_(MAJOR|MINOR|PATCH) = ([0-9]+);$")
        set(PROJECT_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif()
endforeach()
set(CMAKE_PROJECT_VERSION
    ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

message(STATUS "Building sparrow v${CMAKE_PROJECT_VERSION}")

# Binary version
# See the following URL for explanations about the binary versionning
# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html#Updating-version-info

file(STRINGS "${SPARROW_INCLUDE_DIR}/sparrow/config/sparrow_version.hpp" sparrow_version_defines
    REGEX "constexpr int SPARROW_BINARY_(CURRENT|REVISION|AGE)")
foreach(ver ${sparrow_version_defines})
    if(ver MATCHES "constexpr int SPARROW_BINARY_(CURRENT|REVISION|AGE) = ([0-9]+);$")
        set(SPARROW_BINARY_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif()
endforeach()
set(SPARROW_BINARY_VERSION
    ${SPARROW_BINARY_CURRENT}.${SPARROW_BINARY_REVISION}.${SPARROW_BINARY_AGE})

message(STATUS "sparrow binary version: v${SPARROW_BINARY_VERSION}")


# Build options
# =============

OPTION(BUILD_TESTS "Build sparrow test suite" OFF)
OPTION(BUILD_DOCS  "Build sparrow documentation" OFF)
OPTION(BUILD_EXAMPLES "Build sparrow examples" OFF)
OPTION(USE_DATE_POLYFILL "Use date polyfill implementation" ON)
OPTION(USE_LARGE_INT_PLACEHOLDERS "use types without api for big integers" OFF)
OPTION(SPARROW_TARGET_32BIT "Test 32bit support" OFF)

OPTION(ENABLE_COVERAGE "Enable test coverage" OFF)

if(ENABLE_COVERAGE)
    include(code_coverage)
endif()

include(CheckCXXSymbolExists)

if(cxx_std_20 IN_LIST CMAKE_CXX_COMPILE_FEATURES)
  set(header version)
else()
  set(header ciso646)
endif()

check_cxx_symbol_exists(_LIBCPP_VERSION ${header} LIBCPP)
if(LIBCPP)
  message(STATUS "Using libc++")
  # Allow the use of not visible yet availabile features, such
  # as some formatter for new types.
  add_compile_definitions(_LIBCPP_DISABLE_AVAILABILITY)
endif()

# Linter options
# =============

OPTION(ACTIVATE_LINTER "Create targets to run clang-format" OFF)
cmake_dependent_option(ACTIVATE_LINTER_DURING_COMPILATION "Run linter during the compilation" ON "ACTIVATE_LINTER" OFF)

if(ACTIVATE_LINTER)
    include(clang-format)
    include(clang-tidy)
endif()

# Sanitizers
# ==========
include(sanitizers)

# Dependencies
# ============

set(SPARROW_INTERFACE_DEPENDENCIES "" CACHE STRING "List of dependencies to be linked to the sparrow target")

if (USE_DATE_POLYFILL)
    find_package(date CONFIG REQUIRED)
    list(APPEND SPARROW_INTERFACE_DEPENDENCIES date::date date::date-tz)
    add_compile_definitions(SPARROW_USE_DATE_POLYFILL)
endif()

if(USE_LARGE_INT_PLACEHOLDERS)
    message(STATUS "Using large int placeholders")
    add_compile_definitions(SPARROW_USE_LARGE_INT_PLACEHOLDERS)
endif()

# Build
# =====

set(BINARY_BUILD_DIR "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BINARY_BUILD_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BINARY_BUILD_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${BINARY_BUILD_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${BINARY_BUILD_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${BINARY_BUILD_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${BINARY_BUILD_DIR}")

set(SPARROW_HEADERS
    # arrow_interface
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/arrow_array_schema_info_utils.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/arrow_array_schema_utils.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/arrow_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/arrow_array/private_data.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/arrow_flag_utils.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/arrow_schema.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/arrow_schema/private_data.hpp
    # buffer
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/allocator.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/buffer_adaptor.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/buffer_view.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/buffer.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/dynamic_bitset.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/dynamic_bitset/dynamic_bitset_base.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/dynamic_bitset/bitset_iterator.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/dynamic_bitset/bitset_reference.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/dynamic_bitset/dynamic_bitset_view.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp
    # config
    ${SPARROW_INCLUDE_DIR}/sparrow/config/config.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/config/sparrow_version.hpp
    # detail
    ${SPARROW_INCLUDE_DIR}/sparrow/details/3rdparty/float16_t.hpp
    # layout
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/array_base.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/array_helper.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/array_trivial_copyable.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/array_wrapper.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/dictionary_encoded_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/dispatch.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/fixed_width_binary_layout/fixed_width_binary_array_utils.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/fixed_width_binary_layout/fixed_width_binary_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/fixed_width_binary_layout/fixed_width_binary_reference.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/layout_iterator.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/layout_utils.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/list_layout/list_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/list_layout/list_value.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/nested_value_types.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/null_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/primitive_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/struct_layout/struct_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/struct_layout/struct_value.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/temporal/date_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/temporal/duration_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/temporal/timestamp_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/temporal/timestamp_concepts.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/temporal/timestamp_reference.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/temporal/interval_types.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/trivial_copyable_data_access.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/variable_size_binary_layout/variable_size_binary_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/variable_size_binary_layout/variable_size_binary_iterator.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/variable_size_binary_layout/variable_size_binary_reference.hpp
    # array
    ${SPARROW_INCLUDE_DIR}/sparrow/types/data_traits.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/types/data_type.hpp
    # Utils
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/bit.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/buffers.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/contracts.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/functor_index_iterator.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/iterator.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/memory.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/mp_utils.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/nullable.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/offsets.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/reference_wrapper_utils.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/variant_visitor.hpp
    # ../
    ${SPARROW_INCLUDE_DIR}/sparrow/array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/array_api.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/array_factory.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_array_schema_proxy.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/c_interface.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/record_batch.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/sparrow.hpp
)

if (SPARROW_TARGET_32BIT)
    set(SPARROW_SRC
        ${SPARROW_SOURCE_DIR}/dummy32bit.cpp
    )
else()
    set(SPARROW_SRC
        ${SPARROW_SOURCE_DIR}/array_factory.cpp
        ${SPARROW_SOURCE_DIR}/array_helper.cpp
        ${SPARROW_SOURCE_DIR}/array.cpp
        ${SPARROW_SOURCE_DIR}/arrow_array_schema_proxy.cpp
        ${SPARROW_SOURCE_DIR}/arrow_array_schema_proxy.cpp
        ${SPARROW_SOURCE_DIR}/arrow_interface/arrow_array.cpp
        ${SPARROW_SOURCE_DIR}/arrow_interface/arrow_schema.cpp
        ${SPARROW_SOURCE_DIR}/layout/fixed_width_binary_array/fixed_width_binary_array_utils.cpp
        ${SPARROW_SOURCE_DIR}/layout/run_end_encoded_array.cpp
        ${SPARROW_SOURCE_DIR}/list_value.cpp
        ${SPARROW_SOURCE_DIR}/record_batch.cpp
        ${SPARROW_SOURCE_DIR}/struct_value.cpp
        ${SPARROW_SOURCE_DIR}/types/data_type.cpp
    )
endif()

add_library(sparrow SHARED ${SPARROW_HEADERS} ${SPARROW_SRC})

# TODO: handle static lib
if (UNIX)
    # CMake does not compute the version number of so files as libtool
    # does on Linux. Strictly speaking, we should exclude FreeBSD and
    # Apple from this, but that would require having different version
    # numbers depending on the platform. We prefer to follow the
    # libtool pattern everywhere.
    math(EXPR SPARROW_BINARY_COMPATIBLE "${SPARROW_BINARY_CURRENT} - ${SPARROW_BINARY_AGE}")
    set_target_properties(
        sparrow
        PROPERTIES
        VERSION "${SPARROW_BINARY_COMPATIBLE}.${SPARROW_BINARY_REVISION}.${SPARROW_BINARY_AGE}"
        SOVERSION ${SPARROW_BINARY_COMPATIBLE}
        COMPILE_OPTIONS "-fvisibility=hidden"
    )
else ()
    set_target_properties(
        sparrow
        PROPERTIES
        VERSION ${SPARROW_BINARY_VERSION}
        SOVERSION ${SPARROW_BINARY_CURRENT}
        COMPILE_DEFINITIONS "SPARROW_EXPORTS"
    )
endif ()
target_include_directories(sparrow PUBLIC
    $<BUILD_INTERFACE:${SPARROW_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>)
# We do not use non-standard C++
set_target_properties(sparrow PROPERTIES CMAKE_CXX_EXTENSIONS OFF)
target_compile_features(sparrow PUBLIC cxx_std_20)
target_link_libraries(sparrow PUBLIC ${SPARROW_INTERFACE_DEPENDENCIES})
if(ENABLE_COVERAGE)
    enable_coverage(sparrow)
endif()

if (BUILD_TESTS)
    message(STATUS "🧪 Create tests targets")
    enable_testing()
    add_subdirectory(test)
endif ()

# Docs
# ====

if(BUILD_DOCS)
    add_subdirectory(docs)
endif()

# Examples
# ========

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation
# ============

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS sparrow
        EXPORT ${PROJECT_NAME}-targets)

# Makes the project importable from the build directory
export(EXPORT ${PROJECT_NAME}-targets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")

install(DIRECTORY ${SPARROW_INCLUDE_DIR}/sparrow
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        PATTERN ".clang-*" EXCLUDE
        PATTERN "README.md" EXCLUDE)

set(SPARROW_CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}" CACHE
    STRING "install path for sparrowConfig.cmake")

configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${SPARROW_CMAKECONFIG_INSTALL_DIR})

# sparrow is header-only and does not depend on the architecture.
# Remove CMAKE_SIZEOF_VOID_P from sparrowConfigVersion.cmake so that an sparrowConfig.cmake
# generated for a 64 bit target can be used for 32 bit targets and vice versa.
set(_SPARROW_CMAKE_SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P})
unset(CMAKE_SIZEOF_VOID_P)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
                                 VERSION ${CMAKE_PROJECT_VERSION}
                                 COMPATIBILITY AnyNewerVersion)
set(CMAKE_SIZEOF_VOID_P ${_SPARROW_CMAKE_SIZEOF_VOID_P})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        DESTINATION ${SPARROW_CMAKECONFIG_INSTALL_DIR})
install(EXPORT ${PROJECT_NAME}-targets
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION ${SPARROW_CMAKECONFIG_INSTALL_DIR})

