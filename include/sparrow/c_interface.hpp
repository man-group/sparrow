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


extern "C"
{
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

}  // extern "C"

namespace sparrow
{
    template <template <typename> class Allocator = std::allocator, class T>
    std::vector<T*, Allocator<T*>> to_raw_ptr_vec(const std::vector<std::unique_ptr<T>>& vec)
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
            std::vector<std::unique_ptr<ArrowArray>> children,
            std::unique_ptr<ArrowArray> dictionary,
            const V& buffer_sizes
        )
            : buffers_(
                  create_buffers<BufferType>(buffer_sizes, buffer_allocator_, buffers_allocator_),
                  buffers_allocator_
              )
            , buffers_raw_ptr_vec_(to_raw_ptr_vec<Allocator>(buffers_))
            , children_(std::move(children))
            , children_raw_ptr_vec_(to_raw_ptr_vec<Allocator>(children_))
            , dictionary_(std::move(dictionary))
        {
        }

        using buffer_allocator = Allocator<BufferType>;
        using buffers_allocator = Allocator<buffer<BufferType>>;

        buffer_allocator buffer_allocator_ = buffer_allocator();
        buffers_allocator buffers_allocator_ = buffers_allocator();

        std::vector<buffer<BufferType>, buffers_allocator> buffers_;
        std::vector<BufferType*> buffers_raw_ptr_vec_;

        std::vector<std::unique_ptr<ArrowArray>> children_;
        std::vector<ArrowArray*> children_raw_ptr_vec_;

        std::unique_ptr<ArrowArray> dictionary_;
    };

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

        explicit arrow_schema_private_data(
            std::string_view format_view,
            std::string_view name_view,
            const std::optional<std::span<char>>& metadata,
            std::vector<std::unique_ptr<ArrowSchema>> children,
            std::unique_ptr<ArrowSchema> dictionary
        )
            : format_(format_view)
            , name_(name_view)
            , metadata_(
                  metadata.has_value() ? vector_string_type(metadata->begin(), metadata->end())
                                       : vector_string_type()
              )
            , children_(std::move(children))
            , children_raw_ptr_vec_(to_raw_ptr_vec(children_))
            , dictionary_(std::move(dictionary))
        {
        }

        ~arrow_schema_private_data()
        {
            for (const auto& child : children_)
            {
                SPARROW_ASSERT_TRUE(child->release == nullptr)
                if (child->release != nullptr)
                {
                    child->release(child.get());
                }
            }

            if (dictionary_ != nullptr)
            {
                SPARROW_ASSERT_TRUE(dictionary_->release == nullptr)
                if (dictionary_->release != nullptr)
                {
                    dictionary_->release(dictionary_.get());
                }
            }
        }

        any_allocator<char> string_allocator_ = Allocator<char>();

        string_type format_;
        string_type name_;
        std::optional<vector_string_type> metadata_;

        std::vector<std::unique_ptr<ArrowSchema>> children_;
        std::vector<ArrowSchema*> children_raw_ptr_vec_;

        std::unique_ptr<ArrowSchema> dictionary_;
    };

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
        // #if !(defined(__clang__) && __clang_major__ < 15)
        // SPARROW_ASSERT_TRUE(array->release == std::addressof(delete_array<T, Allocator>))
        // #endif

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
    std::unique_ptr<ArrowSchema, arrow_schema_custom_deleter> make_arrow_schema(
        std::string_view format,
        std::string_view name,
        std::optional<std::span<char>> metadata,
        std::optional<ArrowFlag> flags,
        std::vector<std::unique_ptr<ArrowSchema>>&& children,
        std::unique_ptr<ArrowSchema>&& dictionary
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

        std::unique_ptr<ArrowSchema, arrow_schema_custom_deleter> schema(new ArrowSchema());
        schema->flags = flags.has_value() ? static_cast<int64_t>(flags.value()) : 0;

        schema->private_data = new arrow_schema_private_data<Allocator>(
            format,
            name,
            metadata,
            std::move(children),
            std::move(dictionary)
        );

        const auto private_data = static_cast<arrow_schema_private_data<Allocator>*>(schema->private_data);
        schema->format = private_data->format_.data();
        if (!private_data->name_.empty())
        {
            schema->name = private_data->name_.data();
        }
        if (private_data->metadata_.has_value() && !(*(private_data->metadata_)).empty())
        {
            schema->metadata = (*private_data->metadata_).data();
        }
        schema->n_children = static_cast<int64_t>(private_data->children_raw_ptr_vec_.size());
        schema->children = private_data->children_raw_ptr_vec_.data();
        schema->dictionary = private_data->dictionary_.get();
        schema->release = delete_schema<Allocator>;
        return schema;
    };

    struct arrow_array_custom_deleter
    {
        void operator()(ArrowArray* array) const
        {
            if (array->release != nullptr)
            {
                array->release(array);
            }
            delete array;
        }
    };

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
    std::unique_ptr<ArrowArray, arrow_array_custom_deleter> make_arrow_array(
        int64_t length,
        int64_t null_count,
        int64_t offset,
        const R& buffer_sizes,
        std::vector<std::unique_ptr<ArrowArray>>&& children,
        std::unique_ptr<ArrowArray> dictionary
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

        std::unique_ptr<ArrowArray, arrow_array_custom_deleter> array(new ArrowArray());

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
        T** buffer_data = private_data->buffers_raw_ptr_vec_.data();
        const T** const_buffer_ptr = const_cast<const int**>(buffer_data);
        array->buffers = reinterpret_cast<const void**>(const_buffer_ptr);
        array->n_children = static_cast<int64_t>(private_data->children_raw_ptr_vec_.size());
        array->children = private_data->children_raw_ptr_vec_.data();
        array->dictionary = private_data->dictionary_.get();
        array->release = delete_array<T, Allocator>;
        return array;
    }

}  // namespace sparrow
