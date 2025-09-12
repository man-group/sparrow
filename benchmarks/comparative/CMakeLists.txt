# Comparative benchmarks between sparrow and Apache Arrow

cmake_minimum_required(VERSION 3.28)

# Try to find an existing Arrow installation first.
# Users can provide Arrow via CMAKE_PREFIX_PATH, vcpkg, conan, etc.
find_package(Arrow QUIET CONFIG REQUIRED)

add_executable(sparrow_benchmarks_comparative
    primitive_build_compare.cpp
)

# For Arrow, we only need the core array / buffer / type APIs
# Link against arrow::arrow and google benchmark + sparrow

target_link_libraries(sparrow_benchmarks_comparative
    PRIVATE
        sparrow
        arrow::arrow
        benchmark::benchmark
        benchmark::benchmark_main
)

set_target_properties(sparrow_benchmarks_comparative PROPERTIES FOLDER "Benchmarks")

add_custom_target(run_comparative_benchmarks
    COMMAND sparrow_benchmarks_comparative
    DEPENDS sparrow_benchmarks_comparative
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running comparative benchmarks (sparrow vs Arrow)"
)
set_target_properties(run_comparative_benchmarks PROPERTIES FOLDER "Benchmarks")

add_custom_target(run_comparative_benchmarks_json
    COMMAND sparrow_benchmarks_comparative --benchmark_format=json --benchmark_out=sparrow_comparative_benchmarks.json
    DEPENDS sparrow_benchmarks_comparative
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running comparative benchmarks (JSON output)"
)
set_target_properties(run_comparative_benchmarks_json PROPERTIES FOLDER "Benchmarks")
