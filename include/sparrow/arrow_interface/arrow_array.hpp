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

#include <cstddef>
#include <cstdint>

#include "sparrow/arrow_interface/arrow_array/private_data.hpp"
#include "sparrow/arrow_interface/arrow_array/smart_pointers.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/config/config.hpp"

namespace sparrow
{
    /**
     * Creates a unique pointer to an `ArrowArray`.
     *
     * This function creates a unique pointer to an Arrow array with the specified parameters.
     *
     * @tparam B Value, reference or rvalue of `std::vector<sparrow::buffer<uint8_t>>`
     * @param length The logical length of the array (i.e. its number of items). Must be 0 or positive.
     * @param null_count The number of null items in the array. May be `-1` if not yet computed. Must be 0 or
     * positive otherwise.
     * @param offset The logical offset inside the array (i.e. the number of items from the physical start of
     *               the buffers). Must be 0 or positive.
     * @param n_buffers The number of physical buffers backing this array. The number of buffers is a
     *                  function of the data type, as described in the Columnar format specification, except
     *                  for the the binary or utf-8 view type, which has one additional buffer compared to
     *                  the Columnar format specification (see Binary view arrays). Must be 0 or positive.
     * @param buffers Vector of `sparrow::buffer`
     * @param children Pointer to a sequence of `ArrowArray` pointers or `nullptr`. Must be `nullptr` if
     * `n_children` is `0`.
     * @param dictionary `ArrowArray` pointer or `nullptr`.
     * @return The created `arrow_array_unique_ptr`.
     */
    template <class B>
        requires std::constructible_from<arrow_array_private_data::BufferType, B>
    arrow_array_unique_ptr make_arrow_array_unique_ptr(
        int64_t length,
        int64_t null_count,
        int64_t offset,
        int64_t n_buffers,
        B buffers,
        size_t n_children,
        ArrowArray** children,
        ArrowArray* dictionary
    );

    /**
     * Creates a unique pointer to an `ArrowArray`.
     *
     * This function creates a unique pointer to an `ArrowArray` with the specified parameters.
     *
     * @tparam B Value, reference or rvalue of `std::vector<sparrow::buffer<uint8_t>>`
     * @param length The logical length of the array (i.e. its number of items). Must be `0` or positive.
     * @param null_count The number of null items in the array. May be -1 if not yet computed. Must be 0 or
     * positive otherwise.
     * @param offset The logical offset inside the array (i.e. the number of items from the physical start of
     *               the buffers). Must be `0` or positive.
     * @param buffers Vector of `sparrow::buffer<uint8_t>`.
     * @param children Pointer to a sequence of `ArrowArray` pointers or `nullptr`. Must be `nullptr` if
     * `n_children` is `0`.
     * @param dictionary `ArrowArray` pointer or `nullptr`.
     * @return The created `arrow_array_unique_ptr`.
     */
    template <class B>
        requires std::constructible_from<arrow_array_private_data::BufferType, B>
    arrow_array_unique_ptr make_arrow_array_unique_ptr(
        int64_t length,
        int64_t null_count,
        int64_t offset,
        B buffers,
        size_t n_children,
        ArrowArray** children,
        ArrowArray* dictionary
    );

    /**
     * Creates an `ArrowArray`.
     *
     * @tparam B Value, reference or rvalue of `std::vector<sparrow::buffer<uint8_t>>`
     * @param length The logical length of the array (i.e. its number of items). Must be `0` or positive.
     * @param null_count The number of null items in the array. May be `-1` if not yet computed. Must be `0`
     * or positive otherwise.
     * @param offset The logical offset inside the array (i.e. the number of items from the physical start of
     *               the buffers). Must be `0` or positive.
     * @param buffers Vector of `sparrow::buffer<uint8_t>`.
     * @param children Pointer to a sequence of `ArrowArray` pointers or nullptr. Must be `nullptr` if
     * `n_children` is `0`.
     * @param dictionary `ArrowArray` pointer or `nullptr`.
     * @return The created `ArrowArray`.
     */
    template <class B>
        requires std::constructible_from<arrow_array_private_data::BufferType, B>
    ArrowArray make_arrow_array(
        int64_t length,
        int64_t null_count,
        int64_t offset,
        B buffers,
        size_t n_children,
        ArrowArray** children,
        ArrowArray* dictionary
    );

    /**
     * All integers are set to 0 and pointers to `nullptr`.
     * The `ArrowArray` is in an invalid state and should not bu used as is.
     *
     * @return The created `ArrowArray`.
     */
    SPARROW_API arrow_array_unique_ptr default_arrow_array_unique_ptr();

    /**
     * Release function to use for the `ArrowArray.release` member.
     */
    SPARROW_API void release_arrow_array(ArrowArray* array);

    /**
     * Fill an `ArrowArray` object.
     *
     * @tparam B Value, reference or rvalue of `std::vector<sparrow::buffer<uint8_t>>`
     * @param array The `ArrowArray` to fill.
     * @param length The logical length of the array (i.e. its number of items). Must be 0 or positive.
     * @param null_count The number of null items in the array. May be -1 if not yet computed. Must be 0 or
     * positive otherwise.
     * @param offset The logical offset inside the array (i.e. the number of items from the physical start of
     *               the buffers). Must be 0 or positive.
     * @param buffers Vector of `sparrow::buffer<uint8_t>`.
     * @param children Pointer to a sequence of `ArrowArray` pointers or `nullptr`. Must be `nullptr` if
     * `n_children` is `0`.
     * @param dictionary `ArrowArray` pointer or `nullptr`.
     */
    template <class B>
        requires std::constructible_from<arrow_array_private_data::BufferType, B>
    void fill_arrow_array(
        ArrowArray& array,
        int64_t length,
        int64_t null_count,
        int64_t offset,
        B buffers,
        size_t n_children,
        ArrowArray** children,
        ArrowArray* dictionary
    )
    {
        SPARROW_ASSERT_TRUE(length >= 0);
        SPARROW_ASSERT_TRUE(null_count >= -1);
        SPARROW_ASSERT_TRUE(offset >= 0);
        SPARROW_ASSERT_TRUE((n_children == 0) == (children == nullptr));

        array.length = length;
        array.null_count = null_count;
        array.offset = offset;
        array.n_buffers = sparrow::ssize(buffers);
        array.private_data = new arrow_array_private_data(std::move(buffers));
        const auto private_data = static_cast<arrow_array_private_data*>(array.private_data);
        array.buffers = private_data->buffers_ptrs<void>();
        array.n_children = static_cast<int64_t>(n_children);
        array.children = children;
        array.dictionary = dictionary;
        array.release = release_arrow_array;
    }

    template <class B>
        requires std::constructible_from<arrow_array_private_data::BufferType, B>
    arrow_array_unique_ptr make_arrow_array_unique_ptr(
        int64_t length,
        int64_t null_count,
        int64_t offset,
        int64_t n_buffers,
        B buffers,
        size_t n_children,
        ArrowArray** children,
        ArrowArray* dictionary
    )
    {
        SPARROW_ASSERT_TRUE(length >= 0);
        SPARROW_ASSERT_TRUE(null_count >= -1);
        SPARROW_ASSERT_TRUE(offset >= 0);
        SPARROW_ASSERT_TRUE(n_buffers >= 0);
        SPARROW_ASSERT_TRUE((n_children == 0) == (children == nullptr));

        arrow_array_unique_ptr array = default_arrow_array_unique_ptr();
        fill_arrow_array(*array, length, null_count, offset, std::move(buffers), n_children, children, dictionary);
        return array;
    }

    template <class B>
        requires std::constructible_from<arrow_array_private_data::BufferType, B>
    arrow_array_unique_ptr make_arrow_array_unique_ptr(
        int64_t length,
        int64_t null_count,
        int64_t offset,
        B buffers,
        size_t n_children,
        ArrowArray** children,
        ArrowArray* dictionary
    )
    {
        SPARROW_ASSERT_TRUE(length >= 0);
        SPARROW_ASSERT_TRUE(null_count >= -1);
        SPARROW_ASSERT_TRUE(offset >= 0);
        SPARROW_ASSERT_TRUE(buffers.size() >= 0);
        SPARROW_ASSERT_TRUE((n_children == 0) == (children == nullptr));

        const int64_t buffer_count = sparrow::ssize(buffers);
        return make_arrow_array_unique_ptr<B>(
            length,
            null_count,
            offset,
            buffer_count,
            std::move(buffers),
            n_children,
            std::move(children),
            std::move(dictionary)
        );
    }

    template <class B>
        requires std::constructible_from<arrow_array_private_data::BufferType, B>
    ArrowArray make_arrow_array(
        int64_t length,
        int64_t null_count,
        int64_t offset,
        B buffers,
        size_t n_children,
        ArrowArray** children,
        ArrowArray* dictionary
    )
    {
        SPARROW_ASSERT_TRUE(length >= 0);
        SPARROW_ASSERT_TRUE(null_count >= -1);
        SPARROW_ASSERT_TRUE(offset >= 0);
        SPARROW_ASSERT_TRUE(buffers.size() >= 0);
        SPARROW_ASSERT_TRUE((n_children == 0) == (children == nullptr));

        ArrowArray array{};
        fill_arrow_array(array, length, null_count, offset, std::move(buffers), n_children, children, dictionary);
        return array;
    }

    inline arrow_array_unique_ptr default_arrow_array_unique_ptr()
    {
        return arrow_array_unique_ptr(new ArrowArray{});
    }

    SPARROW_API void release_arrow_array(ArrowArray* array);

    SPARROW_API std::vector<sparrow::buffer_view<uint8_t>>
    get_arrow_array_buffers(const ArrowArray& array, const ArrowSchema& schema);

    /**
     * Fill the target ArrowArray with a deep copy of the data from the source ArrowArray.
     */
    SPARROW_API void
    copy_array(const ArrowArray& source_array, const ArrowSchema& source_schema, ArrowArray& target);

    /**
     * Create a deep copy of the source ArrowArray. The buffers, children and dictionary are deep copied.
     */
    inline ArrowArray copy_array(const ArrowArray& source_array, const ArrowSchema& source_schema)
    {
        ArrowArray target{};
        copy_array(source_array, source_schema, target);
        return target;
    }
}
