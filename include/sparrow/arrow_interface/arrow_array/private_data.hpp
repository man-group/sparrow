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

#include "sparrow/arrow_interface/arrow_array_schema_utils.hpp"
#include "sparrow/buffer/buffer.hpp"
#include "sparrow/buffer/buffer_view.hpp"
#include "sparrow/utils/contracts.hpp"

namespace sparrow
{

    /**
     * Private data for ArrowArray.
     *
     * Holds and own buffers, children, and dictionary.
     * It is used in the Sparrow library.
     */

    class arrow_array_private_data : public children_ownership
    {
    public:

        using BufferType = std::vector<buffer<std::uint8_t>>;

        explicit constexpr arrow_array_private_data(BufferType buffers, std::size_t children_size = 0);

        [[nodiscard]] constexpr BufferType& buffers() noexcept;
        [[nodiscard]] constexpr const BufferType& buffers() const noexcept;

        constexpr void resize_buffers(std::size_t size);
        void set_buffer(std::size_t index, buffer<std::uint8_t>&& buffer);
        void set_buffer(std::size_t index, const buffer_view<std::uint8_t>& buffer);
        constexpr void resize_buffer(std::size_t index, std::size_t size, std::uint8_t value);
        constexpr void update_buffers_ptrs();

        template <class T>
        [[nodiscard]] constexpr const T** buffers_ptrs() noexcept;

    private:

        BufferType m_buffers;
        std::vector<std::uint8_t*> m_buffers_pointers;

    };

    constexpr arrow_array_private_data::arrow_array_private_data(BufferType buffers, std::size_t children_size)
        : children_ownership(children_size)
        , m_buffers(std::move(buffers))
        , m_buffers_pointers(to_raw_ptr_vec<std::uint8_t>(m_buffers))
    {
    }

    [[nodiscard]] constexpr std::vector<buffer<std::uint8_t>>& arrow_array_private_data::buffers() noexcept
    {
        return m_buffers;
    }

    [[nodiscard]] constexpr const std::vector<buffer<std::uint8_t>>&
    arrow_array_private_data::buffers() const noexcept
    {
        return m_buffers;
    }

    constexpr void arrow_array_private_data::resize_buffers(std::size_t size)
    {
        m_buffers.resize(size);
        update_buffers_ptrs();
    }

    inline void arrow_array_private_data::set_buffer(std::size_t index, buffer<std::uint8_t>&& buffer)
    {
        SPARROW_ASSERT_TRUE(index < m_buffers.size());
        m_buffers[index] = std::move(buffer);
        update_buffers_ptrs();
    }

    inline void arrow_array_private_data::set_buffer(std::size_t index, const buffer_view<std::uint8_t>& buffer)
    {
        SPARROW_ASSERT_TRUE(index < m_buffers.size());
        m_buffers[index] = buffer;
        update_buffers_ptrs();
    }

    constexpr void
    arrow_array_private_data::resize_buffer(std::size_t index, std::size_t size, std::uint8_t value)
    {
        SPARROW_ASSERT_TRUE(index < m_buffers.size());
        m_buffers[index].resize(size, value);
        update_buffers_ptrs();
    }

    template <class T>
    [[nodiscard]] constexpr const T** arrow_array_private_data::buffers_ptrs() noexcept
    {
        return const_cast<const T**>(reinterpret_cast<T**>(m_buffers_pointers.data()));
    }

    constexpr void arrow_array_private_data::update_buffers_ptrs()
    {
        m_buffers_pointers = to_raw_ptr_vec<std::uint8_t>(m_buffers);
    }
}
