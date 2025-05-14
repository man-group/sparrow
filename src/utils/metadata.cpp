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

#include "sparrow/utils/metadata.hpp"

namespace sparrow
{
    int32_t extract_int32(const char*& ptr)
    {
        int32_t value = 0;
        std::memcpy(&value, ptr, sizeof(int32_t));
        ptr += sizeof(int32_t);
        return value;
    }

    key_value_view::key_value_view(const char* ptr)
        : m_ptr(ptr)
        , m_num_pairs(extract_int32(m_ptr))
    {
    }

    [[nodiscard]] key_value_view_iterator key_value_view::cbegin() const
    {
        return {*this, 0};
    }

    [[nodiscard]] key_value_view_iterator key_value_view::begin() const
    {
        return cbegin();
    }

    [[nodiscard]] key_value_view_iterator key_value_view::cend() const
    {
        return {*this, m_num_pairs};
    }

    [[nodiscard]] key_value_view_iterator key_value_view::end() const
    {
        return cend();
    }

    [[nodiscard]] size_t key_value_view::size() const
    {
        return static_cast<size_t>(m_num_pairs);
    }

    key_value_view_iterator::key_value_view_iterator(const key_value_view& parent, int32_t index)
        : m_parent(&parent)
        , m_index(index)
        , m_current(parent.m_ptr)
    {
        SPARROW_ASSERT_TRUE(m_index >= 0);
        SPARROW_ASSERT_TRUE(m_index < m_parent->m_num_pairs);
        for (int32_t i = 0; i <= m_index; ++i)
        {
            extract_key_value();
        }
    }

    key_value_view_iterator::value_type key_value_view_iterator::operator*() const
    {
        return std::pair(m_key, m_value);
    }

    key_value_view_iterator& key_value_view_iterator::operator++()
    {
        ++m_index;
        if (m_index < m_parent->m_num_pairs)
        {
            extract_key_value();
        }
        return *this;
    }

    std::string_view key_value_view_iterator::extract_string_view()
    {
        const int32_t length = extract_int32(m_current);
        std::string_view str_view(m_current, static_cast<size_t>(length));
        m_current += length;
        return str_view;
    }

    void key_value_view_iterator::extract_key_value()
    {
        m_key = extract_string_view();
        m_value = extract_string_view();
    }
}
