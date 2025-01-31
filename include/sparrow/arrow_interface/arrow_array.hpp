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
#if defined(__cpp_lib_format)
#    include <format>
#endif

#include "sparrow/arrow_interface/arrow_array/private_data.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/config/config.hpp"

namespace sparrow
{
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
     * Release function to use for the `ArrowArray.release` member.
     */
    SPARROW_API void release_arrow_array(ArrowArray* array);

    /**
     * Empty release function to use for the `ArrowArray.release` member. Should be used for view of
     * ArrowArray.
     */
    SPARROW_API void empty_release_arrow_array(ArrowArray* array);

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
        array.private_data = new arrow_array_private_data(std::move(buffers), n_children);
        const auto private_data = static_cast<arrow_array_private_data*>(array.private_data);
        array.buffers = private_data->buffers_ptrs<void>();
        array.n_children = static_cast<int64_t>(n_children);
        array.children = children;
        array.dictionary = dictionary;
        array.release = release_arrow_array;
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

    inline ArrowArray make_empty_arrow_array()
    {
        using buffer_type = arrow_array_private_data::BufferType;
        return make_arrow_array(0, 0, 0, buffer_type{}, 0u, nullptr, nullptr);
    }

    SPARROW_API void release_arrow_array(ArrowArray* array);

    SPARROW_API sparrow::buffer_view<uint8_t> get_bitmap_buffer(const ArrowArray& array);

    SPARROW_API std::vector<sparrow::buffer_view<uint8_t>>
    get_arrow_array_buffers(const ArrowArray& array, const ArrowSchema& schema);

    /**
     * Swaps the contents of the two ArrowArray objects.
     */
    SPARROW_API void swap(ArrowArray& lhs, ArrowArray& rhs);

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

    /**
     * Moves the content of source into a stack-allocated array, and
     * reset the source to an empty ArrowArray.
     */
    inline ArrowArray move_array(ArrowArray&& source)
    {
        ArrowArray target = make_empty_arrow_array();
        swap(source, target);
        return target;
    }

    /**
     * Moves the content of source into a stack-allocated array, and
     * reset the source to an empty ArrowArray.
     */
    inline ArrowArray move_array(ArrowArray& source)
    {
        return move_array(std::move(source));
    }
};

#if defined(__cpp_lib_format)

template <>
struct std::formatter<ArrowArray>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const ArrowArray& obj, std::format_context& ctx) const
    {
        std::string children_str = std::format("{}", static_cast<void*>(obj.children));
        for (int i = 0; i < obj.n_children; ++i)
        {
            children_str += std::format("\n-{}", static_cast<void*>(obj.children[i]));
        }

        std::string buffer_str = std::format("{}", static_cast<void*>(obj.buffers));
        for (int i = 0; i < obj.n_buffers; ++i)
        {
            buffer_str += std::format("\n\t- {}", obj.buffers[i]);
        }

        return std::format_to(
            ctx.out(),
            "ArrowArray - ptr address: {}\n- length: {}\n- null_count: {}\n- offset: {}\n- n_buffers: {}\n- buffers: {}\n- n_children: {}\n- children: {}\n- dictionary: {}\n",
            static_cast<const void*>(&obj),
            obj.length,
            obj.null_count,
            obj.offset,
            obj.n_buffers,
            buffer_str,
            obj.n_children,
            children_str,
            static_cast<const void*>(obj.dictionary)
        );
    }
};

inline std::ostream& operator<<(std::ostream& os, const ArrowArray& value)
{
    os << std::format("{}", value);
    return os;
}

#endif
