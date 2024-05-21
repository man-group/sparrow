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

cmake_minimum_required(VERSION 3.8)

enable_testing()

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    project(sparrow-test CXX)
    find_package(sparrow REQUIRED CONFIG)
    set(SPARROW_INCLUDE_DIR ${sparrow_INCLUDE_DIRS})
endif ()

find_package(doctest REQUIRED)
#find_package(Threads)

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting tests build type to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
else()
    message(STATUS "Tests build type is ${CMAKE_BUILD_TYPE}")
endif()

set(SPARROW_TESTS_SOURCES
    main.cpp
    test_algorithm.cpp
    test_allocator.cpp
    test_array_data_creation.cpp
    test_array_data.cpp
    test_buffer.cpp
    test_dictionary_encoded_layout.cpp
    test_dynamic_bitset.cpp
    test_fixed_size_layout.cpp
    test_iterator.cpp
    test_memory.cpp
    test_mpl.cpp
    test_traits.cpp
    test_typed_array.cpp
    test_variable_size_binary_layout.cpp
)
set(test_target "test_sparrow_lib")
add_executable(${test_target} ${SPARROW_TESTS_SOURCES})
target_link_libraries(${test_target} PRIVATE sparrow doctest::doctest)
add_test(NAME ${test_target} COMMAND ${test_target})

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(compiles_options
        /permissive-
        /WX # treat warnings as errors 
        /W4 # Baseline reasonable warnings
        # /w14242 # 'identifier': conversion from 'type1' to 'type1', possible loss of data
        # /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        # /w14263 # 'function': member function does not override any base class virtual member function
        # /w14265 # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not be destructed correctly
        # /w14287 # 'operator': unsigned/negative constant mismatch
        # /w14296 # 'operator': expression is always 'boolean_value'
        # /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
        # /w14545 # expression before comma evaluates to a function which is missing an argument list
        # /w14546 # function call before comma missing argument list
        # /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
        # /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
        # /w14555 # expression has no effect; expected expression with side- effect
        # /w14619 # pragma warning: there is no warning number 'number'
        # /w14640 # Enable warning on thread un-safe static member initialization
        # /w14826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
        # /w14905 # wide string literal cast to 'LPSTR'
        # /w14906 # string literal cast to 'LPWSTR'
        # /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
        # /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
        # /we4244 # conversion from 'type1' to 'type_2', possible loss of data
        /Wall # all warnings
        /wd4355 # deactivate warning: 'this': used in base member initializer list
        /wd4514 # deactivate warning: 'function' : unreferenced inline function has been removed
        /wd4625 # deactivate warning: 'derived class' : copy constructor was implicitly defined as deleted because a base class copy constructor is inaccessible or deleted
        /wd4626 # deactivate warning: 'derived class' : assignment operator was implicitly defined as deleted because a base class assignment operator is inaccessible or deleted
        /wd4710 # deactivate warning: 'function' : function not inlined
        /wd4711 # deactivate warning: function 'function' selected for inline expansion
        /wd4820 # deactivate warning: 'bytes' bytes padding added after construct 'member_name'
        /wd5026 # deactivate warning: 'type': move constructor was implicitly defined as deleted
        /wd5027 # deactivate warning: 'type': move assignment operator was implicitly defined as deleted
        /wd5045 # deactivate warning: Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
        /external:anglebrackets
        /external:W0
        /Zc:__cplusplus)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(compiles_options
        -pedantic # Warn on language extensions
        -Wall # reasonable and standard
        -Wcast-align # warn for potential performance problem casts
        -Wconversion # warn on type conversions that may lose data
        -Wdouble-promotion # warn if float is implicitly promoted to double
        -Werror # treat warnings as errors 
        -Wextra
        -Wformat=2 # warn on security issues around functions that format output (i.e., printf)
        -Wimplicit-fallthrough # Warns when case statements fall-through. (Included with -Wextra in GCC, not in clang)
        -Wmisleading-indentation #  warn if indentation implies blocks where blocks do not exist
        -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors
        -Wnull-dereference # warn if a null dereference is detected
        -Wold-style-cast # warn for c-style casts
        -Woverloaded-virtual # warn if you overload (not override) a virtual function
        -Wpedantic # warn if non-standard C++ is used
        -Wshadow # warn the user if a variable declaration shadows one from a parent context
        -Wsign-conversion # warn on sign conversions
        -Wunused # warn on anything being unused
        -Wno-c++98-compat 
        $<$<CXX_COMPILER_ID:GNU>:-Wduplicated-branches> # warn if if / else branches have duplicated code
        $<$<CXX_COMPILER_ID:GNU>:-Wduplicated-cond> # warn if if / else chain has duplicated conditions
        $<$<CXX_COMPILER_ID:GNU>:-Wlogical-op> # warn about logical operations being used where bitwise were probably wanted
        $<$<CXX_COMPILER_ID:GNU>:-Wuseless-cast>) # warn if you perform a cast to the same type
endif()

target_compile_options(${test_target} PRIVATE ${compiles_options})

# We do not use non-standard C++
set_target_properties(${test_target} PROPERTIES CMAKE_CXX_EXTENSIONS OFF)
target_compile_features(${test_target} PRIVATE cxx_std_20)
