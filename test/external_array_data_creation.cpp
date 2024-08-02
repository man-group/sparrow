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
}

