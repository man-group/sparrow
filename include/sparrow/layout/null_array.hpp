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

#include <cstddef>
#include <optional>
#include <ranges>

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/metadata.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/utils/repeat_container.hpp"

namespace sparrow
{
    /*
     * @class empty_iterator
     *
     * @brief Iterator used by the null_layout class.
     *
     * @tparam T the value_type of the iterator
     */
    template <class T>
    class empty_iterator : public iterator_base<empty_iterator<T>, T, std::contiguous_iterator_tag, T>
    {
    public:

        using self_type = empty_iterator<T>;
        using base_type = iterator_base<self_type, T, std::contiguous_iterator_tag, T>;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;

        explicit empty_iterator(difference_type index = difference_type()) noexcept;

    private:

        [[nodiscard]] reference dereference() const;
        void increment();
        void decrement();
        void advance(difference_type n);
        [[nodiscard]] difference_type distance_to(const self_type& rhs) const;
        [[nodiscard]] bool equal(const self_type& rhs) const;
        [[nodiscard]] bool less_than(const self_type& rhs) const;

        difference_type m_index;

        friend class iterator_access;
    };

    class null_array;

    /**
     * Checks whether T is a null_array type.
     */
    template <class T>
    constexpr bool is_null_array_v = std::same_as<T, null_array>;

    /**
     * A null array implementation.
     * Asimplified memory-efficient layout for the Null data type where all values are null. In this case no
     * memory buffers are allocated. Related Apache Arrow specification:
     * https://arrow.apache.org/docs/dev/format/Columnar.html#null-layout
     */
    class null_array
    {
    public:

        using inner_value_type = null_type;
        using value_type = nullable<inner_value_type>;
        using iterator = empty_iterator<value_type>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_iterator = empty_iterator<value_type>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using reference = iterator::reference;
        using const_reference = const_iterator::reference;
        using size_type = std::size_t;
        using difference_type = iterator::difference_type;
        using iterator_tag = std::random_access_iterator_tag;

        using const_value_iterator = empty_iterator<int>;
        using const_bitmap_iterator = empty_iterator<bool>;

        using const_value_range = std::ranges::subrange<const_value_iterator>;
        using const_bitmap_range = std::ranges::subrange<const_bitmap_iterator>;

        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        null_array(
            size_t length,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
            : m_proxy(create_proxy(length, std::move(name), std::move(metadata)))
        {
        }

        SPARROW_API explicit null_array(arrow_proxy);

        [[nodiscard]] SPARROW_API std::optional<std::string_view> name() const;
        [[nodiscard]] SPARROW_API std::optional<key_value_view> metadata() const;

        [[nodiscard]] SPARROW_API size_type size() const;

        [[nodiscard]] SPARROW_API reference operator[](size_type i);
        [[nodiscard]] SPARROW_API const_reference operator[](size_type i) const;

        [[nodiscard]] SPARROW_API iterator begin();
        [[nodiscard]] SPARROW_API iterator end();

        [[nodiscard]] SPARROW_API const_iterator begin() const;
        [[nodiscard]] SPARROW_API const_iterator end() const;

        [[nodiscard]] SPARROW_API reverse_iterator rbegin();
        [[nodiscard]] SPARROW_API reverse_iterator rend();

        [[nodiscard]] SPARROW_API const_reverse_iterator rbegin() const;
        [[nodiscard]] SPARROW_API const_reverse_iterator rend() const;

        [[nodiscard]] SPARROW_API const_iterator cbegin() const;
        [[nodiscard]] SPARROW_API const_iterator cend() const;

        [[nodiscard]] SPARROW_API const_reverse_iterator crbegin() const;
        [[nodiscard]] SPARROW_API const_reverse_iterator crend() const;

        [[nodiscard]] SPARROW_API reference front();
        [[nodiscard]] SPARROW_API const_reference front() const;

        [[nodiscard]] SPARROW_API reference back();
        [[nodiscard]] SPARROW_API const_reference back() const;

        [[nodiscard]] SPARROW_API const_value_range values() const;
        [[nodiscard]] SPARROW_API const_bitmap_range bitmap() const;

    private:

        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy
        create_proxy(size_t length, std::optional<std::string_view> name, std::optional<METADATA_RANGE> metadata);

        [[nodiscard]] SPARROW_API difference_type ssize() const;

        [[nodiscard]] SPARROW_API arrow_proxy& get_arrow_proxy();
        [[nodiscard]] SPARROW_API const arrow_proxy& get_arrow_proxy() const;

        arrow_proxy m_proxy;

        friend class detail::array_access;
    };

    SPARROW_API
    bool operator==(const null_array& lhs, const null_array& rhs);

    /*********************************
     * empty_iterator implementation *
     *********************************/

    template <class T>
    empty_iterator<T>::empty_iterator(difference_type index) noexcept
        : m_index(index)
    {
    }

    template <class T>
    auto empty_iterator<T>::dereference() const -> reference
    {
        return T();
    }

    template <class T>
    void empty_iterator<T>::increment()
    {
        ++m_index;
    }

    template <class T>
    void empty_iterator<T>::decrement()
    {
        --m_index;
    }

    template <class T>
    void empty_iterator<T>::advance(difference_type n)
    {
        m_index += n;
    }

    template <class T>
    auto empty_iterator<T>::distance_to(const self_type& rhs) const -> difference_type
    {
        return rhs.m_index - m_index;
    }

    template <class T>
    bool empty_iterator<T>::equal(const self_type& rhs) const
    {
        return m_index == rhs.m_index;
    }

    template <class T>
    bool empty_iterator<T>::less_than(const self_type& rhs) const
    {
        return m_index < rhs.m_index;
    }

    template <input_metadata_container METADATA_RANGE>
    arrow_proxy
    null_array::create_proxy(size_t length, std::optional<std::string_view> name, std::optional<METADATA_RANGE> metadata)
    {
        using namespace std::literals;
        static const std::optional<std::unordered_set<sparrow::ArrowFlag>> flags{{ArrowFlag::NULLABLE}};
        ArrowSchema schema = make_arrow_schema(
            "n"sv,
            std::move(name),
            std::move(metadata),
            flags,
            0,
            repeat_view<bool>(false, 0),
            nullptr,
            false
        );

        using buffer_type = sparrow::buffer<std::uint8_t>;
        std::vector<buffer_type> arr_buffs = {};

        ArrowArray arr = make_arrow_array(
            static_cast<int64_t>(length),
            static_cast<int64_t>(length),
            0,
            std::move(arr_buffs),
            nullptr,
            repeat_view<bool>(false, 0),
            nullptr,
            false
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }
}

#if defined(__cpp_lib_format)

template <>
struct std::formatter<sparrow::null_array>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::null_array& ar, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "Null array [{}]", ar.size());
    }
};

inline std::ostream& operator<<(std::ostream& os, const sparrow::null_array& value)
{
    os << std::format("{}", value);
    return os;
}
#endif
