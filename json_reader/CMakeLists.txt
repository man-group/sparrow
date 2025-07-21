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
)

target_link_libraries(json_reader
    PUBLIC
        sparrow
        nlohmann_json::nlohmann_json)
target_include_directories(json_reader
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
