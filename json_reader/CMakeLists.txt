project(sparrow-json-reader CXX)

include(compile_options)

add_library(json_reader ${SPARROW_LIBRARY_TYPE}
    src/binary_parser.cpp
    src/binaryview_parser.cpp
    src/bool_parser.cpp
    src/comparison.cpp
    src/decimal_parser.cpp
    src/fixedsizebinary_parser.cpp
    src/fixedsizelist_parser.cpp
    src/json_parser.cpp
    src/list_parser.cpp
    src/listview_parser.cpp
    src/map_parser.cpp
    src/null_parser.cpp
    src/primitive_parser.cpp
    src/run_end_encoded_parser.cpp
    src/string_parser.cpp
    src/struct_parser.cpp
    src/temporal_parser.cpp
    src/union_parser.cpp
    src/utils.cpp
    include/sparrow/json_reader/binary_parser.hpp
    include/sparrow/json_reader/binaryview_parser.hpp
    include/sparrow/json_reader/bool_parser.hpp
    include/sparrow/json_reader/comparison.hpp
    include/sparrow/json_reader/config.hpp
    include/sparrow/json_reader/constant.hpp
    include/sparrow/json_reader/decimal_parser.hpp
    include/sparrow/json_reader/fixedsizebinary_parser.hpp
    include/sparrow/json_reader/fixedsizelist_parser.hpp
    include/sparrow/json_reader/list_parser.hpp
    include/sparrow/json_reader/map_parser.hpp
    include/sparrow/json_reader/null_parser.hpp
    include/sparrow/json_reader/primitive_parser.hpp
    include/sparrow/json_reader/run_end_encoded_parser.hpp
    include/sparrow/json_reader/string_parser.hpp
    include/sparrow/json_reader/struct_parser.hpp
    include/sparrow/json_reader/temporal_parser.hpp
    include/sparrow/json_reader/utils.hpp
    )

set_property(TARGET json_reader PROPERTY POSITION_INDEPENDENT_CODE ON)

target_compile_options(json_reader
    PRIVATE
        ${compile_options}
)

target_compile_definitions(json_reader
    PRIVATE
        SPARROW_JSON_READER_EXPORTS
    PUBLIC
        $<$<NOT:$<BOOL:${SPARROW_BUILD_SHARED}>>:SPARROW_JSON_READER_STATIC>)

set_target_properties(json_reader
    PROPERTIES
        VERSION ${SPARROW_BINARY_VERSION}
        SOVERSION ${SPARROW_BINARY_CURRENT}
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        FOLDER json_reader
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG}"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE}"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG}"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}"
        OUTPUT_NAME sparrow_json_reader
        # RPATH settings for Unix-like systems
        $<$<PLATFORM_ID:Linux,Darwin>:INSTALL_RPATH_USE_LINK_PATH> TRUE
        $<$<PLATFORM_ID:Linux,Darwin>:BUILD_WITH_INSTALL_RPATH> FALSE
        $<$<PLATFORM_ID:Linux>:INSTALL_RPATH> "$ORIGIN"
        $<$<PLATFORM_ID:Darwin>:INSTALL_RPATH> "@loader_path"
        BUILD_RPATH_USE_ORIGIN TRUE
)

target_link_libraries(json_reader
    PUBLIC
        sparrow
        nlohmann_json::nlohmann_json)
target_include_directories(json_reader
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

if (BUILD_TESTS)
    add_subdirectory(test)
endif ()

# Installation
# ============
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install target
set(JSON_READER_EXPORTED_TARGETS json_reader)

if(TARGET nlohmann_json)
    get_target_property(is_imported nlohmann_json IMPORTED)
    if(NOT is_imported)
        # This means `nlohmann_json` was fetched using FetchContent
        # We need to export `nlohmann_json` target explicitly
        list(APPEND JSON_READER_EXPORTED_TARGETS nlohmann_json)
    endif()
endif()

install(TARGETS ${JSON_READER_EXPORTED_TARGETS}
    EXPORT ${PROJECT_NAME}-targets)

# Makes the project importable from the build directory
export(EXPORT ${PROJECT_NAME}-targets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake"
    NAMESPACE sparrow::)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/sparrow
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install CMake configuration files
set(JSON_READER_CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}" CACHE
    STRING "install path for sparrow-json-readerConfig.cmake")

configure_package_config_file(${PROJECT_NAME}Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION ${JSON_READER_CMAKECONFIG_INSTALL_DIR})

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${JSON_READER_CMAKECONFIG_INSTALL_DIR})

install(EXPORT ${PROJECT_NAME}-targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE sparrow::
    DESTINATION ${JSON_READER_CMAKECONFIG_INSTALL_DIR})
