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

#include <concepts>
#include <numeric>
#include <ranges>
#include <string_view>
#include <utility>
#include <vector>

#include "sparrow/utils/contracts.hpp"


#if defined(__cpp_lib_format)
#    include <format>
#    include <ostream>
#endif

namespace sparrow
{
    using metadata_key = std::string_view;
    using metadata_value = std::string_view;
    using metadata_pair = std::pair<metadata_key, metadata_value>;

    // Helper function to extract an int32 from a char buffer
    inline int32_t extract_int32(const char*& ptr)
    {
        return *reinterpret_cast<const int32_t*>(ptr++);
    }

    // Custom view to lazily extract key/value pairs from the buffer
    class KeyValueView : public std::ranges::view_interface<KeyValueView>
    {
    public:

        KeyValueView(const char* ptr)
            : m_ptr(ptr)
            , m_num_pairs(extract_int32(m_ptr))
        {
        }

        [[nodiscard]] auto cbegin() const
        {
            return iterator(*this, 0);
        }

        [[nodiscard]] auto begin() const
        {
            return cbegin();
        }

        [[nodiscard]] auto cend() const
        {
            return iterator(*this, m_num_pairs);
        }

        [[nodiscard]] auto end() const
        {
            return cend();
        }

        [[nodiscard]] int32_t size() const
        {
            return m_num_pairs;
        }

    private:

        class iterator
        {
        public:

            using iterator_category = std::input_iterator_tag;
            using value_type = metadata_pair;
            using difference_type = std::ptrdiff_t;
            using pointer = value_type*;
            using reference = value_type&;

            iterator(const KeyValueView& parent, int32_t index)
                : m_parent(parent)
                , m_index(index)
            {
                SPARROW_ASSERT_TRUE(m_index >= 0);
                SPARROW_ASSERT_TRUE(m_index <= m_parent.m_num_pairs);
                if (m_index < m_parent.m_num_pairs)
                {
                    m_current = m_parent.m_ptr;
                    extract_key_value();
                }
            }

            value_type operator*() const
            {
                return {m_key, m_value};
            }

            iterator& operator++()
            {
                ++m_index;
                if (m_index < m_parent.m_num_pairs)
                {
                    m_current = m_parent.m_ptr;
                    extract_key_value();
                }
                return *this;
            }

            friend bool operator==(const iterator& lhs, const iterator& rhs)
            {
                return lhs.m_index == rhs.m_index;
            }

            friend bool operator!=(const iterator& lhs, const iterator& rhs)
            {
                return !(lhs == rhs);
            }

        private:

            std::string_view extract_string_view()
            {
                const int32_t length = extract_int32(m_current);
                std::string_view str_view(m_current, static_cast<size_t>(length));
                m_current += length;
                return str_view;
            }

            void extract_key_value()
            {
                m_key = extract_string_view();
                m_value = extract_string_view();
            }

            const KeyValueView& m_parent;
            int32_t m_index;
            const char* m_current;
            std::string_view m_key;
            std::string_view m_value;
        };

        const char* m_ptr;
        int32_t m_num_pairs = 0;
    };

    template <typename T>
    concept input_metadata_container = std::ranges::input_range<T>
                                       && std::same_as<std::ranges::range_value_t<T>, metadata_pair>;

    std::string get_metadata_from_key_values(const input_metadata_container auto& metadata)
    {
        const auto number_of_key_values = static_cast<int32_t>(metadata.size());
        const size_t metadata_size = std::accumulate(
            metadata.cbegin(),
            metadata.cend(),
            0ull,
            [](size_t acc, const auto& pair)
            {
                return acc + sizeof(int32_t)  // byte length of key
                       + pair.first.size()    // number of bytes of key
                       + sizeof(int32_t)      // byte length of value
                       + pair.second.size();  // number of bytes of value
            }
        );
        const size_t total_size = sizeof(int32_t) + metadata_size;
        std::string metadata_buffer(total_size, '\0');
        char* metadata_ptr = metadata_buffer.data();
        reinterpret_cast<int32_t*>(metadata_ptr)[0] = number_of_key_values;
        metadata_ptr += sizeof(int32_t);
        for (const auto& [key, value] : metadata)
        {
            SPARROW_ASSERT_TRUE(std::cmp_less(key.size(), std::numeric_limits<int32_t>::max()));
            SPARROW_ASSERT_TRUE(std::cmp_less(value.size(), std::numeric_limits<int32_t>::max()));
            reinterpret_cast<int32_t*>(metadata_ptr)[0] = static_cast<int32_t>(key.size());
            metadata_ptr += sizeof(int32_t);
            std::ranges::copy(key, metadata_ptr);
            metadata_ptr += key.size();
            reinterpret_cast<int32_t*>(metadata_ptr)[0] = static_cast<int32_t>(value.size());
            metadata_ptr += sizeof(int32_t);
            std::ranges::copy(value, metadata_ptr);
            metadata_ptr += value.size();
        }
        return metadata_buffer;
    }
}

#if defined(__cpp_lib_format) && !defined(__cpp_lib_format_ranges)

template <>
struct std::formatter<sparrow::KeyValueView>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::KeyValueView& array, std::format_context& ctx) const
    {
        auto out = ctx.out();
        *out++ = '<';

        bool first = true;
        for (const auto& elem : array)
        {
            if (!first)
            {
                *out++ = ',';
                *out++ = ' ';
            }
            out = std::format_to(out, "({}:{})", elem.first, elem.second);
            first = false;
        }

        *out++ = '>';
        return out;
    }
};

#endif
