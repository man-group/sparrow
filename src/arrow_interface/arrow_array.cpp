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

#include "sparrow/arrow_interface/arrow_array.hpp"

#include "sparrow/arrow_interface/arrow_array_schema_info_utils.hpp"
#include "sparrow/layout/fixed_width_binary_layout/fixed_width_binary_array_utils.hpp"
#include "sparrow/types/data_type.hpp"

namespace sparrow
{
    void release_arrow_array(ArrowArray* array)
    {
        SPARROW_ASSERT_FALSE(array == nullptr)
        SPARROW_ASSERT_TRUE(array->release == std::addressof(release_arrow_array))

        release_common_arrow(*array);
        if (array->private_data != nullptr)
        {
            const auto private_data = static_cast<arrow_array_private_data*>(array->private_data);
            delete private_data;
            array->private_data = nullptr;
        }
        array->buffers = nullptr;  // The buffers were deleted with the private data
    }

    void empty_release_arrow_array(ArrowArray* array)
    {
        SPARROW_ASSERT_FALSE(array == nullptr);
        SPARROW_ASSERT_TRUE(array->release == std::addressof(empty_release_arrow_array));
    }

    template <class T>
    auto static_const_ptr_cast(const void* ptr)
    {
        return const_cast<T*>(static_cast<const T*>(ptr));
    }

    sparrow::buffer_view<uint8_t> get_bitmap_buffer(const ArrowArray& array)
    {
        using buffer_view_type = sparrow::buffer_view<uint8_t>;
        const auto size = static_cast<size_t>(array.length + array.offset);
        const auto buffer_size = static_cast<size_t>(size + 7) / 8;
        auto typed_buffer_ptr = static_const_ptr_cast<uint8_t>(array.buffers[0]);
        return typed_buffer_ptr != nullptr ? buffer_view_type(typed_buffer_ptr, buffer_size)
                                           : buffer_view_type(nullptr, 0);
    }

    std::vector<sparrow::buffer_view<uint8_t>>
    get_arrow_array_buffers(const ArrowArray& array, const ArrowSchema& schema)
    {
        using buffer_view_type = sparrow::buffer_view<uint8_t>;
        const auto size = static_cast<size_t>(array.length + array.offset);
        auto make_valid_buffer = [&]()
        {
            return get_bitmap_buffer(array);
        };
        auto make_buffer = [&](auto index, auto size)
        {
            auto buffer_ptr = static_const_ptr_cast<uint8_t>(array.buffers[index]);
            return buffer_view_type(buffer_ptr, size);
        };

        switch (format_to_data_type(schema.format))
        {
            case data_type::NA:
            case data_type::RUN_ENCODED:
            case data_type::MAP:
                return {};
            case data_type::BOOL:
                return {make_valid_buffer(), make_buffer(1, (size + 7) / 8)};
            case data_type::UINT8:
            case data_type::INT8:
                return {make_valid_buffer(), make_buffer(1, size)};
            case data_type::UINT16:
            case data_type::INT16:
            case data_type::HALF_FLOAT:
                return {make_valid_buffer(), make_buffer(1, size * 2)};
            case data_type::UINT32:
            case data_type::INT32:
            case data_type::FLOAT:
                return {make_valid_buffer(), make_buffer(1, size * 4)};
            case data_type::UINT64:
            case data_type::INT64:
            case data_type::DOUBLE:
                return {make_valid_buffer(), make_buffer(1, size * 8)};
            case data_type::STRING:
            case data_type::BINARY:
                return {
                    make_valid_buffer(),
                    make_buffer(1, (size + 1) * 4),
                    make_buffer(2, static_const_ptr_cast<int32_t>(array.buffers[1])[size])
                };
            case data_type::LARGE_STRING:
            case data_type::LARGE_BINARY:
                return {
                    make_valid_buffer(),
                    make_buffer(1, (size + 1) * 4),
                    make_buffer(2, static_const_ptr_cast<int64_t>(array.buffers[1])[size])
                };
            case data_type::LIST:
                return {make_valid_buffer(), make_buffer(1, (size + 1) * 4)};
            case data_type::LARGE_LIST:
                return {make_valid_buffer(), make_buffer(1, (size + 1) * 8)};
            case data_type::LIST_VIEW:
                return {make_valid_buffer(), make_buffer(1, size * 4), make_buffer(2, size * 4)};
            case data_type::LARGE_LIST_VIEW:
                return {make_valid_buffer(), make_buffer(1, size * 8), make_buffer(2, size * 8)};
            case data_type::FIXED_SIZED_LIST:
            case data_type::STRUCT:
                return {make_valid_buffer()};
            case data_type::SPARSE_UNION:
                return {make_buffer(0, size)};
            case data_type::DENSE_UNION:
                return {make_buffer(0, size), make_buffer(1, size * 4)};
            case data_type::DATE_DAYS:
                return {make_valid_buffer(), make_buffer(1, size * 4)};
            case data_type::DATE_MILLISECONDS:
            case data_type::TIMESTAMP_SECONDS:
            case data_type::TIMESTAMP_MILLISECONDS:
            case data_type::TIMESTAMP_MICROSECONDS:
            case data_type::TIMESTAMP_NANOSECONDS:
            case data_type::DURATION_SECONDS:
            case data_type::DURATION_MILLISECONDS:
            case data_type::DURATION_MICROSECONDS:
            case data_type::DURATION_NANOSECONDS:
                return {make_valid_buffer(), make_buffer(1, size * 8)};
            case data_type::INTERVAL_MONTHS:
                return {make_valid_buffer(), make_buffer(1, size * 4)};
            case data_type::INTERVAL_DAYS_TIME:
                return {make_valid_buffer(), make_buffer(1, size * 8)};
            case sparrow::data_type::INTERVAL_MONTHS_DAYS_NANOSECONDS:
                return {make_valid_buffer(), make_buffer(1, size * 16)};
            case data_type::DECIMAL32:
                return {make_valid_buffer(), make_buffer(1, size * 4)};
            case data_type::DECIMAL64:
                return {make_valid_buffer(), make_buffer(1, size * 8)};
            case data_type::DECIMAL128:
                return {make_valid_buffer(), make_buffer(1, size * 16)};
            case data_type::DECIMAL256:
                return {make_valid_buffer(), make_buffer(1, size * 32)};
            case data_type::FIXED_WIDTH_BINARY:
                return {
                    make_valid_buffer(),
                    make_buffer(1, size * num_bytes_for_fixed_sized_binary(schema.format))
                };
            case data_type::STRING_VIEW:
            case data_type::BINARY_VIEW:
                const auto buffer_count = static_cast<size_t>(array.n_buffers);
                const auto num_extra_data_buffers = buffer_count - 3;
                std::vector<buffer_view_type> buffers(buffer_count);
                int64_t* var_buffer_sizes = static_const_ptr_cast<int64_t>(array.buffers[buffer_count - 1]);
                buffers[0] = make_valid_buffer();
                buffers[1] = make_buffer(1, size * 16);
                for (size_t i = 0; i < num_extra_data_buffers; ++i)
                {
                    buffers[i + 2] = make_buffer(i + 2, var_buffer_sizes[i]);
                }
                buffers.back() = make_buffer(buffer_count - 1, size * 4);
                return buffers;
        }
        // To avoid stupid warning "control reaches end of non-void function"
        return {};
    }

    void swap(ArrowArray& lhs, ArrowArray& rhs)
    {
        std::swap(lhs.length, rhs.length);
        std::swap(lhs.null_count, rhs.null_count);
        std::swap(lhs.offset, rhs.offset);
        std::swap(lhs.n_buffers, rhs.n_buffers);
        std::swap(lhs.n_children, rhs.n_children);
        std::swap(lhs.buffers, rhs.buffers);
        std::swap(lhs.children, rhs.children);
        std::swap(lhs.dictionary, rhs.dictionary);
        std::swap(lhs.release, rhs.release);
        std::swap(lhs.private_data, rhs.private_data);
    }

    void copy_array(const ArrowArray& source_array, const ArrowSchema& source_schema, ArrowArray& target)
    {
        SPARROW_ASSERT_TRUE(&source_array != &target);
        SPARROW_ASSERT_TRUE(source_array.release != nullptr);
        SPARROW_ASSERT_TRUE(source_schema.release != nullptr);
        SPARROW_ASSERT_TRUE(source_array.n_children == source_schema.n_children);
        SPARROW_ASSERT_TRUE((source_array.dictionary == nullptr) == (source_schema.dictionary == nullptr));

        target.n_children = source_array.n_children;
        if (source_array.n_children > 0)
        {
            target.children = new ArrowArray*[static_cast<std::size_t>(source_array.n_children)];
            for (int64_t i = 0; i < source_array.n_children; ++i)
            {
                SPARROW_ASSERT_TRUE(source_array.children[i] != nullptr);
                target.children[i] = new ArrowArray{};
                copy_array(*source_array.children[i], *source_schema.children[i], *target.children[i]);
            }
        }

        if (source_array.dictionary != nullptr)
        {
            target.dictionary = new ArrowArray{};
            copy_array(*source_array.dictionary, *source_schema.dictionary, *target.dictionary);
        }

        target.length = source_array.length;
        target.null_count = source_array.null_count;
        target.offset = source_array.offset;
        target.n_buffers = source_array.n_buffers;


        std::vector<buffer<std::uint8_t>> buffers_copy;
        buffers_copy.reserve(static_cast<std::size_t>(source_array.n_buffers));
        const auto buffers = get_arrow_array_buffers(source_array, source_schema);
        SPARROW_ASSERT_TRUE(buffers.size() == static_cast<std::size_t>(source_array.n_buffers));
        for (const auto& buffer : buffers)
        {
            buffers_copy.emplace_back(buffer);
        }
        target.private_data = new arrow_array_private_data(
            std::move(buffers_copy),
            static_cast<std::size_t>(target.n_children)
        );
        const auto private_data = static_cast<arrow_array_private_data*>(target.private_data);
        target.buffers = private_data->buffers_ptrs<void>();
        target.release = release_arrow_array;
    }
}
