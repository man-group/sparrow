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
    template <std::integral T>
    class non_owning_dynamic_bitset : public dynamic_bitset_base<buffer<T>*>
    {
    public:

        using base_type = dynamic_bitset_base<buffer<T>*>;
        using storage_type = typename base_type::storage_type;
        using block_type = typename base_type::block_type;
        using value_type = typename base_type::value_type;
        using size_type = typename base_type::size_type;

        constexpr explicit non_owning_dynamic_bitset(buffer<T>* buffer, size_type n);

        constexpr ~non_owning_dynamic_bitset() = default;
        constexpr non_owning_dynamic_bitset(const non_owning_dynamic_bitset&) = default;
        constexpr non_owning_dynamic_bitset(non_owning_dynamic_bitset&&) noexcept = default;

        constexpr non_owning_dynamic_bitset& operator=(const non_owning_dynamic_bitset&) = default;
        constexpr non_owning_dynamic_bitset& operator=(non_owning_dynamic_bitset&&) noexcept = default;

        using base_type::clear;
        using base_type::emplace;
        using base_type::erase;
        using base_type::insert;
        using base_type::pop_back;
        using base_type::push_back;
        using base_type::resize;
    };

    template <std::integral T>
    constexpr non_owning_dynamic_bitset<T>::non_owning_dynamic_bitset(buffer<T>* buffer, size_type n)
        : base_type(buffer, n)
    {
    }

}