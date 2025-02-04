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
        message(STATUS "📦 Fetching doctest v2.4.11")
        FetchContent_Declare(
            doctest
            GIT_SHALLOW TRUE
            GIT_REPOSITORY https://github.com/doctest/doctest.git
            GIT_TAG v2.4.11
            GIT_PROGRESS TRUE
            SYSTEM
            EXCLUDE_FROM_ALL)
        FetchContent_MakeAvailable(doctest)
    endif()

    # if(NOT Catch2_FOUND)
    #     message(STATUS "📦 Fetching Catch2 v3.7.1")
    #     FetchContent_Declare(
    #         Catch2
    #         GIT_SHALLOW TRUE
    #         GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    #         GIT_TAG v3.7.1
    #         GIT_PROGRESS TRUE
    #         SYSTEM
    #         EXCLUDE_FROM_ALL)
    #     FetchContent_MakeAvailable(Catch2)
    # endif()

    if(NOT nanoarrow_FOUND)
        message(STATUS "📦 Fetching nanoarrow 0.6")
        FetchContent_Declare(
            nanoarrow
            GIT_SHALLOW TRUE
            GIT_REPOSITORY https://github.com/apache/arrow-nanoarrow.git
            GIT_TAG apache-arrow-nanoarrow-0.6.0
            GIT_PROGRESS TRUE
            SYSTEM
            EXCLUDE_FROM_ALL)
        FetchContent_MakeAvailable(nanoarrow)
        target_compile_options(nanoarrow PRIVATE -w)
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
        test_decimal_array.cpp
        test_decimal.cpp
        test_dictionary_encoded_array.cpp
        test_dispatch.cpp
        test_duration_array.cpp
        test_date_array.cpp
        test_dynamic_bitset_view.cpp
        test_dynamic_bitset.cpp
        test_fixed_width_binary_array.cpp
        test_format.cpp
        test_high_level_constructors.cpp
        test_interval_array.cpp
        test_iterator.cpp
        test_list_array.cpp
        test_list_value.cpp
        test_memory.cpp
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
        test_timestamp_array.cpp
        test_traits.cpp
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

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(compiles_options
        /bigobj
        /permissive-
        /WX # treat warnings as errors
        /W4 # Baseline reasonable warnings
        /we4242 # 'identifier': conversion from 'type1' to 'type1', possible loss of data
        /we4244 # conversion from 'type1' to 'type_2', possible loss of data
        /we4254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        /we4263 # 'function': member function does not override any base class virtual member function
        /we4265 # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not be destructed correctly
        /we4287 # 'operator': unsigned/negative constant mismatch
        /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
        /we4296 # 'operator': expression is always 'boolean_value'
        /we4311 # 'variable': pointer truncation from 'type1' to 'type2'
        /we4545 # expression before comma evaluates to a function which is missing an argument list
        /we4546 # function call before comma missing argument list
        /we4547 # 'operator': operator before comma has no effect; expected operator with side-effect
        /we4549 # 'operator': operator before comma has no effect; did you intend 'operator'?
        /we4555 # expression has no effect; expected expression with side- effect
        /we4619 # pragma warning: there is no warning number 'number'
        /we4640 # Enable warning on thread un-safe static member initialization
        /we4826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
        /we4905 # wide string literal cast to 'LPSTR'
        /we4906 # string literal cast to 'LPWSTR'
        /we4928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
        /we5038 # data member 'member1' will be initialized after data member 'member2'
        /Zc:__cplusplus)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(compiles_options
        -Wall # reasonable and standard
        -Wcast-align # warn for potential performance problem casts
        -Wconversion # warn on type conversions that may lose data
        -Wdouble-promotion # warn if float is implicitly promoted to double
        -Werror # treat warnings as errors
        -Wextra
        -Wformat=2 # warn on security issues around functions that format output (i.e., printf)
        -Wimplicit-fallthrough # Warns when case statements fall-through. (Included with -Wextra in GCC, not in clang)
        -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
        -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors
        -Wnull-dereference # warn if a null dereference is detected
        -Wold-style-cast # warn for c-style casts
        -Woverloaded-virtual # warn if you overload (not override) a virtual function
        -Wshadow # warn the user if a variable declaration shadows one from a parent context
        -Wsign-conversion # warn on sign conversions
        -Wunused # warn on anything being unused
        $<$<CXX_COMPILER_ID:Clang>:-Wno-c++98-compat> # do not warn on use of non-C++98 standard
        $<$<CXX_COMPILER_ID:Clang>:-Wno-c++98-compat-pedantic>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-documentation>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-extra-semi-stmt>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-c++20-compat>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-pre-c++20-compat-pedantic>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-reserved-identifier>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-undef>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-switch-default>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-switch-enum>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-missing-prototypes>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-unused-template>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-unsafe-buffer-usage>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-documentation-unknown-command>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-float-equal>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-exit-time-destructors>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-global-constructors>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-newline-eof>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-ctad-maybe-unsupported>
        $<$<CXX_COMPILER_ID:GNU>:-Wno-maybe-uninitialized>
        $<$<CXX_COMPILER_ID:GNU>:-Wno-array-bounds>
        $<$<CXX_COMPILER_ID:GNU>:-Wno-stringop-overread>
        $<$<CXX_COMPILER_ID:GNU>:-Wduplicated-branches> # warn if if / else branches have duplicated code
        $<$<CXX_COMPILER_ID:GNU>:-Wduplicated-cond> # warn if if / else chain has duplicated conditions
        $<$<CXX_COMPILER_ID:GNU>:-Wlogical-op> # warn about logical operations being used where bitwise were probably wanted
        $<$<CXX_COMPILER_ID:GNU>:-Wno-subobject-linkage> # suppress warnings about subobject linkage
    )

    if (NOT "${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC")
        target_compile_options(${test_target} PRIVATE "-ftemplate-backtrace-limit=0")
        target_compile_options(${test_target} PRIVATE "-pedantic")# Warn on language extensions
        # target_compile_options(${test_target_catch2} PRIVATE "-ftemplate-backtrace-limit=0")
        # target_compile_options(${test_target_catch2} PRIVATE "-pedantic")# Warn on language extensions
    endif()
endif()

target_compile_options(${test_target}
    PRIVATE
        ${compiles_options}
        $<$<BOOL:USE_SANITIZER>:${SANITIZER_COMPILE_OPTIONS}>
        )
target_link_options(${test_target}
    PRIVATE
        $<$<BOOL:USE_SANITIZER>:${SANITIZER_LINK_OPTIONS}>)

if(ENABLE_COVERAGE)
    enable_coverage(${test_target})
endif()

# We do not use non-standard C++
set_target_properties(${test_target} PROPERTIES CMAKE_CXX_EXTENSIONS OFF)
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
    