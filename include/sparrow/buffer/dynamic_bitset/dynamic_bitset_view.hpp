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
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset_base.hpp"

namespace sparrow
{
    /**
     * @class dynamic_bitset_view
     *
     * This class represents a view to a dynamic size sequence of bits.
     *
     * @tparam T the integer type used to store the bits.
     */
    template <std::integral T>
    class dynamic_bitset_view : public dynamic_bitset_base<buffer_view<T>>
    {
    public:

        using base_type = dynamic_bitset_base<buffer_view<T>>;
        using storage_type = typename base_type::storage_type;
        using block_type = typename base_type::block_type;
        using size_type = typename base_type::size_type;

        constexpr dynamic_bitset_view(block_type* p, size_type n);
        constexpr dynamic_bitset_view(block_type* p, size_type n, size_type null_count);
        constexpr ~dynamic_bitset_view() = default;

        constexpr dynamic_bitset_view(const dynamic_bitset_view&) = default;
        constexpr dynamic_bitset_view(dynamic_bitset_view&&) noexcept = default;

        constexpr dynamic_bitset_view& operator=(const dynamic_bitset_view&) = default;
        constexpr dynamic_bitset_view& operator=(dynamic_bitset_view&&) noexcept = default;
    };

    template <std::integral T>
    constexpr dynamic_bitset_view<T>::dynamic_bitset_view(block_type* p, size_type n)
        : base_type(storage_type(p, this->compute_block_count(n)), n)
    {
    }

    template <std::integral T>
    constexpr dynamic_bitset_view<T>::dynamic_bitset_view(block_type* p, size_type n, size_type null_count)
        : base_type(storage_type(p, this->compute_block_count(n)), n, null_count)
    {
    }
}
