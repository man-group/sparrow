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

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/buffer/u8_buffer.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/trivial_copyable_type_data_access.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    template <trivial_copyable_type T>
    class array_trivial_copyable_type_base_impl;

    template <trivial_copyable_type T>
    struct array_inner_types<array_trivial_copyable_type_base_impl<T>> : array_inner_types_base
    {
        using array_type = array_trivial_copyable_type_base_impl<T>;

        using inner_value_type = T;
        using inner_reference = T&;
        using inner_const_reference = const T&;
        using pointer = inner_value_type*;
        using const_pointer = const inner_value_type*;

        using value_iterator = pointer_iterator<pointer>;
        using const_value_iterator = pointer_iterator<const_pointer>;

        using bitmap_const_reference = bitmap_type::const_reference;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;

        // using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    template <trivial_copyable_type T>
    class array_trivial_copyable_type_base_impl
        : public mutable_array_bitmap_base<array_trivial_copyable_type_base_impl<T>>
    {
    public:

        using self_type = array_trivial_copyable_type_base_impl<T>;
        using base_type = mutable_array_bitmap_base<array_trivial_copyable_type_base_impl<T>>;
        using size_type = std::size_t;

        using inner_types = array_inner_types<self_type>;
        using inner_value_type = typename inner_types::inner_value_type;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;

        using pointer = typename inner_types::pointer;
        using const_pointer = typename inner_types::const_pointer;

        using value_iterator = typename base_type::value_iterator;
        using const_value_iterator = typename base_type::const_value_iterator;

        explicit array_trivial_copyable_type_base_impl(arrow_proxy);

        /**
         * Constructs a primitive array with the passed range of values and an optional bitmap.
         *
         * The first argument can be any range of values as long as its value type is convertible
         * to \c T.
         * The second argument can be:
         * - a bitmap range, i.e. a range of boolean-like values indicating the non-missing values.
         *   The bitmap range and the value range must have the same size.
         * ```cpp
         * std::vector<bool> a_bitmap(10, true);
         * a_bitmap[3] = false;
         * array_trivial_copyable_type_base_impl<int> pr(std::ranges::iota_view{0, 10}, a_bitmap);
         * ```
         * - a range of indices indicating the missing values.
         * ```cpp
         * std::vector<std::size_t> false_pos  { 3, 8 };
         * array_trivial_copyable_type_base_impl<int> pr(std::ranges::iota_view{0, 10}, a_bitmap);
         * ```
         * - omitted: this is equivalent as passing a bitmap range full of \c true.
         * ```cpp
         * array_trivial_copyable_type_base_impl<int> pr((std::ranges::iota_view{0, 10});
         * ```
         */
        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<array_trivial_copyable_type_base_impl<T>, Args...>)
        explicit array_trivial_copyable_type_base_impl(Args&&... args)
            : base_type(create_proxy(std::forward<Args>(args)...))
            , m_data_access(this, DATA_BUFFER_INDEX)
        {
        }

        /**
         * Constructs a primitive array from an \c initializer_list of raw values.
         */
        array_trivial_copyable_type_base_impl(
            std::initializer_list<inner_value_type> init,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<std::string_view> metadata = std::nullopt
        )
            : base_type(create_proxy(init, std::move(name), std::move(metadata)))
            , m_data_access(this, DATA_BUFFER_INDEX)
        {
        }

        array_trivial_copyable_type_base_impl(const array_trivial_copyable_type_base_impl&);
        array_trivial_copyable_type_base_impl& operator=(const array_trivial_copyable_type_base_impl&);

        array_trivial_copyable_type_base_impl(array_trivial_copyable_type_base_impl&& rhs) noexcept
            : base_type(std::move(rhs))
            , m_data_access(this, DATA_BUFFER_INDEX)
        {
        }

        array_trivial_copyable_type_base_impl& operator=(array_trivial_copyable_type_base_impl&& rhs) noexcept
        {
            base_type::operator=(std::move(rhs));
            m_data_access = details::trivial_copyable_type_data_access<T, self_type>(this, DATA_BUFFER_INDEX);
            return *this;
        }


    protected:

        static arrow_proxy create_proxy(
            size_type n,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<std::string_view> metadata = std::nullopt
        );

        template <validity_bitmap_input R = validity_bitmap>
        static auto create_proxy(
            u8_buffer<T>&& data_buffer,
            R&& bitmaps = validity_bitmap{},
            std::optional<std::string_view> name = std::nullopt,
            std::optional<std::string_view> metadata = std::nullopt
        ) -> arrow_proxy;

        // range of values (no missing values)
        template <std::ranges::input_range R>
            requires std::convertible_to<std::ranges::range_value_t<R>, T>
        static auto create_proxy(
            R&& range,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<std::string_view> metadata = std::nullopt
        ) -> arrow_proxy;

        template <class U>
            requires std::convertible_to<U, T>
        static arrow_proxy create_proxy(
            size_type n,
            const U& value = U{},
            std::optional<std::string_view> name = std::nullopt,
            std::optional<std::string_view> metadata = std::nullopt
        );

        // range of values, validity_bitmap_input
        template <std::ranges::input_range R, validity_bitmap_input R2>
            requires(std::convertible_to<std::ranges::range_value_t<R>, T>)
        static arrow_proxy create_proxy(
            R&&,
            R2&&,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<std::string_view> metadata = std::nullopt
        );

        // range of nullable values
        template <std::ranges::input_range R>
            requires std::is_same_v<std::ranges::range_value_t<R>, nullable<T>>
        static arrow_proxy create_proxy(
            R&&,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<std::string_view> metadata = std::nullopt
        );


        pointer data();
        const_pointer data() const;

        inner_reference value(size_type i);
        inner_const_reference value(size_type i) const;

        value_iterator value_begin();
        value_iterator value_end();

        const_value_iterator value_cbegin() const;
        const_value_iterator value_cend() const;

        // Modifiers

        void resize_values(size_type new_length, inner_value_type value);

        value_iterator insert_value(const_value_iterator pos, inner_value_type value, size_type count);

        template <mpl::iterator_of_type<inner_value_type> InputIt>
        value_iterator insert_values(const_value_iterator pos, InputIt first, InputIt last)
        {
            return m_data_access.insert_values(pos, first, last);
        }

        value_iterator erase_values(const_value_iterator pos, size_type count);

        details::trivial_copyable_type_data_access<T, self_type> m_data_access;

        static constexpr size_type DATA_BUFFER_INDEX = 1;

        friend class run_end_encoded_array;
        friend base_type;
        friend base_type::base_type;
        friend base_type::base_type::base_type;
    };

    /********************************************************
     * array_trivial_copyable_type_base_impl implementation *
     ********************************************************/

    template <trivial_copyable_type T>
    array_trivial_copyable_type_base_impl<T>::array_trivial_copyable_type_base_impl(arrow_proxy proxy_param)
        : base_type(std::move(proxy_param))
        , m_data_access(this, DATA_BUFFER_INDEX)
    {
    }

    template <trivial_copyable_type T>
    array_trivial_copyable_type_base_impl<T>::array_trivial_copyable_type_base_impl(
        const array_trivial_copyable_type_base_impl& rhs
    )
        : base_type(rhs)
        , m_data_access(this, DATA_BUFFER_INDEX)
    {
    }

    template <trivial_copyable_type T>
    array_trivial_copyable_type_base_impl<T>&
    array_trivial_copyable_type_base_impl<T>::operator=(const array_trivial_copyable_type_base_impl& rhs)
    {
        base_type::operator=(rhs);
        m_data_access = details::trivial_copyable_type_data_access<T, self_type>(this, DATA_BUFFER_INDEX);
        return *this;
    }

    template <trivial_copyable_type T>
    auto array_trivial_copyable_type_base_impl<T>::data() -> pointer
    {
        return m_data_access.data();
    }

    template <trivial_copyable_type T>
    auto array_trivial_copyable_type_base_impl<T>::data() const -> const_pointer
    {
        return m_data_access.data();
    }

    template <trivial_copyable_type T>
    auto array_trivial_copyable_type_base_impl<T>::value(size_type i) -> inner_reference
    {
        return m_data_access.value(i);
    }

    template <trivial_copyable_type T>
    auto array_trivial_copyable_type_base_impl<T>::value(size_type i) const -> inner_const_reference
    {
        return m_data_access.value(i);
    }

    template <trivial_copyable_type T>
    auto array_trivial_copyable_type_base_impl<T>::value_begin() -> value_iterator
    {
        return value_iterator{m_data_access.data()};
    }

    template <trivial_copyable_type T>
    auto array_trivial_copyable_type_base_impl<T>::value_end() -> value_iterator
    {
        return sparrow::next(value_begin(), this->size());
    }

    template <trivial_copyable_type T>
    auto array_trivial_copyable_type_base_impl<T>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator{m_data_access.data()};
    }

    template <trivial_copyable_type T>
    auto array_trivial_copyable_type_base_impl<T>::value_cend() const -> const_value_iterator
    {
        return sparrow::next(value_cbegin(), this->size());
    }

    template <trivial_copyable_type T>
    void array_trivial_copyable_type_base_impl<T>::resize_values(size_type new_length, inner_value_type value)
    {
        return m_data_access.resize_values(new_length, value);
    }

    template <trivial_copyable_type T>
    auto array_trivial_copyable_type_base_impl<T>::insert_value(
        const_value_iterator pos,
        inner_value_type value,
        size_type count
    ) -> value_iterator
    {
        return m_data_access.insert_value(pos, value, count);
    }

    template <trivial_copyable_type T>
    auto array_trivial_copyable_type_base_impl<T>::erase_values(const_value_iterator pos, size_type count)
        -> value_iterator
    {
        return m_data_access.erase_values(pos, count);
    }

    template <trivial_copyable_type T>
    template <validity_bitmap_input R>
    auto array_trivial_copyable_type_base_impl<T>::create_proxy(
        u8_buffer<T>&& data_buffer,
        R&& bitmap_input,
        std::optional<std::string_view> name,
        std::optional<std::string_view> metadata
    ) -> arrow_proxy
    {
        const auto size = data_buffer.size();
        validity_bitmap bitmap = ensure_validity_bitmap(size, std::forward<R>(bitmap_input));
        const auto null_count = bitmap.null_count();

        // create arrow schema and array
        ArrowSchema schema = make_arrow_schema(
            sparrow::data_type_format_of<T>(),  // format
            std::move(name),                    // name
            std::move(metadata),                // metadata
            std::nullopt,                       // flags
            0,                                  // n_children
            nullptr,                            // children
            nullptr                             // dictionary
        );

        std::vector<buffer<uint8_t>> buffers(2);
        buffers[0] = std::move(bitmap).extract_storage();
        buffers[1] = std::move(data_buffer).extract_storage();

        // create arrow array
        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(buffers),
            0,        // n_children
            nullptr,  // children
            nullptr   // dictionary
        );
        return arrow_proxy(std::move(arr), std::move(schema));
    }

    template <trivial_copyable_type T>
    template <std::ranges::input_range VALUE_RANGE, validity_bitmap_input R>
        requires(std::convertible_to<std::ranges::range_value_t<VALUE_RANGE>, T>)
    arrow_proxy array_trivial_copyable_type_base_impl<T>::create_proxy(
        VALUE_RANGE&& values,
        R&& validity_input,
        std::optional<std::string_view> name,
        std::optional<std::string_view> metadata
    )
    {
        u8_buffer<T> data_buffer(std::forward<VALUE_RANGE>(values));
        return create_proxy(
            std::move(data_buffer),
            std::forward<R>(validity_input),
            std::move(name),
            std::move(metadata)
        );
    }

    template <trivial_copyable_type T>
    template <class U>
        requires std::convertible_to<U, T>
    arrow_proxy array_trivial_copyable_type_base_impl<T>::create_proxy(
        size_type n,
        const U& value,
        std::optional<std::string_view> name,
        std::optional<std::string_view> metadata
    )
    {
        // create data_buffer
        u8_buffer<T> data_buffer(n, value);
        return create_proxy(std::move(data_buffer), std::move(name), std::move(metadata));
    }

    template <trivial_copyable_type T>
    template <std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_value_t<R>, T>
    arrow_proxy array_trivial_copyable_type_base_impl<T>::create_proxy(
        R&& range,
        std::optional<std::string_view> name,
        std::optional<std::string_view> metadata
    )
    {
        const std::size_t n = range_size(range);
        const auto iota = std::ranges::iota_view{std::size_t(0), n};
        std::ranges::transform_view iota_to_is_non_missing(
            iota,
            [](std::size_t)
            {
                return true;
            }
        );
        return self_type::create_proxy(
            std::forward<R>(range),
            std::move(iota_to_is_non_missing),
            std::move(name),
            std::move(metadata)
        );
    }

    // range of nullable values
    template <trivial_copyable_type T>
    template <std::ranges::input_range R>
        requires std::is_same_v<std::ranges::range_value_t<R>, nullable<T>>
    arrow_proxy array_trivial_copyable_type_base_impl<T>::create_proxy(
        R&& range,
        std::optional<std::string_view> name,
        std::optional<std::string_view> metadata
    )
    {
        // split into values and is_non_null ranges
        auto values = range
                      | std::views::transform(
                          [](const auto& v)
                          {
                              return v.get();
                          }
                      );
        auto is_non_null = range
                           | std::views::transform(
                               [](const auto& v)
                               {
                                   return v.has_value();
                               }
                           );
        return self_type::create_proxy(values, is_non_null, std::move(name), std::move(metadata));
    }
}