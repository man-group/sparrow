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
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <ranges>
#include <string_view>
#include <utility>

#include "sparrow/config/config.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/ranges.hpp"

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
    SPARROW_API int32_t extract_int32(const char*& ptr);

    class key_value_view;

    class key_value_view_iterator
    {
    public:

        using iterator_category = std::input_iterator_tag;
        using value_type = metadata_pair;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        SPARROW_API key_value_view_iterator(const key_value_view& parent, int32_t index);

        SPARROW_API value_type operator*() const;

        SPARROW_API key_value_view_iterator& operator++();

        friend bool operator==(const key_value_view_iterator& lhs, const key_value_view_iterator& rhs)
        {
            return lhs.m_index == rhs.m_index;
        }

    private:

        SPARROW_API std::string_view extract_string_view();
        SPARROW_API void extract_key_value();

        const key_value_view* m_parent;
        int32_t m_index;
        const char* m_current;
        std::string_view m_key;
        std::string_view m_value;
    };

    // Custom view to lazily extract key/value pairs from the buffer
    class key_value_view : public std::ranges::view_interface<key_value_view>
    {
    public:

        SPARROW_API key_value_view(const char* ptr);

        [[nodiscard]] SPARROW_API key_value_view_iterator cbegin() const;

        [[nodiscard]] SPARROW_API key_value_view_iterator begin() const;

        [[nodiscard]] SPARROW_API key_value_view_iterator cend() const;

        [[nodiscard]] SPARROW_API key_value_view_iterator end() const;

        [[nodiscard]] SPARROW_API size_t size() const;

    private:

        const char* m_ptr;
        int32_t m_num_pairs = 0;

        friend key_value_view_iterator;
    };

    template <typename T>
    concept input_metadata_container = std::ranges::input_range<T>
                                       && std::same_as<std::ranges::range_value_t<T>, metadata_pair>;

    template <input_metadata_container T>
    std::string get_metadata_from_key_values(const T& metadata)
    {
        const auto number_of_key_values = static_cast<int32_t>(metadata.size());
        const size_t metadata_size = std::accumulate(
            metadata.cbegin(),
            metadata.cend(),
            size_t(0),
            [](size_t acc, const auto& pair)
            {
                return acc + sizeof(int32_t)  // byte length of key
                       + pair.first.size()    // number of bytes of key
                       + sizeof(int32_t)      // byte length of value
                       + pair.second.size();  // number of bytes of value
            }
        );
        const size_t total_size = sizeof(int32_t) + metadata_size;
        std::string metadata_buf(total_size, '\0');
        char* metadata_ptr = metadata_buf.data();
        std::memcpy(metadata_ptr, &number_of_key_values, sizeof(int32_t));
        metadata_ptr += sizeof(int32_t);
        for (const auto& [key, value] : metadata)
        {
            SPARROW_ASSERT_TRUE(std::cmp_less(key.size(), std::numeric_limits<int32_t>::max()));
            SPARROW_ASSERT_TRUE(std::cmp_less(value.size(), std::numeric_limits<int32_t>::max()));

            const auto key_size = static_cast<int32_t>(key.size());
            std::memcpy(metadata_ptr, &key_size, sizeof(int32_t));
            metadata_ptr += sizeof(int32_t);

            sparrow::ranges::copy(key, metadata_ptr);
            metadata_ptr += key.size();

            const auto value_size = static_cast<int32_t>(value.size());
            std::memcpy(metadata_ptr, &value_size, sizeof(int32_t));
            metadata_ptr += sizeof(int32_t);

            sparrow::ranges::copy(value, metadata_ptr);
            metadata_ptr += value.size();
        }
        return metadata_buf;
    }
}


#if defined(__cpp_lib_format) && !defined(__cpp_lib_format_ranges)

template <>
struct std::formatter<sparrow::key_value_view>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::key_value_view& array, std::format_context& ctx) const
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

inline std::ostream& operator<<(std::ostream& os, const sparrow::key_value_view& value)
{
    os << std::format("{}", value);
    return os;
}

#endif
