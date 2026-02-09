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

#include "sparrow/union_array.hpp"

#include "sparrow/array.hpp"
#include "sparrow/debug/copy_tracker.hpp"
#include "sparrow/utils/metadata.hpp"
#include "sparrow/utils/repeat_container.hpp"

namespace sparrow
{
    namespace copy_tracker
    {
        template <>
        SPARROW_API std::string key<dense_union_array>()
        {
            return "dense_union_array";
        }

        template <>
        SPARROW_API std::string key<sparse_union_array>()
        {
            return "sparse_union_array";
        }
    }

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
        copy_tracker::increase(copy_tracker::key<dense_union_array>());
    }

    dense_union_array& dense_union_array::operator=(const dense_union_array& rhs)
    {
        copy_tracker::increase(copy_tracker::key<dense_union_array>());
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

    sparse_union_array::sparse_union_array(const sparse_union_array& rhs)
        : base_type(rhs)
    {
        copy_tracker::increase(copy_tracker::key<sparse_union_array>());
    }

    sparse_union_array& sparse_union_array::operator=(const sparse_union_array& rhs)
    {
        copy_tracker::increase(copy_tracker::key<sparse_union_array>());
        if (this != &rhs)
        {
            base_type::operator=(rhs);
        }
        return *this;
    }

    std::size_t sparse_union_array::element_offset(std::size_t i) const
    {
        return i + m_proxy.offset();
    }
}
