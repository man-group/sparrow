include(FetchContent)

OPTION(FETCH_DEPENDENCIES_WITH_CMAKE "Fetch dependencies with CMake: Can be OFF, ON, or MISSING, in this case CMake download only dependencies which are not previously found." OFF)
MESSAGE(STATUS "🔧 FETCH_DEPENDENCIES_WITH_CMAKE: ${FETCH_DEPENDENCIES_WITH_CMAKE}")

if(FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "OFF")
    set(FIND_PACKAGE_OPTIONS REQUIRED)
else()
    set(FIND_PACKAGE_OPTIONS QUIET)
endif()

if(${USE_DATE_POLYFILL})
    if(NOT FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON")
        find_package(date CONFIG ${FIND_PACKAGE_OPTIONS})
    endif()

    if(FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON" OR FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "MISSING") 
        if(NOT date_FOUND)
            set(DATE_VERSION "v3.0.3")
            message(STATUS "📦 Fetching HowardHinnant date ${DATE_VERSION}")
            if(NOT DEFINED USE_SYSTEM_TZ_DB)
                set(USE_SYSTEM_TZ_DB ON)
            endif()
            set(BUILD_TZ_LIB ON)
            set(BUILD_SHARED_LIBS ${SPARROW_BUILD_SHARED})
            FetchContent_Declare(
                date
                GIT_SHALLOW TRUE
                GIT_REPOSITORY https://github.com/HowardHinnant/date.git
                GIT_TAG ${DATE_VERSION}
                GIT_PROGRESS TRUE
                SYSTEM
                EXCLUDE_FROM_ALL)
            FetchContent_MakeAvailable(date)
            unset(USE_SYSTEM_TZ_DB CACHE)
            unset(BUILD_TZ_LIB CACHE)
            unset(BUILD_SHARED_LIBS CACHE)
            message(STATUS "\t✅ Fetched HowardHinnant date ${DATE_VERSION}")
        else()
            message(STATUS "📦 date polyfill found here: ${date_DIR}")
        endif()
    endif()
endif()

if(BUILD_TESTS)
    if(NOT FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON")
        find_package(doctest ${FIND_PACKAGE_OPTIONS})
    endif()
    if(FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON" OR FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "MISSING") 
        if(NOT doctest_FOUND)
            set(DOCTEST_VERSION "v2.4.12")
            message(STATUS "📦 Fetching doctest ${DOCTEST_VERSION}")
            FetchContent_Declare(
                doctest
                GIT_SHALLOW TRUE
                GIT_REPOSITORY https://github.com/doctest/doctest.git
                GIT_TAG ${DOCTEST_VERSION}
                GIT_PROGRESS TRUE
                SYSTEM
                EXCLUDE_FROM_ALL)
            FetchContent_MakeAvailable(doctest)
            message(STATUS "\t✅ Fetched doctest ${DOCTEST_VERSION}")
        endif()
    endif()
endif()

if(CREATE_JSON_READER_TARGET)
    if(NOT FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON")
        find_package(nlohmann_json ${FIND_PACKAGE_OPTIONS})
        if( nlohmann_json_FOUND)
            message(STATUS "📦 nlohmann_json found here: ${nlohmann_json_DIR}")
        endif()
    endif()
    if(FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON" OR FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "MISSING")
        if(NOT nlohmann_json_FOUND)
            set(NLOHMANN_JSON_VERSION "v3.12.0")
            message(STATUS "📦 Fetching nlohmann_json ${NLOHMANN_JSON_VERSION}")
            FetchContent_Declare(
                nlohmann_json
                GIT_SHALLOW TRUE
                GIT_REPOSITORY https://github.com/nlohmann/json.git
                GIT_TAG ${NLOHMANN_JSON_VERSION}
                GIT_PROGRESS TRUE
                SYSTEM
                EXCLUDE_FROM_ALL)
            FetchContent_MakeAvailable(nlohmann_json)
            message(STATUS "\t✅ Fetched nlohmann_json ${NLOHMANN_JSON_VERSION}")
        endif()
    endif()
endif()

if(BUILD_BENCHMARKS)
    if(NOT FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON")
        find_package(benchmark ${FIND_PACKAGE_OPTIONS})
    endif()
    if(FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON" OR FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "MISSING")
        if(NOT benchmark_FOUND)
            set(BENCHMARK_VERSION "v1.9.4")
            message(STATUS "📦 Fetching GoogleBenchmark ${BENCHMARK_VERSION}")
            set(BENCHMARK_ENABLE_TESTING OFF)
            set(BENCHMARK_ENABLE_INSTALL OFF)
            set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
            set(DBENCHMARK_DOWNLOAD_DEPENDENCIES ON)
            FetchContent_Declare(
                benchmark
                GIT_SHALLOW TRUE
                GIT_REPOSITORY https://github.com/google/benchmark.git
                GIT_TAG v1.9.4
                GIT_PROGRESS TRUE
                SYSTEM
                EXCLUDE_FROM_ALL)
            FetchContent_MakeAvailable(benchmark)
            unset(BENCHMARK_ENABLE_TESTING CACHE)
            unset(BENCHMARK_ENABLE_INSTALL CACHE)
            unset(BENCHMARK_ENABLE_GTEST_TESTS CACHE)
            message(STATUS "\t✅ Fetched GoogleBenchmark ${BENCHMARK_VERSION}")
            set_target_properties(benchmark PROPERTIES FOLDER "GoogleBenchmark")
            set_target_properties(benchmark_main PROPERTIES FOLDER "GoogleBenchmark")
        endif()
    endif()
endif()
