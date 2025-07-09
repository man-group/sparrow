# Test configuration for benchmarks
# This is a simple test to verify the benchmark setup

cmake_minimum_required(VERSION 3.28)
project(test_benchmark_setup)

# Test if we can find or fetch benchmark
set(BUILD_BENCHMARKS ON)
include(../cmake/external_dependencies.cmake)

# Print success message
message(STATUS "✅ Benchmark dependency configuration test passed!")
