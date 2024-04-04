set(CLANG-FORMAT_MINIMUM_MAJOR_VERSION 18)

function(get_clang_format_version clang_format_path)
    set(CLANG_FORMAT_VERSION_OUTPUT "")
    execute_process(
        COMMAND ${clang_format_path} --version
        OUTPUT_VARIABLE CLANG_FORMAT_VERSION_OUTPUT
    )
    string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" CLANG_FORMAT_VERSION_OUTPUT ${CLANG_FORMAT_VERSION_OUTPUT})
    set(CLANG_FORMAT_MAJOR_VERSION ${CMAKE_MATCH_1} PARENT_SCOPE)
    set(CLANG_FORMAT_MINOR_VERSION ${CMAKE_MATCH_2} PARENT_SCOPE)
    set(CLANG_FORMAT_PATCH_VERSION ${CMAKE_MATCH_3} PARENT_SCOPE)
endfunction()

function(check_clang-format_version validator_result_var item)
    set(${validator_result_var} FALSE PARENT_SCOPE)
    get_clang_format_version(${item})
    if (CLANG_FORMAT_MAJOR_VERSION LESS CLANG-FORMAT_MINIMUM_MAJOR_VERSION)
        message(DEBUG "clang-format found at ${item} | version: ${CLANG_FORMAT_MAJOR_VERSION}.${CLANG_FORMAT_MINOR_VERSION}.${CLANG_FORMAT_PATCH_VERSION}")
        message(DEBUG "but version is lower than ${CLANG-FORMAT_MINIMUM_MAJOR_VERSION}")
        set(${validator_result_var} FALSE PARENT_SCOPE)
    else()
        set(${validator_result_var} TRUE PARENT_SCOPE)
    endif()
endfunction()

function(print_clang_format_install_instructions)
    message(STATUS "🛠️ Please install clang-format to enable code formatting")
    message(STATUS "Can be installed via conda-forge: https://prefix.dev/channels/conda-forge/packages/clang-format")
    if(UNIX)
        if(APPLE)
            message(STATUS "🍎 On MacOS, you can install clang-format with:")
            message(STATUS "\t> brew install clang-format")
        else()
            message(STATUS "🐧 On Ubuntu, you can install clang-format with:")
            message(STATUS "\t> sudo apt-get install clang-format")
        endif()
    elseif(WIN32)
        message(STATUS "🪟 On Windows, you can install clang-format with:")
        message(STATUS "\t> winget llvm")
    endif()
endfunction()

find_program(CLANG_FORMAT clang-format
             VALIDATOR check_clang-format_version)

if(NOT CLANG_FORMAT)
    message(WARNING "❗ clang-format not found")

    print_clang_format_install_instructions()
else()
    get_clang_format_version(${CLANG_FORMAT})
    message(STATUS "✅ clang-format (version: ${CLANG_FORMAT_MAJOR_VERSION}.${CLANG_FORMAT_MINOR_VERSION}.${CLANG_FORMAT_PATCH_VERSION}) found at ${CLANG_FORMAT}")

    # list all files to format
    set(
        FORMAT_PATTERNS
        include/*.hpp
        test/*.cpp
        test/*.hpp
        CACHE STRING
        "; separated patterns relative to the project source dir to format"
    )

    set(ALL_FILES_TO_FORMAT "")
    foreach(PATTERN ${FORMAT_PATTERNS})
        file(GLOB_RECURSE FILES_TO_FORMAT ${CMAKE_SOURCE_DIR}/${PATTERN})
        list(APPEND ALL_FILES_TO_FORMAT ${FILES_TO_FORMAT})
    endforeach()

    add_custom_target(
        clang-format
        COMMAND ${CLANG_FORMAT} -i -style=file ${ALL_FILES_TO_FORMAT}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-format on all files"
    )

    add_custom_target(
        clang-format_dry_run
        COMMAND ${CLANG_FORMAT} --dry-run -style=file ${ALL_FILES_TO_FORMAT}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running dry clang-format on all files"
    )
endif()
