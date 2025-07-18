include(compile_options)

add_library(c_data_integration SHARED
    src/c_data_integration.cpp
    include/sparrow/c_data_integration/c_data_integration.hpp
    include/sparrow/c_data_integration/config.hpp
    )

set_property(TARGET c_data_integration PROPERTY POSITION_INDEPENDENT_CODE ON)
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
)

target_link_libraries(c_data_integration
    PUBLIC
        json_reader)

target_include_directories(c_data_integration
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
        