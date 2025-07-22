include(compile_options)

if(NOT SPARROW_BUILD_SHARED)
    message(FATAL_ERROR "c_data_integration requires SPARROW_BUILD_SHARED to be ON")
endif()

add_library(c_data_integration ${SPARROW_LIBRARY_TYPE}
    src/c_data_integration.cpp
    include/sparrow/c_data_integration/c_data_integration.hpp
    include/sparrow/c_data_integration/config.hpp
    )

set_target_properties(c_data_integration PROPERTIES PREFIX "")

target_compile_options(c_data_integration
    PRIVATE
        ${compile_options}
)

target_compile_definitions(c_data_integration
    PRIVATE
        SPARROW_C_DATA_INTEGRATION_EXPORTS)

set_target_properties(c_data_integration
    PROPERTIES
        VERSION ${SPARROW_BINARY_VERSION}
        SOVERSION ${SPARROW_BINARY_CURRENT}
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        FOLDER c_data_integration
        # RPATH settings for Unix-like systems
        $<$<PLATFORM_ID:Linux,Darwin>:INSTALL_RPATH_USE_LINK_PATH> TRUE
        $<$<PLATFORM_ID:Linux,Darwin>:BUILD_WITH_INSTALL_RPATH> FALSE
        $<$<PLATFORM_ID:Linux>:INSTALL_RPATH> "$ORIGIN"
        $<$<PLATFORM_ID:Darwin>:INSTALL_RPATH> "@loader_path"
        BUILD_RPATH_USE_ORIGIN TRUE
)

target_link_libraries(c_data_integration
    PUBLIC
        json_reader)

target_include_directories(c_data_integration
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
        