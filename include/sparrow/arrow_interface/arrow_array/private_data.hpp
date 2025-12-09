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
#include "sparrow/arrow_interface/private_data_ownership.hpp"
#include "sparrow/buffer/buffer.hpp"
#include "sparrow/buffer/buffer_view.hpp"
#include "sparrow/config/config.hpp"
#include "sparrow/utils/contracts.hpp"

namespace sparrow
{

    /**
     * Private data for ArrowArray.
     *
     * Holds and own buffers, children, and dictionary.
     * It is used in the Sparrow library.
     */

    class SPARROW_API arrow_array_private_data : public children_ownership,
                                     public dictionary_ownership
    {
    public:

        using any_buffer = std::variant<buffer<std::uint8_t>, buffer_view<const std::uint8_t>>;
        using BufferType = std::vector<any_buffer>;

        template <std::ranges::input_range CHILDREN_OWNERSHIP>
            requires std::is_same_v<std::ranges::range_value_t<CHILDREN_OWNERSHIP>, bool>
        explicit constexpr arrow_array_private_data(
            BufferType buffers,
            const CHILDREN_OWNERSHIP& children_ownership,
            bool dictionary_ownership
        );

        [[nodiscard]] constexpr BufferType& buffers() noexcept;
        [[nodiscard]] constexpr const BufferType& buffers() const noexcept;

        SPARROW_CONSTEXPR_GCC_11 void resize_buffers(std::size_t size);
        void set_buffer(std::size_t index, any_buffer&& buffer);
        void set_buffer(std::size_t index, const any_buffer& buffer);
        SPARROW_CONSTEXPR_GCC_11 void resize_buffer(std::size_t index, std::size_t size, std::uint8_t value);
        SPARROW_CONSTEXPR_GCC_11 void update_buffers_ptrs();

        template <class T>
        [[nodiscard]] constexpr const T** buffers_ptrs() noexcept;

    private:

        BufferType m_buffers;
        std::vector<std::uint8_t*> m_buffers_pointers;
    };

    template <std::ranges::input_range CHILDREN_OWNERSHIP>
        requires std::is_same_v<std::ranges::range_value_t<CHILDREN_OWNERSHIP>, bool>
    constexpr arrow_array_private_data::arrow_array_private_data(
        BufferType buffers,
        const CHILDREN_OWNERSHIP& children_ownership_range,
        bool dictionary_ownership_value
    )
        : children_ownership(children_ownership_range)
        , dictionary_ownership(dictionary_ownership_value)
        , m_buffers(std::move(buffers))
    {
        update_buffers_ptrs();
    }

    [[nodiscard]] constexpr auto arrow_array_private_data::buffers() noexcept -> BufferType&
    {
        return m_buffers;
    }

    [[nodiscard]] constexpr auto arrow_array_private_data::buffers() const noexcept -> const BufferType&
    {
        return m_buffers;
    }

    SPARROW_CONSTEXPR_GCC_11 void arrow_array_private_data::resize_buffers(std::size_t size)
    {
        m_buffers.resize(size);
        update_buffers_ptrs();
    }

    inline void arrow_array_private_data::set_buffer(std::size_t index, any_buffer&& buffer)
    {
        SPARROW_ASSERT_TRUE(index < m_buffers.size());
        m_buffers[index] = std::move(buffer);
        std::visit(
            [this, index](const auto& b)
            { m_buffers_pointers[index] = const_cast<std::uint8_t*>(reinterpret_cast<const std::uint8_t*>(b.data())); },
            m_buffers[index]
        );
    }

    inline void arrow_array_private_data::set_buffer(std::size_t index, const any_buffer& buffer)
    {
        SPARROW_ASSERT_TRUE(index < m_buffers.size());
        std::visit([this, index](const auto& v) { m_buffers[index] = v; }, buffer);
        std::visit(
            [this, index](const auto& b)
            { m_buffers_pointers[index] = const_cast<std::uint8_t*>(reinterpret_cast<const std::uint8_t*>(b.data())); },
            m_buffers[index]
        );
    }

    SPARROW_CONSTEXPR_GCC_11 void
    arrow_array_private_data::resize_buffer(std::size_t index, std::size_t size, std::uint8_t value)
    {
        SPARROW_ASSERT_TRUE(index < m_buffers.size());
        std::visit(
            [=](auto& b)
            {
                using T = std::decay_t<decltype(b)>;
                if constexpr (std::is_same_v<T, buffer<uint8_t>>)
                {
                    b.resize(size, value);
                }
                else if constexpr (std::is_same_v<T, buffer_view<const uint8_t>>)
                {
                    throw std::runtime_error("Cannot resize a non-owning buffer.");
                }
            },
            m_buffers[index]
        );
        update_buffers_ptrs();
    }

    template <class T>
    [[nodiscard]] constexpr const T** arrow_array_private_data::buffers_ptrs() noexcept
    {
        return const_cast<const T**>(reinterpret_cast<T**>(m_buffers_pointers.data()));
    }

    SPARROW_CONSTEXPR_GCC_11 void arrow_array_private_data::update_buffers_ptrs()
    {
        m_buffers_pointers.resize(m_buffers.size(), nullptr);
        for (size_t i = 0; i < m_buffers.size(); ++i)
        {
            std::visit(
                [this, i](const auto& b)
                { m_buffers_pointers[i] = const_cast<std::uint8_t*>(reinterpret_cast<const std::uint8_t*>(b.data())); },
                m_buffers[i]
            );
        }
    }
}
