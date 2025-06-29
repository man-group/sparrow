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

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/buffer/u8_buffer.hpp"
#include "sparrow/layout/array_base.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/primitive_layout/primitive_data_access.hpp"
#include "sparrow/layout/temporal/timestamp_concepts.hpp"
#include "sparrow/layout/temporal/timestamp_reference.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/metadata.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/repeat_container.hpp"
#include "sparrow/utils/temporal.hpp"

// tts : timestamp<std::chrono::seconds>
// tsm : timestamp<std::chrono::milliseconds>
// tsu : timestamp<std::chrono::microseconds>
// tsn : timestamp<std::chrono::nanoseconds>

namespace sparrow
{
    template <timestamp_type T>
    class timestamp_array;

    template <timestamp_type T>
    struct array_inner_types<timestamp_array<T>> : array_inner_types_base
    {
        using self_type = timestamp_array<T>;

        using inner_value_type = T;
        using inner_reference = timestamp_reference<self_type>;
        using inner_const_reference = T;

        using functor_type = detail::layout_value_functor<self_type, inner_reference>;
        using const_functor_type = detail::layout_value_functor<const self_type, inner_const_reference>;

        using value_iterator = functor_index_iterator<functor_type>;
        using const_value_iterator = functor_index_iterator<const_functor_type>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    template <typename T>
    struct is_timestamp_array : std::false_type
    {
    };

    template <typename T>
    struct is_timestamp_array<timestamp_array<T>> : std::true_type
    {
    };

    template <typename T>
    constexpr bool is_timestamp_array_v = is_timestamp_array<T>::value;

    using timestamp_second = timestamp<std::chrono::seconds>;
    using timestamp_millisecond = timestamp<std::chrono::milliseconds>;
    using timestamp_microsecond = timestamp<std::chrono::microseconds>;
    using timestamp_nanosecond = timestamp<std::chrono::nanoseconds>;

    using timestamp_seconds_array = timestamp_array<timestamp_second>;
    using timestamp_milliseconds_array = timestamp_array<timestamp_millisecond>;
    using timestamp_microseconds_array = timestamp_array<timestamp_microsecond>;
    using timestamp_nanoseconds_array = timestamp_array<timestamp_nanosecond>;

    /**
     * Array of timestamps.
     *
     * The type of the values in the array are \c timestamp, whose duration/representation is known at compile
     * time.
     * The current implementation supports types whose size is known at compile time only.
     *
     * As the other arrays in sparrow, \c timestamp_array<T> provides an API as if it was holding
     * \c nullable<T> values instead of \c T values.
     *
     * Internally, the array contains a validity bitmap and a contiguous memory buffer
     * holding the values.
     *
     * @tparam T the type of the values in the array.
     * Must be one of these values:
     * - \c timestamp<std::chrono::seconds>
     * - \c timestamp<std::chrono::milliseconds>
     * - \c timestamp<std::chrono::microseconds>
     * - \c timestamp<std::chrono::nanoseconds>
     *
     * @see https://arrow.apache.org/docs/dev/format/Columnar.html#fixed-size-primitive-layout
     */
    template <timestamp_type T>
    class timestamp_array final : public mutable_array_bitmap_base<timestamp_array<T>>
    {
    public:

        using self_type = timestamp_array;
        using base_type = mutable_array_bitmap_base<self_type>;

        using inner_types = array_inner_types<self_type>;
        using inner_value_type = typename inner_types::inner_value_type;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;

        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_reference = typename base_type::bitmap_reference;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;
        using bitmap_iterator = typename base_type::bitmap_iterator;
        using const_bitmap_iterator = typename base_type::const_bitmap_iterator;
        using bitmap_range = typename base_type::bitmap_range;
        using const_bitmap_range = typename base_type::const_bitmap_range;

        using value_type = nullable<inner_value_type>;
        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;

        using size_type = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using iterator_tag = typename base_type::iterator_tag;

        using value_iterator = typename base_type::value_iterator;
        using const_value_iterator = typename base_type::const_value_iterator;

        using iterator = typename base_type::iterator;
        using const_iterator = typename base_type::const_iterator;

        using functor_type = typename inner_types::functor_type;
        using const_functor_type = typename inner_types::const_functor_type;

        using inner_value_type_duration = inner_value_type::duration;
        using buffer_inner_value_type = inner_value_type_duration::rep;
        using buffer_inner_value_iterator = pointer_iterator<buffer_inner_value_type*>;
        using buffer_inner_const_value_iterator = pointer_iterator<const buffer_inner_value_type*>;

        explicit timestamp_array(arrow_proxy);

        /**
         * Construct a timestamp array with the passed range of values and an optional bitmap.
         *
         * The first argument is `const date::time_zone*`. It is the timezone of the timestamps.
         * The second argument can be any range of values as long as its value type is convertible
         * to \c T .
         * The third argument can be:
         * - a bitmap range, i.e. a range of boolean-like values indicating the non-missing values.
         *   The bitmap range and the value range must have the same size.
         * ```cpp
         * std::vector<sparrow::timestamp<std::chrono::seconds>> input_values;
         * *** fill input_values ***
         * std::vector<bool> a_bitmap(input_values.size(), true);
         * a_bitmap[3] = false;
         * const date::time_zone* newyork_tz = date::locate_zone("America/New_York");
         * primitive_array<int> pr(input_values, a_bitmap);
         * ```
         * - a range of indices indicating the missing values.
         * ```cpp
         * std::vector<std::size_t> false_pos  { 3, 8 };
         * primitive_array<int> pr(input_values, a_bitmap);
         * ```
         * - omitted: this is equivalent as passing a bitmap range full of \c true.
         * ```cpp
         * primitive_array<int> pr(input_values);
         * ```
         */
        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<timestamp_array, Args...>)
        constexpr explicit timestamp_array(Args&&... args)
            : base_type(create_proxy(std::forward<Args>(args)...))
            , m_timezone(get_timezone(this->get_arrow_proxy()))
            , m_data_access(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
        {
        }

        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        constexpr timestamp_array(
            const date::time_zone* timezone,
            std::initializer_list<inner_value_type> init,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
            : base_type(create_proxy(timezone, init, std::move(name), std::move(metadata)))
            , m_timezone(timezone)
            , m_data_access(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
        {
        }

        constexpr timestamp_array(const timestamp_array& rhs);
        constexpr timestamp_array& operator=(const timestamp_array& rhs);

        constexpr timestamp_array(timestamp_array&& rhs);
        constexpr timestamp_array& operator=(timestamp_array&& rhs);

    private:

        [[nodiscard]] constexpr inner_reference value(size_type i);
        [[nodiscard]] constexpr inner_const_reference value(size_type i) const;

        [[nodiscard]] constexpr value_iterator value_begin();
        [[nodiscard]] constexpr value_iterator value_end();

        [[nodiscard]] constexpr const_value_iterator value_cbegin() const;
        [[nodiscard]] constexpr const_value_iterator value_cend() const;

        template <input_metadata_container METADATA_RANGE>
        [[nodiscard]] static arrow_proxy create_proxy(
            const date::time_zone* timezone,
            size_type n,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        template <
            validity_bitmap_input R = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static auto create_proxy(
            const date::time_zone* timezone,
            u8_buffer<buffer_inner_value_type>&& data_buffer,
            R&& bitmaps = validity_bitmap{},
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        // range of values (no missing values)
        template <std::ranges::input_range R, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<std::ranges::range_value_t<R>, T>
        [[nodiscard]] static auto create_proxy(
            const date::time_zone* timezone,
            R&& range,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        template <typename U, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<U, T>
        [[nodiscard]] static arrow_proxy create_proxy(
            const date::time_zone* timezone,
            size_type n,
            const U& value = U{},
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        // range of values, validity_bitmap_input
        template <
            std::ranges::input_range VALUE_RANGE,
            validity_bitmap_input VALIDITY_RANGE,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(std::convertible_to<std::ranges::range_value_t<VALUE_RANGE>, T>)
        [[nodiscard]] static arrow_proxy create_proxy(
            const date::time_zone* timezone,
            VALUE_RANGE&&,
            VALIDITY_RANGE&&,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        // range of nullable values
        template <std::ranges::input_range R, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::is_same_v<std::ranges::range_value_t<R>, nullable<T>>
        [[nodiscard]] static arrow_proxy create_proxy(
            const date::time_zone* timezone,
            R&&,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy_impl(
            const date::time_zone* timezone,
            u8_buffer<buffer_inner_value_type>&& data_buffer,
            std::optional<validity_bitmap>&& bitmap_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        // Modifiers

        constexpr void resize_values(size_type new_length, inner_value_type value);

        constexpr value_iterator
        insert_value(const_value_iterator pos, inner_value_type value, size_type count);

        template <mpl::iterator_of_type<typename timestamp_array<T>::inner_value_type> InputIt>
        constexpr auto insert_values(const_value_iterator pos, InputIt first, InputIt last) -> value_iterator
        {
            const auto input_range = std::ranges::subrange(first, last);
            const auto values = input_range
                                | std::views::transform(
                                    [](const auto& v)
                                    {
                                        return v.get_sys_time().time_since_epoch();
                                    }
                                );
            const size_t idx = static_cast<size_t>(std::distance(value_cbegin(), pos));
            m_data_access.insert_values(idx, values.begin(), values.end());
            return sparrow::next(value_begin(), idx);
        }

        constexpr value_iterator erase_values(const_value_iterator pos, size_type count);

        constexpr void assign(const T& rhs, size_type index);
        constexpr void assign(T&& rhs, size_type index);


        const date::time_zone* m_timezone;
        details::primitive_data_access<inner_value_type_duration> m_data_access;

        static constexpr size_type DATA_BUFFER_INDEX = 1;
        friend class timestamp_reference<self_type>;
        friend base_type;
        friend base_type::base_type;
        friend base_type::base_type::base_type;
        friend functor_type;
        friend const_functor_type;
    };

    template <timestamp_type T>
    constexpr timestamp_array<T>::timestamp_array(const timestamp_array& rhs)
        : base_type(rhs)
        , m_timezone(rhs.m_timezone)
        , m_data_access(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
    {
    }

    template <timestamp_type T>
    constexpr timestamp_array<T>& timestamp_array<T>::operator=(const timestamp_array& rhs)
    {
        base_type::operator=(rhs);
        m_timezone = rhs.m_timezone;
        m_data_access.reset_proxy(this->get_arrow_proxy());
        return *this;
    }

    template <timestamp_type T>
    constexpr timestamp_array<T>::timestamp_array(timestamp_array&& rhs)
        : base_type(std::move(rhs))
        , m_timezone(rhs.m_timezone)
        , m_data_access(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
    {
    }

    template <timestamp_type T>
    constexpr timestamp_array<T>& timestamp_array<T>::operator=(timestamp_array&& rhs)
    {
        base_type::operator=(std::move(rhs));
        m_timezone = rhs.m_timezone;
        m_data_access.reset_proxy(this->get_arrow_proxy());
        return *this;
    }

    template <timestamp_type T>
    timestamp_array<T>::timestamp_array(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , m_timezone(get_timezone(this->get_arrow_proxy()))
        , m_data_access(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
    {
    }

    template <timestamp_type T>
    template <validity_bitmap_input R, input_metadata_container METADATA_RANGE>
    auto timestamp_array<T>::create_proxy(
        const date::time_zone* timezone,
        u8_buffer<buffer_inner_value_type>&& data_buffer,
        R&& bitmap_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        const auto size = data_buffer.size();
        validity_bitmap bitmap = ensure_validity_bitmap(size, std::forward<R>(bitmap_input));
        return create_proxy_impl(
            timezone,
            std::forward<u8_buffer<buffer_inner_value_type>>(data_buffer),
            std::move(bitmap),
            std::move(name),
            std::move(metadata)
        );
    }

    template <timestamp_type T>
    template <std::ranges::input_range VALUE_RANGE, validity_bitmap_input VALIDITY_RANGE, input_metadata_container METADATA_RANGE>
        requires(std::convertible_to<std::ranges::range_value_t<VALUE_RANGE>, T>)
    arrow_proxy timestamp_array<T>::create_proxy(
        const date::time_zone* timezone,
        VALUE_RANGE&& values,
        VALIDITY_RANGE&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto range = values
                           | std::views::transform(
                               [](const auto& v)
                               {
                                   return v.get_sys_time().time_since_epoch().count();
                               }
                           );


        u8_buffer<buffer_inner_value_type> data_buffer(range);
        return create_proxy(
            timezone,
            std::move(data_buffer),
            std::forward<VALIDITY_RANGE>(validity_input),
            std::move(name),
            std::move(metadata)
        );
    }

    template <timestamp_type T>
    template <typename U, input_metadata_container METADATA_RANGE>
        requires std::convertible_to<U, T>
    arrow_proxy timestamp_array<T>::create_proxy(
        const date::time_zone* timezone,
        size_type n,
        const U& value,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        // create data_buffer
        u8_buffer<buffer_inner_value_type> data_buffer(n, to_days_since_the_UNIX_epoch(value));
        return create_proxy(timezone, std::move(data_buffer), std::move(name), std::move(metadata));
    }

    template <timestamp_type T>
    template <std::ranges::input_range R, input_metadata_container METADATA_RANGE>
        requires std::convertible_to<std::ranges::range_value_t<R>, T>
    arrow_proxy timestamp_array<T>::create_proxy(
        const date::time_zone* timezone,
        R&& range,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        std::optional<validity_bitmap> bitmap = nullable ? std::make_optional<validity_bitmap>(nullptr, 0)
                                                         : std::nullopt;
        const auto values = range
                            | std::views::transform(
                                [](const auto& v)
                                {
                                    return v.get_sys_time().time_since_epoch().count();
                                }
                            );
        u8_buffer<buffer_inner_value_type> data_buffer(values);
        return self_type::create_proxy_impl(
            timezone,
            std::move(data_buffer),
            std::move(bitmap),
            std::move(name),
            std::move(metadata)
        );
    }

    // range of nullable values
    template <timestamp_type T>
    template <std::ranges::input_range R, input_metadata_container METADATA_RANGE>
        requires std::is_same_v<std::ranges::range_value_t<R>, nullable<T>>
    arrow_proxy timestamp_array<T>::create_proxy(
        const date::time_zone* timezone,
        R&& range,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {  // split into values and is_non_null ranges
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
        return self_type::create_proxy(timezone, values, is_non_null, std::move(name), std::move(metadata));
    }

    template <timestamp_type T>
    template <input_metadata_container METADATA_RANGE>
    arrow_proxy timestamp_array<T>::create_proxy_impl(
        const date::time_zone* timezone,
        u8_buffer<buffer_inner_value_type>&& data_buffer,
        std::optional<validity_bitmap>&& bitmap,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto size = data_buffer.size();
        const auto null_count = bitmap.has_value() ? bitmap->null_count() : 0;

        std::string format(data_type_to_format(arrow_traits<T>::type_id));
        format += timezone->name();

        const repeat_view<bool> children_ownership{true, 0};

        const std::optional<std::unordered_set<sparrow::ArrowFlag>>
            flags = bitmap.has_value()
                        ? std::make_optional<std::unordered_set<sparrow::ArrowFlag>>({ArrowFlag::NULLABLE})
                        : std::nullopt;

        // create arrow schema and array
        ArrowSchema schema = make_arrow_schema(
            std::move(format),    // format
            std::move(name),      // name
            std::move(metadata),  // metadata
            flags,                // flags
            nullptr,              // children
            children_ownership,   // children ownership
            nullptr,              // dictionary,
            true                  // dictionary ownership
        );

        std::vector<buffer<uint8_t>> buffers{
            bitmap.has_value() ? std::move(bitmap.value()).extract_storage() : buffer<uint8_t>{nullptr, 0},
            std::move(data_buffer).extract_storage()
        };

        // create arrow array
        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(buffers),
            nullptr,             // children
            children_ownership,  // children ownership
            nullptr,             // dictionary
            true                 // dicitonary ownership
        );
        return arrow_proxy(std::move(arr), std::move(schema));
    }

    template <timestamp_type T>
    constexpr void timestamp_array<T>::assign(const T& rhs, size_type index)
    {
        SPARROW_ASSERT_TRUE(index < this->size());
        m_data_access.value(index) = rhs.get_sys_time().time_since_epoch();
    }

    template <timestamp_type T>
    constexpr void timestamp_array<T>::assign(T&& rhs, size_type index)
    {
        SPARROW_ASSERT_TRUE(index < this->size());
        m_data_access.value(index) = rhs.get_sys_time().time_since_epoch();
    }

    template <timestamp_type T>
    constexpr auto timestamp_array<T>::value(size_type i) -> inner_reference
    {
        SPARROW_ASSERT_TRUE(i < this->size());
        return inner_reference(this, i);
    }

    template <timestamp_type T>
    constexpr auto timestamp_array<T>::value(size_type i) const -> inner_const_reference
    {
        SPARROW_ASSERT_TRUE(i < this->size());
        const auto& val = m_data_access.value(i);
        using time_duration = typename T::duration;
        const auto sys_time = std::chrono::sys_time<time_duration>{val};
        return T{m_timezone, sys_time};
    }

    template <timestamp_type T>
    constexpr auto timestamp_array<T>::value_begin() -> value_iterator
    {
        return value_iterator(functor_type(this), 0);
    }

    template <timestamp_type T>
    constexpr auto timestamp_array<T>::value_end() -> value_iterator
    {
        return value_iterator(functor_type(this), this->size());
    }

    template <timestamp_type T>
    constexpr auto timestamp_array<T>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator(const_functor_type(this), 0);
    }

    template <timestamp_type T>
    constexpr auto timestamp_array<T>::value_cend() const -> const_value_iterator
    {
        return const_value_iterator(const_functor_type(this), this->size());
    }

    template <timestamp_type T>
    constexpr void timestamp_array<T>::resize_values(size_type new_length, inner_value_type value)
    {
        m_data_access.resize_values(new_length, value.get_sys_time().time_since_epoch());
    }

    template <timestamp_type T>
    constexpr auto
    timestamp_array<T>::insert_value(const_value_iterator pos, inner_value_type value, size_type count)
        -> value_iterator
    {
        SPARROW_ASSERT_TRUE(pos <= value_cend());
        const size_t idx = static_cast<size_t>(std::distance(value_cbegin(), pos));
        m_data_access.insert_value(idx, value.get_sys_time().time_since_epoch(), count);
        return value_iterator(functor_type(this), idx);
    }

    template <timestamp_type T>
    constexpr auto timestamp_array<T>::erase_values(const_value_iterator pos, size_type count) -> value_iterator
    {
        SPARROW_ASSERT_TRUE(pos < value_cend());
        const size_t idx = static_cast<size_t>(std::distance(value_cbegin(), pos));
        m_data_access.erase_values(idx, count);
        return value_iterator(functor_type(this), idx);
    }
}
