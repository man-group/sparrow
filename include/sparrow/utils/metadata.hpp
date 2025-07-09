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
    /**
     * @brief Type alias for metadata keys.
     *
     * Represents the key portion of a metadata key-value pair as a string view.
     * String views are used for efficiency to avoid unnecessary string copies.
     */
    using metadata_key = std::string_view;

    /**
     * @brief Type alias for metadata values.
     *
     * Represents the value portion of a metadata key-value pair as a string view.
     * String views are used for efficiency to avoid unnecessary string copies.
     */
    using metadata_value = std::string_view;

    /**
     * @brief Type alias for metadata key-value pairs.
     *
     * Represents a complete metadata entry consisting of a key and value,
     * both as string views for efficient processing.
     */
    using metadata_pair = std::pair<metadata_key, metadata_value>;

    /**
     * @brief Helper function to extract a 32-bit integer from a character buffer.
     *
     * Reads a 32-bit integer from the current position in the buffer and advances
     * the pointer past the read data. Used for parsing binary metadata formats.
     *
     * @param ptr Reference to character pointer that will be advanced
     * @return The extracted 32-bit integer value
     *
     * @pre ptr must point to a valid buffer with at least sizeof(int32_t) bytes available
     * @post ptr is advanced by sizeof(int32_t) bytes
     * @post Return value contains the integer read from the original ptr position
     */
    SPARROW_API int32_t extract_int32(const char*& ptr);

    class key_value_view;

    /**
     * @brief Iterator for traversing key-value pairs in a binary metadata buffer.
     *
     * This iterator provides sequential access to metadata key-value pairs stored
     * in a binary format. It lazily extracts pairs as the iteration progresses,
     * making it memory-efficient for large metadata sets.
     *
     * The iterator follows the input iterator concept, allowing single-pass traversal
     * of the metadata pairs.
     *
     * @pre The underlying buffer must remain valid for the iterator's lifetime
     * @post Iterator correctly extracts key-value pairs from binary format
     * @post Dereferencing provides valid metadata_pair objects
     */
    class key_value_view_iterator
    {
    public:

        // using iterator_category = std::input_iterator_tag;
        using value_type = metadata_pair;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_concept = std::forward_iterator_tag;


        /**
         * @brief Constructs an iterator for the given view at the specified index.
         *
         * @param parent Reference to the parent key_value_view
         * @param index Starting index for the iterator
         *
         * @pre parent must remain valid for the iterator's lifetime
         * @pre index must be >= 0 and <= parent.size()
         * @post Iterator is positioned at the specified index
         * @post Iterator state is valid for dereferencing (if index < parent.size())
         */
        SPARROW_API key_value_view_iterator(const key_value_view& parent, int32_t index);

        /**
         * @brief Dereferences the iterator to get the current key-value pair.
         *
         * @return The metadata pair at the current iterator position
         *
         * @pre Iterator must not be at end position
         * @pre Current position must be valid within the metadata buffer
         * @post Returns valid metadata_pair with extracted key and value
         * @post Key and value string_views are valid while buffer remains unchanged
         */
        SPARROW_API value_type operator*() const;

        /**
         * @brief Advances the iterator to the next key-value pair.
         *
         * @return Reference to this iterator after advancement
         *
         * @post Iterator is advanced to next position
         * @post Internal buffer pointer is correctly positioned for next extraction or past the end
         */
        SPARROW_API key_value_view_iterator& operator++();

        /**
         * @brief Advances the iterator to the next key-value pair.
         *
         * @return Copy of this iterator before advancement
         * @post Iterator is advanced to next position
         * @post Internal buffer pointer is correctly positioned for next extraction or past the end
         */
        SPARROW_API key_value_view_iterator operator++(int);

        /**
         * @brief Equality comparison operator for iterators.
         *
         * @param lhs First iterator to compare
         * @param rhs Second iterator to compare
         * @return true if iterators point to the same position
         *
         * @post Two iterators are equal iff they have the same index
         */
        friend bool operator==(const key_value_view_iterator& lhs, const key_value_view_iterator& rhs)
        {
            return lhs.m_index == rhs.m_index;
        }

    private:

        /**
         * @brief Extracts a string view from the current buffer position.
         *
         * Reads a length-prefixed string from the buffer and advances the position.
         *
         * @return String view of the extracted string
         *
         * @pre Current buffer position must contain a valid length-prefixed string
         * @post Buffer position is advanced past the extracted string
         * @post Returned string_view is valid while buffer remains unchanged
         */
        SPARROW_API std::string_view extract_string_view();

        /**
         * @brief Extracts the current key-value pair from the buffer.
         *
         * Updates the internal key and value members by reading from the current
         * buffer position.
         *
         * @pre Current buffer position must contain a valid key-value pair
         * @post m_key and m_value contain valid string views
         * @post Buffer position is advanced past the extracted pair
         */
        SPARROW_API void extract_key_value();

        const key_value_view* m_parent;  ///< Pointer to parent view
        int32_t m_index;                 ///< Current iterator index
        const char* m_current;           ///< Current position in buffer
        std::string_view m_key;          ///< Currently extracted key
        std::string_view m_value;        ///< Currently extracted value
    };

    /**
     * @brief Custom view for lazily extracting key-value pairs from a binary buffer.
     *
     * This class provides a range-like interface over metadata stored in binary format.
     * It implements lazy extraction, meaning key-value pairs are only parsed when
     * accessed through iteration. This approach is memory-efficient and allows
     * processing of large metadata sets without upfront parsing overhead.
     *
     * The binary format consists of:
     * - int32_t: number of key-value pairs
     * - For each pair:
     *   - int32_t: key length
     *   - char[]: key data
     *   - int32_t: value length
     *   - char[]: value data
     *
     * @pre Input buffer must be valid and properly formatted
     * @pre Buffer must remain valid for the lifetime of the view and its iterators
     * @post Provides range-compatible interface for metadata iteration
     * @post Supports both const and non-const iteration
     */
    class key_value_view : public std::ranges::view_interface<key_value_view>
    {
    public:

        /**
         * @brief Constructs a view over the given binary metadata buffer.
         *
         * @param ptr Pointer to the start of the binary metadata buffer
         *
         * @pre ptr must point to a valid binary metadata buffer
         * @pre Buffer must contain a valid int32_t count followed by the metadata pairs
         * @post View is ready for iteration over the metadata pairs
         * @post size() returns the number of pairs in the buffer
         */
        SPARROW_API key_value_view(const char* ptr);

        /**
         * @brief Gets const iterator to the beginning of the metadata pairs.
         *
         * @return Const iterator pointing to the first key-value pair
         *
         * @post Returned iterator is valid for reading metadata pairs
         * @post Iterator can be used with standard algorithms
         */
        [[nodiscard]] SPARROW_API key_value_view_iterator cbegin() const;

        /**
         * @brief Gets iterator to the beginning of the metadata pairs.
         *
         * @return Iterator pointing to the first key-value pair
         *
         * @post Returned iterator is valid for reading metadata pairs
         * @post Iterator can be used with standard algorithms
         */
        [[nodiscard]] SPARROW_API key_value_view_iterator begin() const;

        /**
         * @brief Gets const iterator to the end of the metadata pairs.
         *
         * @return Const iterator pointing past the last key-value pair
         *
         * @post Returned iterator marks the end of the range
         * @post Can be compared with begin() for range-based operations
         */
        [[nodiscard]] SPARROW_API key_value_view_iterator cend() const;

        /**
         * @brief Gets iterator to the end of the metadata pairs.
         *
         * @return Iterator pointing past the last key-value pair
         *
         * @post Returned iterator marks the end of the range
         * @post Can be compared with begin() for range-based operations
         */
        [[nodiscard]] SPARROW_API key_value_view_iterator end() const;

        /**
         * @brief Gets the number of key-value pairs in the metadata.
         *
         * @return Number of metadata pairs
         *
         * @post Returns non-negative count of pairs
         * @post Value corresponds to the count stored in the buffer header
         */
        [[nodiscard]] SPARROW_API size_t size() const;

    private:

        const char* m_ptr;        ///< Pointer to the binary metadata buffer
        int32_t m_num_pairs = 0;  ///< Number of key-value pairs in the buffer

        friend key_value_view_iterator;
    };

    /**
     * @brief Concept for input containers that can provide metadata pairs.
     *
     * Defines the requirements for containers that can be used as input
     * for metadata processing functions. The container must be an input range
     * with elements of type metadata_pair.
     *
     * @tparam T Type to check for metadata container compatibility
     */
    template <typename T>
    concept input_metadata_container = std::ranges::input_range<T>
                                       && std::same_as<std::ranges::range_value_t<T>, metadata_pair>;

    /**
     * @brief Converts a container of key-value pairs to binary metadata format.
     *
     * Takes a container of metadata pairs and serializes them into a binary
     * format suitable for storage or transmission. The resulting string contains
     * a packed representation with length-prefixed strings.
     *
     * Binary format:
     * - int32_t: number of key-value pairs
     * - For each pair:
     *   - int32_t: key length
     *   - char[]: key data (no null terminator)
     *   - int32_t: value length
     *   - char[]: value data (no null terminator)
     *
     * @tparam T Type of input metadata container
     * @param metadata Container of key-value pairs to serialize
     * @return String containing the binary representation
     *
     * @pre T must satisfy input_metadata_container concept
     * @pre All keys and values must have size < INT32_MAX
     * @post Returned string contains valid binary metadata format
     * @post String can be used to reconstruct the metadata pairs via key_value_view
     *
     * @note Internal assertions verify that key and value sizes fit in int32_t:
     *   - SPARROW_ASSERT_TRUE(std::cmp_less(key.size(), std::numeric_limits<int32_t>::max()))
     *   - SPARROW_ASSERT_TRUE(std::cmp_less(value.size(), std::numeric_limits<int32_t>::max()))
     */
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

#if defined(__cpp_lib_format)


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
