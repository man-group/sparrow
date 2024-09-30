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

        arr.buffers = nullptr;

        arr.children = new ArrowArray*[children_arrays.size()];
        for (std::size_t i = 0; i < children_arrays.size(); ++i)
        {
            arr.children[i] = &children_arrays[i];
        }

        arr.dictionary = nullptr;
        arr.release = &release_arrow_array;
    }

}

