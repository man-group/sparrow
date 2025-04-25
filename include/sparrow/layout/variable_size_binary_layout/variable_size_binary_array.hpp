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

#include <cstdint>
#include <iterator>
#include <numeric>
#include <ranges>
#include <string>
#include <vector>

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/variable_size_binary_layout/variable_size_binary_iterator.hpp"
#include "sparrow/layout/variable_size_binary_layout/variable_size_binary_reference.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/utils/repeat_container.hpp"

namespace sparrow
{
    namespace detail
    {
        template <class T, class OT>
        struct variable_size_binary_format;

        template <>
        struct variable_size_binary_format<std::string, std::int32_t>
        {
            [[nodiscard]] static std::string format()
            {
                return "u";
            }
        };

        template <>
        struct variable_size_binary_format<std::string, std::int64_t>
        {
            [[nodiscard]] static std::string format()
            {
                return "U";
            }
        };

        template <>
        struct variable_size_binary_format<std::vector<byte_t>, std::int32_t>
        {
            [[nodiscard]] static std::string format()
            {
                return "z";
            }
        };

        template <>
        struct variable_size_binary_format<std::vector<byte_t>, std::int64_t>
        {
            [[nodiscard]] static std::string format()
            {
                return "Z";
            }
        };
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    class variable_size_binary_array_impl;

    using binary_traits = arrow_traits<std::vector<byte_t>>;

    using string_array = variable_size_binary_array_impl<std::string, std::string_view, std::int32_t>;
    using big_string_array = variable_size_binary_array_impl<std::string, std::string_view, std::int64_t>;
    using binary_array = variable_size_binary_array_impl<binary_traits::value_type, binary_traits::const_reference, std::int32_t>;
    using big_binary_array = variable_size_binary_array_impl<
        binary_traits::value_type,
        binary_traits::const_reference,
        std::int64_t>;

    namespace detail
    {
        template <class T>
        struct get_data_type_from_array;

        template <>
        struct get_data_type_from_array<sparrow::string_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::STRING;
            }
        };

        template <>
        struct get_data_type_from_array<sparrow::big_string_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::LARGE_STRING;
            }
        };

        template <>
        struct get_data_type_from_array<sparrow::binary_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::BINARY;
            }
        };

        template <>
        struct get_data_type_from_array<sparrow::big_binary_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::LARGE_BINARY;
            }
        };
    }

    /**
     * Checks whether T is a string_array type.
     */
    template <class T>
    constexpr bool is_string_array_v = std::same_as<T, string_array>;

    /**
     * Checks whether T is a big_string_array type.
     */
    template <class T>
    constexpr bool is_big_string_array_v = std::same_as<T, big_string_array>;

    /**
     * Checks whether T is a binary_array type.
     */
    template <class T>
    constexpr bool is_binary_array_v = std::same_as<T, binary_array>;

    /**
     * Checks whether T is a big_binary_array type.
     */
    template <class T>
    constexpr bool is_big_binary_array_v = std::same_as<T, big_binary_array>;

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    struct array_inner_types<variable_size_binary_array_impl<T, CR, OT>> : array_inner_types_base
    {
        using array_type = variable_size_binary_array_impl<T, CR, OT>;

        using inner_value_type = T;
        using inner_reference = variable_size_binary_reference<array_type>;
        using inner_const_reference = CR;
        using offset_type = OT;

        using data_value_type = typename T::value_type;

        using offset_iterator = OT*;
        using const_offset_iterator = const OT*;

        using data_iterator = data_value_type*;
        using const_data_iterator = const data_value_type*;

        using iterator_tag = std::random_access_iterator_tag;

        using const_bitmap_iterator = bitmap_type::const_iterator;

        struct iterator_types
        {
            using value_type = inner_value_type;
            using reference = inner_reference;
            using value_iterator = data_iterator;
            using bitmap_iterator = bitmap_type::iterator;
            using iterator_tag = array_inner_types<variable_size_binary_array_impl<T, CR, OT>>::iterator_tag;
        };

        using value_iterator = variable_size_binary_value_iterator<array_type, iterator_types>;

        struct const_iterator_types
        {
            using value_type = inner_value_type;
            using reference = inner_const_reference;
            using value_iterator = const_data_iterator;
            using bitmap_iterator = const_bitmap_iterator;
            using iterator_tag = array_inner_types<variable_size_binary_array_impl<T, CR, OT>>::iterator_tag;
        };

        using const_value_iterator = variable_size_binary_value_iterator<array_type, const_iterator_types>;

        // using iterator = layout_iterator<array_type, false>;
        // using const_iterator = layout_iterator<array_type, true, CR>;
    };

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    class variable_size_binary_array_impl final
        : public mutable_array_bitmap_base<variable_size_binary_array_impl<T, CR, OT>>
    {
    private:

        static_assert(
            sizeof(std::ranges::range_value_t<T>) == sizeof(std::uint8_t),
            "Only sequences of types with the same size as uint8_t are supported"
        );

    public:

        using self_type = variable_size_binary_array_impl<T, CR, OT>;
        using base_type = mutable_array_bitmap_base<self_type>;

        using inner_types = array_inner_types<self_type>;
        using inner_value_type = typename inner_types::inner_value_type;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;

        using offset_type = typename inner_types::offset_type;
        using offset_buffer_type = u8_buffer<std::remove_const_t<offset_type>>;
        using char_buffer_type = u8_buffer<char>;
        using byte_buffer_type = u8_buffer<std::byte>;
        using uint8_buffer_type = u8_buffer<std::uint8_t>;

        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_reference = typename base_type::bitmap_reference;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;
        using const_bitmap_range = typename base_type::const_bitmap_range;

        using value_type = nullable<inner_value_type>;
        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;

        using offset_iterator = typename inner_types::offset_iterator;
        using const_offset_iterator = typename inner_types::const_offset_iterator;

        using size_type = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using iterator_tag = typename base_type::iterator_tag;
        using data_iterator = typename inner_types::data_iterator;

        using const_data_iterator = typename inner_types::const_data_iterator;
        using data_value_type = typename inner_types::data_value_type;

        using value_iterator = typename inner_types::value_iterator;
        using const_value_iterator = typename inner_types::const_value_iterator;

        explicit variable_size_binary_array_impl(arrow_proxy);

        template <class... ARGS>
            requires(mpl::excludes_copy_and_move_ctor_v<variable_size_binary_array_impl<T, CR, OT>, ARGS...>)
        variable_size_binary_array_impl(ARGS&&... args)
            : self_type(create_proxy(std::forward<ARGS>(args)...))
        {
        }

        using base_type::get_arrow_proxy;
        using base_type::size;

        [[nodiscard]] inner_reference value(size_type i);
        [[nodiscard]] inner_const_reference value(size_type i) const;

        template <std::ranges::range SIZES_RANGE>
        [[nodiscard]] static auto offset_from_sizes(SIZES_RANGE&& sizes) -> offset_buffer_type;

    private:

        template <
            mpl::char_like C,
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            u8_buffer<C>&& data_buffer,
            offset_buffer_type&& list_offsets,
            VB&& validity_input = validity_bitmap{},
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        template <
            std::ranges::input_range R,
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(
                std::ranges::input_range<std::ranges::range_value_t<R>> &&  // a range of ranges
                mpl::char_like<std::ranges::range_value_t<std::ranges::range_value_t<R>>>  // inner range is a
                                                                                           // range of
                                                                                           // char-like
            )
        [[nodiscard]] static arrow_proxy create_proxy(
            R&& values,
            VB&& validity_input = validity_bitmap{},
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        // range of nullable values
        template <std::ranges::input_range R, input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::is_same_v<std::ranges::range_value_t<R>, nullable<T>>
        [[nodiscard]] static arrow_proxy create_proxy(
            R&&,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        static constexpr size_t OFFSET_BUFFER_INDEX = 1;
        static constexpr size_t DATA_BUFFER_INDEX = 2;

        [[nodiscard]] offset_iterator offset(size_type i);
        [[nodiscard]] offset_iterator offsets_begin();
        [[nodiscard]] offset_iterator offsets_end();
        [[nodiscard]] data_iterator data(size_type i);

        [[nodiscard]] value_iterator value_begin();
        [[nodiscard]] value_iterator value_end();

        [[nodiscard]] const_value_iterator value_cbegin() const;
        [[nodiscard]] const_value_iterator value_cend() const;

        [[nodiscard]] const_offset_iterator offset(size_type i) const;
        [[nodiscard]] const_offset_iterator offsets_cbegin() const;
        [[nodiscard]] const_offset_iterator offsets_cend() const;
        [[nodiscard]] const_data_iterator data(size_type i) const;

        // Modifiers

        template <std::ranges::sized_range U>
            requires mpl::convertible_ranges<U, T>
        void resize_values(size_type new_length, U value);

        void resize_offsets(size_type new_length, offset_type offset_value);

        template <std::ranges::sized_range U>
            requires mpl::convertible_ranges<U, T>
        value_iterator insert_value(const_value_iterator pos, U value, size_type count);

        offset_iterator insert_offset(const_offset_iterator pos, offset_type size, size_type count);

        template <mpl::iterator_of_type<T> InputIt>
        value_iterator insert_values(const_value_iterator pos, InputIt first, InputIt last);

        template <mpl::iterator_of_type<OT> InputIt>
        offset_iterator insert_offsets(const_offset_iterator pos, InputIt first, InputIt last);

        value_iterator erase_values(const_value_iterator pos, size_type count);

        offset_iterator erase_offsets(const_offset_iterator pos, size_type count);

        template <std::ranges::sized_range U>
            requires mpl::convertible_ranges<U, T>
        void assign(U&& rhs, size_type index);

        friend class variable_size_binary_reference<self_type>;
        friend const_value_iterator;
        friend base_type;
        friend base_type::base_type;
        friend base_type::base_type::base_type;
    };

    /*********************************************
     * variable_size_binary_array_impl implementation *
     *********************************************/

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    variable_size_binary_array_impl<T, CR, OT>::variable_size_binary_array_impl(arrow_proxy proxy)
        : base_type(std::move(proxy))
    {
        const auto type = this->get_arrow_proxy().data_type();
        SPARROW_ASSERT_TRUE(type == data_type::STRING || type == data_type::BINARY);  // TODO: Add
                                                                                      // data_type::LARGE_STRING
                                                                                      // and
                                                                                      // data_type::LARGE_BINARY
        SPARROW_ASSERT_TRUE((
            (type == data_type::STRING || type == data_type::BINARY) && std::same_as<OT, int32_t>
        ) );
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    template <std::ranges::range SIZES_RANGE>
    auto variable_size_binary_array_impl<T, CR, OT>::offset_from_sizes(SIZES_RANGE&& sizes)
        -> offset_buffer_type
    {
        return detail::offset_buffer_from_sizes<std::remove_const_t<offset_type>>(std::forward<SIZES_RANGE>(sizes
        ));
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    template <mpl::char_like C, validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
    arrow_proxy variable_size_binary_array_impl<T, CR, OT>::create_proxy(
        u8_buffer<C>&& data_buffer,
        offset_buffer_type&& offsets,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto size = offsets.size() - 1;
        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<VB>(validity_input));
        const auto null_count = vbitmap.null_count();

        ArrowSchema schema = make_arrow_schema(
            detail::variable_size_binary_format<T, OT>::format(),
            std::move(name),      // name
            std::move(metadata),  // metadata
            std::nullopt,         // flags,
            nullptr,              // children
            repeat_view<bool>(true, 0),
            nullptr,  // dictionary
            true

        );
        std::vector<buffer<std::uint8_t>> arr_buffs = {
            std::move(vbitmap).extract_storage(),
            std::move(offsets).extract_storage(),
            std::move(data_buffer).extract_storage()
        };

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(arr_buffs),
            nullptr,  // children
            repeat_view<bool>(true, 0),
            nullptr,  // dictionary
            true
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    template <std::ranges::input_range R, validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
        requires(
            std::ranges::input_range<std::ranges::range_value_t<R>> &&                 // a range of ranges
            mpl::char_like<std::ranges::range_value_t<std::ranges::range_value_t<R>>>  // inner range is a
                                                                                       // range of char-like
        )
    arrow_proxy variable_size_binary_array_impl<T, CR, OT>::create_proxy(
        R&& values,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        auto size_range = values
                          | std::views::transform(
                              [](const auto& v)
                              {
                                  return std::ranges::size(v);
                              }
                          );
        auto offset_buffer = offset_from_sizes(size_range);
#if SPARROW_BUILT_WITH_GCC_10
        auto data_buffer = workaround::join_ranges(values);
#else
        auto data_buffer = u8_buffer<ranges_of_ranges_value_t<R>>(std::ranges::views::join(values));
#endif
        return create_proxy(
            std::move(data_buffer),
            std::move(offset_buffer),
            std::forward<VB>(validity_input),
            std::forward<std::optional<std::string_view>>(name),
            std::forward<std::optional<METADATA_RANGE>>(metadata)
        );
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    template <std::ranges::input_range R, input_metadata_container METADATA_RANGE>
        requires std::is_same_v<std::ranges::range_value_t<R>, nullable<T>>
    arrow_proxy variable_size_binary_array_impl<T, CR, OT>::create_proxy(
        R&& range,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        // split into values and is_non_null ranges
        const auto values = range
                            | std::views::transform(
                                [](const auto& v)
                                {
                                    return v.get();
                                }
                            );
        const auto is_non_null = range
                                 | std::views::transform(
                                     [](const auto& v)
                                     {
                                         return v.has_value();
                                     }
                                 );
        return self_type::create_proxy(values, is_non_null, std::move(name), std::move(metadata));
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::data(size_type i) -> data_iterator
    {
        arrow_proxy& proxy = get_arrow_proxy();
        SPARROW_ASSERT_TRUE(proxy.buffers()[DATA_BUFFER_INDEX].size() >= i);
        return proxy.buffers()[DATA_BUFFER_INDEX].template data<data_value_type>() + i;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::data(size_type i) const -> const_data_iterator
    {
        const arrow_proxy& proxy = this->get_arrow_proxy();
        SPARROW_ASSERT_TRUE(proxy.buffers()[DATA_BUFFER_INDEX].size() >= i);
        return proxy.buffers()[DATA_BUFFER_INDEX].template data<const data_value_type>() + i;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    template <std::ranges::sized_range U>
        requires mpl::convertible_ranges<U, T>
    void variable_size_binary_array_impl<T, CR, OT>::assign(U&& rhs, size_type index)
    {
        SPARROW_ASSERT_TRUE(index < size());
        const auto offset_beg = *offset(index);
        const auto offset_end = *offset(index + 1);
        const auto initial_value_length = offset_end - offset_beg;
        const auto new_value_length = static_cast<OT>(std::ranges::size(rhs));
        const OT shift_byte_count = new_value_length - initial_value_length;
        auto& data_buffer = this->get_arrow_proxy().get_array_private_data()->buffers()[DATA_BUFFER_INDEX];
        if (shift_byte_count != 0)
        {
            const auto shift_val_abs = static_cast<size_type>(std::abs(shift_byte_count));
            const auto new_data_buffer_size = shift_byte_count < 0 ? data_buffer.size() - shift_val_abs
                                                                   : data_buffer.size() + shift_val_abs;

            if (shift_byte_count > 0)
            {
                data_buffer.resize(new_data_buffer_size);
                // Move elements to make space for the new value
                std::move_backward(
                    data_buffer.begin() + offset_end,
                    data_buffer.end() - shift_byte_count,
                    data_buffer.end()
                );
            }
            else
            {
                std::move(
                    data_buffer.begin() + offset_end,
                    data_buffer.end(),
                    data_buffer.begin() + offset_end + shift_byte_count
                );
                data_buffer.resize(new_data_buffer_size);
            }
            // Adjust offsets for subsequent elements
            std::for_each(
                offset(index + 1),
                offset(size() + 1),
                [shift_byte_count](auto& offset)
                {
                    offset += shift_byte_count;
                }
            );
        }
        auto tmp = std::views::transform(
            rhs,
            [](const auto& val)
            {
                return static_cast<std::uint8_t>(val);
            }
        );
        // Copy the new value into the buffer
        std::copy(std::ranges::begin(tmp), std::ranges::end(tmp), data_buffer.begin() + offset_beg);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::offset(size_type i) -> offset_iterator
    {
        SPARROW_ASSERT_TRUE(i <= size() + this->get_arrow_proxy().offset());
        return get_arrow_proxy().buffers()[OFFSET_BUFFER_INDEX].template data<OT>()
               + static_cast<size_type>(this->get_arrow_proxy().offset()) + i;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::offset(size_type i) const -> const_offset_iterator
    {
        SPARROW_ASSERT_TRUE(i <= this->size() + this->get_arrow_proxy().offset());
        return this->get_arrow_proxy().buffers()[OFFSET_BUFFER_INDEX].template data<OT>()
               + static_cast<size_type>(this->get_arrow_proxy().offset()) + i;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::offsets_begin() -> offset_iterator
    {
        return offset(0);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::offsets_cbegin() const -> const_offset_iterator
    {
        return offset(0);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::offsets_end() -> offset_iterator
    {
        return offset(size() + 1);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::offsets_cend() const -> const_offset_iterator
    {
        return offset(size() + 1);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::value(size_type i) -> inner_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return inner_reference(this, i);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::value(size_type i) const -> inner_const_reference
    {
        SPARROW_ASSERT_TRUE(i < this->size());
        const OT offset_begin = *offset(i);
        SPARROW_ASSERT_TRUE(offset_begin >= 0);
        const OT offset_end = *offset(i + 1);
        SPARROW_ASSERT_TRUE(offset_end >= 0);
        const const_data_iterator pointer_begin = data(static_cast<size_type>(offset_begin));
        const const_data_iterator pointer_end = data(static_cast<size_type>(offset_end));
        return inner_const_reference(pointer_begin, pointer_end);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::value_begin() -> value_iterator
    {
        return value_iterator{this, 0};
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::value_end() -> value_iterator
    {
        return sparrow::next(value_begin(), size());
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator{this, 0};
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::value_cend() const -> const_value_iterator
    {
        return sparrow::next(value_cbegin(), this->size());
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    template <std::ranges::sized_range U>
        requires mpl::convertible_ranges<U, T>
    void variable_size_binary_array_impl<T, CR, OT>::resize_values(size_type new_length, U value)
    {
        const size_t new_size = new_length + static_cast<size_t>(this->get_arrow_proxy().offset());
        auto& buffers = this->get_arrow_proxy().get_array_private_data()->buffers();
        if (new_length < size())
        {
            const auto offset_begin = static_cast<size_t>(*offset(new_length));
            auto& data_buffer = buffers[DATA_BUFFER_INDEX];
            data_buffer.resize(offset_begin);
            auto& offset_buffer = buffers[OFFSET_BUFFER_INDEX];
            auto offset_buffer_adaptor = make_buffer_adaptor<OT>(offset_buffer);
            offset_buffer_adaptor.resize(new_size + 1);
        }
        else if (new_length > size())
        {
            insert_value(value_cend(), value, new_length - size());
        }
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    template <std::ranges::sized_range U>
        requires mpl::convertible_ranges<U, T>
    auto
    variable_size_binary_array_impl<T, CR, OT>::insert_value(const_value_iterator pos, U value, size_type count)
        -> value_iterator
    {
        const auto idx = static_cast<size_t>(std::distance(value_cbegin(), pos));
        const OT offset_begin = *offset(idx);
        const std::vector<uint8_t> casted_value{value.cbegin(), value.cend()};
        const repeat_view<std::vector<uint8_t>> my_repeat_view{casted_value, count};
        const auto joined_repeated_value_range = std::ranges::views::join(my_repeat_view);
        auto& data_buffer = this->get_arrow_proxy().get_array_private_data()->buffers()[DATA_BUFFER_INDEX];
        const auto pos_to_insert = sparrow::next(data_buffer.cbegin(), offset_begin);
        data_buffer.insert(pos_to_insert, joined_repeated_value_range.begin(), joined_repeated_value_range.end());
        insert_offset(offsets_cbegin() + idx + 1, static_cast<offset_type>(value.size()), count);
        return sparrow::next(value_begin(), idx);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::insert_offset(
        const_offset_iterator pos,
        offset_type value_size,
        size_type count
    ) -> offset_iterator
    {
        auto& offset_buffer = get_arrow_proxy().get_array_private_data()->buffers()[OFFSET_BUFFER_INDEX];
        const auto idx = static_cast<size_t>(std::distance(offsets_cbegin(), pos));
        auto offset_buffer_adaptor = make_buffer_adaptor<OT>(offset_buffer);
        const offset_type cumulative_size = value_size * static_cast<offset_type>(count);
        // Adjust offsets for subsequent elements
        std::for_each(
            sparrow::next(offset_buffer_adaptor.begin(), idx + 1),
            offset_buffer_adaptor.end(),
            [cumulative_size](auto& offset)
            {
                offset += cumulative_size;
            }
        );
        offset_buffer_adaptor.insert(sparrow::next(offset_buffer_adaptor.cbegin(), idx + 1), count, 0);
        // Put the right values in the new offsets
        for (size_t i = idx + 1; i < idx + 1 + count; ++i)
        {
            offset_buffer_adaptor[i] = offset_buffer_adaptor[i - 1] + value_size;
        }
        return offsets_begin() + idx;
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    template <mpl::iterator_of_type<T> InputIt>
    auto
    variable_size_binary_array_impl<T, CR, OT>::insert_values(const_value_iterator pos, InputIt first, InputIt last)
        -> value_iterator
    {
        auto& data_buffer = get_arrow_proxy().get_array_private_data()->buffers()[DATA_BUFFER_INDEX];
        auto data_buffer_adaptor = make_buffer_adaptor<data_value_type>(data_buffer);
        auto values = std::ranges::subrange(first, last);
        const size_t cumulative_sizes = std::accumulate(
            values.begin(),
            values.end(),
            size_t(0),
            [](size_t acc, const T& value)
            {
                return acc + value.size();
            }
        );
        data_buffer_adaptor.resize(data_buffer_adaptor.size() + cumulative_sizes);
        const auto idx = static_cast<size_t>(std::distance(value_cbegin(), pos));
        const OT offset_begin = *offset(idx);
        auto insert_pos = sparrow::next(data_buffer_adaptor.begin(), offset_begin);

        // Move elements to make space for the new value
        std::move_backward(
            insert_pos,
            sparrow::next(data_buffer_adaptor.end(), -static_cast<difference_type>(cumulative_sizes)),
            data_buffer_adaptor.end()
        );

        for (const T& value : values)
        {
            std::copy(value.begin(), value.end(), insert_pos);
            std::advance(insert_pos, value.size());
        }

        const auto sizes_of_each_value = std::ranges::views::transform(
            values,
            [](const T& value) -> offset_type
            {
                return static_cast<offset_type>(value.size());
            }
        );
        insert_offsets(offset(idx + 1), sizes_of_each_value.begin(), sizes_of_each_value.end());
        return sparrow::next(value_begin(), idx);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    template <mpl::iterator_of_type<OT> InputIt>
    auto variable_size_binary_array_impl<T, CR, OT>::insert_offsets(
        const_offset_iterator pos,
        InputIt first_sizes,
        InputIt last_sizes
    ) -> offset_iterator
    {
        SPARROW_ASSERT_TRUE(pos >= offsets_cbegin());
        SPARROW_ASSERT_TRUE(pos <= offsets_cend());
        SPARROW_ASSERT_TRUE(first_sizes <= last_sizes);
        auto& offset_buffer = get_arrow_proxy().get_array_private_data()->buffers()[OFFSET_BUFFER_INDEX];
        auto offset_buffer_adaptor = make_buffer_adaptor<OT>(offset_buffer);
        const auto idx = std::distance(offsets_cbegin(), pos);
        const OT cumulative_sizes = std::reduce(first_sizes, last_sizes, OT(0));
        const auto sizes_count = std::distance(first_sizes, last_sizes);
        offset_buffer_adaptor.resize(offset_buffer_adaptor.size() + static_cast<size_t>(sizes_count));
        // Move the offsets to make space for the new offsets
        std::move_backward(
            offset_buffer_adaptor.begin() + idx,
            offset_buffer_adaptor.end() - sizes_count,
            offset_buffer_adaptor.end()
        );
        // Adjust offsets for subsequent elements
        std::for_each(
            offset_buffer_adaptor.begin() + idx + sizes_count,
            offset_buffer_adaptor.end(),
            [cumulative_sizes](auto& offset)
            {
                offset += cumulative_sizes;
            }
        );
        // Put the right values in the new offsets
        InputIt it = first_sizes;
        for (size_t i = static_cast<size_t>(idx + 1); i < static_cast<size_t>(idx + sizes_count + 1); ++i)
        {
            offset_buffer_adaptor[i] = offset_buffer_adaptor[i - 1] + *it;
            ++it;
        }
        return offset(static_cast<size_t>(idx));
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::erase_values(const_value_iterator pos, size_type count)
        -> value_iterator
    {
        SPARROW_ASSERT_TRUE(pos >= value_cbegin());
        SPARROW_ASSERT_TRUE(pos <= value_cend());
        const size_t index = static_cast<size_t>(std::distance(value_cbegin(), pos));
        if (count == 0)
        {
            return sparrow::next(value_begin(), index);
        }
        auto& data_buffer = get_arrow_proxy().get_array_private_data()->buffers()[DATA_BUFFER_INDEX];
        const auto offset_begin = *offset(index);
        const auto offset_end = *offset(index + count);
        const size_t difference = static_cast<size_t>(offset_end - offset_begin);
        // move the values after the erased ones
        std::move(data_buffer.begin() + offset_end, data_buffer.end(), data_buffer.begin() + offset_begin);
        data_buffer.resize(data_buffer.size() - difference);
        // adjust the offsets for the subsequent elements
        erase_offsets(offset(index), count);
        return sparrow::next(value_begin(), index);
    }

    template <std::ranges::sized_range T, class CR, layout_offset OT>
    auto variable_size_binary_array_impl<T, CR, OT>::erase_offsets(const_offset_iterator pos, size_type count)
        -> offset_iterator
    {
        SPARROW_ASSERT_TRUE(pos >= offsets_cbegin());
        SPARROW_ASSERT_TRUE(pos <= offsets_cend());
        const size_t index = static_cast<size_t>(std::distance(offsets_cbegin(), pos));
        if (count == 0)
        {
            return offset(index);
        }
        auto& offset_buffer = get_arrow_proxy().get_array_private_data()->buffers()[OFFSET_BUFFER_INDEX];
        auto offset_buffer_adaptor = make_buffer_adaptor<OT>(offset_buffer);
        const OT offset_start_value = *offset(index);
        const OT offset_end_value = *offset(index + count);
        const OT difference = offset_end_value - offset_start_value;
        // move the offsets after the erased ones
        std::move(
            sparrow::next(offset_buffer_adaptor.begin(), index + count + 1),
            offset_buffer_adaptor.end(),
            sparrow::next(offset_buffer_adaptor.begin(), index + 1)
        );
        offset_buffer_adaptor.resize(offset_buffer_adaptor.size() - count);
        // adjust the offsets for the subsequent elements
        std::for_each(
            sparrow::next(offset_buffer_adaptor.begin(), index + 1),
            offset_buffer_adaptor.end(),
            [difference](OT& offset)
            {
                offset -= difference;
            }
        );
        return offset(index);
    }

}
