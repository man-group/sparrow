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

#include <cstddef>
#include <ranges>

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/buffer/u8_buffer.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/utils/ranges.hpp"
#include "sparrow/utils/repeat_container.hpp"

namespace sparrow
{
    template <typename T>
    concept variable_size_binary_view_impl_types = std::is_same_v<T, std::string_view>
                                                   || std::is_same_v<T, std::span<const std::byte>>;


    template <variable_size_binary_view_impl_types T>
    class variable_size_binary_view_array_impl;

    /**
     * A variable-size string view layout implementation.
     * Related Apache Arrow specification:
     * https://arrow.apache.org/docs/dev/format/Columnar.html#variable-size-binary-view-layout
     * @see binary_view_array
     * @see variable_size_binary_view_array_impl
     */
    using string_view_array = variable_size_binary_view_array_impl<std::string_view>;

    /**
     * A variable-size binary view layout implementation.
     * Related Apache Arrow specification:
     * https://arrow.apache.org/docs/dev/format/Columnar.html#variable-size-binary-view-layout
     * @see string_view_array
     * @see variable_size_binary_view_array_impl
     */
    using binary_view_array = variable_size_binary_view_array_impl<std::span<const std::byte>>;

    namespace detail
    {
        template <class T>
        struct get_data_type_from_array;

        template <>
        struct get_data_type_from_array<sparrow::string_view_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::STRING_VIEW;
            }
        };

        template <>
        struct get_data_type_from_array<sparrow::binary_view_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::BINARY_VIEW;
            }
        };
    }

    template <variable_size_binary_view_impl_types T>
    struct array_inner_types<variable_size_binary_view_array_impl<T>> : array_inner_types_base
    {
        using array_type = variable_size_binary_view_array_impl<T>;
        using inner_value_type = T;
        using inner_reference = T;
        using inner_const_reference = inner_reference;

        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_value_type>>;
        using const_value_iterator = functor_index_iterator<
            detail::layout_value_functor<const array_type, inner_reference>>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    template <variable_size_binary_view_impl_types T>
    struct is_variable_size_binary_view_array_impl : std::false_type
    {
    };

    template <variable_size_binary_view_impl_types T>
    struct is_variable_size_binary_view_array_impl<variable_size_binary_view_array_impl<T>> : std::true_type
    {
    };

    /**
     * Checks whether T is a variable_size_binary_view_array_impl type.
     */
    template <variable_size_binary_view_impl_types T>
    constexpr bool is_variable_size_binary_view_array = is_variable_size_binary_view_array_impl<T>::value;

    template <variable_size_binary_view_impl_types T>
    class variable_size_binary_view_array_impl final
        : public mutable_array_bitmap_base<variable_size_binary_view_array_impl<T>>
    {
    public:

        using self_type = variable_size_binary_view_array_impl<T>;
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

        explicit variable_size_binary_view_array_impl(arrow_proxy);

        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<variable_size_binary_view_array_impl<T>, Args...>)
        explicit variable_size_binary_view_array_impl(Args&&... args)
            : variable_size_binary_view_array_impl(create_proxy(std::forward<Args>(args)...))
        {
        }

    private:

        struct buffers
        {
            buffer<uint8_t> length_buffer;
            buffer<uint8_t> long_string_storage;
            u8_buffer<int64_t> buffer_sizes;
        };

        template <std::ranges::input_range R>
            requires std::convertible_to<std::ranges::range_value_t<R>, T>
        static buffers create_buffers(R&& range);

        template <std::ranges::input_range R, validity_bitmap_input VB = validity_bitmap, input_metadata_container METADATA_RANGE>
            requires std::convertible_to<std::ranges::range_value_t<R>, T>
        [[nodiscard]] static arrow_proxy create_proxy(
            R&& range,
            VB&& bitmap_input = validity_bitmap{},
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        template <std::ranges::input_range NULLABLE_RANGE, input_metadata_container METADATA_RANGE>
            requires std::convertible_to<std::ranges::range_value_t<NULLABLE_RANGE>, nullable<T>>
        [[nodiscard]] static arrow_proxy create_proxy(
            NULLABLE_RANGE&& nullable_range,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        template <std::ranges::input_range R, input_metadata_container METADATA_RANGE>
            requires std::convertible_to<std::ranges::range_value_t<R>, T>
        [[nodiscard]] static arrow_proxy create_proxy(
            R&& range,
            bool = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        [[nodiscard]] inner_reference value(size_type i);
        [[nodiscard]] inner_const_reference value(size_type i) const;

        [[nodiscard]] value_iterator value_begin();
        [[nodiscard]] value_iterator value_end();

        [[nodiscard]] const_value_iterator value_cbegin() const;
        [[nodiscard]] const_value_iterator value_cend() const;

        static constexpr size_type LENGTH_BUFFER_INDEX = 1;
        static constexpr std::size_t DATA_BUFFER_SIZE = 16;
        static constexpr std::size_t SHORT_STRING_SIZE = 12;
        static constexpr std::size_t PREFIX_SIZE = 4;
        static constexpr std::ptrdiff_t PREFIX_OFFSET = 4;
        static constexpr std::ptrdiff_t SHORT_STRING_OFFSET = 4;
        static constexpr std::ptrdiff_t BUFFER_INDEX_OFFSET = 8;
        static constexpr std::ptrdiff_t BUFFER_OFFSET_OFFSET = 12;
        static constexpr std::size_t FIRST_VAR_DATA_BUFFER_INDEX = 2;

        friend base_type;
        friend base_type::base_type;
        friend class detail::layout_value_functor<self_type, inner_value_type>;
        friend class detail::layout_value_functor<const self_type, inner_value_type>;
    };

    template <variable_size_binary_view_impl_types T>
    variable_size_binary_view_array_impl<T>::variable_size_binary_view_array_impl(arrow_proxy proxy)
        : base_type(std::move(proxy))
    {
    }

    template <variable_size_binary_view_impl_types T>
    template <std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_value_t<R>, T>
    auto variable_size_binary_view_array_impl<T>::create_buffers(R&& range) -> buffers
    {
#ifdef __GNUC__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-align"
#endif

        const auto size = range_size(range);
        buffer<uint8_t> length_buffer(size * DATA_BUFFER_SIZE);

        std::size_t long_string_storage_size = 0;
        std::size_t i = 0;
        for (auto&& val : range)
        {
            auto val_casted = val
                              | std::ranges::views::transform(
                                  [](const auto& v)
                                  {
                                      return static_cast<std::uint8_t>(v);
                                  }
                              );

            const auto length = val.size();
            auto length_ptr = length_buffer.data() + (i * DATA_BUFFER_SIZE);

            // write length
            *reinterpret_cast<std::int32_t*>(length_ptr) = static_cast<std::int32_t>(length);

            if (length <= SHORT_STRING_SIZE)
            {
                // write data itself
                sparrow::ranges::copy(val_casted, length_ptr + SHORT_STRING_OFFSET);
            }
            else
            {
                // write the prefix of the data
                auto prefix_sub_range = val_casted | std::ranges::views::take(PREFIX_SIZE);
                sparrow::ranges::copy(prefix_sub_range, length_ptr + PREFIX_OFFSET);

                // write the buffer index
                *reinterpret_cast<std::int32_t*>(
                    length_ptr + BUFFER_INDEX_OFFSET
                ) = static_cast<std::int32_t>(FIRST_VAR_DATA_BUFFER_INDEX);

                // write the buffer offset
                *reinterpret_cast<std::int32_t*>(
                    length_ptr + BUFFER_OFFSET_OFFSET
                ) = static_cast<std::int32_t>(long_string_storage_size);

                // count the size of the long string storage
                long_string_storage_size += length;
            }
            ++i;
        }

        // write the long string storage
        buffer<uint8_t> long_string_storage(long_string_storage_size);
        std::size_t long_string_storage_offset = 0;
        for (auto&& val : range)
        {
            const auto length = val.size();
            if (length > SHORT_STRING_SIZE)
            {
                auto val_casted = val
                                  | std::ranges::views::transform(
                                      [](const auto& v)
                                      {
                                          return static_cast<std::uint8_t>(v);
                                      }
                                  );

                sparrow::ranges::copy(val_casted, long_string_storage.data() + long_string_storage_offset);
                long_string_storage_offset += length;
            }
        }

        // For binary or utf-8 view arrays, an extra buffer is appended which stores
        // the lengths of each variadic data buffer as int64_t.
        // This buffer is necessary since these buffer lengths are not trivially
        // extractable from other data in an array of binary or utf-8 view type.
        u8_buffer<int64_t> buffer_sizes(
            static_cast<std::size_t>(1),
            static_cast<int64_t>(long_string_storage_size)
        );

        return {std::move(length_buffer), std::move(long_string_storage), std::move(buffer_sizes)};

#ifdef __GNUC__
#    pragma GCC diagnostic pop
#endif
    }

    template <variable_size_binary_view_impl_types T>
    template <std::ranges::input_range R, validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
        requires std::convertible_to<std::ranges::range_value_t<R>, T>
    arrow_proxy variable_size_binary_view_array_impl<T>::create_proxy(
        R&& range,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto size = range_size(range);
        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<VB>(validity_input));
        const auto null_count = vbitmap.null_count();

        const repeat_view<bool> children_ownership(true, 0);

        static const std::optional<std::unordered_set<sparrow::ArrowFlag>> flags{{ArrowFlag::NULLABLE}};

        // create arrow schema and array
        ArrowSchema schema = make_arrow_schema(
            std::is_same<T, std::string_view>::value ? std::string_view("vu") : std::string_view("vz"),
            std::move(name),      // name
            std::move(metadata),  // metadata
            flags,                // flags
            nullptr,              // children
            children_ownership,
            nullptr,  // dictionary
            true
        );

        // create buffers
        auto buffers_parts = create_buffers(std::forward<R>(range));

        std::vector<buffer<uint8_t>> buffers{
            std::move(vbitmap).extract_storage(),
            std::move(buffers_parts.length_buffer),
            std::move(buffers_parts.long_string_storage),
            std::move(buffers_parts.buffer_sizes).extract_storage()
        };

        // create arrow array
        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(buffers),
            nullptr,  // children
            children_ownership,
            nullptr,  // dictionary
            true
        );

        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <variable_size_binary_view_impl_types T>
    template <std::ranges::input_range NULLABLE_RANGE, input_metadata_container METADATA_RANGE>
        requires std::convertible_to<std::ranges::range_value_t<NULLABLE_RANGE>, nullable<T>>
    [[nodiscard]] arrow_proxy variable_size_binary_view_array_impl<T>::create_proxy(
        NULLABLE_RANGE&& nullable_range,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        auto values = nullable_range
                      | std::views::transform(
                          [](const auto& v)
                          {
                              return static_cast<std::string_view>(v.value());
                          }
                      );

        auto is_non_null = nullable_range
                           | std::views::transform(
                               [](const auto& v)
                               {
                                   return v.has_value();
                               }
                           );

        return create_proxy(
            std::forward<decltype(values)>(values),
            std::forward<decltype(is_non_null)>(is_non_null),
            name,
            metadata
        );
    }

    template <variable_size_binary_view_impl_types T>
    template <std::ranges::input_range R, input_metadata_container METADATA_RANGE>
        requires std::convertible_to<std::ranges::range_value_t<R>, T>
    [[nodiscard]] arrow_proxy variable_size_binary_view_array_impl<T>::create_proxy(
        R&& range,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        if (nullable)
        {
            return create_proxy(std::forward<R>(range), validity_bitmap{}, std::move(name), std::move(metadata));
        }
        else
        {
            // create arrow schema and array
            const repeat_view<bool> children_ownership(true, 0);
            ArrowSchema schema = make_arrow_schema(
                std::is_same<T, std::string_view>::value ? std::string_view("vu") : std::string_view("vz"),
                std::move(name),      // name
                std::move(metadata),  // metadata
                std::nullopt,         // flags
                nullptr,              // children
                children_ownership,
                nullptr,  // dictionary
                true
            );

            // create buffers
            auto buffers_parts = create_buffers(std::forward<R>(range));

            std::vector<buffer<uint8_t>> buffers{
                buffer<uint8_t>{nullptr, 0},  // validity bitmap
                std::move(buffers_parts.length_buffer),
                std::move(buffers_parts.long_string_storage),
                std::move(buffers_parts.buffer_sizes).extract_storage()
            };
            const auto size = range_size(range);

            // create arrow array
            ArrowArray arr = make_arrow_array(
                static_cast<std::int64_t>(size),  // length
                static_cast<int64_t>(0),
                0,  // offset
                std::move(buffers),
                nullptr,  // children
                children_ownership,
                nullptr,  // dictionary
                true
            );

            return arrow_proxy{std::move(arr), std::move(schema)};
        }
    }

    template <variable_size_binary_view_impl_types T>
    auto variable_size_binary_view_array_impl<T>::value(size_type i) -> inner_reference
    {
        return static_cast<const self_type*>(this)->value(i);
    }

    template <variable_size_binary_view_impl_types T>
    auto variable_size_binary_view_array_impl<T>::value(size_type i) const -> inner_const_reference
    {
#ifdef __GNUC__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-align"
#endif

        SPARROW_ASSERT_TRUE(i < this->size());

        constexpr std::size_t element_size = 16;
        auto data_ptr = this->get_arrow_proxy().buffers()[LENGTH_BUFFER_INDEX].template data<uint8_t>()
                        + (i * element_size);

        auto length = static_cast<std::size_t>(*reinterpret_cast<const std::int32_t*>(data_ptr));
        using char_or_byte = typename inner_const_reference::value_type;

        if (length <= 12)
        {
            constexpr std::ptrdiff_t data_offset = 4;
            auto ptr = reinterpret_cast<const char_or_byte*>(data_ptr);
            const auto ret = inner_const_reference(ptr + data_offset, length);
            return ret;
        }
        else
        {
            constexpr std::ptrdiff_t buffer_index_offset = 8;
            constexpr std::ptrdiff_t buffer_offset_offset = 12;
            auto buffer_index = static_cast<std::size_t>(
                *reinterpret_cast<const std::int32_t*>(data_ptr + buffer_index_offset)
            );
            auto buffer_offset = static_cast<std::size_t>(
                *reinterpret_cast<const std::int32_t*>(data_ptr + buffer_offset_offset)
            );
            auto buffer = this->get_arrow_proxy().buffers()[buffer_index].template data<const char_or_byte>();
            return inner_const_reference(buffer + buffer_offset, length);
        }

#ifdef __GNUC__
#    pragma GCC diagnostic pop
#endif
    }

    template <variable_size_binary_view_impl_types T>
    auto variable_size_binary_view_array_impl<T>::value_begin() -> value_iterator
    {
        return value_iterator(detail::layout_value_functor<self_type, inner_value_type>(), 0);
    }

    template <variable_size_binary_view_impl_types T>
    auto variable_size_binary_view_array_impl<T>::value_end() -> value_iterator
    {
        return value_iterator(detail::layout_value_functor<self_type, inner_value_type>(this), this->size());
    }

    template <variable_size_binary_view_impl_types T>
    auto variable_size_binary_view_array_impl<T>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator(detail::layout_value_functor<const self_type, inner_value_type>(this), 0);
    }

    template <variable_size_binary_view_impl_types T>
    auto variable_size_binary_view_array_impl<T>::value_cend() const -> const_value_iterator
    {
        return const_value_iterator(
            detail::layout_value_functor<const self_type, inner_value_type>(this),
            this->size()
        );
    }
}
