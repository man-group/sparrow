############################################################################
# Copyright (c) QuantStack                                                 #
#                                                                          #
# Distributed under the terms of the BSD 3-Clause License.                 #
#                                                                          #
# The full license is in the file LICENSE, distributed with this software. #
############################################################################

cmake_minimum_required(VERSION 3.8)
project(xparrow CXX)

set(XPARROW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Versionning
# ===========

file(STRINGS "${XPARROW_INCLUDE_DIR}/xparrow/xparrow_version.hpp" xparrow_version_defines
     REGEX "#define XPARROW_VERSION_(MAJOR|MINOR|PATCH)")
foreach(ver ${xparrow_version_defines})
    if(ver MATCHES "#define XPARROW_VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$")
        set(XPARROW_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif()
endforeach()
set(${PROJECT_NAME}_VERSION
    ${XPARROW_VERSION_MAJOR}.${XPARROW_VERSION_MINOR}.${XPARROW_VERSION_PATCH})
message(STATUS "Building xparrow v${${PROJECT_NAME}_VERSION}")

# Build options
# =============

OPTION(BUILD_TESTS "xparrow test suite" OFF)

# Dependencies
# ============

# Build
# =====

set(XPARROW_HEADERS
    ${XPARROW_INCLUDE_DIR}/xparrow/xparrow_version.pp
)

add_library(xparrow INTERFACE)

target_include_directories(xparrow INTERFACE
    $<BUILD_INTERFACE:${XPARROW_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>)

target_compile_features(xparrow INTERFACE cxx_std_20)

if (BUILD_TESTS)
    add_subdirectory(test)
endif ()

# Installation
# ============

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS xparrow
        EXPORT ${PROJECT_NAME}-targets)

# Makes the project importable from the build directory
export(EXPORT ${PROJECT_NAME}-targets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")

install(FILES ${Xparrow_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/xparrow)

set(XPARROW_CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}" CACHE
    STRING "install path for xparrowConfig.cmake")

configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${XPARROW_CMAKECONFIG_INSTALL_DIR})

# xparrow is header-only and does not depend on the architecture.
# Remove CMAKE_SIZEOF_VOID_P from xparrowConfigVersion.cmake so that an xparrowConfig.cmake
# generated for a 64 bit target can be used for 32 bit targets and vice versa.
set(_XPARROW_CMAKE_SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P})
unset(CMAKE_SIZEOF_VOID_P)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
                                 VERSION ${${PROJECT_NAME}_VERSION}
                                 COMPATIBILITY AnyNewerVersion)
set(CMAKE_SIZEOF_VOID_P ${_XPARROW_CMAKE_SIZEOF_VOID_P})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        DESTINATION ${XPARROW_CMAKECONFIG_INSTALL_DIR})
install(EXPORT ${PROJECT_NAME}-targets
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION ${XPARROW_CMAKECONFIG_INSTALL_DIR})

