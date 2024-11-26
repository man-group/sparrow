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


    template<class T>
    auto static_const_ptr_cast(const void *ptr)
    {
        return const_cast<T*>(static_cast<const T*>(ptr));
    }

    std::vector<sparrow::buffer_view<uint8_t>>
    get_arrow_array_buffers_new(const ArrowArray& array, const ArrowSchema& schema)
    {   
        using buffer_view_type = sparrow::buffer_view<uint8_t>;

        auto make_valid_buffer = [](auto buffer_ptr, const auto array_length)
        {   
            // const cast 
            const auto buffer_size = static_cast<size_t>(array_length + 7) / 8;
            auto typed_buffer_ptr = static_const_ptr_cast<uint8_t>(buffer_ptr);
            return typed_buffer_ptr != nullptr ? buffer_view_type(typed_buffer_ptr, buffer_size) : buffer_view_type(nullptr, 0);
        };



        std::vector<buffer_view_type> buffers;
        const auto buffer_count = static_cast<size_t>(array.n_buffers);
        buffers.reserve(buffer_count);
        const enum data_type data_type = format_to_data_type(schema.format);
        const auto length = static_cast<size_t>(array.length);
        switch(data_type)
        {   
            // no buffers
            case data_type::NA:
            case data_type::RUN_ENCODED:
                return buffers;

            // primitive types
            case data_type::BOOL:   
                const auto compact_size = (length + 7) / 8;
                buffers.emplace_back(make_valid_buffer(array.buffers[0], length));
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[1]), compact_size);
            case data_type::UINT8:
            case data_type::INT8:
                buffers.emplace_back(make_valid_buffer(array.buffers[0], length));
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[1]), length);
                break;
            case data_type::UINT16:
            case data_type::INT16:
            case data_type::HALF_FLOAT:
                buffers.emplace_back(make_valid_buffer(array.buffers[0], length));
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[1]), length * 2);
                break;
            case data_type::UINT32:
            case data_type::INT32:
            case data_type::FLOAT:
                buffers.emplace_back(make_valid_buffer(array.buffers[0], length));
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[1]), length * 4);
                break;
            case data_type::UINT64:
            case data_type::INT64:
            case data_type::DOUBLE:
                buffers.emplace_back(make_valid_buffer(array.buffers[0], length));
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[1]), length * 8);
                break;

            // strings and binary
            case data_type::STRING:
            case data_type::BINARY:
                buffers.emplace_back(make_valid_buffer(array.buffers[0], length));
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[1]), (length + 1) * 4);
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[2]), length);
                break;
            // large strings and binary
            case data_type::LARGE_STRING:
            case data_type::LARGE_BINARY:
                buffers.emplace_back(make_valid_buffer(array.buffers[0], length));
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[1]), length * 8);
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[2]), length);
                break;
            
            // list
            case data_type::LIST:
                buffers.emplace_back(make_valid_buffer(array.buffers[0], length));
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[1]), (length + 1) * 4);
                break;
            // large list
            case data_type::LARGE_LIST:
                buffers.emplace_back(make_valid_buffer(array.buffers[0], length));
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[1]), (length + 1) * 8);
                break;
            
            // list view
            case data_type::LIST_VIEW:
                buffers.emplace_back(make_valid_buffer(array.buffers[0], length));
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[1]), length * 4);
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[2]), length * 4);
                break;
            // large list view
            case data_type::LARGE_LIST_VIEW:
                buffers.emplace_back(make_valid_buffer(array.buffers[0], length));
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[1]), length * 8);
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[2]), length * 8);
                break;
            

            // layouts with only validity buffer
            // fixed size list
            // struct
            case data_type::FIXED_SIZE_LIST:
            case data_type::STRUCT:
                buffers.emplace_back(make_valid_buffer(array.buffers[0], length));
                break;

            // sparse union
            case data_type::SPARSE_UNION:
                // type ids
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[0]), length);
                break;
            // dense union
            case data_type::DENSE_UNION:
                // type ids
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[0]), length);
                // offsets
                buffers.emplace_back(static_const_ptr_cast<uint8_t>(array.buffers[1]), length * 4);
                break;
            





            
        }
    }











    std::vector<sparrow::buffer_view<uint8_t>>
    get_arrow_array_buffers(const ArrowArray& array, const ArrowSchema& schema)
    {
        std::vector<sparrow::buffer_view<uint8_t>> buffers;
        const auto buffer_count = static_cast<size_t>(array.n_buffers);
        buffers.reserve(buffer_count);
        const enum data_type data_type = format_to_data_type(schema.format);
        const auto buffers_type = get_buffer_types_from_data_type(data_type);
        SPARROW_ASSERT_TRUE(buffers_type.size() == buffer_count);
        for (std::size_t i = 0; i < buffer_count; ++i)
        {
            const auto buffer_type = buffers_type[i];
            auto buffer = array.buffers[i];
            const std::size_t buffer_size = compute_buffer_size(
                buffer_type,
                static_cast<size_t>(array.length),
                static_cast<size_t>(array.offset),
                data_type,
                buffers,
                i == 0 ? buffer_type : buffers_type[i - 1]
            );
            auto* ptr = static_cast<uint8_t*>(const_cast<void*>(buffer));
            buffers.emplace_back(ptr, buffer_size);
        }
        return buffers;
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
