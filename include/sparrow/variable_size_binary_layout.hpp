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

#pragma once

#include "sparrow/array_data.hpp"
#include "sparrow/iterator.hpp"

namespace sparrow
{
    template <class T, class CR>
    class variable_size_binary_layout
    {
    public:

        using self_type = variable_size_binary_layout<T, CR>;
        using inner_value_type = T;
        using inner_const_reference = CR;
        using value_type = std::optional<inner_value_type>;
        using const_reference = const_reference_proxy<self_type>;
        using size_type = std::size_t;

        explicit variable_size_binary_layout(array_data data);

        size_type size() const;
        const_reference operator[](size_type i) const;

    private:

        using sequence_value_type = typename T::value_type;

        bool has_value(size_type i) const;
        inner_const_reference value(size_type i) const;

        // We use the bitmap and the first two buffers
        // The first buffer contains the offsets of
        // the elements in the second buffer
        array_data m_data;

        friend class const_reference_proxy<self_type>;
    };

    /**********************************************
     * variable_size_binary_layout implementation *
     **********************************************/

    template <class T, class CR>
    variable_size_binary_layout<T, CR>::variable_size_binary_layout(array_data data)
        : m_data(std::move(data))
    {
        assert(m_data.buffers.size() == 2u);
    }

    template <class T, class CR>
    auto variable_size_binary_layout<T, CR>::size() const -> size_type
    {
        return static_cast<size_type>(m_data.length - m_data.offset);
    }

    template <class T, class CR>
    auto variable_size_binary_layout<T, CR>::operator[](size_type i) const -> const_reference
    {
        return const_reference(*this, i);
    }

    template <class T, class CR>
    auto variable_size_binary_layout<T, CR>::has_value(size_type i) const -> bool
    {
        return m_data.bitmap.test(i + m_data.offset);
    }

    template <class T, class CR>
    auto variable_size_binary_layout<T, CR>::value(size_type i) const -> inner_const_reference
    {
        const auto& offsets = m_data.buffers[0];
        size_type first_index = offsets[m_data.offset + i];
        size_type last_index = offsets[m_data.offset + i + 1];
        const auto* seq = m_data.buffers[1].data<sequence_value_type>();
        return CR(seq + first_index, seq + last_index);
    }
}

