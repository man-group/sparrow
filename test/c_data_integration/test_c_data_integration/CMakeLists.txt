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
