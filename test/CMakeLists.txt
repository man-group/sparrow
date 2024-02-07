############################################################################
# Copyright (c) QuantStack                                                 #
#                                                                          #
# Distributed under the terms of the BSD 3-Clause License.                 #
#                                                                          #
# The full license is in the file LICENSE, distributed with this software. #
############################################################################

cmake_minimum_required(VERSION 3.8)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    project(xtensor-test CXX)

    enable_testing()

    find_package(xparrowREQUIRED CONFIG)
    set(XPARROW_INCLUDE_DIR ${xparrow_INCLUDE_DIRS})
endif ()

find_package(doctest REQUIRED)
#find_package(Threads)

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting tests build type to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
else()
    message(STATUS "Tests build type is ${CMAKE_BUILD_TYPE}")
endif()

set(XPARROW_TESTS
    main.cpp
)
set(test_target "test_xsparrow_lib")
add_executable(${test_target} ${XPARROW_TESTS})
target_link_libraries(${test_target} PRIVATE doctest::doctest)
