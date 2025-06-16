# Copyright 2024 Man Group Operations Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

include(compile_options)

add_library(backtrace STATIC
    src/backtrace.cpp
    include/backtrace.hpp
)

target_compile_options(backtrace
    PRIVATE
        ${compile_options}
)

target_include_directories(backtrace
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Platform-specific linking for Windows debug symbols
if(WIN32)
    target_link_libraries(backtrace PRIVATE dbghelp)
else()
    target_link_libraries(backtrace PRIVATE execinfo)
endif()

set_target_properties(backtrace
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        FOLDER "Test Libraries"
)

# Test the backtrace library
add_executable(test_backtrace
    test/test_backtrace.cpp
)
target_link_libraries(test_backtrace
    PRIVATE
        backtrace
        better_junit_reporter
        doctest::doctest
)
