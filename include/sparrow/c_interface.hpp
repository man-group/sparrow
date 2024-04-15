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

#include <cstdint>
#include <cstdlib>
#include <optional>

#include "sparrow/allocator.hpp"
#include "sparrow/contracts.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

    struct ArrowSchema
    {
        // Array type description
        const char* format = nullptr;
        const char* name = nullptr;
        const char* metadata = nullptr;
        int64_t flags = 0;
        int64_t n_children = 0;
        struct ArrowSchema** children = nullptr;
        struct ArrowSchema* dictionary = nullptr;

        // Release callback
        void (*release)(struct ArrowSchema*) = nullptr;
        // Opaque producer-specific data
        void* private_data = nullptr;
    };

    struct ArrowArray
    {
        // Array data description
        int64_t length = 0;
        int64_t null_count = 0;
        int64_t offset = 0;
        int64_t n_buffers = 0;
        int64_t n_children = 0;
        const void** buffers = nullptr;
        struct ArrowArray** children = nullptr;
        struct ArrowArray* dictionary = nullptr;

        // Release callback
        void (*release)(struct ArrowArray*) = nullptr;
        // Opaque producer-specific data
        void* private_data = nullptr;
    };

#ifdef __cplusplus
}  // extern "C"
#endif

namespace sparrow
{
    /**
     * Holds the allocator for the buffers of an ArrowArray.
     *
     * @tparam BufferType The type of the buffers.
     * @tparam Allocator An allocator for BufferType.
     */
    template <class BufferType, template <typename> class Allocator>
        requires sparrow::allocator<Allocator<BufferType>>
    struct ArrowArrayPrivateData
    {
        ArrowArrayPrivateData()
            : buffer_allocator(std::move(Allocator<BufferType>()))
        {
        }

        explicit ArrowArrayPrivateData(const Allocator<BufferType>& allocator)
            : buffer_allocator(allocator)
        {
        }

        any_allocator<BufferType> buffer_allocator;
    };

    /**
     * Holds the allocator for the strings of an ArrowSchema.
     *
     * @tparam Allocator An allocator for char.
     */
    template <template <typename> class Allocator>
        requires sparrow::allocator<Allocator<char>>
    struct ArrowSchemaPrivateData
    {
        ArrowSchemaPrivateData()
            : string_allocator(std::move(Allocator<char>()))
        {
        }

        explicit ArrowSchemaPrivateData(const Allocator<char>& allocator)
            : string_allocator(allocator)
        {
        }

        any_allocator<char> string_allocator;
        size_t name_size = 0;
        size_t format_size = 0;
        size_t metadata_size = 0;
    };

    template <typename T>
    concept ArrowType = std::is_same_v<T, ArrowArray> || std::is_same_v<T, ArrowSchema>;

    /**
     * Releases the children and dictionary of an ArrowArray or ArrowSchema.
     *
     * @tparam T The type of the object.
     * @param obj The object to release the children and dictionary of.
     */
    template <ArrowType T>
    void release_children(T& obj)
    {
        for (int64_t i = 0; i < obj.n_children; ++i)
        {
            T* child = obj.children[i];
            if (child->release != nullptr)
            {
                child->release(child);
                SPARROW_ASSERT_TRUE(child->release == nullptr);
            }
            delete child;
        }
        delete[] obj.children;
        obj.children = nullptr;
        obj.n_children = 0;
    }

    /**
     * Releases the dictionary of an ArrowArray or ArrowSchema.
     *
     * @tparam T The type of the object.
     * @param obj The object to release the dictionary of.
     */
    template <ArrowType T>
    void release_dictionary(T& obj)
    {
        if (obj.dictionary != nullptr && obj.dictionary->release != nullptr)
        {
            obj.dictionary->release(obj.dictionary);
            SPARROW_ASSERT_TRUE(obj.dictionary->release == nullptr);
        }
        delete obj.dictionary;
        obj.dictionary = nullptr;
    }

    /**
     * Releases the children and dictionary of an ArrowArray or ArrowSchema.
     *
     * @tparam T The type of the object.
     * @param obj The object to release the children and dictionary of.
     */
    template <ArrowType T>
    void common_deleter(T& obj)
    {
        release_children(obj);
        release_dictionary(obj);
    }

    /**
     * Deletes an ArrowArray.
     *
     * @tparam T The type of the buffers of the ArrowArray.
     * @tparam Allocator The allocator for the buffers of the ArrowArray.
     * @param array The ArrowArray to delete.
     */
    template <typename T, template <typename> typename Allocator>
        requires sparrow::allocator<Allocator<T>>
    void delete_array(ArrowArray* array)
    {
        SPARROW_ASSERT_FALSE(array == nullptr);
        SPARROW_ASSERT_FALSE(array->release == nullptr);

        common_deleter(*array);

        const auto private_data = static_cast<ArrowArrayPrivateData<T, Allocator>*>(array->private_data);
        const size_t buffers_size = array->length * sizeof(T);
        for (int64_t i = 0; i < array->n_buffers; ++i)
        {
            auto buffer = const_cast<void*>(array->buffers[i]);
            private_data->buffer_allocator.deallocate(static_cast<T*>(buffer), buffers_size);
            buffer = nullptr;
        }
        delete array->buffers;
        array->buffers = nullptr;
        array->n_buffers = 0;

        array->length = 0;
        array->null_count = 0;
        array->offset = 0;

        delete private_data;
        array->private_data = nullptr;
        array->release = nullptr;
    }

    /**
     * Deletes an ArrowSchema.
     *
     * @tparam Allocator The allocator for the strings of the ArrowSchema.
     * @param schema The ArrowSchema to delete.
     */
    template <template <typename> class Allocator>
        requires sparrow::allocator<Allocator<char>>
    void delete_schema(ArrowSchema* schema)
    {
        SPARROW_ASSERT_FALSE(schema == nullptr);
        SPARROW_ASSERT_FALSE(schema->release == nullptr);

        common_deleter(*schema);

        const auto private_data = static_cast<ArrowSchemaPrivateData<Allocator>*>(schema->private_data);
        auto& string_allocator = private_data->string_allocator;
        const auto dealocate_string = [&string_allocator](const char*& str, size_t& size) -> void
        {
            string_allocator.deallocate(const_cast<char*>(str), size * sizeof(char));
            str = nullptr;
            size = 0;
        };

        dealocate_string(schema->format, private_data->format_size);
        dealocate_string(schema->name, private_data->name_size);
        dealocate_string(schema->metadata, private_data->metadata_size);

        delete private_data;
        schema->n_children = 0;
        schema->flags = 0;

        schema->private_data = nullptr;
        schema->release = nullptr;
    }

    enum class ArrowFlag : int64_t
    {
        DICTIONARY_ORDERED = 1,  // Whether this field is semantically nullable (regardless of whether it
                                 // actually has null values).
        NULLABLE = 2,        // For dictionary-encoded types, whether the ordering of dictionary indices is
                             // semantically meaningful.
        MAP_KEYS_SORTED = 4  // For map types, whether the keys within each map value are sorted.
    };

    /**
     * Creates an ArrowSchema.
     *
     * @tparam Allocator The allocator for the strings of the ArrowSchema.
     * @param format A mandatory, null-terminated, UTF8-encoded string describing the data type. If the data
     * type is nested, child types are not encoded here but in the ArrowSchema.children structures.
     * @param name An optional (nullptr), null-terminated, UTF8-encoded string of the field or array name. This is
     * mainly used to reconstruct child fields of nested types.
     * @param metadata An optional (nullptr), binary string describing the type’s metadata. If the data type is nested,
     * child types are not encoded here but in the ArrowSchema.children structures.
     * @param flags A bitfield of flags enriching the type description. Its value is computed by OR’ing
     * together the flag values.
     * @param n_children The number of children this type has.
     * @param children  A C array of pointers to each child type of this type. There must be
     * ArrowSchema.n_children pointers. May be nullptr only if ArrowSchema.n_children is 0.
     * @param dictionary Optional. A pointer to the type of dictionary values. Must be present if the
     * ArrowSchema represents a dictionary-encoded type. Must be nullptr otherwise.
     * @return The created ArrowSchema.
     */
    template <template <typename> class Allocator>
        requires sparrow::allocator<Allocator<char>>
    ArrowSchema* make_arrow_schema(
        const char* format,
        const char* name,
        const char* metadata,
        std::optional<ArrowFlag> flags,
        int64_t n_children,
        ArrowSchema** children,
        ArrowSchema* dictionary
    )
    {
        SPARROW_ASSERT_TRUE(format != nullptr);
        SPARROW_ASSERT_TRUE(n_children > 0 ? children == nullptr ? false : true : true);

        ArrowSchema* schema = new ArrowSchema;
        schema->private_data = new ArrowSchemaPrivateData<Allocator>;
        schema->release = delete_schema<Allocator>;

        auto private_data = static_cast<ArrowSchemaPrivateData<Allocator>*>(schema->private_data);

        const auto build_string = [&private_data](size_t& size, const char* str) -> char*
        {
            size = strlen(str) + 1;
            char* dest = private_data->string_allocator.allocate(size);
            std::uninitialized_copy(str, str + size, dest);
            return dest;
        };

        schema->format = build_string(private_data->format_size, format);

        if (name != nullptr)
        {
            schema->name = build_string(private_data->name_size, name);
        }
        if (metadata != nullptr)
        {
            schema->metadata = build_string(private_data->metadata_size, metadata);
        }

        schema->flags = flags.has_value() ? static_cast<int64_t>(flags.value()) : 0;
        schema->n_children = n_children;
        schema->children = children;
        schema->dictionary = dictionary;
        return schema;
    };

    /**
     * Creates an ArrowArray.
     *
     * @tparam T The type of the buffers of the ArrowArray.
     * @tparam Allocator The allocator for the buffers of the ArrowArray.
     * @param length The logical length of the array (i.e. its number of items).
     * @param null_count The number of null items in the array. MAY be -1 if not yet computed.
     * @param offset The logical offset inside the array (i.e. the number of items from the physical start of
     * the buffers). Must be 0 or positive.
     * @param n_buffers  The number of physical buffers backing this array. The number of buffers is a
     * function of the data type, as described in the Columnar format specification, except for the the binary
     * or utf-8 view type, which has one additional buffer compared to the Columnar format specification (see
     * Binary view arrays).
     * @param n_children The number of children this array has. The number of children is a function of the
     * data type, as described in the Columnar format specification.
     * @param children Optional. A C array of pointers to each child array of this array. There must be
     * n_children pointers. May be nullptr only if n_children is 0.
     * @param dictionary A pointer to the underlying array of dictionary values. Must be present if the
     * ArrowArray represents a dictionary-encoded array. Must be null otherwise.
     * @return The created ArrowArray.
     */
    template <class T, template <typename> class Allocator>
        requires sparrow::allocator<Allocator<T>>
    ArrowArray* make_array_constructor(
        int64_t length,
        int64_t null_count,
        int64_t offset,
        int64_t n_buffers,
        int64_t n_children,
        ArrowArray** children,
        ArrowArray* dictionary
    )
    {
        SPARROW_ASSERT_TRUE(length >= 0);
        SPARROW_ASSERT_TRUE(null_count >= -1);
        SPARROW_ASSERT_TRUE(offset >= 0);
        SPARROW_ASSERT_TRUE(n_buffers >= 0);
        SPARROW_ASSERT_TRUE(n_children >= 0);
        SPARROW_ASSERT_TRUE(n_children > 0 ? children == nullptr ? false : true : true);

        ArrowArray* array = new ArrowArray;
        array->private_data = new ArrowArrayPrivateData<T, Allocator>;
        array->release = delete_array<T, Allocator>;
        array->length = length;
        array->null_count = null_count;
        array->offset = offset;
        array->n_buffers = n_buffers;
        const size_t buffers_size = length * sizeof(T);
        array->buffers = new const void*[n_buffers];
        for (int64_t i = 0; i < n_buffers; ++i)
        {
            array->buffers[i] = static_cast<ArrowArrayPrivateData<T, Allocator>*>(array->private_data)
                                    ->buffer_allocator.allocate(buffers_size);
        }
        array->n_children = n_children;
        array->children = children;
        array->dictionary = dictionary;

        return array;
    }

}  // namespace sparrow
