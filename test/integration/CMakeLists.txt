if(NOT FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON")
    find_package(nlohmann_json ${FIND_PACKAGE_OPTIONS})
endif()

if(FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON" OR FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "MISSING") 
    include(FetchContent)
    if(NOT nlohmann_json_FOUND)
        set(NLOHMANN_JSON_VERSION "v3.11.3")
        message(STATUS "📦 Fetching nlohmann_json ${NLOHMANN_JSON_VERSION}")
        FetchContent_Declare(
            nlohmann_json
            GIT_SHALLOW TRUE
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG ${NLOHMANN_JSON_VERSION}
            GIT_PROGRESS TRUE
            SYSTEM
            EXCLUDE_FROM_ALL)
        FetchContent_MakeAvailable(nlohmann_json)
        message(STATUS "\t✅ Fetched nlohmann_json ${NLOHMANN_JSON_VERSION}")
    endif()
endif()

find_package(doctest REQUIRED)

add_library(c_data_integration SHARED
    c_data_integration.cpp
    c_data_integration.hpp)

target_link_libraries(c_data_integration
    PUBLIC
        sparrow
    PRIVATE
        nlohmann_json::nlohmann_json)

add_executable(test_c_data_integration test_c_data_integration.cpp)
target_link_libraries(test_c_data_integration
    PRIVATE
        c_data_integration
        doctest::doctest)

target_compile_definitions(test_c_data_integration
    PRIVATE
        JSON_FILES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/jsons/")
        