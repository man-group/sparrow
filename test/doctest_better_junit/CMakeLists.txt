add_library(better_junit_reporter STATIC
    src/junit.cpp
    include/junit.hpp
    include/junit_xml_writer.hpp
    include/better_junit_reporter.hpp
)

target_link_libraries(better_junit_reporter
    PUBLIC
        doctest::doctest
)

target_include_directories(better_junit_reporter
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if(EMSCRIPTEN)
    target_compile_options(better_junit_reporter
        PUBLIC "SHELL: -fPIC"
    )
endif()