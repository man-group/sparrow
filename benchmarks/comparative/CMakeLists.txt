# Comparative benchmarks between sparrow and Apache Arrow

message(STATUS "Configuring comparative benchmarks...")

add_executable(sparrow_benchmarks_comparative
    benchmark_primitive.cpp
)

target_link_libraries(sparrow_benchmarks_comparative
    PRIVATE
        sparrow
        arrow_static
        benchmark::benchmark
        benchmark::benchmark_main
)

set(ARROW_INCLUDE_DIRS  ${CMAKE_BINARY_DIR}/_deps/arrow-src/cpp/src ${CMAKE_BINARY_DIR}/_deps/arrow-build/src)

target_include_directories(sparrow_benchmarks_comparative
    PRIVATE
        ${ARROW_INCLUDE_DIRS}
)

message("Arrow include dir: ${ARROW_INCLUDE_DIR}")

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
