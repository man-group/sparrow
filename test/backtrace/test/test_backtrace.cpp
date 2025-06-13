// Copyright 2024 Man Group Operations Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define DOCTEST_CONFIG_IMPLEMENT

#include <memory>

#include "backtrace.hpp"
#include "doctest/doctest.h"

int main(int argc, char** argv)
{
    // Initialize backtrace system for crash reporting
    sparrow::test::initialize_backtrace_on_crash();

    // Initialize and run doctest
    doctest::Context context;
    context.applyCommandLine(argc, argv);

    int res = context.run();  // run queries, or run tests unless --no-run

    if (context.shouldExit())  // important - query flags (help/version) rely on this
    {
        return res;  // propagate the result of the tests
    }

    return res;
}

TEST_SUITE("backtrace")
{
    TEST_CASE("capture_backtrace")
    {
        // Test that we can capture a backtrace without crashing
        std::string backtrace = sparrow::test::capture_backtrace();

        // The backtrace should not be empty and should contain function names
        CHECK_FALSE(backtrace.empty());
        CHECK(backtrace.find("capture_backtrace") != std::string::npos);
    }

    // Disabled by default - uncomment to test crash detection
    // WARNING: This will actually crash the test and terminate the program

    // TEST_CASE("segmentation_fault_test")
    // {
    //     // This test intentionally causes a segmentation fault
    //     // to verify the backtrace system works
    //     int* null_ptr = nullptr;
    //     *null_ptr = 42;  // This will cause a segmentation fault
    // }
}
