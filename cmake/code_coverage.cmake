if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(WARNING "Code coverage is only supported with Clang or GCC")
    return()
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(WARNING "Code coverage results with an optimised (non-Debug) build may be misleading")
endif()

set(CLANG_COVERAGE_FLAGS --coverage -fprofile-instr-generate -fcoverage-mapping -fno-inline -fno-elide-constructors)
set(GCC_COVERAGE_FLAGS --coverage -fno-inline -fno-inline-small-functions -fno-default-inline)

find_program(gcovr gcovr REQUIRED)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(gcov_executable llvm-cov gcov)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(gcov_executable gcov)
endif()

message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

add_custom_target(generate_html_with_gcovr
    COMMAND ${gcovr} 
        --gcov-executable="${gcov_executable}"
        --html --html-details
        --filter ${CMAKE_SOURCE_DIR}/include/
        --filter ${CMAKE_SOURCE_DIR}/src/
        -r ${CMAKE_SOURCE_DIR}
        -o coverage.html
    DEPENDS test_sparrow_lib run_tests 
    # WORKING_DIRECTORY $<TARGET_FILE_DIR:test_sparrow_lib>
    COMMENT "Generating coverage report with gcovr: ${CMAKE_BINARY_DIR}/coverage.html"
)
set_target_properties(generate_html_with_gcovr PROPERTIES FOLDER "Code Coverage")
# add_dependencies(generate_html_with_gcovr test_sparrow_lib)

function(enable_coverage target)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:Clang>:${CLANG_COVERAGE_FLAGS}>
        $<$<CXX_COMPILER_ID:GNU>:${GCC_COVERAGE_FLAGS}>
    )
    target_link_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:Clang>:${CLANG_COVERAGE_FLAGS}>
        $<$<CXX_COMPILER_ID:GNU>:${GCC_COVERAGE_FLAGS}>
    )  
    add_dependencies(generate_html_with_gcovr ${target})  
endfunction(enable_coverage target)
