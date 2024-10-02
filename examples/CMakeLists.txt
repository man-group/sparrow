# build all examples
SET(EXAMPLES 
    typed_array_low_level.cpp
    typed_array_high_level.cpp
)

# iterate over all examples
foreach(EXAMPLE ${EXAMPLES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE sparrow)
endforeach()


# custom target to run all examples
add_custom_target(run_examples)
foreach(EXAMPLE ${EXAMPLES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE} NAME_WE)
    add_custom_command(TARGET run_examples
        POST_BUILD
        COMMAND ${EXAMPLE_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running ${EXAMPLE_NAME}"
    )
endforeach()



add_executable(bar bar.cpp)
target_link_libraries(bar PRIVATE sparrow)