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

#include <cstdint>
#include <optional>
#include <ranges>
#include <string_view>
#include <vector>

#include "sparrow/layout/array_access.hpp"
#include "sparrow/primitive_array.hpp"
#include "sparrow/types/data_type.hpp"

namespace sparrow
{
    // Forward declaration
    class bool8_array;

    /**
     * @brief Reference proxy for bool8_array elements.
     *
     * Provides automatic conversion between bool and int8_t storage.
     * Similar to std::vector<bool>::reference.
     */
    class bool8_reference
    {
    public:

        bool8_reference(typename primitive_array<std::int8_t>::reference ref)
            : m_ref(ref)
        {
        }

        // Copy constructor
        bool8_reference(const bool8_reference&) = default;

        // Implicit conversion to bool
        operator bool() const
        {
            return m_ref.has_value() && m_ref.value() != 0;
        }

        // Assignment from bool
        bool8_reference& operator=(bool value)
        {
            m_ref = static_cast<std::int8_t>(value ? 1 : 0);
            return *this;
        }

        // Assignment from another bool8_reference
        bool8_reference& operator=(const bool8_reference& other)
        {
            if (other.has_value())
            {
                m_ref = static_cast<std::int8_t>(static_cast<bool>(other) ? 1 : 0);
            }
            else
            {
                // Assign null - this assumes nullable supports default construction for null
                m_ref = typename primitive_array<std::int8_t>::reference::value_type{};
            }
            return *this;
        }

        // Check if has value (not null)
        bool has_value() const
        {
            return m_ref.has_value();
        }

        // Get the boolean value (throws if null)
        bool get() const
        {
            return m_ref.value() != 0;
        }

    private:

        typename primitive_array<std::int8_t>::reference m_ref;
    };

    /**
     * @brief Const reference proxy for bool8_array elements.
     */
    class bool8_const_reference
    {
    public:

        bool8_const_reference(typename primitive_array<std::int8_t>::const_reference ref)
            : m_ref(ref)
        {
        }

        // Implicit conversion to nullable<bool> for nullable_variant
        operator nullable<bool>() const
        {
            if (m_ref.has_value())
            {
                return nullable<bool>(m_ref.value() != 0);
            }
            return nullable<bool>();
        }

        // Explicit conversion to bool
        explicit operator bool() const
        {
            return m_ref.has_value() && m_ref.value() != 0;
        }

        // Check if has value (not null)
        bool has_value() const
        {
            return m_ref.has_value();
        }

        // Get the boolean value (throws if null)
        bool get() const
        {
            return m_ref.value() != 0;
        }

    private:

        typename primitive_array<std::int8_t>::const_reference m_ref;
    };

    // Comparison operators for bool8_const_reference
    inline bool operator==(const bool8_const_reference& lhs, const bool8_const_reference& rhs)
    {
        if (lhs.has_value() != rhs.has_value())
        {
            return false;
        }
        if (!lhs.has_value())
        {
            return true;  // Both are null
        }
        return lhs.get() == rhs.get();
    }

    inline bool operator!=(const bool8_const_reference& lhs, const bool8_const_reference& rhs)
    {
        return !(lhs == rhs);
    }

    /**
     * @brief Iterator wrapper that converts values to bool.
     */
    template <typename BaseIterator, typename ReferenceType>
    class bool8_iterator_impl
    {
    public:

        using iterator_category = typename std::iterator_traits<BaseIterator>::iterator_category;
        using difference_type = typename std::iterator_traits<BaseIterator>::difference_type;
        using value_type = bool;
        using pointer = void;  // Not used for proxy references
        using reference = ReferenceType;

        bool8_iterator_impl() = default;

        explicit bool8_iterator_impl(BaseIterator it)
            : m_it(it)
        {
        }

        reference operator*() const
        {
            return reference(*m_it);
        }

        bool8_iterator_impl& operator++()
        {
            ++m_it;
            return *this;
        }

        bool8_iterator_impl operator++(int)
        {
            auto tmp = *this;
            ++m_it;
            return tmp;
        }

        bool8_iterator_impl& operator--()
        {
            --m_it;
            return *this;
        }

        bool8_iterator_impl operator--(int)
        {
            auto tmp = *this;
            --m_it;
            return tmp;
        }

        bool8_iterator_impl& operator+=(difference_type n)
        {
            m_it += n;
            return *this;
        }

        bool8_iterator_impl& operator-=(difference_type n)
        {
            m_it -= n;
            return *this;
        }

        bool8_iterator_impl operator+(difference_type n) const
        {
            return bool8_iterator_impl(m_it + n);
        }

        bool8_iterator_impl operator-(difference_type n) const
        {
            return bool8_iterator_impl(m_it - n);
        }

        difference_type operator-(const bool8_iterator_impl& other) const
        {
            return m_it - other.m_it;
        }

        reference operator[](difference_type n) const
        {
            return reference(m_it[n]);
        }

        bool operator==(const bool8_iterator_impl& other) const
        {
            return m_it == other.m_it;
        }

        bool operator!=(const bool8_iterator_impl& other) const
        {
            return m_it != other.m_it;
        }

        bool operator<(const bool8_iterator_impl& other) const
        {
            return m_it < other.m_it;
        }

        bool operator<=(const bool8_iterator_impl& other) const
        {
            return m_it <= other.m_it;
        }

        bool operator>(const bool8_iterator_impl& other) const
        {
            return m_it > other.m_it;
        }

        bool operator>=(const bool8_iterator_impl& other) const
        {
            return m_it >= other.m_it;
        }

    private:

        BaseIterator m_it;
    };

    /**
     * @brief Bool8 array class with boolean-based access.
     *
     * Bool8 represents a boolean value using 1 byte (8 bits) to store each value
     * instead of only 1 bit as in the original Arrow Boolean type. Although less
     * compact than the original representation, Bool8 may have better zero-copy
     * compatibility with various systems that also store booleans using 1 byte.
     *
     * The Bool8 extension type is defined as:
     * - Extension name: "arrow.bool8"
     * - Storage type: Int8
     * - false is denoted by the value 0
     * - true can be specified using any non-zero value (preferably 1)
     * - Extension metadata: empty string
     *
     * This class provides a convenient wrapper around primitive_array<int8_t>
     * with boolean-specific constructors and access methods.
     *
     * Related Apache Arrow specification:
     * https://arrow.apache.org/docs/format/CanonicalExtensions.html
     *
     * @example
     * ```cpp
     * // Create from boolean values
     * std::vector<bool> values = {true, false, true, true, false};
     * bool8_array arr(values);
     *
     * // Access as booleans
     * bool value = arr.get_bool(0); // returns true
     * arr.set_bool(1, true);        // sets second element to true
     * ```
     */
    class bool8_array : public primitive_array<std::int8_t>
    {
    public:

        static constexpr std::string_view EXTENSION_NAME = "arrow.bool8";

        using primitive_base = primitive_array<std::int8_t>;
        using size_type = typename primitive_base::size_type;
        using reference = bool8_reference;
        using const_reference = bool8_const_reference;
        using iterator = bool8_iterator_impl<typename primitive_base::iterator, reference>;
        using const_iterator = bool8_iterator_impl<typename primitive_base::const_iterator, const_reference>;
        using value_type = bool;
        using inner_value_type = std::int8_t;
        using inner_reference = std::int8_t&;
        using inner_const_reference = const std::int8_t&;

        /**
         * @brief Construct from an Arrow proxy.
         */
        SPARROW_API explicit bool8_array(arrow_proxy proxy);

        /**
         * @brief Construct from a range of int8_t values.
         *
         * @tparam R Range type with int8_t-convertible values (but not bool)
         * @param range Input range of int8_t values
         * @param nullable Whether the array should support null values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         */
        template <std::ranges::input_range R>
            requires(!std::same_as<std::ranges::range_value_t<R>, bool>)
                    && std::convertible_to<std::ranges::range_value_t<R>, std::int8_t>
        explicit bool8_array(
            R&& range,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<std::vector<metadata_pair>> metadata = std::nullopt
        )
            : primitive_base(std::forward<R>(range), nullable, name, metadata)
        {
            init_extension_metadata(detail::array_access::get_arrow_proxy(*this));
        }

        /**
         * @brief Construct from a range of boolean values.
         *
         * @tparam R Range type with boolean values
         * @param range Input range of boolean values
         * @param nullable Whether the array should support null values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         */
        template <std::ranges::input_range R>
            requires std::same_as<std::ranges::range_value_t<R>, bool>
        explicit bool8_array(
            R&& range,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<std::vector<metadata_pair>> metadata = std::nullopt
        )
            : primitive_base(
                  std::forward<R>(range)
                      | std::views::transform(
                          [](bool b)
                          {
                              return static_cast<std::int8_t>(b ? 1 : 0);
                          }
                      ),
                  nullable,
                  name,
                  metadata
              )
        {
            init_extension_metadata(detail::array_access::get_arrow_proxy(*this));
        }

        /**
         * @brief Construct from a range of boolean values with validity bitmap.
         *
         * @tparam R Range type with boolean values
         * @tparam R2 Validity bitmap input type
         * @param values Input range of boolean values
         * @param validity_input Validity bitmap specification
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         */
        template <std::ranges::input_range R, validity_bitmap_input R2>
            requires std::same_as<std::ranges::range_value_t<R>, bool>
        bool8_array(
            R&& values,
            R2&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<std::vector<metadata_pair>> metadata = std::nullopt
        )
            : primitive_base(
                  std::forward<R>(values)
                      | std::views::transform(
                          [](bool b)
                          {
                              return static_cast<std::int8_t>(b ? 1 : 0);
                          }
                      ),
                  std::forward<R2>(validity_input),
                  name,
                  metadata
              )
        {
            init_extension_metadata(detail::array_access::get_arrow_proxy(*this));
        }

        /**
         * @brief Construct from a range of int8_t values with validity bitmap.
         *
         * @tparam R Range type with int8_t values (but not bool)
         * @tparam R2 Validity bitmap input type
         * @param values Input range of int8_t values
         * @param validity_input Validity bitmap specification
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         */
        template <std::ranges::input_range R, validity_bitmap_input R2>
            requires(!std::same_as<std::ranges::range_value_t<R>, bool>)
                    && std::convertible_to<std::ranges::range_value_t<R>, std::int8_t>
        bool8_array(
            R&& values,
            R2&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<std::vector<metadata_pair>> metadata = std::nullopt
        )
            : primitive_base(std::forward<R>(values), std::forward<R2>(validity_input), name, metadata)
        {
            init_extension_metadata(detail::array_access::get_arrow_proxy(*this));
        }

        /**
         * @brief Construct from initializer list of boolean values.
         *
         * @param init Initializer list of boolean values
         * @param nullable Whether the array should support null values
         * @param name Optional name for the array
         * @param metadata Optional metadata for the array
         */
        bool8_array(
            std::initializer_list<bool> init,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<std::vector<metadata_pair>> metadata = std::nullopt
        )
            : primitive_base(
                  std::views::transform(
                      init,
                      [](bool b)
                      {
                          return static_cast<std::int8_t>(b ? 1 : 0);
                      }
                  ),
                  nullable,
                  name,
                  metadata
              )
        {
            init_extension_metadata(detail::array_access::get_arrow_proxy(*this));
        }

        /**
         * @brief Get the raw int8_t value at the specified index.
         *
         * @param index Index of the element
         * @return Raw int8_t value
         */
        [[nodiscard]] SPARROW_API inner_const_reference value(size_type index) const;
        
        using primitive_base::size;
        using primitive_base::empty;

        // Data type is always INT8 for bool8_array
        [[nodiscard]] SPARROW_API sparrow::data_type data_type() const;

        // Element access (returns boolean proxy references)
        [[nodiscard]] SPARROW_API reference operator[](size_type index);

        [[nodiscard]] SPARROW_API const_reference operator[](size_type index) const;

        // Iterator access with bool8 wrapper
        [[nodiscard]] SPARROW_API iterator begin();

        [[nodiscard]] SPARROW_API iterator end();

        [[nodiscard]] SPARROW_API const_iterator begin() const;

        [[nodiscard]] SPARROW_API const_iterator end() const;

        [[nodiscard]] SPARROW_API const_iterator cbegin() const;

        [[nodiscard]] SPARROW_API const_iterator cend() const;

        // Comparison operator
        SPARROW_API friend bool operator==(const bool8_array& lhs, const bool8_array& rhs);

    private:

        // Initialize extension metadata
        static void init_extension_metadata(arrow_proxy& proxy)
        {
            // Check if extension metadata already exists
            std::optional<key_value_view> metadata = proxy.metadata();

            if (metadata.has_value())
            {
                const bool has_extension = std::find_if(
                                               metadata->begin(),
                                               metadata->end(),
                                               [](const auto& pair)
                                               {
                                                   return pair.first == "ARROW:extension:name"
                                                          && pair.second == EXTENSION_NAME;
                                               }
                                           )
                                           != metadata->end();
                if (has_extension)
                {
                    // Extension metadata already present, nothing to do
                    return;
                }
            }

            // Copy existing metadata and add extension metadata
            std::vector<metadata_pair> extension_metadata = metadata.has_value()
                                                                ? std::vector<metadata_pair>(
                                                                      metadata->begin(),
                                                                      metadata->end()
                                                                  )
                                                                : std::vector<metadata_pair>{};
            extension_metadata.emplace_back("ARROW:extension:name", EXTENSION_NAME);
            extension_metadata.emplace_back("ARROW:extension:metadata", "");
            proxy.set_metadata(std::make_optional(extension_metadata));
        }

        // Arrow proxy access (required for array_wrapper integration)
        // Made private with friend access for detail::array_access
        [[nodiscard]] SPARROW_API arrow_proxy& get_arrow_proxy();

        [[nodiscard]] SPARROW_API const arrow_proxy& get_arrow_proxy() const;

        friend class detail::array_access;
    };

    namespace detail
    {
        template <>
        struct get_data_type_from_array<sparrow::bool8_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::INT8;
            }
        };
    }
}

#if defined(__cpp_lib_format)
#    include <format>

// Formatter specialization for bool8_const_reference
template <>
struct std::formatter<sparrow::bool8_const_reference>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const sparrow::bool8_const_reference& ref, std::format_context& ctx) const
    {
        if (ref.has_value())
        {
            return std::format_to(ctx.out(), "{}", ref.get() ? "true" : "false");
        }
        else
        {
            return std::format_to(ctx.out(), "null");
        }
    }
};

// Formatter specialization for bool8_array
template <>
struct std::formatter<sparrow::bool8_array>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const sparrow::bool8_array& ar, std::format_context& ctx) const
    {
        std::format_to(ctx.out(), "Bool8 array [{}]: [", ar.size());
        for (std::size_t i = 0; i < ar.size(); ++i)
        {
            if (i > 0)
            {
                std::format_to(ctx.out(), ", ");
            }
            if (ar[i].has_value())
            {
                std::format_to(ctx.out(), "{}", static_cast<bool>(ar[i]) ? "true" : "false");
            }
            else
            {
                std::format_to(ctx.out(), "null");
            }
        }
        return std::format_to(ctx.out(), "]");
    }
};

namespace sparrow
{
    template <std::integral IT>
    std::ostream& operator<<(std::ostream& os, const bool8_array& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

#endif
