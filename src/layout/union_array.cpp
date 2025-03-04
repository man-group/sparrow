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

#include "sparrow/layout/union_array.hpp"

#include "sparrow/array.hpp"
#include "sparrow/utils/metadata.hpp"
#include "sparrow/utils/repeat_container.hpp"

namespace sparrow
{
    /************************************
     * dense_union_array implementation *
     ************************************/

#ifdef __GNUC__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-align"
#endif

    dense_union_array::dense_union_array(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , p_offsets(reinterpret_cast<std::int32_t*>(m_proxy.buffers()[1 /*index of offsets*/].data()))
    {
    }

    dense_union_array::dense_union_array(const dense_union_array& rhs)
        : dense_union_array(rhs.m_proxy)
    {
    }

    dense_union_array& dense_union_array::operator=(const dense_union_array& rhs)
    {
        if (this != &rhs)
        {
            base_type::operator=(rhs);
            p_offsets = reinterpret_cast<std::int32_t*>(m_proxy.buffers()[1 /*index of offsets*/].data());
        }
        return *this;
    }

#ifdef __GNUC__
#    pragma GCC diagnostic pop
#endif

    std::size_t dense_union_array::element_offset(std::size_t i) const
    {
        return static_cast<std::size_t>(p_offsets[i]) + m_proxy.offset();
    }

    /*************************************
     * sparse_union_array implementation *
     *************************************/

    sparse_union_array::sparse_union_array(arrow_proxy proxy)
        : base_type(std::move(proxy))
    {
    }

    std::size_t sparse_union_array::element_offset(std::size_t i) const
    {
        return i + m_proxy.offset();
    }

    auto sparse_union_array::create_proxy_impl(
        std::vector<array>&& children,
        type_id_buffer_type&& element_type,
        std::string&& format,
        type_id_map&& tim
    ) -> arrow_proxy
    {
        const auto n_children = children.size();
        ArrowSchema** child_schemas = new ArrowSchema*[n_children];
        ArrowArray** child_arrays = new ArrowArray*[n_children];
        const auto size = element_type.size();

        // count nulls (expensive!)
        int64_t null_count = 0;
        for (std::size_t i = 0; i < size; ++i)
        {
            // child_id from type_id
            const auto type_id = static_cast<std::uint8_t>(element_type[i]);
            const auto child_index = tim[type_id];
            // check if child is null
            if (!children[child_index][i].has_value())
            {
                ++null_count;
            }
        }

        for (std::size_t i = 0; i < n_children; ++i)
        {
            auto& child = children[i];
            auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(child));
            child_arrays[i] = new ArrowArray(std::move(flat_arr));
            child_schemas[i] = new ArrowSchema(std::move(flat_schema));
        }

        ArrowSchema schema = make_arrow_schema(
            std::move(format),
            std::nullopt,                                 // name
            std::optional<std::vector<metadata_pair>>{},  // metadata
            std::nullopt,                                 // flags,
            child_schemas,                                // children
            repeat_view<bool>(true, n_children),          // children_ownership
            nullptr,                                      // dictionary,
            true                                          // dictionary ownership
        );

        std::vector<buffer<std::uint8_t>> arr_buffs = {std::move(element_type).extract_storage()};

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<std::int64_t>(null_count),
            0,  // offset
            std::move(arr_buffs),
            child_arrays,                         // children
            repeat_view<bool>(true, n_children),  // children_ownership
            nullptr,                              // dictionary
            true
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }
}
