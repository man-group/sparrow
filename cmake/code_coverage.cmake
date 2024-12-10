if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(WARNING "Code coverage results with an optimised (non-Debug) build may be misleading")
endif()

set(COVERAGE_REPORT_PATH "${CMAKE_BINARY_DIR}/coverage_reports" CACHE PATH "Path to store coverage reports")
set(COBERTURA_REPORT_PATH "${COVERAGE_REPORT_PATH}/cobertura.xml" CACHE PATH "Path to store cobertura report")
set(COVERAGE_TARGETS_FOLDER "Tests utilities/Code Coverage")

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    find_program(OpenCPPCoverage OpenCppCoverage.exe opencppcoverage.exe REQUIRED
        PATHS "C:/Program Files/OpenCppCoverage" "C:/Program Files (x86)/OpenCppCoverage")

    cmake_path(CONVERT ${CMAKE_SOURCE_DIR} TO_NATIVE_PATH_LIST OPENCPPCOVERAGE_SOURCES)
    set(OPENCPPCOVERAGE_COMMON_ARGS --sources=${OPENCPPCOVERAGE_SOURCES} --modules=${OPENCPPCOVERAGE_SOURCES} --excluded_sources=test*)

    add_custom_target(generate_cobertura
        COMMAND ${OpenCPPCoverage}
            ${OPENCPPCOVERAGE_COMMON_ARGS}
            --export_type=cobertura:${COBERTURA_REPORT_PATH}
            -- $<TARGET_FILE:test_sparrow_lib>
        DEPENDS test_sparrow_lib
        COMMENT "Generating coverage cobertura report with OpenCppCoverage: ${COBERTURA_REPORT_PATH}"
    )
    set(TARGET_PROPERTIES generate_cobertura PROPERTIES FOLDER ${COVERAGE_TARGETS_FOLDER})

    add_custom_target(generate_html_coverage_report
        COMMAND ${OpenCPPCoverage}
            ${OPENCPPCOVERAGE_COMMON_ARGS}
            --export_type=html:${COVERAGE_REPORT_PATH}
            -- $<TARGET_FILE:test_sparrow_lib>
        DEPENDS test_sparrow_lib
        COMMENT "Generating coverage report with OpenCppCoverage: ${COVERAGE_REPORT_PATH}"
    )
    set(TARGET_PROPERTIES generate_cobertura PROPERTIES FOLDER "Tests utilities/Code Coverage")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    find_program(llvm-cov llvm-cov REQUIRED)
    find_program(gcovr gcovr REQUIRED)
    find_program(doxide doxide REQUIRED PATHS ${doxide_ROOT})       

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(gcov_executable llvm-cov gcov)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(gcov_executable gcov)
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        add_custom_target(merge_gcov
            COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_BINARY_DIR}/coverage.gcov"
            COMMAND for /r %f in (*.gcda) do ${gcov_executable} --stdout "%f" >> ${CMAKE_BINARY_DIR}/coverage.gcov
            DEPENDS run_tests)
    else()
        add_custom_target(merge_gcov
            COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_BINARY_DIR}/coverage.gcov"
            COMMAND find . -name '*.gcda' | xargs ${gcov_executable} --stdout > ${CMAKE_BINARY_DIR}/coverage.gcov
            DEPENDS run_tests)
    endif()
    set_target_properties(merge_gcov PROPERTIES FOLDER ${COVERAGE_TARGETS_FOLDER})

    add_custom_target(doxide_generate_json
        COMMAND ${doxide} cover --coverage ${CMAKE_BINARY_DIR}/coverage.gcov > ${CMAKE_BINARY_DIR}/doxide_coverage.json
        DEPENDS merge_gcov
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating coverage report with doxide: ${CMAKE_BINARY_DIR}/doxide_coverage.json"
    )
    set_target_properties(doxide_generate_json PROPERTIES FOLDER ${COVERAGE_TARGETS_FOLDER})

    add_custom_target(generate_html_coverage_report
        COMMAND ${gcovr} --html-details ${COVERAGE_REPORT_PATH}/coverage.html
            --exclude-function-lines
            --exclude-noncode-lines
            --json-add-tracefile doxide_coverage.json
            -r ${CMAKE_SOURCE_DIR}
        DEPENDS doxide_generate_json
        COMMENT "Generating coverage report with gcovr: ${COVERAGE_REPORT_PATH}/coverage.html")
    set_target_properties(generate_html_coverage_report PROPERTIES FOLDER ${COVERAGE_TARGETS_FOLDER})

    add_custom_target(generate_cobertura
        COMMAND ${gcovr} --xml -o ${COBERTURA_REPORT_PATH}
            --exclude-function-lines
            --exclude-noncode-lines
            --json-add-tracefile doxide_coverage.json
            -r ${CMAKE_SOURCE_DIR}
        DEPENDS doxide_generate_json
        COMMENT "Generating coverage report with gcovr: ${CMAKE_BINARY_DIR}/cobertura.xml")
    set_target_properties(generate_cobertura PROPERTIES FOLDER ${COVERAGE_TARGETS_FOLDER})
endif()

function(enable_coverage target)
    set(CLANG_COVERAGE_FLAGS --coverage -fprofile-instr-generate -fcoverage-mapping -fno-inline -fno-elide-constructors)
    set(GCC_COVERAGE_FLAGS --coverage -fno-inline -fno-inline-small-functions -fno-default-inline)

    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:Clang>:${CLANG_COVERAGE_FLAGS}>
        $<$<CXX_COMPILER_ID:GNU>:${GCC_COVERAGE_FLAGS}>)
    target_link_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:Clang>:${CLANG_COVERAGE_FLAGS}>
        $<$<CXX_COMPILER_ID:GNU>:${GCC_COVERAGE_FLAGS}>)
endfunction(enable_coverage target)
