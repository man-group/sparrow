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
#include "sparrow/arrow_interface/arrow_schema/private_data.hpp"
#include "sparrow/c_interface.hpp"

namespace sparrow
{
    /**
     * Release the children and dictionnary of an `ArrowArray` or `ArrowSchema`.
     *
     * @tparam T `ArrowArray` or `ArrowSchema`
     * @param t The `ArrowArray` or `ArrowSchema` to release.
     */
    template <class T>
        requires std::same_as<T, ArrowArray> || std::same_as<T, ArrowSchema>
    void release_common_arrow(T& t)
    {
        using private_data_type = std::
            conditional_t<std::same_as<T, ArrowArray>, arrow_array_private_data, arrow_schema_private_data>;
        if (t.release == nullptr)
        {
            return;
        }
        SPARROW_ASSERT_TRUE(t.private_data != nullptr);
        const auto private_data = static_cast<const private_data_type*>(t.private_data);

        if (t.dictionary)
        {
            if (private_data->has_dictionary_ownership())
            {
                if (t.dictionary->release)
                {
                    t.dictionary->release(t.dictionary);
                }
                delete t.dictionary;
                t.dictionary = nullptr;
            }
        }

        if (t.children)
        {
            for (int64_t i = 0; i < t.n_children; ++i)
            {
                T* child = t.children[i];
                if (child)
                {
                    if (private_data->has_child_ownership(static_cast<std::size_t>(i)))
                    {
                        if (child->release)
                        {
                            child->release(child);
                        }
                        delete child;
                        child = nullptr;
                    }
                }
            }
            delete[] t.children;
            t.children = nullptr;
        }
        t.release = nullptr;
    }
}
