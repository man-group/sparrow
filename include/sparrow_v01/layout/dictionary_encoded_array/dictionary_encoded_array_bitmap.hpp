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

#include <algorithm>

#include "sparrow_v01/layout/dictionary_encoded_array/dictionary_encoded_array_bitmap_iterator.hpp"

#pragma once

namespace sparrow
{

    template <typename KeysArray, typename ValuesArrayBitmapRange>
    class dictionary_bitmap
    {
    public:

        using size_type = size_t;
        using reference = bool;
        using const_reference = bool;
        using iterator = validity_iterator<KeysArray, ValuesArrayBitmapRange>;
        using const_iterator = validity_iterator<KeysArray, ValuesArrayBitmapRange>;

        explicit dictionary_bitmap(KeysArray& keys, ValuesArrayBitmapRange values_bitmap_range);

        [[nodiscard]] size_type size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;
        [[nodiscard]] size_type null_count() const noexcept;

        [[nodiscard]] bool test(size_type pos) const;

        [[nodiscard]] const_reference at(size_type pos) const;

        [[nodiscard]] const_reference operator[](size_type i) const;

        [[nodiscard]] const_iterator begin() const;
        [[nodiscard]] const_iterator end() const;
        [[nodiscard]] const_iterator cbegin() const;
        [[nodiscard]] const_iterator cend() const;

        [[nodiscard]] const_reference front() const;
        [[nodiscard]] const_reference back() const;

    private:

        [[nodiscard]] size_t calculate_null_count() const;

        KeysArray* m_keys;
        ValuesArrayBitmapRange m_values_bitmap_range;
        size_t m_null_count = 0;
    };

    /************************************
     * dictionary_bitmap implementation *
     ************************************/

    template <typename KeysArray, typename ValuesArrayBitmapRange>
    size_t dictionary_bitmap<KeysArray, ValuesArrayBitmapRange>::calculate_null_count() const
    {
        return static_cast<size_t>(std::count_if(
            m_keys->begin(),
            m_keys->end(),
            [this](const auto& index)
            {
                return !index.has_value() || !m_values_bitmap_range[index.value()];
            }
        ));
    }

    template <typename KeysArray, typename ValuesArrayBitmapRange>
    dictionary_bitmap<KeysArray, ValuesArrayBitmapRange>::dictionary_bitmap(
        KeysArray& keys,
        ValuesArrayBitmapRange values_array_bitmap_range
    )
        : m_keys(&keys)
        , m_values_bitmap_range(values_array_bitmap_range)
        , m_null_count(calculate_null_count())
    {
    }

    template <typename KeysArray, typename ValuesArrayBitmapRange>
    auto dictionary_bitmap<KeysArray, ValuesArrayBitmapRange>::size() const noexcept -> size_type
    {
        return m_keys->size();
    }

    template <typename KeysArray, typename ValuesArrayBitmapRange>
    auto dictionary_bitmap<KeysArray, ValuesArrayBitmapRange>::empty() const noexcept -> bool
    {
        return m_keys->empty();
    }

    template <typename KeysArray, typename ValuesArrayBitmapRange>
    auto dictionary_bitmap<KeysArray, ValuesArrayBitmapRange>::null_count() const noexcept -> size_type
    {
        return m_null_count;
    }

    template <typename KeysArray, typename ValuesArrayBitmapRange>
    auto dictionary_bitmap<KeysArray, ValuesArrayBitmapRange>::test(size_type pos) const -> bool
    {
        const auto index = (*m_keys)[pos];
        return index.has_value() && m_values_bitmap_range[index.value()];
    }

    template <typename KeysArray, typename ValuesArrayBitmapRange>
    auto dictionary_bitmap<KeysArray, ValuesArrayBitmapRange>::at(size_type pos) const -> const_reference
    {
        return test(pos);
    }

    template <typename KeysArray, typename ValuesArrayBitmapRange>
    auto dictionary_bitmap<KeysArray, ValuesArrayBitmapRange>::operator[](size_type i) const -> const_reference
    {
        return test(i);
    }

    template <typename KeysArray, typename ValuesArrayBitmapRange>
    auto dictionary_bitmap<KeysArray, ValuesArrayBitmapRange>::begin() const -> const_iterator
    {
        return const_iterator(*m_keys, m_values_bitmap_range, 0);
    }

    template <typename KeysArray, typename ValuesArrayBitmapRange>
    auto dictionary_bitmap<KeysArray, ValuesArrayBitmapRange>::end() const -> const_iterator
    {
        return const_iterator(*m_keys, m_values_bitmap_range, m_keys->size());
    }

    template <typename KeysArray, typename ValuesArrayBitmapRange>
    auto dictionary_bitmap<KeysArray, ValuesArrayBitmapRange>::cbegin() const -> const_iterator
    {
        return begin();
    }

    template <typename KeysArray, typename ValuesArrayBitmapRange>
    auto dictionary_bitmap<KeysArray, ValuesArrayBitmapRange>::cend() const -> const_iterator
    {
        return end();
    }

    template <typename KeysArray, typename ValuesArrayBitmapRange>
    auto dictionary_bitmap<KeysArray, ValuesArrayBitmapRange>::front() const -> const_reference
    {
        return test(0);
    }

    template <typename KeysArray, typename ValuesArrayBitmapRange>
    auto dictionary_bitmap<KeysArray, ValuesArrayBitmapRange>::back() const -> const_reference
    {
        return test(m_keys->size() - 1);
    }
}
