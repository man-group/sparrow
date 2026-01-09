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

#include "sparrow/buffer/buffer_view.hpp"
#include "sparrow/buffer/bit_vector/bit_vector_base.hpp"

namespace sparrow
{
    /**
     * @class bit_vector_view
     *
     * A non-owning view to a bit-packed boolean sequence stored in external memory.
     *
     * This class provides a lightweight, non-owning interface to manipulate bit-packed
     * boolean data stored in external memory buffers. Unlike bit_vector, this class
     * does not manage memory allocation or deallocation.
     *
     * Use this for accessing bit-packed boolean data values. For validity/null tracking,
     * use validity_bitmap_view instead.
     *
     * @tparam T The integer type used to store the bits. Must satisfy std::integral.
     *
     * @warning The user is responsible for ensuring that the viewed memory remains valid
     *          for the lifetime of the view object.
     */
    template <std::integral T>
    class bit_vector_view : public bit_vector_base<buffer_view<T>>
    {
    public:

        using base_type = bit_vector_base<buffer_view<T>>;
        using storage_type = typename base_type::storage_type;
        using block_type = typename base_type::block_type;
        using size_type = typename base_type::size_type;

        constexpr bit_vector_view(block_type* p, size_type n);

        constexpr ~bit_vector_view() = default;
        constexpr bit_vector_view(const bit_vector_view&) = default;
        constexpr bit_vector_view(bit_vector_view&&) noexcept = default;

        constexpr bit_vector_view& operator=(const bit_vector_view&) = default;
        constexpr bit_vector_view& operator=(bit_vector_view&&) noexcept = default;
    };

    template <std::integral T>
    constexpr bit_vector_view<T>::bit_vector_view(block_type* p, size_type n)
        : base_type(storage_type(p, p != nullptr ? this->compute_block_count(n) : 0), n)
    {
    }
}
