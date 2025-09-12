# Copyright 2024 Man Group Operations Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
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

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()
message(STATUS "🔧 C++ standard: ${CMAKE_CXX_STANDARD}")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF) # We don't use modules
include(CMakeDependentOption)

set(SPARROW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SPARROW_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(CMAKE_CXX_COMPILER_LAUNCHER)
    message(STATUS "Using C++ compiler launcher: ${CMAKE_CXX_COMPILER_LAUNCHER}")
endif()

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
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
MESSAGE(STATUS "🔧 Build tests: ${BUILD_TESTS}")
OPTION(BUILD_BENCHMARKS "Build sparrow benchmark suite" OFF)
MESSAGE(STATUS "🔧 Build benchmarks: ${BUILD_BENCHMARKS}")
OPTION(BUILD_COMPARATIVE_BENCHMARKS "Build comparative benchmarks" OFF)
MESSAGE(STATUS "🔧 Build comparative benchmarks: ${BUILD_COMPARATIVE_BENCHMARKS}")
OPTION(BUILD_DOCS "Build sparrow documentation" OFF)
MESSAGE(STATUS "🔧 Build docs: ${BUILD_DOCS}")
OPTION(BUILD_EXAMPLES "Build sparrow examples" OFF)
MESSAGE(STATUS "🔧 Build examples: ${BUILD_EXAMPLES}")
OPTION(USE_DATE_POLYFILL "Use date polyfill implementation" ON)
MESSAGE(STATUS "🔧 Use date polyfill: ${USE_DATE_POLYFILL}")
OPTION(ENABLE_INTEGRATION_TEST "Enable integration tests" OFF)
MESSAGE(STATUS "🔧 Enable integration tests: ${ENABLE_INTEGRATION_TEST}")
OPTION(ENABLE_INTEGRATION_TEST "Creates json_reader target and enable integration tests" OFF)
OPTION(CREATE_JSON_READER_TARGET "Create json_reader target, automatically set when ENABLE_INTEGRATION_TEST is ON" OFF)

if(ENABLE_INTEGRATION_TEST)
    set(CREATE_JSON_READER_TARGET ON)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 4 OR MSVC)
    set(DEFAULT_USE_LARGE_INT_PLACEHOLDERS ON)
else()
    set(DEFAULT_USE_LARGE_INT_PLACEHOLDERS OFF)
endif()

OPTION(USE_LARGE_INT_PLACEHOLDERS "Use types without api for big integers 'ON' by default on 32 bit systems and MSVC compilers" ${DEFAULT_USE_LARGE_INT_PLACEHOLDERS})
MESSAGE(STATUS "🔧 Use large int placeholders: ${USE_LARGE_INT_PLACEHOLDERS}")
OPTION(SPARROW_CONTRACTS_THROW_ON_FAILURE "Throw exceptions instead of aborting on contract failures" OFF)
MESSAGE(STATUS "🔧 Contracts throw on failure: ${SPARROW_CONTRACTS_THROW_ON_FAILURE}")

OPTION(ENABLE_COVERAGE "Enable test coverage" OFF)
MESSAGE(STATUS "🔧 Enable coverage: ${ENABLE_COVERAGE}")

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
MESSAGE(STATUS "🔧 Activate linter: ${ACTIVATE_LINTER}")
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

include(external_dependencies)

set(SPARROW_INTERFACE_DEPENDENCIES "" CACHE STRING "List of dependencies to be linked to the sparrow target")
set(SPARROW_COMPILE_DEFINITIONS "" CACHE STRING "List of public compile definitions of the sparrow target")

if(USE_DATE_POLYFILL)
    list(APPEND SPARROW_INTERFACE_DEPENDENCIES date::date date::date-tz)
    list(APPEND SPARROW_COMPILE_DEFINITIONS SPARROW_USE_DATE_POLYFILL)
endif()

if(USE_LARGE_INT_PLACEHOLDERS)
    message(STATUS "Using large int placeholders")
    list(APPEND SPARROW_COMPILE_DEFINITIONS SPARROW_USE_LARGE_INT_PLACEHOLDERS)
endif()

if(SPARROW_CONTRACTS_THROW_ON_FAILURE)
    message(STATUS "Using contract exceptions")
    list(APPEND SPARROW_COMPILE_DEFINITIONS SPARROW_CONTRACTS_THROW_ON_FAILURE=1)
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
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/arrow_array/private_data.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/arrow_schema/private_data.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/arrow_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/arrow_array_schema_common_release.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/arrow_array_schema_info_utils.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/arrow_array_schema_proxy.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/arrow_array_schema_utils.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/arrow_flag_utils.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/arrow_schema.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/arrow_interface/private_data_ownership.hpp

    # buffer
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/dynamic_bitset/bitset_iterator.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/dynamic_bitset/bitset_reference.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/dynamic_bitset/dynamic_bitset_base.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/dynamic_bitset/dynamic_bitset_view.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/allocator.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/buffer.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/buffer_adaptor.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/buffer_view.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/buffer/dynamic_bitset.hpp

    # builder
    ${SPARROW_INCLUDE_DIR}/sparrow/builder/builder_utils.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/builder/nested_eq.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/builder/nested_less.hpp

    # config
    ${SPARROW_INCLUDE_DIR}/sparrow/config/config.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/config/sparrow_version.hpp

    # detail
    ${SPARROW_INCLUDE_DIR}/sparrow/details/3rdparty/float16_t.hpp

    # layout
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/array_access.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/array_base.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/array_bitmap_base.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/array_factory.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/array_helper.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/array_wrapper.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/date_types.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/decimal_reference.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/dispatch.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/fixed_width_binary_array_utils.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/fixed_width_binary_reference.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/interval_types.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/layout_concept.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/layout_iterator.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/layout_utils.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/list_value.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/map_value.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/mutable_array_base.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/nested_value_types.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/primitive_array_impl.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/primitive_data_access.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/run_end_encoded_iterator.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/struct_value.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/temporal_types.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/time_types.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/timestamp_concepts.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/timestamp_reference.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/timestamp_without_timezone_types.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/variable_size_binary_iterator.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/layout/variable_size_binary_reference.hpp

    # types
    ${SPARROW_INCLUDE_DIR}/sparrow/types/data_traits.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/types/data_type.hpp

    # Utils
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/bit.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/buffers.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/contracts.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/crtp_base.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/decimal.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/format.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/functor_index_iterator.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/iterator.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/large_int.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/memory.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/metadata.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/mp_utils.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/nullable.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/offsets.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/pair.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/ranges.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/repeat_container.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/sequence_view.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/sparrow_exception.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/temporal.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/utils/variant_visitor.hpp

    # ./
    ${SPARROW_INCLUDE_DIR}/sparrow/array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/array_api.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/builder.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/c_interface.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/date_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/decimal_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/dictionary_encoded_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/duration_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/fixed_width_binary_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/list_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/null_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/primitive_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/record_batch.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/run_end_encoded_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/struct_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/time_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/timestamp_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/timestamp_without_timezone_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/u8_buffer.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/union_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/variable_size_binary_array.hpp
    ${SPARROW_INCLUDE_DIR}/sparrow/variable_size_binary_view_array.hpp

    #../
    ${SPARROW_INCLUDE_DIR}/sparrow.hpp
)


set(SPARROW_SRC
    ${SPARROW_SOURCE_DIR}/arrow_interface/arrow_array.cpp
    ${SPARROW_SOURCE_DIR}/arrow_interface/arrow_array_schema_proxy.cpp
    ${SPARROW_SOURCE_DIR}/arrow_interface/arrow_schema.cpp
    ${SPARROW_SOURCE_DIR}/arrow_interface/private_data_ownership.cpp
    ${SPARROW_SOURCE_DIR}/layout/array_factory.cpp
    ${SPARROW_SOURCE_DIR}/layout/array_helper.cpp
    ${SPARROW_SOURCE_DIR}/layout/fixed_width_binary_array_utils.cpp
    ${SPARROW_SOURCE_DIR}/layout/list_value.cpp
    ${SPARROW_SOURCE_DIR}/layout/map_value.cpp
    ${SPARROW_SOURCE_DIR}/layout/struct_value.cpp
    ${SPARROW_SOURCE_DIR}/types/data_type.cpp
    ${SPARROW_SOURCE_DIR}/utils/metadata.cpp
    ${SPARROW_SOURCE_DIR}/utils/temporal.cpp
    ${SPARROW_SOURCE_DIR}/array.cpp
    ${SPARROW_SOURCE_DIR}/map_array.cpp
    ${SPARROW_SOURCE_DIR}/null_array.cpp
    ${SPARROW_SOURCE_DIR}/record_batch.cpp
    ${SPARROW_SOURCE_DIR}/run_end_encoded_array.cpp
    ${SPARROW_SOURCE_DIR}/struct_array.cpp
    ${SPARROW_SOURCE_DIR}/union_array.cpp
)

OPTION(SPARROW_BUILD_SHARED "Build sparrow as a shared library" ON)

if(SPARROW_BUILD_SHARED)
    message(STATUS "🔧 Build shared library")
    set(SPARROW_LIBRARY_TYPE SHARED)
else()
    message(STATUS "🔧 Build static library")
    set(SPARROW_LIBRARY_TYPE STATIC)
    list(APPEND SPARROW_COMPILE_DEFINITIONS SPARROW_STATIC_LIB)
endif()

add_library(sparrow ${SPARROW_LIBRARY_TYPE} ${SPARROW_SRC} ${SPARROW_HEADERS})

target_compile_definitions(sparrow PUBLIC ${SPARROW_COMPILE_DEFINITIONS})

set_property(TARGET sparrow PROPERTY POSITION_INDEPENDENT_CODE ON)

if(UNIX)
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
    )
    target_compile_options(sparrow PRIVATE "-fvisibility=hidden")
else()
    set_target_properties(
        sparrow
        PROPERTIES
        VERSION ${SPARROW_BINARY_VERSION}
        SOVERSION ${SPARROW_BINARY_CURRENT}
    )
    target_compile_definitions(sparrow PRIVATE SPARROW_EXPORTS)
endif()

target_include_directories(sparrow PUBLIC
    $<BUILD_INTERFACE:${SPARROW_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>)

# We do not use non-standard C++
set_target_properties(sparrow PROPERTIES CMAKE_CXX_EXTENSIONS OFF)
target_compile_features(sparrow PUBLIC cxx_std_20)
target_link_libraries(sparrow PUBLIC ${SPARROW_INTERFACE_DEPENDENCIES})

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND WIN32)
    cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH CLANG_BIN_PATH)
    cmake_path(GET CLANG_BIN_PATH PARENT_PATH CLANG_ROOT_PATH)
    set(CLANG_VERSION ${CMAKE_CXX_COMPILER_VERSION})
    string(REGEX REPLACE "^([0-9]+)\\..*" "\\1" CLANG_MAJOR_VERSION ${CLANG_VERSION})
    set(CLANG_LIB_PATH ${CLANG_ROOT_PATH}/lib/clang/${CLANG_MAJOR_VERSION}/lib/windows)
    find_library(CLANGRT_BUILTINS REQUIRED 
        NAMES
            clang_rt.builtins
            clang_rt.builtins-x86_64
        PATHS
            ${CLANG_LIB_PATH})
    message(STATUS "Using clang builtins: ${CLANGRT_BUILTINS}")
    target_link_libraries(sparrow PRIVATE ${CLANGRT_BUILTINS})
endif()

if(ENABLE_COVERAGE)
    enable_coverage(sparrow)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND WIN32)
    target_compile_options(sparrow
        PUBLIC
        -Wa,-mbig-obj)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (EMSCRIPTEN)
        target_compile_options(sparrow
        PRIVATE
        -fPIC)
    else ()
        target_compile_options(sparrow
        PUBLIC
        -Wno-c99-extensions)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(sparrow
        PUBLIC
            /bigobj
    )
endif()

if(CREATE_JSON_READER_TARGET)
    add_subdirectory(json_reader)
endif()

if(ENABLE_INTEGRATION_TEST)
    add_subdirectory(c_data_integration_tests)
endif()

if(BUILD_TESTS)
    message(STATUS "🧪 Create tests targets")
    enable_testing()
    add_subdirectory(test)
endif()

if(BUILD_BENCHMARKS)
    message(STATUS "🏁 Create benchmark targets")
    add_subdirectory(benchmarks)
endif()

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

# Install target
install(TARGETS sparrow
    EXPORT ${PROJECT_NAME}-targets)

# Makes the project importable from the build directory
export(EXPORT ${PROJECT_NAME}-targets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake"
    NAMESPACE sparrow::)

# Install headers
install(DIRECTORY ${SPARROW_INCLUDE_DIR}/sparrow
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    PATTERN ".clang-*" EXCLUDE
    PATTERN "README.md" EXCLUDE)

install(FILES ${SPARROW_INCLUDE_DIR}/sparrow.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(CREATE_JSON_READER_TARGET)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/json_reader/include/sparrow
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

# Install CMake configuration files
set(SPARROW_CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}" CACHE
    STRING "install path for sparrowConfig.cmake")

configure_package_config_file(${PROJECT_NAME}Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION ${SPARROW_CMAKECONFIG_INSTALL_DIR})

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${SPARROW_CMAKECONFIG_INSTALL_DIR})

install(EXPORT ${PROJECT_NAME}-targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE sparrow::
    DESTINATION ${SPARROW_CMAKECONFIG_INSTALL_DIR})
