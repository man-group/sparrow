find_package(Doxygen REQUIRED)

# Fetch doxygen-awesome-css theme
include(FetchContent)
message(STATUS "Fetching doxygen-awesome-css theme...")
FetchContent_Declare(
    doxygen-awesome-css
    GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
    GIT_TAG v2.4.1
)
FetchContent_MakeAvailable(doxygen-awesome-css)
message(STATUS "Fetched doxygen-awesome-css theme.")

# Get the source directory where doxygen-awesome-css was downloaded
set(DOXYGEN_AWESOME_CSS_DIR "${doxygen-awesome-css_SOURCE_DIR}")

# configure the doxygen.in file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

# add a target to generate API documentation with Doxygen
add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
)

# add install capabilities
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION share/doc)