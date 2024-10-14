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

#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <stdexcept>
#include <vector>

#ifndef ARROW_C_DATA_INTERFACE
#    define ARROW_C_DATA_INTERFACE

extern "C"
{
    struct ArrowSchema
    {
        // Array type description
        const char* format;
        const char* name;
        const char* metadata;
        int64_t flags;
        int64_t n_children;
        struct ArrowSchema** children;
        struct ArrowSchema* dictionary;

        // Release callback
        void (*release)(struct ArrowSchema*);
        // Opaque producer-specific data
        void* private_data;
    };

    struct ArrowArray
    {
        // Array data description
        int64_t length;
        int64_t null_count;
        int64_t offset;
        int64_t n_buffers;
        int64_t n_children;
        const void** buffers;
        struct ArrowArray** children;
        struct ArrowArray* dictionary;

        // Release callback
        void (*release)(struct ArrowArray*);
        // Opaque producer-specific data
        void* private_data;
    };

}  // extern "C"

#endif  // ARROW_C_DATA_INTERFACE

namespace sparrow
{
    enum class ArrowFlag : int64_t
    {
        DICTIONARY_ORDERED = 1,  // For dictionary-encoded types, whether the ordering of dictionary indices
                                 // is semantically meaningful.
        NULLABLE = 2,            // Whether this field is semantically nullable (regardless of whether it
                                 // actually has null values).
        MAP_KEYS_SORTED = 4      // For map types, whether the keys within each map value are sorted.
    };

    /// Specifies the ownership model when passing Arrow data to another system.
    enum class ownership : bool
    {
        not_owning,  ///< The system handling the related Arrow data do not own that data,
                     ///< that system must not and will not release it.

        owning,  ///< The system handling the related Arrow data owns that data
                 ///< and is responsible for releasing it through the release
                 ///< function associated to the Arrow data.
    };

    /// Specifies the ownership model when passing Arrow data to another system
    /// through `ArrowArray` and `ArrowSchema`
    struct arrow_data_ownership
    {
        ///< Specifies if the ownership of the schema data
        ownership schema = ownership::not_owning;

        ///< Specifies if the ownership of the array data
        ownership array = ownership::not_owning;
    };

    /// Useful shortcut value to specify non-owning handled Arrow data.
    inline constexpr auto doesnt_own_arrow_data = arrow_data_ownership{
        .schema = ownership::not_owning,
        .array = ownership::not_owning,
    };

    /// Useful shortcut value to specify full owning of handled Arrow data.
    inline constexpr auto owns_arrow_data = arrow_data_ownership{
        .schema = ownership::owning,
        .array = ownership::owning,
    };

    /// Matches only the C interface structs for Arrow.
    template <class T>
    concept any_arrow_c_interface = std::same_as<std::remove_cvref_t<T>, ArrowArray>
                                    or std::same_as<std::remove_cvref_t<T>, ArrowSchema>;


    /// Matches `ArrowSchema` or  a non-const pointer to an `ArrowSchema`.
    template <class T>
    concept arrow_schema_or_ptr = std::same_as<T, ArrowSchema>
                                  or std::same_as<std::remove_reference_t<T>, ArrowSchema*>;

    /// Matches `ArrowArray` or  a non-const pointer to an `ArrowArray`.
    template <class T>
    concept arrow_array_or_ptr = std::same_as<T, ArrowArray>
                                 or std::same_as<std::remove_reference_t<T>, ArrowArray*>;


}  // namespace sparrow
