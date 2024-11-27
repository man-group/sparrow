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

    // get the bit width for fixed width binary from format
    std::size_t num_bytes_for_fixed_sized_binary(const char* format)
    {
        //    w:42   -> 42 bytes
        //    w:1    -> 1 bytes
        const auto width = std::atoi(format + 2);
        return static_cast<std::size_t>(width);
    }

    // get the bit width for decimal value type from format
    std::size_t num_bytes_for_decimal(const char* format)
    {
        //    d:19,10     -> 16 bytes / 128 bits
        //    d:38,10,32  -> 4 bytes / 32 bits
        //    d:38,10,64  -> 8 bytes / 64 bits
        //    d:38,10,128 -> 16 bytes / 128 bits
        //    d:38,10,256 -> 32 bytes / 256 bits

        // count the number of commas
        const auto len = std::strlen(format);
        const auto num_commas = std::count(format, format + std::strlen(format), ',');

        if(num_commas <= 1)
        {
            return 16;
        }
        else
        {
            // get the position of second comma
            const auto second_comma = std::strchr(format, ',') + 1;
            const auto num_bits = std::atoi(second_comma);
            SPARROW_ASSERT(num_bits == 32 || num_bits == 64 || num_bits == 128 || num_bits == 256);
            return num_bits / 8;
        }       
    }
    

    std::vector<sparrow::buffer_view<uint8_t>>
    get_arrow_array_buffers(const ArrowArray& array, const ArrowSchema& schema)
    {   
        using buffer_view_type = sparrow::buffer_view<uint8_t>;
        const auto size = static_cast<size_t>(array.length + array.offset);
        auto make_valid_buffer = [&]()
        {
            const auto buffer_size = static_cast<size_t>(size + 7) / 8;
            auto typed_buffer_ptr = static_const_ptr_cast<uint8_t>(array.buffers[0]);
            return typed_buffer_ptr != nullptr ? buffer_view_type(typed_buffer_ptr, buffer_size) : buffer_view_type(nullptr, 0);
        };
        auto make_buffer =  [](auto index, auto size)
        {
            auto buffer_ptr = static_const_ptr_cast<uint8_t>(array.buffers[index]);
        };

        switch(format_to_data_type(schema.format))
        {   
            case data_type::NA:
            case data_type::RUN_ENCODED:
                return {};
            case data_type::BOOL:   
                return {maske_valid_buffer(), make_buffer(1, (size + 7) / 8)};
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
                return {make_valid_buffer(), make_buffer(1, (size + 1) * 4), make_buffer(2, size)};
            case data_type::LARGE_STRING:
            case data_type::LARGE_BINARY:
                return {make_valid_buffer(), make_buffer(1, (size + 1) * 4), make_buffer(2, size)};
            case data_type::LIST:
                return {make_valid_buffer(), make_buffer(1, (size + 1) * 4)};
            case data_type::LARGE_LIST:
                return {make_valid_buffer(), make_buffer(1, (size + 1) * 8)};
            case data_type::LIST_VIEW:
                return {make_valid_buffer(), make_buffer(1, size * 4), make_buffer(2, size * 4)};
            case data_type::LARGE_LIST_VIEW:
                return {make_valid_buffer(), make_buffer(1, size * 8), make_buffer(2, size * 8)};
            case data_type::FIXED_SIZE_LIST:
            case data_type::STRUCT:
                return  {make_valid_buffer()};
            case data_type::SPARSE_UNION:
                return {make_buffer(0, size)};
            case data_type::DENSE_UNION:
                return {make_buffer(0, size), make_buffer(1, size*4)};
            case data_type::TIMESTAMP:
                return {make_valid_buffer(), make_buffer(1, size * 8)}; // CHECK
            case data_type::DECIMAL:
                return {make_valid_buffer(), make_buffer(1, size * 16)}; // stored as 128 bit integer
            case data_type::FIXED_WIDTH_BINARY:
                return {make_valid_buffer(), make_buffer(1, size *  num_bytes_for_fixed_sized_binary(schema.format))};
            // string view and binary view
            case data_type::STRING_VIEW:
            case data_type::BINARY_VIEW:
                // tricky since they have an arbitary number of buffers
                const auto buffer_count = static_cast<size_t>(array.n_buffers);
                const auto num_extra_data_buffers = buffer_count - 3; // {valid, length,...,buffer-length}
                std::vector<buffer_view_type> buffers(buffer_count);
                // the valid buffer is always the first one
                buffer[0] = make_valid_buffer();
                // the second buffer holds the length and
                // * data itself for short-strings
                // * offsets for long strings
                buffer[1] = make_buffer(1, size * 16); 
                // the last buffer is a buffer with the sizes    
                buffer[buffer_count - 1] = make_buffer(buffer_count - 1, size);

                for(size_t i = 0; i<num_extra_data_buffers; ++i)
                {
                    const auto buffer_size = buffer[buffer_count - 1][i];
                    buffer[i+2] = make_buffer(i+2, buffer_size);
                }
                return buffers;

        }
    }











    // std::vector<sparrow::buffer_view<uint8_t>>
    // get_arrow_array_buffers(const ArrowArray& array, const ArrowSchema& schema)
    // {
    //     std::vector<sparrow::buffer_view<uint8_t>> buffers;
    //     const auto buffer_count = static_cast<size_t>(array.n_buffers);
    //     buffers.reserve(buffer_count);
    //     const enum data_type data_type = format_to_data_type(schema.format);
    //     const auto buffers_type = get_buffer_types_from_data_type(data_type);
    //     SPARROW_ASSERT_TRUE(buffers_type.size() == buffer_count);
    //     for (std::size_t i = 0; i < buffer_count; ++i)
    //     {
    //         const auto buffer_type = buffers_type[i];
    //         auto buffer = array.buffers[i];
    //         const std::size_t buffer_size = compute_buffer_size(
    //             buffer_type,
    //             static_cast<size_t>(array.length),
    //             static_cast<size_t>(array.offset),
    //             data_type,
    //             buffers,
    //             i == 0 ? buffer_type : buffers_type[i - 1]
    //         );
    //         auto* ptr = static_cast<uint8_t*>(const_cast<void*>(buffer));
    //         buffers.emplace_back(ptr, buffer_size);
    //     }
    //     return buffers;
    // }

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
