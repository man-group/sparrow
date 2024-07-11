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
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <utility>

#include "sparrow/allocator.hpp"
#include "sparrow/buffer.hpp"
#include "sparrow/contracts.hpp"

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
    template <template <typename> class Allocator = std::allocator, class T, class U>
    std::vector<T*, Allocator<T*>> to_raw_ptr_vec(const std::vector<std::unique_ptr<T, U>>& vec)
    {
        std::vector<T*, Allocator<T*>> raw_ptr_vec;
        raw_ptr_vec.reserve(vec.size());
        std::ranges::transform(
            vec,
            std::back_inserter(raw_ptr_vec),
            [](const auto& child)
            {
                return child.get();
            }
        );
        return raw_ptr_vec;
    }

    template <template <typename> class Allocator = std::allocator, class T>
    std::vector<T*, Allocator<T*>> to_raw_ptr_vec(std::vector<buffer<T>>& vectors)
    {
        std::vector<T*, Allocator<T*>> raw_ptr_vec;
        raw_ptr_vec.reserve(vectors.size());
        std::ranges::transform(
            vectors,
            std::back_inserter(raw_ptr_vec),
            [](buffer<T>& vector) -> T*
            {
                return vector.data();
            }
        );
        return raw_ptr_vec;
    }

    template <class BufferType, std::ranges::input_range R, template <typename> class Allocator = std::allocator>
        requires std::is_integral_v<std::ranges::range_value_t<R>>
                 && sparrow::allocator<Allocator<buffer<BufferType>>>
    std::vector<buffer<BufferType>, Allocator<buffer<BufferType>>> create_buffers(
        const R& buffer_sizes,
        const Allocator<BufferType>& buffer_allocator,
        const Allocator<buffer<BufferType>>& buffers_allocator_
    )
    {
        std::vector<buffer<BufferType>, Allocator<buffer<BufferType>>> buffers(buffers_allocator_);
        for (const auto buffer_size : buffer_sizes)
        {
            buffers.emplace_back(buffer_size, buffer_allocator);
        }
        return buffers;
    }

    struct arrow_array_custom_deleter
    {
        void operator()(ArrowArray* array) const
        {
            if (array == nullptr)
            {
                return;
            }
            if (array->release != nullptr)
            {
                array->release(array);
            }
            delete array;
        }
    };

    using arrow_array_unique_ptr = std::unique_ptr<ArrowArray, arrow_array_custom_deleter>;

    /**
     * Struct representing private data for ArrowArray.
     *
     * This struct holds the private data for ArrowArray, including buffers, children, and dictionary.
     * It is used in the Sparrow library.
     *
     * @tparam BufferType The type of the buffers.
     * @tparam Allocator An allocator for BufferType.
     */
    template <class BufferType, template <typename> class Allocator = std::allocator>
        requires sparrow::allocator<Allocator<BufferType>>
    struct arrow_array_private_data
    {
        template <std::ranges::input_range V>
            requires std::is_integral_v<std::ranges::range_value_t<V>>
        explicit arrow_array_private_data(
            std::vector<arrow_array_unique_ptr> children,
            arrow_array_unique_ptr dictionary,
            const V& buffer_sizes
        );

        using buffer_allocator = Allocator<BufferType>;
        using buffers_allocator = Allocator<buffer<BufferType>>;

        buffer_allocator m_buffer_allocator = buffer_allocator();
        buffers_allocator m_buffers_allocator = buffers_allocator();

        std::vector<buffer<BufferType>, buffers_allocator> m_buffers;
        std::vector<BufferType*> m_buffers_raw_ptr_vec;

        std::vector<arrow_array_unique_ptr> m_children;
        std::vector<ArrowArray*> m_children_raw_ptr_vec;

        arrow_array_unique_ptr m_dictionary;
    };

    template <class BufferType, template <typename> class Allocator>
        requires sparrow::allocator<Allocator<BufferType>>
    template <std::ranges::input_range V>
        requires std::is_integral_v<std::ranges::range_value_t<V>>
    arrow_array_private_data<BufferType, Allocator>::arrow_array_private_data(
        std::vector<arrow_array_unique_ptr> children,
        arrow_array_unique_ptr dictionary,
        const V& buffer_sizes
    )
        : m_buffers(
              create_buffers<BufferType>(buffer_sizes, m_buffer_allocator, m_buffers_allocator),
              m_buffers_allocator
          )
        , m_buffers_raw_ptr_vec(to_raw_ptr_vec<Allocator>(m_buffers))
        , m_children(std::move(children))
        , m_children_raw_ptr_vec(to_raw_ptr_vec<Allocator>(m_children))
        , m_dictionary(std::move(dictionary))
    {
    }

    struct arrow_schema_custom_deleter
    {
        void operator()(ArrowSchema* schema) const
        {
            if (schema->release != nullptr)
            {
                schema->release(schema);
            }
            delete schema;
        }
    };

    using arrow_schema_unique_ptr = std::unique_ptr<ArrowSchema, arrow_schema_custom_deleter>;

    /**
     * Struct representing private data for ArrowSchema.
     *
     * This struct holds the private data for ArrowArray, including format, name and metadata strings,
     * children, and dictionary. It is used in the Sparrow library.
     *
     * @tparam Allocator An allocator for char.
     */
    template <template <typename> class Allocator = std::allocator>
        requires sparrow::allocator<Allocator<char>>
    struct arrow_schema_private_data
    {
        using string_type = std::basic_string<char, std::char_traits<char>, Allocator<char>>;
        using vector_string_type = std::vector<char, Allocator<char>>;

        arrow_schema_private_data() = delete;
        arrow_schema_private_data(const arrow_schema_private_data&) = delete;
        arrow_schema_private_data(arrow_schema_private_data&&) = delete;
        arrow_schema_private_data& operator=(const arrow_schema_private_data&) = delete;
        arrow_schema_private_data& operator=(arrow_schema_private_data&&) = delete;

        explicit arrow_schema_private_data(
            std::string_view format_view,
            std::string_view name_view,
            const std::optional<std::span<char>>& metadata,
            std::vector<arrow_schema_unique_ptr> children,
            arrow_schema_unique_ptr dictionary
        );

        ~arrow_schema_private_data();

        any_allocator<char> string_allocator_ = Allocator<char>();
        string_type m_format;
        string_type m_name;
        std::optional<vector_string_type> m_metadata;
        std::vector<arrow_schema_unique_ptr> m_children;
        std::vector<ArrowSchema*> m_children_raw_ptr_vec;
        arrow_schema_unique_ptr m_dictionary;
    };

    template <template <typename> class Allocator>
        requires sparrow::allocator<Allocator<char>>
    arrow_schema_private_data<Allocator>::arrow_schema_private_data(
        std::string_view format_view,
        std::string_view name_view,
        const std::optional<std::span<char>>& metadata,
        std::vector<arrow_schema_unique_ptr> children,
        arrow_schema_unique_ptr dictionary
    )
        : m_format(format_view)
        , m_name(name_view)
        , m_metadata(
              metadata.has_value() ? vector_string_type(metadata->begin(), metadata->end())
                                   : vector_string_type()
          )
        , m_children(std::move(children))
        , m_children_raw_ptr_vec(to_raw_ptr_vec(m_children))
        , m_dictionary(std::move(dictionary))
    {
    }

    template <template <typename> class Allocator>
        requires sparrow::allocator<Allocator<char>>
    arrow_schema_private_data<Allocator>::~arrow_schema_private_data()
    {
        for (const auto& child : m_children)
        {
            SPARROW_ASSERT_TRUE(child->release == nullptr)
            if (child->release != nullptr)
            {
                child->release(child.get());
            }
        }

        if (m_dictionary != nullptr)
        {
            SPARROW_ASSERT_TRUE(m_dictionary->release == nullptr)
            if (m_dictionary->release != nullptr)
            {
                m_dictionary->release(m_dictionary.get());
            }
        }
    }

    template <typename T>
    concept any_arrow_array = std::is_same_v<T, ArrowArray> || std::is_same_v<T, ArrowSchema>;

    /**
     * Deletes an ArrowArray.
     *
     * @tparam T The type of the buffers of the ArrowArray.
     * @tparam Allocator The allocator for the buffers of the ArrowArray.
     * @param array The ArrowArray to delete. Should be a valid pointer to an ArrowArray. Its release callback
     * should be set.
     */
    template <typename T, template <typename> typename Allocator>
        requires sparrow::allocator<Allocator<T>>
    void delete_array(ArrowArray* array)
    {
        SPARROW_ASSERT_FALSE(array == nullptr)

        array->buffers = nullptr;
        array->n_buffers = 0;
        array->length = 0;
        array->null_count = 0;
        array->offset = 0;
        array->n_children = 0;
        array->children = nullptr;
        array->dictionary = nullptr;
        const auto private_data = static_cast<arrow_array_private_data<T, Allocator>*>(array->private_data);
        delete private_data;
        array->private_data = nullptr;
        array->release = nullptr;
    }

    /**
     * Deletes an ArrowSchema.
     *
     * @tparam Allocator The allocator for the strings of the ArrowSchema.
     * @param schema The ArrowSchema to delete. Should be a valid pointer to an ArrowSchema. Its release
     * callback should be set.
     */
    template <template <typename> class Allocator>
        requires sparrow::allocator<Allocator<char>>
    void delete_schema(ArrowSchema* schema)
    {
        SPARROW_ASSERT_FALSE(schema == nullptr)
        SPARROW_ASSERT_TRUE(schema->release == std::addressof(delete_schema<Allocator>))

        schema->flags = 0;
        schema->n_children = 0;
        schema->children = nullptr;
        schema->dictionary = nullptr;
        schema->name = nullptr;
        schema->format = nullptr;
        schema->metadata = nullptr;

        const auto private_data = static_cast<arrow_schema_private_data<Allocator>*>(schema->private_data);
        delete private_data;
        schema->private_data = nullptr;
        schema->release = nullptr;
    }

    enum class ArrowFlag : int64_t
    {
        DICTIONARY_ORDERED = 1,  // For dictionary-encoded types, whether the ordering of dictionary indices
                                 // is semantically meaningful.
        NULLABLE = 2,            // Whether this field is semantically nullable (regardless of whether it
                                 // actually has null values).
        MAP_KEYS_SORTED = 4      // For map types, whether the keys within each map value are sorted.
    };

    inline arrow_schema_unique_ptr default_arrow_schema()
    {
        auto ptr = arrow_schema_unique_ptr(new ArrowSchema());
        ptr->format = nullptr;
        ptr->name = nullptr;
        ptr->metadata = nullptr;
        ptr->flags = 0;
        ptr->n_children = 0;
        ptr->children = nullptr;
        ptr->dictionary = nullptr;
        ptr->release = nullptr;
        ptr->private_data = nullptr;
        return ptr;
    }

    /**
     * Creates an ArrowSchema.
     *
     * @tparam Allocator The allocator for the strings of the ArrowSchema.
     * @param format A mandatory, null-terminated, UTF8-encoded string describing the data type. If the data
     *               type is nested, child types are not encoded here but in the ArrowSchema.children
     *               structures.
     * @param name An optional (nullptr), null-terminated, UTF8-encoded string of the field or array name.
     *             This is mainly used to reconstruct child fields of nested types.
     * @param metadata An optional (nullptr), binary string describing the type’s metadata. If the data type
     *                 is nested, the metadata for child types are not encoded here but in the
     * ArrowSchema.children structures.
     * @param flags A bitfield of flags enriching the type description. Its value is computed by OR’ing
     *              together the flag values.
     * @param children Vector of unique pointer of ArrowSchema. Children must not be null.
     * @param dictionary Optional. A pointer to the type of dictionary values. Must be present if the
     *                   ArrowSchema represents a dictionary-encoded type. Must be nullptr otherwise.
     * @return The created ArrowSchema.
     */
    template <template <typename> class Allocator>
        requires sparrow::allocator<Allocator<char>>
    arrow_schema_unique_ptr make_arrow_schema(
        std::string_view format,
        std::string_view name,
        std::optional<std::span<char>> metadata,
        std::optional<ArrowFlag> flags,
        std::vector<arrow_schema_unique_ptr>&& children,
        arrow_schema_unique_ptr&& dictionary
    )
    {
        SPARROW_ASSERT_FALSE(format.empty())
        SPARROW_ASSERT_TRUE(std::ranges::none_of(
            children,
            [](const auto& child)
            {
                return child == nullptr;
            }
        ))

        arrow_schema_unique_ptr schema = default_arrow_schema();

        schema->flags = flags.has_value() ? static_cast<int64_t>(flags.value()) : 0;

        schema->private_data = new arrow_schema_private_data<Allocator>(
            format,
            name,
            metadata,
            std::move(children),
            std::move(dictionary)
        );

        const auto private_data = static_cast<arrow_schema_private_data<Allocator>*>(schema->private_data);
        schema->format = private_data->m_format.data();
        if (!private_data->m_name.empty())
        {
            schema->name = private_data->m_name.data();
        }
        if (private_data->m_metadata.has_value() && !(*(private_data->m_metadata)).empty())
        {
            schema->metadata = (*private_data->m_metadata).data();
        }
        schema->n_children = static_cast<int64_t>(private_data->m_children_raw_ptr_vec.size());
        schema->children = private_data->m_children_raw_ptr_vec.data();
        schema->dictionary = private_data->m_dictionary.get();
        schema->release = delete_schema<Allocator>;
        return schema;
    };

    inline arrow_array_unique_ptr default_arrow_array()
    {
        auto ptr = arrow_array_unique_ptr(new ArrowArray());
        ptr->length = 0;
        ptr->null_count = 0;
        ptr->offset = 0;
        ptr->n_buffers = 0;
        ptr->n_children = 0;
        ptr->buffers = nullptr;
        ptr->children = nullptr;
        ptr->dictionary = nullptr;
        ptr->release = nullptr;
        ptr->private_data = nullptr;
        return ptr;
    }

    /**
     * Creates an ArrowArray.
     *
     * @tparam T The type of the buffers of the ArrowArray
     * @tparam Allocator The allocator for the buffers of the ArrowArray.
     * @param length The logical length of the array (i.e. its number of items). Must be 0 or positive.
     * @param null_count The number of null items in the array. May be -1 if not yet computed. Must be 0 or
     * positive otherwise.
     * @param offset The logical offset inside the array (i.e. the number of items from the physical start of
     *               the buffers). Must be 0 or positive.
     * @param n_buffers The number of physical buffers backing this array. The number of buffers is a
     *                  function of the data type, as described in the Columnar format specification, except
     *                  for the the binary or utf-8 view type, which has one additional buffer compared to the
     *                  Columnar format specification (see Binary view arrays). Must be 0 or positive.
     * @param children Vector of child arrays. Children must not be null.
     * @param dictionary A pointer to the underlying array of dictionary values. Must be present if the
     *                   ArrowArray represents a dictionary-encoded array. Must be null otherwise.
     * @return The created ArrowArray.
     */
    template <class T, template <typename> class Allocator, std::ranges::input_range R>
        requires sparrow::allocator<Allocator<T>> && std::is_integral_v<std::ranges::range_value_t<R>>
    arrow_array_unique_ptr make_arrow_array(
        int64_t length,
        int64_t null_count,
        int64_t offset,
        const R& buffer_sizes,
        std::vector<arrow_array_unique_ptr>&& children,
        arrow_array_unique_ptr dictionary
    )
    {
        SPARROW_ASSERT_TRUE(length >= 0)
        SPARROW_ASSERT_TRUE(null_count >= -1)
        SPARROW_ASSERT_TRUE(offset >= 0)
        SPARROW_ASSERT_TRUE(std::ranges::none_of(
            children,
            [](const auto& child)
            {
                return child == nullptr;
            }
        ))

        arrow_array_unique_ptr array = default_arrow_array();

        array->private_data = new arrow_array_private_data<T, Allocator>(
            std::move(children),
            std::move(dictionary),
            buffer_sizes
        );

        array->length = length;
        array->null_count = null_count;
        array->offset = offset;
        array->n_buffers = static_cast<int64_t>(buffer_sizes.size());

        const auto private_data = static_cast<arrow_array_private_data<T, Allocator>*>(array->private_data);
        T** buffer_data = private_data->m_buffers_raw_ptr_vec.data();
        const T** const_buffer_ptr = const_cast<const int**>(buffer_data);
        array->buffers = reinterpret_cast<const void**>(const_buffer_ptr);
        array->n_children = static_cast<int64_t>(private_data->m_children_raw_ptr_vec.size());
        array->children = private_data->m_children_raw_ptr_vec.data();
        array->dictionary = private_data->m_dictionary.get();
        array->release = delete_array<T, Allocator>;
        return array;
    }

}  // namespace sparrow
