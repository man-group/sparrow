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

#include <optional>
#include <vector>

#include "sparrow/buffer.hpp"
#include "sparrow/arrow_interface/arrow_array/smart_pointers.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_utils.hpp"

namespace sparrow
{

    /**
     * Private data for ArrowArray.
     *
     * Holds and own buffers, children, and dictionary.
     * It is used in the Sparrow library.
     */

    class arrow_array_private_data
    {
    public:

        using BufferType = std::vector<buffer<std::uint8_t>>;
        using ChildrenType = std::optional<std::vector<arrow_array_shared_ptr>>;
        using DictionaryType = arrow_array_shared_ptr;

        arrow_array_private_data(BufferType buffers, ChildrenType children, DictionaryType dictionary);

        [[nodiscard]] const BufferType& buffers() const noexcept;

        template <class T>
        [[nodiscard]] const T** buffers_ptrs() noexcept;

        [[nodiscard]] const ChildrenType& children() const noexcept;

        [[nodiscard]] ArrowArray** children_pointers() noexcept;

        [[nodiscard]] const DictionaryType& dictionary() const noexcept;

        [[nodiscard]] ArrowArray* dictionary_pointer() noexcept;

    private:

        BufferType m_buffers;
        std::vector<std::uint8_t*> m_buffers_pointers;
        ChildrenType m_children;
        std::vector<ArrowArray*> m_children_pointers;
        DictionaryType m_dictionary;
    };

    inline arrow_array_private_data::arrow_array_private_data(
        BufferType buffers,
        ChildrenType children,
        DictionaryType dictionary
    )
        : m_buffers(std::move(buffers))
        , m_buffers_pointers(to_raw_ptr_vec<std::uint8_t>(m_buffers))
        , m_children(std::move(children))
        , m_children_pointers(to_raw_ptr_vec<ArrowArray>(m_children))
        , m_dictionary(std::move(dictionary))
    {
    }

    [[nodiscard]] inline const std::vector<buffer<std::uint8_t>>&
    arrow_array_private_data::buffers() const noexcept
    {
        return m_buffers;
    }

    template <class T>
    [[nodiscard]] inline const T** arrow_array_private_data::buffers_ptrs() noexcept
    {
        return const_cast<const T**>(reinterpret_cast<T**>(m_buffers_pointers.data()));
    }

    [[nodiscard]] inline const std::optional<std::vector<arrow_array_shared_ptr>>&
    arrow_array_private_data::children() const noexcept
    {
        return m_children;
    }

    [[nodiscard]] inline ArrowArray** arrow_array_private_data::children_pointers() noexcept
    {
        if (m_children_pointers.empty())
        {
            return nullptr;
        }
        return m_children_pointers.data();
    }

    [[nodiscard]] inline const arrow_array_shared_ptr& arrow_array_private_data::dictionary() const noexcept
    {
        return m_dictionary;
    }

    [[nodiscard]] inline ArrowArray* arrow_array_private_data::dictionary_pointer() noexcept
    {
        if (!m_dictionary)
        {
            return nullptr;
        }
        return m_dictionary.get();
    }
}