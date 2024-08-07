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

#include "sparrow/arrow_interface/arrow_array/private_data.hpp"

namespace sparrow
{
    /**
     * Creates an ArrowArray with provided data, with unique ownership.  
     * 
     * @tparam B Value, reference or rvalue of std::vector<sparrow::buffer<uint8_t>>
     * @tparam C Value, reference or rvalue of std::vector<arrow_array_shared_ptr>
     * @tparam D Value, reference or rvalue of arrow_array_shared_ptr
     * @param length The logical length of the array (i.e. its number of items). Must be 0 or positive.
     * @param null_count The number of null items in the array. May be -1 if not yet computed. Must be 0 or
     * positive otherwise.
     * @param offset The logical offset inside the array (i.e. the number of items from the physical start of
     *               the buffers). Must be 0 or positive.
     * @param n_buffers The number of physical buffers backing this array. The number of buffers is a
     *                  function of the data type, as described in the Columnar format specification, except
     *                  for the the binary or utf-8 view type, which has one additional buffer compared to
     *                  the Columnar format specification (see Binary view arrays). Must be 0 or positive.
     * @param buffers Vector of sparrow::buffer
     * @param children Vector of arrow_array_shared_ptr representing the children of the ArrowArray.
     * @param dictionary arrow_array_shared_ptr or nullptr.
     * @return The created ArrowArray.
     */
    template <class B, class C, class D>
        requires std::constructible_from<arrow_array_private_data::BufferType, B>
                 && std::constructible_from<arrow_array_private_data::ChildrenType, C>
                 && std::constructible_from<arrow_array_private_data::DictionaryType, D>
    arrow_array_unique_ptr make_arrow_array_unique_ptr(
        int64_t length,
        int64_t null_count,
        int64_t offset,
        int64_t n_buffers,
        B buffers,
        int64_t n_children,
        C children,
        D dictionary
    );

    /**
     * Creates a unique pointer to an Arrow array.
     *
     * This function creates a unique pointer to an Arrow array with the specified parameters.
     *
     * @tparam B Value, reference or rvalue of std::vector<sparrow::buffer<uint8_t>>
     * @tparam C Value, reference or rvalue of std::vector<arrow_array_shared_ptr>
     * @tparam D Value, reference or rvalue of arrow_array_shared_ptr
     * @param length The logical length of the array (i.e. its number of items). Must be 0 or positive.
     * @param null_count The number of null items in the array. May be -1 if not yet computed. Must be 0 or
     * positive otherwise.
     * @param offset The logical offset inside the array (i.e. the number of items from the physical start of
     *               the buffers). Must be 0 or positive.
     * @param buffers Vector of sparrow::buffer<uint8_t>.
     * @param children Vector of arrow_array_shared_ptr representing the children of the ArrowArray.
        * @param dictionary arrow_array_shared_ptr or nullptr.mp
     * @return The created ArrowArray.
     */
    template <class B, class C, class D>
        requires std::constructible_from<arrow_array_private_data::BufferType, B>
                 && std::constructible_from<arrow_array_private_data::ChildrenType, C>
                 && std::constructible_from<arrow_array_private_data::DictionaryType, D>
    arrow_array_unique_ptr
    make_arrow_array_unique_ptr(int64_t length, int64_t null_count, int64_t offset, B buffers, C children, D dictionary);

    /**
     * Creates a unique pointer to an ArrowArray with default values.
     * All integers are set to 0 and pointers to nullptr.
     * The ArrowArray is in an invalid state and should not bu used as is.
     *
     * @return The created ArrowArray.
     */
    arrow_array_unique_ptr default_arrow_array_unique_ptr();

    /**
     * Release function to use for the `ArrowArray.release` member.
     */
    void release_arrow_array(ArrowArray* array);

    template <class B, class C, class D>
        requires std::constructible_from<arrow_array_private_data::BufferType, B>
                 && std::constructible_from<arrow_array_private_data::ChildrenType, C>
                 && std::constructible_from<arrow_array_private_data::DictionaryType, D>
    arrow_array_unique_ptr make_arrow_array_unique_ptr(
        int64_t length,
        int64_t null_count,
        int64_t offset,
        int64_t n_buffers,
        B buffers,
        int64_t n_children,
        C children,
        D dictionary
    )
    {
        SPARROW_ASSERT_TRUE(length >= 0);
        SPARROW_ASSERT_TRUE(null_count >= -1);
        SPARROW_ASSERT_TRUE(offset >= 0);
        SPARROW_ASSERT_TRUE(n_buffers >= 0);
        SPARROW_ASSERT_TRUE(n_children >= 0);

        arrow_array_unique_ptr array = default_arrow_array_unique_ptr();

        array->length = length;
        array->null_count = null_count;
        array->offset = offset;
        array->n_buffers = n_buffers;

        array->private_data = new arrow_array_private_data(
            std::move(buffers),
            std::move(children),
            std::move(dictionary)
        );
        const auto private_data = static_cast<arrow_array_private_data*>(array->private_data);
        array->buffers = private_data->buffers_ptrs<void>();
        array->n_children = n_children;
        array->children = private_data->children_pointers();
        array->dictionary = private_data->dictionary_pointer();
        array->release = release_arrow_array;
        return array;
    }

    template <class B, class C, class D>
        requires std::constructible_from<arrow_array_private_data::BufferType, B>
                 && std::constructible_from<arrow_array_private_data::ChildrenType, C>
                 && std::constructible_from<arrow_array_private_data::DictionaryType, D>
    arrow_array_unique_ptr
    make_arrow_array_unique_ptr(int64_t length, int64_t null_count, int64_t offset, B buffers, C children, D dictionary)
    {
        const int64_t buffer_count = sparrow::ssize(buffers);
        const int64_t children_count =  sparrow::ssize(children);
        return make_arrow_array_unique_ptr<B, C, D>(
            length,
            null_count,
            offset,
            buffer_count,
            std::move(buffers),
            children_count,
            std::move(children),
            std::move(dictionary)
        );
    }

    inline arrow_array_unique_ptr default_arrow_array_unique_ptr()
    {
        return arrow_array_unique_ptr(new ArrowArray{});
    }

    inline void release_arrow_array(ArrowArray* array)
    {
        SPARROW_ASSERT_FALSE(array == nullptr)
        SPARROW_ASSERT_TRUE(array->release == std::addressof(release_arrow_array))

        array->buffers = nullptr;
        array->n_buffers = 0;
        array->length = 0;
        array->null_count = 0;
        array->offset = 0;
        array->n_children = 0;
        array->children = nullptr;
        array->dictionary = nullptr;
        if (array->private_data != nullptr)
        {
            const auto private_data = static_cast<arrow_array_private_data*>(array->private_data);
            delete private_data;
        }
        array->private_data = nullptr;
        array->release = nullptr;
    }
}
