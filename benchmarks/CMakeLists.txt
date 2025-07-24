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

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    project(sparrow-benchmarks CXX)
    find_package(sparrow REQUIRED CONFIG)
    set(SPARROW_INCLUDE_DIR ${sparrow_INCLUDE_DIRS})
endif()

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting benchmarks build type to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
else()
    message(STATUS "Benchmarks build type is ${CMAKE_BUILD_TYPE}")
endif()

set(SPARROW_BENCHMARK_SOURCES
    main.cpp
    bench_dynamic_bitset.cpp
    bench_fixed_width_binary_array.cpp
    bench_primitive_array.cpp
    bench_std_vector.cpp
)

add_executable(sparrow_benchmarks ${SPARROW_BENCHMARK_SOURCES})

target_link_libraries(sparrow_benchmarks
    PRIVATE
        sparrow
        benchmark::benchmark
        benchmark::benchmark_main
)

set_target_properties(sparrow_benchmarks
    PROPERTIES
        FOLDER "Benchmarks"
)

# Benchmark targets
add_custom_target(run_benchmarks
    COMMAND sparrow_benchmarks
    DEPENDS sparrow_benchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running sparrow benchmarks with console output"
)

set_target_properties(run_benchmarks
    PROPERTIES
        FOLDER "Benchmarks"
)

# Benchmark target with JSON output
add_custom_target(run_benchmarks_json
    COMMAND sparrow_benchmarks --benchmark_format=json --benchmark_out=sparrow_benchmarks.json
    DEPENDS sparrow_benchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running sparrow benchmarks with JSON output"
)

set_target_properties(run_benchmarks_json
    PROPERTIES
        FOLDER "Benchmarks"
)
