add_executable(test_json_reader test_json_reader.cpp)
target_link_libraries(test_json_reader
    PRIVATE
        json_reader
        better_junit_reporter
        doctest::doctest)

target_compile_definitions(test_json_reader
    PRIVATE
        JSON_FILES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../resources/jsons/")

set_target_properties(test_json_reader PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    FOLDER json_reader)

add_custom_target(run_test_json_reader_tests
    COMMAND test_json_reader
    DEPENDS 
        test_json_reader
    COMMENT "Running test_json_reader tests"
    USES_TERMINAL
)

set_target_properties(run_test_json_reader_tests PROPERTIES FOLDER "Tests utilities")

set(JUNIT_REPORT_FILE_JSON_READER_TESTS ${CMAKE_CURRENT_BINARY_DIR}/test_json_reader.xml)

add_custom_target(run_test_json_reader_with_junit_report
    COMMAND test_json_reader --reporters=better_junit --out=${JUNIT_REPORT_FILE_JSON_READER_TESTS} --no-path-filenames=true
    DEPENDS
        test_json_reader
    COMMENT "Running tests with JUnit reports saved to: ${JUNIT_REPORT_FILE_JSON_READER_TESTS}"
    USES_TERMINAL
)

set_target_properties(run_test_json_reader_with_junit_report PROPERTIES FOLDER "Tests utilities")
