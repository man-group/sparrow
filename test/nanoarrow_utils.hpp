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
#include <utility>

#include "sparrow/types/data_type.hpp"
#include "sparrow/utils/nullable.hpp"

#include "doctest/doctest.h"
#include "nanoarrow/nanoarrow.h"

namespace sparrow
{
    template <typename T>
    constexpr ArrowType nanoarrow_type_from()
    {
        if constexpr (std::is_same_v<T, bool>)
        {
            return ArrowType::NANOARROW_TYPE_BOOL;
        }
        else if constexpr (std::is_same_v<T, int8_t>)
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
        else if constexpr (std::is_same_v<T, std::string>)
        {
            return ArrowType::NANOARROW_TYPE_STRING;
        }
        else
        {
            static_assert(mpl::dependent_false<T>::value, "nanoarrow_type_from: Unsupported type.");
            mpl::unreachable();
        }
    }

    template <typename T>
    inline ArrowErrorCode nanoarrow_append(ArrowArray* array, T value)
    {
        if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> || std::is_same_v<T, int32_t>
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
        else if constexpr (std::is_same_v<T, std::string>)
        {
            ArrowStringView value_view = ArrowCharView(value.c_str());
            return ArrowArrayAppendString(array, value_view);
        }
        else
        {
            static_assert(mpl::dependent_false<T>::value, "nanoarrow_type_from: Unsupported type.");
            mpl::unreachable();
        }
    }

    template <typename T>
    inline T nanoarrow_get(ArrowArrayView* array, int64_t index)
    {
        if constexpr (std::is_same<T, int8_t>::value || std::is_same<T, int16_t>::value
                      || std::is_same<T, int32_t>::value || std::is_same<T, int64_t>::value)
        {
            return static_cast<T>(ArrowArrayViewGetIntUnsafe(array, index));
        }
        else if constexpr (std::is_same<T, bool>::value || std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value
                           || std::is_same<T, uint32_t>::value || std::is_same<T, uint64_t>::value)
        {
            return static_cast<T>(ArrowArrayViewGetUIntUnsafe(array, index));
        }
        else if constexpr (std::is_same_v<T, float16_t> || std::is_same_v<T, float32_t>
                           || std::is_same_v<T, float64_t>)
        {
            return static_cast<T>(ArrowArrayViewGetDoubleUnsafe(array, index));
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            const ArrowStringView string_view = ArrowArrayViewGetStringUnsafe(array, index);
            return std::string{string_view.data, static_cast<size_t>(string_view.size_bytes)};
        }
        else
        {
            static_assert(mpl::dependent_false<T>::value, "nanoarrow_get: Unsupported type.");
            mpl::unreachable();
        }
    }

    template <typename T>
    inline std::pair<ArrowArray, ArrowSchema> nanoarrow_create(const std::vector<nullable<T>>& values)
    {
        ArrowSchema arrow_schema{};
        REQUIRE_EQ(ArrowSchemaInitFromType(&arrow_schema, nanoarrow_type_from<T>()), NANOARROW_OK);
        ArrowError error{};
        ArrowArray arrow_array{};
        REQUIRE_EQ(ArrowArrayInitFromSchema(&arrow_array, &arrow_schema, &error), NANOARROW_OK);
        REQUIRE_EQ(ArrowArrayStartAppending(&arrow_array), NANOARROW_OK);
        for (auto value : values)
        {
            if (value.has_value())
            {
                REQUIRE_EQ(nanoarrow_append(&arrow_array, value.value()), NANOARROW_OK);
            }
            else
            {
                REQUIRE_EQ(ArrowArrayAppendNull(&arrow_array, 1), NANOARROW_OK);
            }
        }
        REQUIRE_EQ(ArrowArrayFinishBuildingDefault(&arrow_array, &error), NANOARROW_OK);
        return {std::move(arrow_array), std::move(arrow_schema)};
    }

    template <typename T>
    inline void nanoarrow_validation(const ArrowArray* arrow_array, const std::vector<nullable<T>>& values)
    {
        ArrowError error;
        ArrowArrayView input_view;
        const ArrowType type = nanoarrow_type_from<T>();
        ArrowArrayViewInitFromType(&input_view, type);
        REQUIRE_EQ(ArrowArrayViewSetArray(&input_view, arrow_array, &error), NANOARROW_OK);
        REQUIRE_EQ(
            ArrowArrayViewValidate(&input_view, ArrowValidationLevel::NANOARROW_VALIDATION_LEVEL_FULL, &error),
            NANOARROW_OK
        );
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            CHECK_EQ(ArrowArrayViewIsNull(&input_view, static_cast<int64_t>(i)) == 0, values[i].has_value());
            const T value = nanoarrow_get<T>(&input_view, static_cast<int64_t>(i));
            if (values[i].has_value())
            {
                CHECK_EQ(value, values[i].value());
            }
        }
    }
}
