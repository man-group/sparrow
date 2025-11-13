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

#include "sparrow/c_interface.hpp"


#ifndef ARROW_C_STREAM_INTERFACE
#    define ARROW_C_STREAM_INTERFACE
extern "C"
{
    struct ArrowArrayStream
    {
        // Callbacks providing stream functionality
        int (*get_schema)(struct ArrowArrayStream*, struct ArrowSchema* out);
        int (*get_next)(struct ArrowArrayStream*, struct ArrowArray* out);
        const char* (*get_last_error)(struct ArrowArrayStream*);

        // Release callback
        void (*release)(struct ArrowArrayStream*);

        // Opaque producer-specific data
        void* private_data;
    };
}  // extern "C"

#endif  // ARROW_C_STREAM_INTERFACE