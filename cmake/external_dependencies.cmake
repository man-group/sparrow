include(FetchContent)

OPTION(FETCH_DEPENDENCIES_WITH_CMAKE "Fetch dependencies with CMake: Can be OFF, ON, or MISSING, in this case CMake download only dependencies which are not previously found." OFF)
MESSAGE(STATUS "🔧 FETCH_DEPENDENCIES_WITH_CMAKE: ${FETCH_DEPENDENCIES_WITH_CMAKE}")

if(FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "OFF")
    set(FIND_PACKAGE_OPTIONS REQUIRED)
else()
    set(FIND_PACKAGE_OPTIONS QUIET)
endif()

if(NOT FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON")
    find_package(date CONFIG ${FIND_PACKAGE_OPTIONS})
endif()
    
if(FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON" OR FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "MISSING") 
    if(NOT date_FOUND)
        set(DATE_VERSION "v3.0.3")
        message(STATUS "📦 Fetching HowardHinnant date ${DATE_VERSION}")
        set(USE_SYSTEM_TZ_DB ON)
        set(BUILD_TZ_LIB ON)
        set(BUILD_SHARED_LIBS OFF)
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
    if(BUILD_COMPARATIVE_BENCHMARKS)
        if(NOT FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON")
            find_package(arrow CONFIG ${FIND_PACKAGE_OPTIONS})
        endif()
        if(FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "ON" OR FETCH_DEPENDENCIES_WITH_CMAKE STREQUAL "MISSING")
            if(NOT arrow_FOUND)
                set(ARROW_VERSION "21.0.0")
                message(STATUS "📦 Fetching Apache Arrow ${ARROW_VERSION}")
                
                # Set minimal build options to avoid dependencies
                set(ARROW_BUILD_SHARED OFF)
                set(ARROW_BUILD_STATIC ON)
                set(ARROW_BUILD_TESTS OFF)
                set(ARROW_BUILD_BENCHMARKS OFF)
                set(ARROW_BUILD_EXAMPLES OFF)
                set(ARROW_BUILD_INTEGRATION OFF)
                set(ARROW_BUILD_UTILITIES OFF)
                
                # Disable all optional features and dependencies
                set(ARROW_GANDIVA OFF)
                set(ARROW_PARQUET OFF)
                set(ARROW_SUBSTRAIT OFF)
                set(ARROW_ACERO OFF)
                set(ARROW_COMPUTE OFF)
                set(ARROW_DATASET OFF)
                set(ARROW_FILESYSTEM OFF)
                set(ARROW_HDFS OFF)
                set(ARROW_FLIGHT OFF)
                set(ARROW_FLIGHT_SQL OFF)
                set(ARROW_CUDA OFF)
                set(ARROW_CSV OFF)
                set(ARROW_JSON OFF)
                set(ARROW_S3 OFF)
                set(ARROW_GCS OFF)
                set(ARROW_ORC OFF)
                
                # Disable compression libraries
                set(ARROW_WITH_BROTLI OFF)
                set(ARROW_WITH_BZ2 OFF)
                set(ARROW_WITH_LZ4 OFF)
                set(ARROW_WITH_SNAPPY OFF)
                set(ARROW_WITH_ZLIB OFF)
                set(ARROW_WITH_ZSTD OFF)
                
                # Disable other optional dependencies
                set(ARROW_WITH_BACKTRACE OFF)
                set(ARROW_WITH_THRIFT OFF)
                set(ARROW_WITH_PROTOBUF OFF)
                set(ARROW_WITH_GRPC OFF)
                set(ARROW_WITH_GFLAGS OFF)
                set(ARROW_WITH_GLOG OFF)
                set(ARROW_USE_GLOG OFF)
                set(ARROW_WITH_UTF8PROC OFF)
                set(ARROW_WITH_RE2 OFF)
                set(ARROW_USE_OPENSSL OFF)
                set(ARROW_WITH_OPENSSL OFF)
                set(ARROW_JEMALLOC OFF)
                set(ARROW_MIMALLOC OFF)
                set(ARROW_USE_BOOST OFF)
                set(ARROW_BOOST_REQUIRED OFF)
                
                # Set SIMD level to none to avoid xsimd dependency
                set(ARROW_SIMD_LEVEL NONE)
                set(ARROW_RUNTIME_SIMD_LEVEL NONE)
                
                # Disable deprecated API
                set(ARROW_NO_DEPRECATED_API ON)
                
                # Use bundled dependencies for remaining required components
                set(ARROW_DEPENDENCY_SOURCE BUNDLED)
                
                FetchContent_Declare(
                    arrow
                    GIT_SHALLOW TRUE
                    GIT_REPOSITORY https://github.com/apache/arrow.git
                    GIT_TAG apache-arrow-${ARROW_VERSION}
                    GIT_PROGRESS TRUE
                    SYSTEM
                    EXCLUDE_FROM_ALL
                    SOURCE_SUBDIR cpp)
                FetchContent_MakeAvailable(arrow)
                
                # Clean up cache variables
                unset(ARROW_BUILD_SHARED CACHE)
                unset(ARROW_BUILD_STATIC CACHE)
                unset(ARROW_BUILD_TESTS CACHE)
                unset(ARROW_BUILD_BENCHMARKS CACHE)
                unset(ARROW_BUILD_EXAMPLES CACHE)
                unset(ARROW_BUILD_INTEGRATION CACHE)
                unset(ARROW_BUILD_UTILITIES CACHE)
                unset(ARROW_GANDIVA CACHE)
                unset(ARROW_PARQUET CACHE)
                unset(ARROW_SUBSTRAIT CACHE)
                unset(ARROW_ACERO CACHE)
                unset(ARROW_COMPUTE CACHE)
                unset(ARROW_DATASET CACHE)
                unset(ARROW_FILESYSTEM CACHE)
                unset(ARROW_HDFS CACHE)
                unset(ARROW_FLIGHT CACHE)
                unset(ARROW_FLIGHT_SQL CACHE)
                unset(ARROW_CUDA CACHE)
                unset(ARROW_CSV CACHE)
                unset(ARROW_JSON CACHE)
                unset(ARROW_S3 CACHE)
                unset(ARROW_GCS CACHE)
                unset(ARROW_ORC CACHE)
                unset(ARROW_WITH_BROTLI CACHE)
                unset(ARROW_WITH_BZ2 CACHE)
                unset(ARROW_WITH_LZ4 CACHE)
                unset(ARROW_WITH_SNAPPY CACHE)
                unset(ARROW_WITH_ZLIB CACHE)
                unset(ARROW_WITH_ZSTD CACHE)
                unset(ARROW_WITH_BACKTRACE CACHE)
                unset(ARROW_WITH_THRIFT CACHE)
                unset(ARROW_WITH_PROTOBUF CACHE)
                unset(ARROW_WITH_GRPC CACHE)
                unset(ARROW_WITH_GFLAGS CACHE)
                unset(ARROW_WITH_GLOG CACHE)
                unset(ARROW_USE_GLOG CACHE)
                unset(ARROW_WITH_UTF8PROC CACHE)
                unset(ARROW_WITH_RE2 CACHE)
                unset(ARROW_USE_OPENSSL CACHE)
                unset(ARROW_WITH_OPENSSL CACHE)
                unset(ARROW_JEMALLOC CACHE)
                unset(ARROW_MIMALLOC CACHE)
                unset(ARROW_USE_BOOST CACHE)
                unset(ARROW_BOOST_REQUIRED CACHE)
                unset(ARROW_SIMD_LEVEL CACHE)
                unset(ARROW_RUNTIME_SIMD_LEVEL CACHE)
                unset(ARROW_NO_DEPRECATED_API CACHE)
                unset(ARROW_DEPENDENCY_SOURCE CACHE)
                
                message(STATUS "\t✅ Fetched Apache Arrow ${ARROW_VERSION}")
            endif()
        endif()
    endif()
endif()
