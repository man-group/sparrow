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
#include <ranges>
#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/buffer/buffer_adaptor.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/layout/primitive_array.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/buffer/u8_buffer.hpp"

namespace sparrow
{
    template <class T>
    class primitive_array;

    template <class T>
    struct array_inner_types<primitive_array<T>> : array_inner_types_base
    {
        using array_type = primitive_array<T>;

        using inner_value_type = T;
        using inner_reference = T&;
        using inner_const_reference = const T&;
        using pointer = inner_value_type*;
        using const_pointer = const inner_value_type*;

        using value_iterator = pointer_iterator<pointer>;
        using const_value_iterator = pointer_iterator<const_pointer>;
        using bitmap_const_reference = bitmap_type::const_reference;

        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;

        using iterator_tag = std::random_access_iterator_tag;
    };

    /**
     * Array of values of whose type has fixed binary size.
     *
     * The type of the values in the array can be a primitive type, whose size is known at compile
     * time, or an arbitrary binary type whose fixed size is known at runtime only.
     * The current implementation supports types whose size is known at compile time only.
     *
     * As the other arrays in sparrow, \c primitive_array<T> provides an API as if it was holding
     * \c nullable<T> values instead of \c T values.
     *
     * Internally, the array contains a validity bitmap and a contiguous memory buffer
     * holding the values.
     *
     * @tparam T the type of the values in the array.
     * @see https://arrow.apache.org/docs/dev/format/Columnar.html#fixed-size-primitive-layout
     */
    template <class T>
    class primitive_array final : public mutable_array_bitmap_base<primitive_array<T>>
    {
    public:

        using self_type = primitive_array<T>;
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

        using pointer = typename inner_types::pointer;
        using const_pointer = typename inner_types::const_pointer;

        using size_type = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using iterator_tag = typename base_type::iterator_tag;

        using value_iterator = typename base_type::value_iterator;
        using const_value_iterator = typename base_type::const_value_iterator;

        using iterator = typename base_type::iterator;
        using const_iterator = typename base_type::const_iterator;

        explicit primitive_array(arrow_proxy);

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
         * primitive_array<int> pr(std::ranges::iota_view{0, 10}, a_bitmap);
         * ```
         * - a range of indices indicating the missing values.
         * ```cpp
         * std::vector<std::size_t> false_pos  { 3, 8 };
         * primitive_array<int> pr(std::ranges::iota_view{0, 10}, a_bitmap);
         * ```
         * - omitted: this is equivalent as passing a bitmap range full of \c true.
         * ```cpp
         * primitive_array<int> pr((std::ranges::iota_view{0, 10});
         * ```
         */
        template <class ... Args>
        requires(mpl::excludes_copy_and_move_ctor_v<primitive_array<T>, Args...>)
        explicit primitive_array(Args&& ... args)
            : base_type(create_proxy(std::forward<Args>(args) ...))
        {
        }

        /**
         * Constructs a primitive array from an \c initializer_list of raw values.
         */
        primitive_array(std::initializer_list<inner_value_type> init)
            : base_type(create_proxy(init))
        {
        }

    private:


        pointer data();
        const_pointer data() const;

        inner_reference value(size_type i);
        inner_const_reference value(size_type i) const;

        value_iterator value_begin();
        value_iterator value_end();

        const_value_iterator value_cbegin() const;
        const_value_iterator value_cend() const;

        static arrow_proxy create_proxy(size_type n);

        template <validity_bitmap_input R = validity_bitmap>
        static auto create_proxy(
            u8_buffer<T>&& data_buffer,    
            R && bitmaps = validity_bitmap{}
        ) -> arrow_proxy;

        // range of values (no missing values)
        template <std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_value_t<R>, T>
        static auto create_proxy(R&& range) -> arrow_proxy;

        template<class U>
        requires std::convertible_to<U, T>
        static arrow_proxy create_proxy(size_type n, const U& value = U{});

        // range of values, validity_bitmap_input
        template <std::ranges::input_range R, validity_bitmap_input R2>
        requires(std::convertible_to<std::ranges::range_value_t<R>, T>)
        static arrow_proxy create_proxy(R&&, R2&&);

        // range of nullable values
        template <std::ranges::input_range R>
        requires std::is_same_v<
            std::ranges::range_value_t<R>, nullable<T>>
        static arrow_proxy create_proxy(R&&);

        // Modifiers

        void resize_values(size_type new_length, inner_value_type value);

        value_iterator insert_value(const_value_iterator pos, inner_value_type value, size_type count);

        template <mpl::iterator_of_type<T> InputIt>
        value_iterator insert_values(const_value_iterator pos, InputIt first, InputIt last);

        value_iterator erase_values(const_value_iterator pos, size_type count);

        buffer_adaptor<T, buffer<uint8_t>&> get_data_buffer();

        static constexpr size_type DATA_BUFFER_INDEX = 1;

        friend class run_end_encoded_array;
        friend base_type;
        friend base_type::base_type;
        friend base_type::base_type::base_type;
    };

    /**********************************
     * primitive_array implementation *
     **********************************/

    namespace detail
    {
        inline bool check_primitive_data_type(data_type dt)
        {
            constexpr std::array<data_type, 14> dtypes = {
                data_type::BOOL,
                data_type::UINT8,
                data_type::INT8,
                data_type::UINT16,
                data_type::INT16,
                data_type::UINT32,
                data_type::INT32,
                data_type::UINT64,
                data_type::INT64,
                data_type::HALF_FLOAT,
                data_type::FLOAT,
                data_type::DOUBLE,
                data_type::FIXED_SIZE_BINARY,
                data_type::TIMESTAMP
            };
            return std::find(dtypes.cbegin(), dtypes.cend(), dt) != dtypes.cend();
        }
    }

    template <class T>
    primitive_array<T>::primitive_array(arrow_proxy proxy)
        : base_type(std::move(proxy))
    {
        SPARROW_ASSERT_TRUE(this->get_arrow_proxy().data_type() == arrow_traits<T>::type_id);
    }
    
    template <class T>
    template <validity_bitmap_input R >
    auto primitive_array<T>::create_proxy(
            u8_buffer<T>&& data_buffer,    
            R && bitmap_input
    ) -> arrow_proxy
    {
        const auto size = data_buffer.size();
        validity_bitmap bitmap = ensure_validity_bitmap(size, std::forward<R>(bitmap_input));
        const auto null_count =  bitmap.null_count();

        // create arrow schema and array
        ArrowSchema schema = make_arrow_schema(
            sparrow::data_type_format_of<T>(),
            std::nullopt, // name
            std::nullopt, // metadata
            std::nullopt, // flags
            0, // n_children
            nullptr, // children
            nullptr // dictionary
        );

        std::vector<buffer<uint8_t>> buffers(2);
        buffers[0] = std::move(bitmap).extract_storage();
        buffers[1] = std::move(data_buffer).extract_storage();

        // create arrow array
        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size), // length
            static_cast<int64_t>(null_count),
            0, // offset
            std::move(buffers),
            0, // n_children
            nullptr, // children
            nullptr // dictionary
        );
        return arrow_proxy(std::move(arr), std::move(schema));
    }

    template <class T>
    template <std::ranges::input_range VALUE_RANGE, validity_bitmap_input R>
    requires(std::convertible_to<std::ranges::range_value_t<VALUE_RANGE>, T>)
    arrow_proxy primitive_array<T>::create_proxy(VALUE_RANGE && values, R && validity_input)
    {
        u8_buffer<T> data_buffer(std::forward<VALUE_RANGE>(values));
        return create_proxy(std::move(data_buffer), std::forward<R>(validity_input));
    }

    template <class T>
    template<class U>
    requires std::convertible_to<U, T>
    arrow_proxy primitive_array<T>::create_proxy(size_type n, const U& value)
    {   
        // create data_buffer
        u8_buffer<T> data_buffer(n, value);
        return create_proxy(std::move(data_buffer));
    }

    template <class T>
    template <std::ranges::input_range R>
    requires std::convertible_to<std::ranges::range_value_t<R>, T>
    arrow_proxy primitive_array<T>::create_proxy(R&& range)
    {
        const std::size_t n = std::ranges::size(range);
        auto iota = std::ranges::iota_view{std::size_t(0), n};
        std::ranges::transform_view iota_to_is_non_missing(iota, [](std::size_t) { return true; });
        return self_type::create_proxy(std::forward<R>(range), std::move(iota_to_is_non_missing));
    }

    // range of nullable values
    template <class T>
    template <std::ranges::input_range R>
    requires std::is_same_v<std::ranges::range_value_t<R>, nullable<T>>
    arrow_proxy primitive_array<T>::create_proxy(R&& range)
    {
        // split into values and is_non_null ranges
        auto values = range | std::views::transform([](const auto& v) { return v.get(); });
        auto is_non_null = range | std::views::transform([](const auto& v) { return v.has_value(); });
        return self_type::create_proxy(values, is_non_null);
    }

    template <class T>
    auto primitive_array<T>::data() -> pointer
    {
        return this->get_arrow_proxy().buffers()[DATA_BUFFER_INDEX].template data<inner_value_type>()
               + static_cast<size_type>(this->get_arrow_proxy().offset());
    }

    template <class T>
    auto primitive_array<T>::data() const -> const_pointer
    {
        return this->get_arrow_proxy().buffers()[DATA_BUFFER_INDEX].template data<const inner_value_type>()
               + static_cast<size_type>(this->get_arrow_proxy().offset());
    }

    template <class T>
    auto primitive_array<T>::value(size_type i) -> inner_reference
    {
        SPARROW_ASSERT_TRUE(i < this->size());
        return data()[i];
    }

    template <class T>
    auto primitive_array<T>::value(size_type i) const -> inner_const_reference
    {
        SPARROW_ASSERT_TRUE(i < this->size());
        return data()[i];
    }

    template <class T>
    auto primitive_array<T>::value_begin() -> value_iterator
    {
        return value_iterator{data()};
    }

    template <class T>
    auto primitive_array<T>::value_end() -> value_iterator
    {
        return sparrow::next(value_begin(), this->size());
    }

    template <class T>
    auto primitive_array<T>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator{data()};
    }

    template <class T>
    auto primitive_array<T>::value_cend() const -> const_value_iterator
    {
        return sparrow::next(value_cbegin(), this->size());
    }

    template <class T>
    buffer_adaptor<T, buffer<uint8_t>&> primitive_array<T>::get_data_buffer()
    {
        auto& buffers = this->get_arrow_proxy().get_array_private_data()->buffers();
        return make_buffer_adaptor<T>(buffers[DATA_BUFFER_INDEX]);
    }

    template <class T>
    void primitive_array<T>::resize_values(size_type new_length, inner_value_type value)
    {
        const size_t new_size = new_length + static_cast<size_t>(this->get_arrow_proxy().offset());
        get_data_buffer().resize(new_size, value);
    }

    template <class T>
    auto primitive_array<T>::insert_value(const_value_iterator pos, inner_value_type value, size_type count)
        -> value_iterator
    {
        SPARROW_ASSERT_TRUE(value_cbegin() <= pos)
        SPARROW_ASSERT_TRUE(pos <= value_cend());
        const auto distance = std::distance(value_cbegin(), sparrow::next(pos, this->get_arrow_proxy().offset()));
        get_data_buffer().insert(pos, count, value);
        return sparrow::next(this->value_begin(), distance);
    }

    template <class T>
    template <mpl::iterator_of_type<T> InputIt>
    auto
    primitive_array<T>::insert_values(const_value_iterator pos, InputIt first, InputIt last) -> value_iterator
    {
        SPARROW_ASSERT_TRUE(value_cbegin() <= pos)
        SPARROW_ASSERT_TRUE(pos <= value_cend());
        const auto distance = std::distance(value_cbegin(), sparrow::next(pos, this->get_arrow_proxy().offset()));
        get_data_buffer().insert(pos, first, last);
        return sparrow::next(this->value_begin(), distance);
    }

    template <class T>
    auto primitive_array<T>::erase_values(const_value_iterator pos, size_type count) -> value_iterator
    {
        SPARROW_ASSERT_TRUE(this->value_cbegin() <= pos)
        SPARROW_ASSERT_TRUE(pos < this->value_cend());
        const size_type distance = static_cast<size_t>(
            std::distance(this->value_cbegin(), sparrow::next(pos, this->get_arrow_proxy().offset()))
        );
        auto data_buffer = get_data_buffer();
        const auto first = sparrow::next(data_buffer.cbegin(), distance);
        const auto last = sparrow::next(first, count);
        data_buffer.erase(first, last);
        return sparrow::next(this->value_begin(), distance);
    }
}
