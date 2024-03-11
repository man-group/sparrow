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

namespace sparrow
{
    // TODO: does not support all types specified by the Arrow specification
    // yet
    enum class data_type
    {
        NA = 0,
        BOOL,
        UINT8,
        INT8,
        UINT16,
        INT16,
        UINT32,
        INT32,
        UINT64,
        INT64,
        HALF_FLOAT,
        FLOAT,
        DOUBLE,
        // UTF8 variable-length string
        STRING,
        // Variable-length bytes (no guarantee of UTF8-ness)
        BINARY,
        // Fixed-size binary. Each value occupies the same number of bytes
        FIXED_SIZE_BINARY
    };

    // For now, a tiny wrapper around data_type
    // More data and functions to come
    class data_descriptor
    {
    public:

        constexpr explicit data_descriptor(data_type id = data_type::UINT8)
            : m_id(id)
        {
        }

        constexpr data_type id() const { return m_id; }

    private:

        data_type m_id;
    };
}

