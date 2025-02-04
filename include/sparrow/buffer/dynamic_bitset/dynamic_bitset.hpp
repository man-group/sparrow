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

#include <type_traits>

#include "sparrow/buffer/buffer.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset_base.hpp"
#include "sparrow/utils/ranges.hpp"

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

        template <std::ranges::input_range R>
            requires std::convertible_to<std::ranges::range_value_t<R>, value_type>
        explicit dynamic_bitset(const R& r)
            : dynamic_bitset(std::ranges::size(r), true)
        {
            std::size_t i = 0;
            for (auto value : r)
            {
                if (!value)
                {
                    this->set(i, false);
                }
                i++;
            }
        }

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

    using validity_bitmap = dynamic_bitset<std::uint8_t>;

    namespace detail
    {
        using validity_bitmap = sparrow::validity_bitmap;

        inline validity_bitmap ensure_validity_bitmap_impl(std::size_t size, const validity_bitmap& bitmap)
        {
            if (bitmap.size() == 0)
            {
                return validity_bitmap(size, true);
            }
            return bitmap;  // copy
        }

        inline validity_bitmap ensure_validity_bitmap_impl(std::size_t size, validity_bitmap&& bitmap)
        {
            if (bitmap.size() == 0)
            {
                bitmap.resize(size, true);
            }
            return std::move(bitmap);
        }

        // range of booleans
        template <std::ranges::input_range R>
            requires(std::same_as<std::ranges::range_value_t<R>, bool>)
        validity_bitmap ensure_validity_bitmap_impl(std::size_t size, R&& range)
        {
            SPARROW_ASSERT_TRUE(size == range_size(range) || range_size(range) == 0);
            validity_bitmap bitmap(size, true);
            std::size_t i = 0;
            for (auto value : range)
            {
                if (!value)
                {
                    bitmap.set(i, false);
                }
                i++;
            }
            return bitmap;
        }

        // range of indices / integers (but not booleans)
        template <std::ranges::input_range R>
            requires(std::unsigned_integral<std::ranges::range_value_t<R>> && !std::same_as<std::ranges::range_value_t<R>, bool> && !std::same_as<std::decay_t<R>, validity_bitmap>)
        validity_bitmap ensure_validity_bitmap_impl(std::size_t size, R&& range_of_indices)
        {
            validity_bitmap bitmap(size, true);
            for (auto index : range_of_indices)
            {
                bitmap.set(index, false);
            }
            return bitmap;
        }
    }  // namespace detail

    template <class T>
    concept validity_bitmap_input = (std::same_as<T, validity_bitmap> || std::same_as<T, const validity_bitmap&>
                                     || (std::ranges::input_range<T>
                                         && std::same_as<std::ranges::range_value_t<T>, bool>)
                                     || (std::ranges::input_range<T>
                                         && std::unsigned_integral<std::ranges::range_value_t<T>>) )
                                    && (!std::same_as<std::remove_cvref_t<T>, std::string>
                                        && !std::same_as<std::remove_cvref_t<T>, std::string_view>
                                        && !std::same_as<std::decay_t<T>, const char*>);

    template <validity_bitmap_input R>
    validity_bitmap ensure_validity_bitmap(std::size_t size, R&& validity_input)
    {
        return detail::ensure_validity_bitmap_impl(size, std::forward<R>(validity_input));
    }

}  // namespace sparrow
