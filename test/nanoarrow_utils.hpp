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

#include <type_traits>

#include "sparrow/types/data_type.hpp"

#include "nanoarrow/common/inline_types.h"


namespace sparrow
{
    template <typename T>
    ArrowType nanoarrow_type_from()
    {
        if constexpr (std::is_same_v<T, int8_t>)
        {
            return ArrowType::NANOARROW_TYPE_INT8;
        }
        else if constexpr (std::is_same_v<T, int16_t>)
        {
            return ArrowType::NANOARROW_TYPE_INT16;
        }
        else if constexpr (std::is_same_v<T, int32_t>)
        {
            return ArrowType::NANOARROW_TYPE_INT32;
        }
        else if constexpr (std::is_same_v<T, int64_t>)
        {
            return ArrowType::NANOARROW_TYPE_INT64;
        }
        else if constexpr (std::is_same_v<T, uint8_t>)
        {
            return ArrowType::NANOARROW_TYPE_UINT8;
        }
        else if constexpr (std::is_same_v<T, uint16_t>)
        {
            return ArrowType::NANOARROW_TYPE_UINT16;
        }
        else if constexpr (std::is_same_v<T, uint32_t>)
        {
            return ArrowType::NANOARROW_TYPE_UINT32;
        }
        else if constexpr (std::is_same_v<T, uint64_t>)
        {
            return ArrowType::NANOARROW_TYPE_UINT64;
        }
        else if constexpr (std::is_same_v<T, float16_t>)
        {
            return ArrowType::NANOARROW_TYPE_HALF_FLOAT;
        }
        else if constexpr (std::is_same_v<T, float32_t>)
        {
            return ArrowType::NANOARROW_TYPE_FLOAT;
        }
        else if constexpr (std::is_same_v<T, float64_t>)
        {
            return ArrowType::NANOARROW_TYPE_DOUBLE;
        }
    }

    template <typename T>
    ArrowErrorCode nanoarrow_append(ArrowArray* array, T value)
    {
        if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> || std::is_same_v<T, int32_t>
                      || std::is_same_v<T, int64_t>)
        {
            return ArrowArrayAppendInt(array, static_cast<int64_t>(value));
        }
        else if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t>
                           || std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t>)
        {
            return ArrowArrayAppendUInt(array, static_cast<uint64_t>(value));
        }
        else if constexpr (std::is_same_v<T, float16_t> || std::is_same_v<T, float32_t>
                           || std::is_same_v<T, float64_t>)
        {
            return ArrowArrayAppendDouble(array, static_cast<double>(value));
        }
        else
        {
            return -1;
        }
    }

    template <typename T>
    T nanoarrow_get(ArrowArrayView* array, int64_t index)
    {
        if constexpr (std::is_same<T, int8_t>::value || std::is_same<T, int16_t>::value
                      || std::is_same<T, int32_t>::value || std::is_same<T, int64_t>::value)
        {
            return static_cast<T>(ArrowArrayViewGetIntUnsafe(array, index));
        }
        else if constexpr (std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value
                           || std::is_same<T, uint32_t>::value || std::is_same<T, uint64_t>::value)
        {
            return static_cast<T>(ArrowArrayViewGetUIntUnsafe(array, index));
        }
        else if constexpr (std::is_same_v<T, float16_t> || std::is_same_v<T, float32_t>
                           || std::is_same_v<T, float64_t>)
        {
            return static_cast<T>(ArrowArrayViewGetDoubleUnsafe(array, index));
        }
        else
        {
            static_assert(mpl::dependent_false<T>::value, "nanoarrow_get: Unsupported type.");
            mpl::unreachable();
        }
    }
}
