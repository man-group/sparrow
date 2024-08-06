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

#include <vector>

#include "sparrow/array/data_type.hpp"
#include "sparrow/buffer/buffer.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/memory.hpp"

namespace sparrow
{
    /**
     * Structure holding the raw data.
     *
     * array_data is meant to be used by the
     * different layout classes to implement
     * the array API, based on the type
     * specified in the type attribute.
     */
    struct array_data
    {
        using block_type = std::uint8_t;
        using bitmap_type = dynamic_bitset<block_type>;
        using buffer_type = buffer<block_type>;
        using length_type = std::int64_t;

        data_descriptor type;
        length_type length = 0;
        std::int64_t offset = 0;
        // bitmap buffer and null_count
        bitmap_type bitmap;
        // Other buffers
        std::vector<buffer_type> buffers;
        std::vector<array_data> child_data;
        value_ptr<array_data> dictionary;
    };

    constexpr data_descriptor type_descriptor(const array_data& data) noexcept;
    constexpr array_data::length_type length(const array_data& data) noexcept;
    constexpr std::int64_t offset(const array_data& data) noexcept;

    constexpr array_data::bitmap_type& bitmap(array_data& data) noexcept;
    constexpr const array_data::bitmap_type& bitmap(const array_data& data) noexcept;

    constexpr std::size_t buffers_size(const array_data& data) noexcept;
    constexpr void buffers_clear() noexcept;
    constexpr array_data::buffer_type& buffer_at(array_data& data, std::size_t i);
    constexpr const array_data::buffer_type& buffer_at(const array_data& data, std::size_t i);

    constexpr std::size_t child_data_size(const array_data& data) noexcept;
    constexpr void child_data_clear() noexcept;
    array_data& child_data_at(array_data& data, std::size_t i);
    const array_data& child_data_at(const array_data& data, std::size_t i);

    value_ptr<array_data>& dictionary(array_data& data);
    const value_ptr<array_data>& dictionary(const array_data& data);

    /***********************************
     * getter functions for array_data *
     ***********************************/

    constexpr data_descriptor type_descriptor(const array_data& data) noexcept
    {
        return data.type;
    }

    constexpr array_data::length_type length(const array_data& data) noexcept
    {
        return data.length;
    }

    constexpr std::int64_t offset(const array_data& data) noexcept
    {
        return data.offset;
    }

    constexpr array_data::bitmap_type& bitmap(array_data& data) noexcept
    {
        return data.bitmap;
    }

    constexpr const array_data::bitmap_type& bitmap(const array_data& data) noexcept
    {
        return data.bitmap;
    }

    constexpr std::size_t buffers_size(const array_data& data) noexcept
    {
        return data.buffers.size();
    }

    constexpr void buffers_clear(array_data& data) noexcept
    {
        data.buffers.clear();
    }

    constexpr array_data::buffer_type& buffer_at(array_data& data, std::size_t i)
    {
        SPARROW_ASSERT_TRUE(i < buffers_size(data));
        return data.buffers[i];
    }

    constexpr const array_data::buffer_type& buffer_at(const array_data& data, std::size_t i)
    {
        SPARROW_ASSERT_TRUE(i < buffers_size(data));
        return data.buffers[i];
    }

    constexpr std::size_t child_data_size(const array_data& data) noexcept
    {
        return data.child_data.size();
    }

    constexpr void child_data_clear(array_data& data) noexcept
    {
        data.child_data.clear();
    }

    inline array_data& child_data_at(array_data& data, std::size_t i)
    {
        SPARROW_ASSERT_TRUE(i < child_data_size(data));
        return data.child_data[i];
    }

    inline const array_data& child_data_at(const array_data& data, std::size_t i)
    {
        SPARROW_ASSERT_TRUE(i < child_data_size(data));
        return data.child_data[i];
    }

    inline value_ptr<array_data>& dictionary(array_data& data)
    {
        return data.dictionary;
    }

    inline const value_ptr<array_data>& dictionary(const array_data& data)
    {
        return data.dictionary;
    }
}
