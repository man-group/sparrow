set(CLANG-TIDY_MINIMUM_MAJOR_VERSION 18)

function(get_clang_tidy_version clang_tidy_path)
    set(CLANG_TIDY_VERSION_OUTPUT "")
    execute_process(
        COMMAND ${clang_tidy_path} --version
        OUTPUT_VARIABLE CLANG_TIDY_VERSION_OUTPUT
    )
    string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" CLANG_TIDY_VERSION_OUTPUT ${CLANG_TIDY_VERSION_OUTPUT})
    set(CLANG_TIDY_MAJOR_VERSION ${CMAKE_MATCH_1} PARENT_SCOPE)
    set(CLANG_TIDY_MINOR_VERSION ${CMAKE_MATCH_2} PARENT_SCOPE)
    set(CLANG_TIDY_PATCH_VERSION ${CMAKE_MATCH_3} PARENT_SCOPE)
endfunction()

function(check_clang-tidy_version validator_result_var item)
    set(${validator_result_var} FALSE PARENT_SCOPE)
    get_clang_tidy_version(${item})
    if (CLANG_TIDY_MAJOR_VERSION LESS CLANG-TIDY_MINIMUM_MAJOR_VERSION)
        message(DEBUG "clang-tidy found at ${item} | version: ${CLANG_TIDY_MAJOR_VERSION}.${CLANG_TIDY_MINOR_VERSION}.${CLANG_TIDY_PATCH_VERSION}")
        message(DEBUG "but version is lower than ${CLANG-TIDY_MINIMUM_MAJOR_VERSION}")
        set(${validator_result_var} FALSE PARENT_SCOPE)
    else()
        set(${validator_result_var} TRUE PARENT_SCOPE)
    endif()
endfunction()

function(print_clang_tidy_install_instructions)
    message(STATUS "🛠️ Please install clang-tidy to enable code formatting")
    if(UNIX)
        if(APPLE)
            message(STATUS "🍎 On MacOS, you can install clang-tidy with:")
            message(STATUS "\t> brew install clang-tidy")
        else()
            message(STATUS "🐧 On Ubuntu, you can install clang-tidy with:")
            message(STATUS "\t> sudo apt-get install clang-tidy")
        endif()
    elseif(WIN32)
        message(STATUS "🪟 On Windows, you can install clang-tidy with:")
        message(STATUS "\t> winget llvm")
    endif()
endfunction()

find_program(CLANG_TIDY clang-tidy
             VALIDATOR check_clang-tidy_version)

if(NOT CLANG_TIDY)
    message(WARNING "❗clang-tidy with version >= ${CLANG-TIDY_MINIMUM_MAJOR_VERSION} not found")

    print_clang_tidy_install_instructions()
else()
    get_clang_tidy_version(${CLANG_TIDY})
    message(STATUS "✅clang-tidy found at ${CLANG_TIDY} | version: ${CLANG_TIDY_MAJOR_VERSION}.${CLANG_TIDY_MINOR_VERSION}.${CLANG_TIDY_PATCH_VERSION}")

    if(ACTIVATE_LINTER_DURING_COMPILATION)
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY})
    else()
        set(CMAKE_CXX_CLANG_TIDY "")
    endif()

    find_package (Python COMPONENTS Interpreter)
    if(Python_Interpreter_FOUND)
        message(DEBUG "Python found at ${Python_EXECUTABLE}")
        get_filename_component(CLANG_TIDY_FOLDER ${CLANG_TIDY} DIRECTORY)
        find_file(CLANG_TIDY_PYTHON_SCRIPT run-clang-tidy PATHS ${CLANG_TIDY_FOLDER} NO_DEFAULT_PATH)
        if(CLANG_TIDY_PYTHON_SCRIPT)
            message(DEBUG "run-clang-tidy.py found at ${CLANG_TIDY_PYTHON_SCRIPT}")
        endif()
        set(CLANG_TIDY_COMMAND ${Python_EXECUTABLE} ${CLANG_TIDY_PYTHON_SCRIPT})
    else()
        set(CLANG_TIDY_COMMAND ${CLANG_TIDY})
    endif()

    set(CLANG_TIDY_COMMON_ARGUMENTS
        $<$<NOT:$<BOOL:CLANG_TIDY_PYTHON_SCRIPT>>:->-use-color
        $<$<NOT:$<BOOL:CLANG_TIDY_PYTHON_SCRIPT>>:->-config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
        -p ${CMAKE_BINARY_DIR})

    set(
        PATTERNS
        include/*.hpp
        test/*.cpp
        test/*.hpp
        CACHE STRING
        "; separated patterns relative to the project source dir to analyse"
    )

    set(ALL_FILES_TO_FORMAT "")
    foreach(PATTERN ${PATTERNS})
        file(GLOB_RECURSE FILES_TO_ANALYZE ${CMAKE_SOURCE_DIR}/${PATTERN})
        list(APPEND ALL_FILES_TO_ANALYZE ${FILES_TO_ANALYZE})
    endforeach()

    add_custom_target(
        clang-tidy
        COMMAND ${CLANG_TIDY_COMMAND} $<$<NOT:$<BOOL:CLANG_TIDY_PYTHON_SCRIPT>>:->-fix ${CLANG_TIDY_COMMON_ARGUMENTS} ${ALL_FILES_TO_ANALYZE}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy on all files"
    )

    add_custom_target(
        clang-tidy_dry_run
        COMMAND ${CLANG_TIDY_COMMAND} ${CLANG_TIDY_COMMON_ARGUMENTS}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running dry clang-tidy on all files"
    )
endif()
