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
                std::cout<<"delete dictionary"<<std::endl;
                delete t->dictionary;
                t->dictionary = nullptr;
            }

            std::cout<<"delete n = "<<t->n_children<<" children"<<std::endl;
            for (std::int64_t i = 0; i < t->n_children; ++i)
            {
                std::cout<<"delete "<<i<<"-th child at address "<<t->children[i]<<std::endl;
                std::cout<<"addr of release = "<<t->children[i]->release<<std::endl;
                t->children[i]->release(t->children[i]);
            }
            std::cout<<"delete children array"<<std::endl;
            delete[] t->children;
            t->children = nullptr;

            t->release = nullptr;
        }
    }

    void release_arrow_schema(ArrowSchema* schema)
    {
        std::cout<<"release_arrow_schema"<<std::endl;
        detail::release_common_arrow(schema);
        std::cout<<"release_arrow_schema done"<<std::endl;        
    }

    void release_arrow_array(ArrowArray* arr)
    {
        std::cout<<"release_arrow_array with "<<arr->n_buffers<<" buffers"<<std::endl;
        for (std::int64_t i = 0; i < arr->n_buffers; ++i)
        {
            std::cout<<"delete buffer "<<i<<std::endl;
            delete[] reinterpret_cast<const std::uint8_t*>(arr->buffers[i]);
            std::cout<<"delete buffer "<<i<<" done"<<std::endl;
        }
        delete[] reinterpret_cast<const std::uint8_t**>(arr->buffers);
        arr->buffers = nullptr;
        std::cout<<"release common"<<std::endl;
        detail::release_common_arrow(arr);
        std::cout<<"release_arrow_array done"<<std::endl;
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


        std::cout<<"addres flat_value_arr = "<<&flat_value_arr<<std::endl;
        std::cout<<"addres flat_value_schema = "<<&flat_value_schema<<std::endl;

        std::cout<<"addres of releaser of flat_value_arr = "<<flat_value_arr.release<<std::endl;
        std::cout<<"addres 2 "<<arr.children[0]->release<<std::endl;
        std::cout<<"addres of expected function = "<<&release_arrow_array<<std::endl;
    }
}

