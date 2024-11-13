# build all examples
SET(EXAMPLES 
    typed_array_low_level.cpp
    typed_array_high_level.cpp
    record_batch_example.cpp
)

# iterate over all examples
foreach(EXAMPLE ${EXAMPLES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE sparrow)
    set_target_properties(${EXAMPLE_NAME} PROPERTIES
        FOLDER "Examples"
    )
endforeach()

# custom target to run each example
foreach(EXAMPLE ${EXAMPLES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE} NAME_WE)
    add_custom_target(run_${EXAMPLE_NAME}
        COMMAND ${EXAMPLE_NAME}
        DEPENDS ${EXAMPLE_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running ${EXAMPLE_NAME}"
    )
    set_target_properties(run_${EXAMPLE_NAME} PROPERTIES
        FOLDER "Examples"
    )
endforeach()

# custom target to run all examples
add_custom_target(run_examples
    COMMENT "Running all examples")
set_target_properties(run_examples PROPERTIES
    FOLDER "Examples"
)
foreach(EXAMPLE ${EXAMPLES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE} NAME_WE)
    add_dependencies(run_examples run_${EXAMPLE_NAME})
endforeach()
