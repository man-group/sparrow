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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or mplied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "external_array_data_creation.hpp"

namespace sparrow::test
{
    namespace detail
    {
        template <class T>
        void release_common_arrow(T* t)
        {
            if (t->dictionary)
            {
                delete t->dictionary;
                t->dictionary = nullptr;
            }

            for (std::int64_t i = 0; i < t->n_children; ++i)
            {
                t->children[i]->release(t->children[i]);
            }
            delete[] t->children;
            t->children = nullptr;

            t->release = nullptr;
        }
    }

    void release_arrow_schema(ArrowSchema* schema)
    {
        detail::release_common_arrow(schema);
    }

    void release_arrow_array(ArrowArray* arr)
    {
        for (std::int64_t i = 0; i < arr->n_buffers; ++i)
        {
            delete[] reinterpret_cast<const std::uint8_t*>(arr->buffers[i]);
        }
        delete[] reinterpret_cast<const std::uint8_t**>(arr->buffers);
        arr->buffers = nullptr;
        detail::release_common_arrow(arr);
    }


    void fill_schema_and_array_for_list_layout(
        ArrowSchema& schema,
        ArrowArray& arr,
        ArrowSchema & flat_value_schema,
        ArrowArray & flat_value_arr,
        const std::vector<std::size_t> & list_lengths,
        const std::vector<std::size_t> & false_postions,
        bool big_list
    ){
        schema.format = big_list ? "+L" : "+l";
        schema.name = "test";
        schema.metadata = "test metadata";

        schema.n_children = 1;
        schema.children = new ArrowSchema*[1];
        schema.children[0] = &flat_value_schema;

        schema.dictionary = nullptr;
        schema.release = &release_arrow_schema;


        arr.length = static_cast<std::int64_t>(list_lengths.size());
        arr.null_count = static_cast<std::int64_t>(false_postions.size());
        arr.offset = 0;

        arr.n_buffers = 2;
        arr.n_children = 1;

        std::uint8_t** buf = new std::uint8_t*[2];
        buf[0] = make_bitmap_buffer(static_cast<std::size_t>(arr.length), false_postions);

        buf[1] = make_offset_buffer_from_sizes(list_lengths, big_list);

        arr.buffers = const_cast<const void**>(reinterpret_cast<void**>(buf));

        arr.children = new ArrowArray*[1];
        arr.children[0] = &flat_value_arr;

        arr.dictionary = nullptr;
        arr.release = &release_arrow_array;

    }




    void fill_schema_and_array_for_fixed_size_list_layout(
        ArrowSchema& schema,
        ArrowArray& arr,
        ArrowSchema & flat_value_schema,
        ArrowArray & flat_value_arr,
        const std::vector<std::size_t> & false_postions,
        std::size_t list_size
    ){
        SPARROW_ASSERT(list_size > 0, "list size must be greater than 0");
        SPARROW_ASSERT(list_size < 10, "just a test limitation st. format string can be on stack");
        // convert list size to string
        std::string list_size_str = std::to_string(list_size);
        schema.format = new char[5]{'+', 'w',':', list_size_str[0], '\0'};

        schema.name = "test";
        schema.metadata = "test metadata";

        schema.n_children = 1;
        schema.children = new ArrowSchema*[1];
        schema.children[0] = &flat_value_schema;

        schema.dictionary = nullptr;
        schema.release = &release_arrow_schema;


        arr.length = static_cast<std::int64_t>(static_cast<std::uint64_t>(flat_value_arr.length) / list_size);
        arr.null_count = static_cast<std::int64_t>(false_postions.size());
        arr.offset = 0;

        arr.n_buffers = 1;
        arr.n_children = 1;

        std::uint8_t** buf = new std::uint8_t*[1];
        buf[0] = make_bitmap_buffer(static_cast<std::size_t>(arr.length), false_postions);
        arr.buffers = const_cast<const void**>(reinterpret_cast<void**>(buf));

        arr.children = new ArrowArray*[1];
        arr.children[0] = &flat_value_arr;

        arr.dictionary = nullptr;
        arr.release = &release_arrow_array;

    }


    void fill_schema_and_array_for_list_view_layout(
        ArrowSchema& schema,
        ArrowArray& arr,
        ArrowSchema & flat_value_schema,
        ArrowArray & flat_value_arr,
        const std::vector<std::size_t> & list_lengths,
        const std::vector<std::size_t> & false_postions,
        bool big_list
    ){
        schema.format = big_list ? "+vL" : "+vl";
        schema.name = "test";
        schema.metadata = "test metadata";

        schema.n_children = 1;
        schema.children = new ArrowSchema*[1];
        schema.children[0] = &flat_value_schema;

        schema.dictionary = nullptr;
        schema.release = &release_arrow_schema;


        arr.length = static_cast<std::int64_t>(list_lengths.size());
        arr.null_count = static_cast<std::int64_t>(false_postions.size());
        arr.offset = 0;

        arr.n_buffers = 3;
        arr.n_children = 1;

        std::uint8_t** buf = new std::uint8_t*[3];
        buf[0] = make_bitmap_buffer(static_cast<std::size_t>(arr.length), false_postions);

        buf[1] = make_offset_buffer_from_sizes(list_lengths, big_list);
        buf[2] = new std::uint8_t[list_lengths.size()  * (big_list ? sizeof(std::uint64_t) : sizeof(std::uint32_t))];

        // ignore -Werror=cast-align]
        #ifdef __GNUC__
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wcast-align"
        #endif

        if(big_list)
        {
            std::uint64_t* size_buf = reinterpret_cast<std::uint64_t*>(buf[2]);
            for(std::size_t i = 0; i < list_lengths.size(); ++i)
            {
                size_buf[i] = list_lengths[i];
            }
        }
        else{
            std::uint32_t* size_buf = reinterpret_cast<std::uint32_t*>(buf[2]);
            for(std::size_t i = 0; i < list_lengths.size(); ++i)
            {
                size_buf[i] = static_cast<std::uint32_t>(list_lengths[i]);
            }
        }
        #ifdef __GNUC__
        #pragma GCC diagnostic pop
        #endif
        arr.buffers = const_cast<const void**>(reinterpret_cast<void**>(buf));

        arr.children = new ArrowArray*[1];
        arr.children[0] = &flat_value_arr;

        arr.dictionary = nullptr;
        arr.release = &release_arrow_array;

    }

    void fill_schema_and_array_for_struct_layout(
        ArrowSchema& schema,
        ArrowArray& arr,
        std::vector<ArrowSchema> & children_schemas,
        std::vector<ArrowArray> & children_arrays,
        const std::vector<std::size_t> & false_postions
    )
    {
        schema.format = "+s";
        schema.name = "test";
        schema.metadata = "test metadata";

        schema.n_children = static_cast<std::int64_t>(children_schemas.size());
        schema.children = new ArrowSchema*[children_schemas.size()];
        for (std::size_t i = 0; i < children_schemas.size(); ++i)
        {
            schema.children[i] = &children_schemas[i];
        }

        schema.dictionary = nullptr;
        schema.release = &release_arrow_schema;
        
        arr.length = children_arrays.front().length;

        arr.null_count = static_cast<std::int64_t>(false_postions.size());
        arr.offset = 0;

        arr.n_buffers = 1;
        std::uint8_t** buf = new std::uint8_t*[2];
        buf[0] = make_bitmap_buffer(static_cast<std::size_t>(arr.length), false_postions);

        arr.n_children = static_cast<std::int64_t>(children_arrays.size());

        arr.buffers = const_cast<const void**>(reinterpret_cast<void**>(buf));

        arr.children = new ArrowArray*[children_arrays.size()];
        for (std::size_t i = 0; i < children_arrays.size(); ++i)
        {
            arr.children[i] = &children_arrays[i];
        }

        arr.dictionary = nullptr;
        arr.release = &release_arrow_array;
    }

    void fill_schema_and_array_for_run_end_encoded(
        ArrowSchema& schema,
        ArrowArray& arr,
        ArrowSchema &  acc_length_schema,
        ArrowArray &   acc_length_arr,
        ArrowSchema &  value_schema,
        ArrowArray &   value_arr,
        std::size_t  length
    ){
        schema.format = "+r";
        schema.name = "test";
        schema.metadata = "test metadata";

        schema.n_children = 2;
        schema.children = new ArrowSchema*[2];
        schema.children[0] = &acc_length_schema;
        schema.children[1] = &value_schema;

        schema.dictionary = nullptr;
        schema.release = &release_arrow_schema;

        arr.length = static_cast<std::int64_t>(length);


        arr.null_count = 0;
        arr.offset = 0;

        arr.n_buffers = 0;
        arr.n_children = 2;

        arr.buffers = nullptr;

        arr.children = new ArrowArray*[2];
        arr.children[0] = &acc_length_arr;
        arr.children[1] = &value_arr;

        arr.dictionary = nullptr;
        arr.release = &release_arrow_array;

    }

    void fill_schema_and_array_for_sparse_union(
        ArrowSchema& schema,
        ArrowArray& arr,
        std::vector<ArrowSchema>  & children_schemas,
        std::vector<ArrowArray>   & children_arrays,
        const std::vector<std::uint8_t> & type_ids,
        const std::string & format
    ){
        schema.format = format.c_str();
        schema.name = "test";
        schema.metadata = "test metadata";

        schema.n_children = static_cast<std::int64_t>(children_schemas.size());
        schema.children = new ArrowSchema*[children_schemas.size()];
        for (std::size_t i = 0; i < children_schemas.size(); ++i)
        {
            schema.children[i] = &children_schemas[i];
        }

        schema.dictionary = nullptr;
        schema.release = &release_arrow_schema;

        arr.length = static_cast<std::int64_t>(type_ids.size());

        arr.null_count = 0;
        arr.offset = 0;

        arr.n_buffers = 1;
        std::uint8_t** buf = new std::uint8_t*[1];
        buf[0] = new std::uint8_t[type_ids.size()];
        std::copy(type_ids.begin(), type_ids.end(), buf[0]);

        arr.n_children = static_cast<std::int64_t>(children_arrays.size());

        arr.buffers = const_cast<const void**>(reinterpret_cast<void**>(buf));

        arr.children = new ArrowArray*[children_arrays.size()];
        for (std::size_t i = 0; i < children_arrays.size(); ++i)
        {
            arr.children[i] = &children_arrays[i];
        }

        arr.dictionary = nullptr;
        arr.release = &release_arrow_array;
    }

    void fill_schema_and_array_for_dense_union(
        ArrowSchema& schema,
        ArrowArray& arr,
        std::vector<ArrowSchema>  & children_schemas,
        std::vector<ArrowArray>   & children_arrays,
        const std::vector<std::uint8_t> & type_ids,
        const std::vector<std::int32_t> & offsets,
        const std::string & format
    ){
        schema.format = format.c_str();
        schema.name = "test";
        schema.metadata = "test metadata";

        schema.n_children = static_cast<std::int64_t>(children_schemas.size());
        schema.children = new ArrowSchema*[children_schemas.size()];
        for (std::size_t i = 0; i < children_schemas.size(); ++i)
        {
            schema.children[i] = &children_schemas[i];
        }

        schema.dictionary = nullptr;
        schema.release = &release_arrow_schema;

        arr.length = static_cast<std::int64_t>(type_ids.size());

        arr.null_count = 0;
        arr.offset = 0;

        arr.n_buffers = 2;
        std::uint8_t** buf = new std::uint8_t*[2];
        
        buf[0] = new std::uint8_t[type_ids.size()];
        std::copy(type_ids.begin(), type_ids.end(), buf[0]);

        buf[1] = new std::uint8_t[offsets.size() * sizeof(std::int32_t)];
        std::copy(offsets.begin(), offsets.end(), reinterpret_cast<std::int32_t*>(buf[1]));


        arr.n_children = static_cast<std::int64_t>(children_arrays.size());

        arr.buffers = const_cast<const void**>(reinterpret_cast<void**>(buf));

        arr.children = new ArrowArray*[children_arrays.size()];
        for (std::size_t i = 0; i < children_arrays.size(); ++i)
        {
            arr.children[i] = &children_arrays[i];
        }

        arr.dictionary = nullptr;
        arr.release = &release_arrow_array;
    }

}

