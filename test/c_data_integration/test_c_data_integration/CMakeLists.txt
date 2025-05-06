add_executable(test_c_data_integration test_c_data_integration.cpp)
target_link_libraries(test_c_data_integration
    PRIVATE
        c_data_integration
        doctest::doctest)

target_compile_definitions(test_c_data_integration
    PRIVATE
        JSON_FILES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/jsons/")

set_target_properties(test_c_data_integration PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    FOLDER c_data_integration)

add_custom_target(run_c_data_integration_tests
    COMMAND test_c_data_integration
    DEPENDS 
        c_data_integration
    COMMENT "Running c_data_integration tests"
    USES_TERMINAL
)

set_target_properties(run_c_data_integration_tests PROPERTIES FOLDER "Tests utilities")

set(JUNIT_REPORT_FILE_C_DATA_INTEGRATION_TESTS ${CMAKE_CURRENT_BINARY_DIR}/test_c_data_integration.xml)

add_custom_target(run_c_data_integration_with_junit_report
    COMMAND test_c_data_integration --reporters=better_junit --out=${JUNIT_REPORT_FILE_C_DATA_INTEGRATION_TESTS} --no-path-filenames=true
    DEPENDS
        test_c_data_integration
    COMMENT "Running tests with JUnit reports saved to: ${JUNIT_REPORT_FILE_C_DATA_INTEGRATION_TESTS}"
    USES_TERMINAL
)

set_target_properties(run_c_data_integration_with_junit_report PROPERTIES FOLDER "Tests utilities")
