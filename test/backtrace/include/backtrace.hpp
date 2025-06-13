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

#pragma once

#include <string>

namespace sparrow::test
{
    /**
     * @brief Initializes the backtrace system for crash reporting
     *
     * This function sets up signal handlers to capture and display
     * backtraces when crashes occur during unit tests.
     */
    void initialize_backtrace_on_crash();

    /**
     * @brief Captures and returns the current stack trace
     *
     * @param skip_frames Number of stack frames to skip (default: 0)
     * @return std::string String representation of the stack trace
     */
    std::string capture_backtrace(int skip_frames = 0);

    /**
     * @brief Prints a formatted backtrace to stderr
     *
     * @param skip_frames Number of stack frames to skip (default: 1 to skip this function)
     */
    void print_backtrace(int skip_frames = 1);
}
