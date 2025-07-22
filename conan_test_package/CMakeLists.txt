cmake_minimum_required(VERSION 3.28)
project(test_package LANGUAGES CXX)

find_package(sparrow REQUIRED CONFIG)

add_executable(standalone main.cpp)

target_link_libraries(standalone PRIVATE sparrow::sparrow)
target_compile_features(standalone PRIVATE cxx_std_20)

# Check if json_reader target is available and create a separate test if it is
if(TARGET sparrow::json_reader)
    message(STATUS "json_reader target is available, creating json_reader test")
    add_executable(json_reader_test json_reader_test.cpp)
    target_link_libraries(json_reader_test PRIVATE sparrow::sparrow sparrow::json_reader)
    target_compile_features(json_reader_test PRIVATE cxx_std_20)
else()
    message(STATUS "json_reader target is not available, skipping json_reader test")
endif()
