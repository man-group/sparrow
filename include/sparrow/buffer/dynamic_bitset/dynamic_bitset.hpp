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

#include "sparrow/buffer/buffer.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset_base.hpp"

namespace sparrow
{
    /**
     * @class dynamic_bitset
     *
     * This class represents a dynamic size sequence of bits.
     *
     * @tparam T the integer type used to store the bits.
     */
    template <std::integral T>
    class dynamic_bitset : public dynamic_bitset_base<buffer<T>>
    {
    public:

        using base_type = dynamic_bitset_base<buffer<T>>;
        using storage_type = typename base_type::storage_type;
        using block_type = typename base_type::block_type;
        using value_type = typename base_type::value_type;
        using size_type = typename base_type::size_type;

        constexpr dynamic_bitset();
        constexpr explicit dynamic_bitset(size_type n);
        constexpr dynamic_bitset(size_type n, value_type v);
        constexpr dynamic_bitset(block_type* p, size_type n);
        constexpr dynamic_bitset(block_type* p, size_type n, size_type null_count);

        constexpr ~dynamic_bitset() = default;
        constexpr dynamic_bitset(const dynamic_bitset&) = default;
        constexpr dynamic_bitset(dynamic_bitset&&) noexcept = default;

        constexpr dynamic_bitset& operator=(const dynamic_bitset&) = default;
        constexpr dynamic_bitset& operator=(dynamic_bitset&&) noexcept = default;

        using base_type::clear;
        using base_type::emplace;
        using base_type::erase;
        using base_type::insert;
        using base_type::pop_back;
        using base_type::push_back;
        using base_type::resize;
    };

    template <std::integral T>
    constexpr dynamic_bitset<T>::dynamic_bitset()
        : base_type(storage_type(), 0u)
    {
    }

    template <std::integral T>
    constexpr dynamic_bitset<T>::dynamic_bitset(size_type n)
        : dynamic_bitset(n, false)
    {
    }

    template <std::integral T>
    constexpr dynamic_bitset<T>::dynamic_bitset(size_type n, value_type value)
        : base_type(
              storage_type(this->compute_block_count(n), value ? block_type(~block_type(0)) : block_type(0)),
              n,
              value ? 0u : n
          )
    {
    }

    template <std::integral T>
    constexpr dynamic_bitset<T>::dynamic_bitset(block_type* p, size_type n)
        : base_type(storage_type(p, this->compute_block_count(n)), n)
    {
    }

    template <std::integral T>
    constexpr dynamic_bitset<T>::dynamic_bitset(block_type* p, size_type n, size_type null_count)
        : base_type(storage_type(p, this->compute_block_count(n)), n, null_count)
    {
    }
}
