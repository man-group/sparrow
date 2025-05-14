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
#include "sparrow/layout/primitive_layout/primitive_data_access.hpp"
#include "sparrow/layout/temporal/date_types.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/repeat_container.hpp"

namespace sparrow
{
    template <trivial_copyable_type T>
    class primitive_array_impl;

    template <trivial_copyable_type T>
    struct array_inner_types<primitive_array_impl<T>> : array_inner_types_base
    {
        using array_type = primitive_array_impl<T>;

        using data_access_type = details::primitive_data_access<T>;
        using inner_value_type = typename data_access_type::inner_value_type;
        using inner_reference = typename data_access_type::inner_reference;
        using inner_const_reference = typename data_access_type::inner_const_reference;
        using pointer = typename data_access_type::inner_pointer;
        using const_pointer = typename data_access_type::inner_const_pointer;

        using value_iterator = typename data_access_type::value_iterator;
        using const_value_iterator = typename data_access_type::const_value_iterator;

        using bitmap_const_reference = bitmap_type::const_reference;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;

        using iterator_tag = std::random_access_iterator_tag;
    };

    template <trivial_copyable_type T>
    class primitive_array_impl final : public mutable_array_bitmap_base<primitive_array_impl<T>>,
                                       private details::primitive_data_access<T>
    {
    public:

        using self_type = primitive_array_impl<T>;
        using base_type = mutable_array_bitmap_base<primitive_array_impl<T>>;
        using access_class_type = details::primitive_data_access<T>;
        using size_type = std::size_t;

        using inner_types = array_inner_types<self_type>;
        using inner_value_type = typename inner_types::inner_value_type;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;

        using pointer = typename inner_types::pointer;
        using const_pointer = typename inner_types::const_pointer;

        using value_iterator = typename base_type::value_iterator;
        using const_value_iterator = typename base_type::const_value_iterator;

        explicit primitive_array_impl(arrow_proxy);

        /**
         * Constructs an array of trivial copyable type, with the passed range of values and an optional
         * bitmap.
         *
         * The first argument can be any range of values as long as its value type is convertible
         * to \c T.
         * The second argument can be:
         * - a bitmap range, i.e. a range of boolean-like values indicating the non-missing values.
         *   The bitmap range and the value range must have the same size.
         * ```cpp
         * std::vector<bool> a_bitmap(10, true);
         * a_bitmap[3] = false;
         * primitive_array_impl<int> pr(std::ranges::iota_view{0, 10}, a_bitmap);
         * ```
         * - a range of indices indicating the missing values.
         * ```cpp
         * std::vector<std::size_t> false_pos  { 3, 8 };
         * primitive_array_impl<int> pr(std::ranges::iota_view{0, 10}, a_bitmap);
         * ```
         * - omitted: this is equivalent as passing a bitmap range full of \c true.
         * ```cpp
         * primitive_array_impl<int> pr((std::ranges::iota_view{0, 10});
         * ```
         */
        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<primitive_array_impl<T>, Args...>)
        explicit primitive_array_impl(Args&&... args)
            : base_type(create_proxy(std::forward<Args>(args)...))
            , access_class_type(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
        {
        }

        /**
         * Constructs a primitive array from an \c initializer_list of raw values.
         */
        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        primitive_array_impl(
            std::initializer_list<inner_value_type> init,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
            : base_type(create_proxy(init, nullable, std::move(name), std::move(metadata)))
            , access_class_type(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
        {
        }

        primitive_array_impl(const primitive_array_impl&);
        primitive_array_impl& operator=(const primitive_array_impl&);

        primitive_array_impl(primitive_array_impl&&);
        primitive_array_impl& operator=(primitive_array_impl&&);

    private:

        template <
            validity_bitmap_input VALIDITY_RANGE = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static auto create_proxy(
            u8_buffer<T>&& data_buffer,
            size_t size,
            VALIDITY_RANGE&& bitmaps,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        template <
            validity_bitmap_input VALIDITY_RANGE = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static auto create_proxy(
            u8_buffer<T>&& data_buffer,
            size_t size,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        // range of values (no missing values)
        template <std::ranges::input_range R, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(std::convertible_to<std::ranges::range_value_t<R>, T> && !mpl::is_type_instance_of_v<R, u8_buffer>)
        [[nodiscard]] static auto create_proxy(
            R&& range,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        template <class U, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<U, T>
        [[nodiscard]] static arrow_proxy create_proxy(
            size_type n,
            const U& value = U{},
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        // range of values, validity_bitmap_input
        template <
            std::ranges::input_range R,
            validity_bitmap_input R2,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(std::convertible_to<std::ranges::range_value_t<R>, T>)
        [[nodiscard]] static arrow_proxy create_proxy(
            R&&,
            R2&&,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        // range of nullable values
        template <std::ranges::input_range NULLABLE_RANGE, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::is_same_v<std::ranges::range_value_t<NULLABLE_RANGE>, nullable<T>>
        [[nodiscard]] static arrow_proxy create_proxy(
            NULLABLE_RANGE&&,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy_impl(
            u8_buffer<T>&& data_buffer,
            size_t size,
            std::optional<validity_bitmap>&& bitmap,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        using access_class_type::value;
        using access_class_type::value_begin;
        using access_class_type::value_cbegin;
        using access_class_type::value_cend;
        using access_class_type::value_end;

        // Modifiers

        using access_class_type::erase_values;
        using access_class_type::insert_value;
        using access_class_type::insert_values;
        using access_class_type::resize_values;

        static constexpr size_type DATA_BUFFER_INDEX = 1;

        friend class run_end_encoded_array;
        friend base_type;
        friend base_type::base_type;
        friend base_type::base_type::base_type;
    };

    /********************************************************
     * primitive_array_impl implementation *
     ********************************************************/

    template <trivial_copyable_type T>
    primitive_array_impl<T>::primitive_array_impl(arrow_proxy proxy_param)
        : base_type(std::move(proxy_param))
        , access_class_type(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
    {
    }

    template <trivial_copyable_type T>
    primitive_array_impl<T>::primitive_array_impl(const primitive_array_impl& rhs)
        : base_type(rhs)
        , access_class_type(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
    {
    }

    template <trivial_copyable_type T>
    primitive_array_impl<T>& primitive_array_impl<T>::operator=(const primitive_array_impl& rhs)
    {
        base_type::operator=(rhs);
        access_class_type::reset_proxy(this->get_arrow_proxy());
        return *this;
    }

    template <trivial_copyable_type T>
    primitive_array_impl<T>::primitive_array_impl(primitive_array_impl&& rhs)
        : base_type(std::move(rhs))
        , access_class_type(this->get_arrow_proxy(), DATA_BUFFER_INDEX)
    {
    }

    template <trivial_copyable_type T>
    primitive_array_impl<T>& primitive_array_impl<T>::operator=(primitive_array_impl&& rhs)
    {
        base_type::operator=(std::move(rhs));
        access_class_type::reset_proxy(this->get_arrow_proxy());
        return *this;
    }

    template <trivial_copyable_type T>
    template <validity_bitmap_input VALIDITY_RANGE, input_metadata_container METADATA_RANGE>
    auto primitive_array_impl<T>::create_proxy(
        u8_buffer<T>&& data_buffer,
        size_t size,
        VALIDITY_RANGE&& bitmap_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        return create_proxy_impl(
            std::forward<u8_buffer<T>>(data_buffer),
            size,
            ensure_validity_bitmap(size, std::forward<VALIDITY_RANGE>(bitmap_input)),
            std::move(name),
            std::move(metadata)
        );
    }

    template <trivial_copyable_type T>
    template <std::ranges::input_range VALUE_RANGE, validity_bitmap_input VALIDITY_RANGE, input_metadata_container METADATA_RANGE>
        requires(std::convertible_to<std::ranges::range_value_t<VALUE_RANGE>, T>)
    arrow_proxy primitive_array_impl<T>::create_proxy(
        VALUE_RANGE&& values,
        VALIDITY_RANGE&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        auto size = static_cast<size_t>(std::ranges::distance(values));
        u8_buffer<T> data_buffer = details::primitive_data_access<T>::make_data_buffer(
            std::forward<VALUE_RANGE>(values)
        );
        return create_proxy(
            std::move(data_buffer),
            size,
            std::forward<VALIDITY_RANGE>(validity_input),
            std::move(name),
            std::move(metadata)
        );
    }

    template <trivial_copyable_type T>
    template <class U, input_metadata_container METADATA_RANGE>
        requires std::convertible_to<U, T>
    arrow_proxy primitive_array_impl<T>::create_proxy(
        size_type n,
        const U& value,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        // create data_buffer
        u8_buffer<T> data_buffer(n, value);
        return create_proxy_impl(
            std::move(data_buffer),
            n,
            nullable ? std::make_optional<validity_bitmap>(nullptr, 0) : std::nullopt,
            std::move(name),
            std::move(metadata)
        );
    }

    template <trivial_copyable_type T>
    template <validity_bitmap_input VALIDITY_RANGE, input_metadata_container METADATA_RANGE>
    arrow_proxy primitive_array_impl<T>::create_proxy(
        u8_buffer<T>&& data_buffer,
        size_t size,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        std::optional<validity_bitmap> bitmap = nullable ? std::make_optional<validity_bitmap>(nullptr, 0)
                                                         : std::nullopt;
        return create_proxy_impl(
            std::move(data_buffer),
            size,
            std::move(bitmap),
            std::move(name),
            std::move(metadata)
        );
    }

    template <trivial_copyable_type T>
    template <std::ranges::input_range R, input_metadata_container METADATA_RANGE>
        requires(std::convertible_to<std::ranges::range_value_t<R>, T> && !mpl::is_type_instance_of_v<R, u8_buffer>)
    arrow_proxy primitive_array_impl<T>::create_proxy(
        R&& range,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        auto data_buffer = details::primitive_data_access<T>::make_data_buffer(std::forward<R>(range));
        auto distance = static_cast<size_t>(std::ranges::distance(range));
        std::optional<validity_bitmap> bitmap = nullable ? std::make_optional<validity_bitmap>(nullptr, 0)
                                                         : std::nullopt;
        return create_proxy_impl(
            std::move(data_buffer),
            distance,
            std::move(bitmap),
            std::move(name),
            std::move(metadata)
        );
    }

    // range of nullable values
    template <trivial_copyable_type T>
    template <std::ranges::input_range NULLABLE_RANGE, input_metadata_container METADATA_RANGE>
        requires std::is_same_v<std::ranges::range_value_t<NULLABLE_RANGE>, nullable<T>>
    arrow_proxy primitive_array_impl<T>::create_proxy(
        NULLABLE_RANGE&& nullable_range,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        // split into values and is_non_null ranges
        auto values = nullable_range
                      | std::views::transform(
                          [](const auto& v)
                          {
                              return v.get();
                          }
                      );
        auto is_non_null = nullable_range
                           | std::views::transform(
                               [](const auto& v)
                               {
                                   return v.has_value();
                               }
                           );
        return self_type::create_proxy(values, is_non_null, std::move(name), std::move(metadata));
    }

    template <trivial_copyable_type T>
    template <input_metadata_container METADATA_RANGE>
    [[nodiscard]] arrow_proxy primitive_array_impl<T>::create_proxy_impl(
        u8_buffer<T>&& data_buffer,
        size_t size,
        std::optional<validity_bitmap>&& bitmap,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const bool bitmap_has_value = bitmap.has_value();
        const auto null_count = bitmap_has_value ? bitmap->null_count() : 0;
        const auto flags = bitmap_has_value
                               ? std::make_optional<std::unordered_set<sparrow::ArrowFlag>>({ArrowFlag::NULLABLE})
                               : std::nullopt;

        // create arrow schema and array
        ArrowSchema schema = make_arrow_schema(
            sparrow::data_type_format_of<T>(),  // format
            std::move(name),                    // name
            std::move(metadata),                // metadata
            flags,                              // flags
            nullptr,                            // children
            repeat_view<bool>(true, 0),         // children_ownership
            nullptr,                            // dictionary
            true                                // dictionary ownership
        );

        buffer<uint8_t> bitmap_buffer = bitmap_has_value ? std::move(*bitmap).extract_storage()
                                                         : buffer<uint8_t>{nullptr, 0};

        std::vector<buffer<uint8_t>> buffers(2);
        buffers[0] = std::move(bitmap_buffer);
        buffers[1] = std::move(data_buffer).extract_storage();

        // create arrow array
        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(buffers),
            nullptr,                     // children
            repeat_view<bool>(true, 0),  // children_ownership
            nullptr,                     // dictionary,
            true                         // dictionary ownership
        );
        return arrow_proxy(std::move(arr), std::move(schema));
    }
}
