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

OPTION(FETCH_DEPENDENCIES_WITH_CMAKE "Fetch dependencies with CMake: Can be OFF, ON, or MISSING, in this case CMake download only dependencies which are not previously found." OFF)
MESSAGE(STATUS "🔧 FETCH_DEPENDENCIES_WITH_CMAKE: ${FETCH_DEPENDENCIES_WITH_CMAKE}")
enable_testing()

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    project(sparrow-test CXX)
    find_package(sparrow REQUIRED CONFIG)
    set(SPARROW_INCLUDE_DIR ${sparrow_INCLUDE_DIRS})
endif()

if(FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "OFF")
    set(FIND_PACKAGE_OPTIONS REQUIRED)
else()
    set(FIND_PACKAGE_OPTIONS QUIET)
endif()

if(NOT FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON")
    find_package(doctest ${FIND_PACKAGE_OPTIONS})
    # find_package(Catch2 3 ${FIND_PACKAGE_OPTIONS})
    find_package(nanoarrow ${FIND_PACKAGE_OPTIONS})
endif()

Set(FETCHCONTENT_QUIET FALSE)

if(FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON" OR FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "MISSING") 
    include(FetchContent)

    if(NOT doctest_FOUND)
        set(DOCTEST_VERSION "v2.4.11")
        message(STATUS "📦 Fetching doctest ${DOCTEST_VERSION}")
        FetchContent_Declare(
            doctest
            GIT_SHALLOW TRUE
            GIT_REPOSITORY https://github.com/doctest/doctest.git
            GIT_TAG ${DOCTEST_VERSION}
            GIT_PROGRESS TRUE
            SYSTEM
            EXCLUDE_FROM_ALL)
        FetchContent_MakeAvailable(doctest)
        message(STATUS "\t✅ Fetched doctest ${DOCTEST_VERSION}")
    endif()

    if(NOT Catch2_FOUND)
        set(CATCH2_VERSION "v3.7.1")
        message(STATUS "📦 Fetching Catch2 ${CATCH2_VERSION}")
        FetchContent_Declare(
            Catch2
            GIT_SHALLOW TRUE
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG ${CATCH2_VERSION}
            GIT_PROGRESS TRUE
            SYSTEM
            EXCLUDE_FROM_ALL)
        FetchContent_MakeAvailable(Catch2)
        message(STATUS "\t✅ Fetched Catch2 ${CATCH2_VERSION}")
    endif()

    if(NOT nanoarrow_FOUND)
        set(NANOARROW_VERSION "0.6.0")
        message(STATUS "📦 Fetching nanoarrow ${NANOARROW_VERSION}")
        FetchContent_Declare(
            nanoarrow
            GIT_SHALLOW TRUE
            GIT_REPOSITORY https://github.com/apache/arrow-nanoarrow.git
            GIT_TAG apache-arrow-nanoarrow-${NANOARROW_VERSION}
            GIT_PROGRESS TRUE
            SYSTEM
            EXCLUDE_FROM_ALL)
        FetchContent_MakeAvailable(nanoarrow)
        target_compile_options(nanoarrow PRIVATE -w)
        message(STATUS "\t✅ Fetched nanoarrow ${NANOARROW_VERSION}")
    endif()
endif()

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting tests build type to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
else()
    message(STATUS "Tests build type is ${CMAKE_BUILD_TYPE}")
endif()

if (SPARROW_TARGET_32BIT)
    set(SPARROW_TESTS_SOURCES
        junit.cpp
        junit.hpp
        junit_xml_writer.hpp
        main.cpp
    )
else()
    set(SPARROW_TESTS_SOURCES
        arrow_array_schema_creation.hpp
        better_junit_reporter.hpp
        external_array_data_creation.cpp
        external_array_data_creation.hpp
        nanoarrow_utils.hpp
        junit.cpp
        junit.hpp
        junit_xml_writer.hpp
        main.cpp
        test_allocator.cpp
        test_array_wrapper.cpp
        test_array.cpp
        test_arrow_array_schema_proxy.cpp
        test_arrow_array_schema_utils.cpp
        test_arrow_array.cpp
        test_arrow_schema.cpp
        test_binary_array.cpp
        test_bit.cpp
        test_buffer_adaptor.cpp
        test_buffer.cpp
        test_builder_dict_encoded.cpp
        test_builder_dict_encoded.cpp
        test_builder_run_end_encoded.cpp
        test_builder_run_end_encoded.cpp
        test_builder_utils.cpp
        test_builder_utils.cpp
        test_builder.cpp
        test_builder.cpp
        test_date_array.cpp
        test_date_array.cpp
        test_decimal_array.cpp
        test_decimal.cpp
        test_dictionary_encoded_array.cpp
        test_dispatch.cpp
        test_duration_array.cpp
        test_dynamic_bitset_view.cpp
        test_dynamic_bitset.cpp
        test_fixed_width_binary_array.cpp
        test_format.cpp
        test_high_level_constructors.cpp
        test_interval_array.cpp
        test_iterator.cpp
        test_large_int.cpp
        test_list_array.cpp
        test_list_value.cpp
        test_memory.cpp
        test_metadata.cpp
        test_mpl.cpp
        test_nested_comperators.cpp
        test_nested_comperators.cpp
        test_null_array.cpp
        test_nullable.cpp
        test_primitive_array.cpp
        test_ranges.cpp
        test_record_batch.cpp
        test_repeat_container.cpp
        test_run_end_encoded_array.cpp
        test_string_array.cpp
        test_struct_array.cpp
        test_time_array.cpp
        test_timestamp_array.cpp
        test_traits.cpp
        test_u8_buffer.cpp
        test_union_array.cpp
        test_utils_buffers.cpp
        test_utils_offsets.cpp
        test_utils.hpp
        test_variable_size_binary_view_array.cpp
    )
endif()

set(test_target "test_sparrow_lib")
add_executable(${test_target} ${SPARROW_TESTS_SOURCES})
target_link_libraries(${test_target}
    PRIVATE
        sparrow
        doctest::doctest
        nanoarrow::nanoarrow
        $<$<BOOL:USE_SANITIZER>:${SANITIZER_LINK_LIBRARIES}>)

# set(test_target_catch2 "test_sparrow_lib_catch2")

# add_executable(${test_target_catch2})
# target_link_libraries(${test_target_catch2}
#     PRIVATE
#         sparrow
#         Catch2::Catch2WithMain
#         $<$<BOOL:USE_SANITIZER>:${SANITIZER_LINK_LIBRARIES}>)

# include(Catch)
# catch_discover_tests(${test_target_catch2})

include(compiles_options)

target_compile_options(${test_target}
    PRIVATE
        ${compiles_options}
        $<$<BOOL:USE_SANITIZER>:${SANITIZER_COMPILE_OPTIONS}>
        )
target_link_options(${test_target}
    PRIVATE
        $<$<BOOL:USE_SANITIZER>:${SANITIZER_LINK_OPTIONS}>)
target_compile_definitions(${test_target}
    PRIVATE
        DOCTEST_CONFIG_VOID_CAST_EXPRESSIONS)

if(ENABLE_COVERAGE)
    enable_coverage(${test_target})
endif()

# We do not use non-standard C++
set_target_properties(${test_target} 
    PROPERTIES
        CMAKE_CXX_EXTENSIONS OFF)
target_compile_features(${test_target} PRIVATE cxx_std_20)

# target_compile_options(${test_target_catch2}
#     PRIVATE
#         ${compiles_options}
#         $<$<BOOL:USE_SANITIZER>:${SANITIZER_COMPILE_OPTIONS}>
#         )
# target_link_options(${test_target_catch2}
#     PRIVATE
#         $<$<BOOL:USE_SANITIZER>:${SANITIZER_LINK_OPTIONS}>)
# We do not use non-standard C++
# set_target_properties(${test_target_catch2} PROPERTIES CMAKE_CXX_EXTENSIONS OFF)
# target_compile_features(${test_target_catch2} PRIVATE cxx_std_20)

add_custom_target(run_tests
    COMMAND ${test_target}
    # COMMAND ${test_target_catch2} -v high
    DEPENDS 
        ${test_target}
        # ${test_target_catch2}
    COMMENT "Running tests for Doctest and Catch2"
    USES_TERMINAL
)

set_target_properties(run_tests PROPERTIES FOLDER "Tests utilities")

set(JUNIT_REPORT_FILE_DOCTEST ${CMAKE_CURRENT_BINARY_DIR}/test_sparrow_lib_report_doctest.xml)
set(JUNIT_REPORT_FILE_CATCH2 ${CMAKE_CURRENT_BINARY_DIR}/test_sparrow_lib_report_catch2.xml)

add_custom_target(run_tests_with_junit_report
    COMMAND ${test_target} --reporters=better_junit --out=${JUNIT_REPORT_FILE_DOCTEST} --no-path-filenames=true
    # COMMAND ${test_target_catch2} --reporter JUnit -o ${JUNIT_REPORT_FILE_CATCH2}
    DEPENDS
        ${test_target} 
        # ${test_target_catch2}
    COMMENT "Running tests with JUnit reports saved to: ${JUNIT_REPORT_FILE_DOCTEST} and ${JUNIT_REPORT_FILE_CATCH2}"
    USES_TERMINAL
)

set_target_properties(run_tests_with_junit_report PROPERTIES FOLDER "Tests utilities")


OPTION(ENABLE_INTEGRATION_TEST "Enable integration tests" ON)
if(ENABLE_INTEGRATION_TEST)
    add_subdirectory(c_data_integration)
endif()
