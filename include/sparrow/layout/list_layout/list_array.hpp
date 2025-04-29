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

#include <ranges>
#include <string>  // for std::stoull
#include <type_traits>
#include <vector>

#include "sparrow/array_api.hpp"
#include "sparrow/array_factory.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/memory.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/utils/repeat_container.hpp"

namespace sparrow
{
    template <class DERIVED>
    class list_array_crtp_base;

    template <bool BIG>
    class list_array_impl;

    template <bool BIG>
    class list_view_array_impl;

    using list_array = list_array_impl<false>;
    using big_list_array = list_array_impl<true>;

    using list_view_array = list_view_array_impl<false>;
    using big_list_view_array = list_view_array_impl<true>;

    class fixed_sized_list_array;

    /**
     * Checks whether T is a list_array type.
     */
    template <class T>
    constexpr bool is_list_array_v = std::same_as<T, list_array>;

    /**
     * Checks whether T is a big_list_array type.
     */
    template <class T>
    constexpr bool is_big_list_array_v = std::same_as<T, big_list_array>;

    /**
     * Checks whether T is a list_view_array type.
     */
    template <class T>
    constexpr bool is_list_view_array_v = std::same_as<T, list_view_array>;

    /**
     * Checks whether T is a big_list_view_array type.
     */
    template <class T>
    constexpr bool is_big_list_view_array_v = std::same_as<T, big_list_view_array>;

    /**
     * Checks whether T is a fixed_sized_list_array type.
     */
    template <class T>
    constexpr bool is_fixed_sized_list_array_v = std::same_as<T, fixed_sized_list_array>;

    namespace detail
    {
        template <class T>
        struct get_data_type_from_array;

        template <>
        struct get_data_type_from_array<sparrow::list_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::LIST;
            }
        };

        template <>
        struct get_data_type_from_array<sparrow::big_list_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::LARGE_LIST;
            }
        };

        template <>
        struct get_data_type_from_array<sparrow::list_view_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::LIST_VIEW;
            }
        };

        template <>
        struct get_data_type_from_array<sparrow::big_list_view_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::LARGE_LIST_VIEW;
            }
        };

        template <>
        struct get_data_type_from_array<sparrow::fixed_sized_list_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::FIXED_SIZED_LIST;
            }
        };
    }

    template <bool BIG>
    struct array_inner_types<list_array_impl<BIG>> : array_inner_types_base
    {
        using list_size_type = std::conditional_t<BIG, std::uint64_t, std::uint32_t>;
        using array_type = list_array_impl<BIG>;
        using inner_value_type = list_value;
        using inner_reference = list_value;
        using inner_const_reference = list_value;
        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_value_type>>;
        using const_value_iterator = functor_index_iterator<
            detail::layout_value_functor<const array_type, inner_value_type>>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    template <bool BIG>
    struct array_inner_types<list_view_array_impl<BIG>> : array_inner_types_base
    {
        using list_size_type = std::conditional_t<BIG, std::uint64_t, std::uint32_t>;
        using array_type = list_view_array_impl<BIG>;
        using inner_value_type = list_value;
        using inner_reference = list_value;
        using inner_const_reference = list_value;
        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_value_type>>;
        using const_value_iterator = functor_index_iterator<
            detail::layout_value_functor<const array_type, inner_value_type>>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    template <>
    struct array_inner_types<fixed_sized_list_array> : array_inner_types_base
    {
        using list_size_type = std::uint64_t;
        using array_type = fixed_sized_list_array;
        using inner_value_type = list_value;
        using inner_reference = list_value;
        using inner_const_reference = list_value;
        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_value_type>>;
        using const_value_iterator = functor_index_iterator<
            detail::layout_value_functor<const array_type, inner_value_type>>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    // using list_array = list_array_crtp_base<false>;
    // using big_list_array = list_array_crtp_base<true>;

    // this is the base class for
    // - list-array
    // - big-list-array
    // - list-view-array
    // - big-list-view-array
    // - fixed-size-list-array
    template <class DERIVED>
    class list_array_crtp_base : public array_bitmap_base<DERIVED>
    {
    public:

        using self_type = list_array_crtp_base<DERIVED>;
        using base_type = array_bitmap_base<DERIVED>;
        using inner_types = array_inner_types<DERIVED>;
        using value_iterator = typename inner_types::value_iterator;
        using const_value_iterator = typename inner_types::const_value_iterator;
        using size_type = typename base_type::size_type;

        using bitmap_type = typename base_type::bitmap_type;
        // using bitmap_reference = typename base_type::bitmap_reference;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;

        // using bitmap_range = typename base_type::bitmap_range;
        using const_bitmap_range = typename base_type::const_bitmap_range;

        using inner_value_type = list_value;
        using inner_reference = list_value;
        using inner_const_reference = list_value;

        using value_type = nullable<inner_value_type>;
        // using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using iterator_tag = typename base_type::iterator_tag;

        [[nodiscard]] const array_wrapper* raw_flat_array() const;
        [[nodiscard]] array_wrapper* raw_flat_array();

    protected:

        explicit list_array_crtp_base(arrow_proxy proxy);

        list_array_crtp_base(const self_type&);
        list_array_crtp_base& operator=(const self_type&);

        list_array_crtp_base(self_type&&) = default;
        list_array_crtp_base& operator=(self_type&&) = default;

    private:

        using list_size_type = inner_types::list_size_type;

        [[nodiscard]] value_iterator value_begin();
        [[nodiscard]] value_iterator value_end();
        [[nodiscard]] const_value_iterator value_cbegin() const;
        [[nodiscard]] const_value_iterator value_cend() const;

        [[nodiscard]] inner_reference value(size_type i);
        [[nodiscard]] inner_const_reference value(size_type i) const;

        [[nodiscard]] cloning_ptr<array_wrapper> make_flat_array();

        // data members
        cloning_ptr<array_wrapper> p_flat_array;

        // friend classes
        friend class array_crtp_base<DERIVED>;

        // needs access to this->value(i)
        friend class detail::layout_value_functor<DERIVED, inner_value_type>;
        friend class detail::layout_value_functor<const DERIVED, inner_value_type>;
    };

    template <bool BIG>
    class list_array_impl final : public list_array_crtp_base<list_array_impl<BIG>>
    {
    public:

        using self_type = list_array_impl<BIG>;
        using inner_types = array_inner_types<self_type>;
        using base_type = list_array_crtp_base<list_array_impl<BIG>>;
        using list_size_type = inner_types::list_size_type;
        using size_type = typename base_type::size_type;
        using offset_type = std::conditional_t<BIG, const std::int64_t, const std::int32_t>;
        using offset_buffer_type = u8_buffer<std::remove_const_t<offset_type>>;

        explicit list_array_impl(arrow_proxy proxy);

        list_array_impl(const self_type&);
        list_array_impl& operator=(const self_type&);

        list_array_impl(self_type&&) = default;
        list_array_impl& operator=(self_type&&) = default;

        template <class... ARGS>
            requires(mpl::excludes_copy_and_move_ctor_v<list_array_impl<BIG>, ARGS...>)
        explicit list_array_impl(ARGS&&... args)
            : self_type(create_proxy(std::forward<ARGS>(args)...))
        {
        }

        template <std::ranges::range SIZES_RANGE>
        [[nodiscard]] static auto offset_from_sizes(SIZES_RANGE&& sizes) -> offset_buffer_type;

    private:

        template <
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_values,
            offset_buffer_type&& list_offsets,
            VB&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        template <
            validity_bitmap_input VB = validity_bitmap,
            std::ranges::input_range OFFSET_BUFFER_RANGE,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<std::ranges::range_value_t<OFFSET_BUFFER_RANGE>, offset_type>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_values,
            OFFSET_BUFFER_RANGE&& list_offsets_range,
            VB&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
        {
            offset_buffer_type list_offsets{std::move(list_offsets_range)};
            return list_array_impl<BIG>::create_proxy(
                std::forward<array>(flat_values),
                std::move(list_offsets),
                std::forward<VB>(validity_input),
                std::forward<>(name),
                std::forward<std::optional<METADATA_RANGE>>(metadata)
            );
        }

        template <
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_values,
            offset_buffer_type&& list_offsets,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        template <
            validity_bitmap_input VB = validity_bitmap,
            std::ranges::input_range OFFSET_BUFFER_RANGE,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires std::convertible_to<std::ranges::range_value_t<OFFSET_BUFFER_RANGE>, offset_type>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_values,
            OFFSET_BUFFER_RANGE&& list_offsets_range,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
        {
            offset_buffer_type list_offsets{std::move(list_offsets_range)};
            return list_array_impl<BIG>::create_proxy(
                std::forward<array>(flat_values),
                std::move(list_offsets),
                nullable,
                std::forward<std::optional<std::string_view>>(name),
                std::forward<std::optional<METADATA_RANGE>>(metadata)
            );
        }

        static constexpr std::size_t OFFSET_BUFFER_INDEX = 1;
        [[nodiscard]] std::pair<offset_type, offset_type> offset_range(size_type i) const;

        [[nodiscard]] offset_type* make_list_offsets();

        offset_type* p_list_offsets;

        // friend classes
        friend class array_crtp_base<self_type>;
        friend class list_array_crtp_base<self_type>;
    };

    template <bool BIG>
    class list_view_array_impl final : public list_array_crtp_base<list_view_array_impl<BIG>>
    {
    public:

        using self_type = list_view_array_impl<BIG>;
        using inner_types = array_inner_types<self_type>;
        using base_type = list_array_crtp_base<list_view_array_impl<BIG>>;
        using list_size_type = inner_types::list_size_type;
        using size_type = typename base_type::size_type;
        using offset_type = std::conditional_t<BIG, const std::int64_t, const std::int32_t>;
        using offset_buffer_type = u8_buffer<std::remove_const_t<offset_type>>;
        using size_buffer_type = u8_buffer<std::remove_const_t<list_size_type>>;

        explicit list_view_array_impl(arrow_proxy proxy);

        list_view_array_impl(const self_type&);
        list_view_array_impl& operator=(const self_type&);

        list_view_array_impl(self_type&&) = default;
        list_view_array_impl& operator=(self_type&&) = default;

        template <class... ARGS>
            requires(mpl::excludes_copy_and_move_ctor_v<list_view_array_impl<BIG>, ARGS...>)
        list_view_array_impl(ARGS&&... args)
            : self_type(create_proxy(std::forward<ARGS>(args)...))
        {
        }

    private:

        template <
            std::ranges::input_range OFFSET_BUFFER_RANGE,
            std::ranges::input_range SIZE_RANGE,
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(
                std::convertible_to<std::ranges::range_value_t<OFFSET_BUFFER_RANGE>, offset_type>
                && std::convertible_to<std::ranges::range_value_t<SIZE_RANGE>, list_size_type>
            )
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_values,
            OFFSET_BUFFER_RANGE&& list_offsets,
            SIZE_RANGE&& list_sizes,
            VB&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
        {
            return list_view_array_impl<BIG>::create_proxy(
                std::move(flat_values),
                offset_buffer_type(std::move(list_offsets)),
                size_buffer_type(std::move(list_sizes)),
                std::forward<VB>(validity_input),
                name,
                metadata
            );
        }

        template <
            validity_bitmap_input VB = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_values,
            offset_buffer_type&& list_offsets,
            size_buffer_type&& list_sizes,
            VB&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        template <
            std::ranges::input_range OFFSET_BUFFER_RANGE,
            std::ranges::input_range SIZE_RANGE,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(
                std::convertible_to<std::ranges::range_value_t<OFFSET_BUFFER_RANGE>, offset_type>
                && std::convertible_to<std::ranges::range_value_t<SIZE_RANGE>, list_size_type>
            )
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_values,
            OFFSET_BUFFER_RANGE&& list_offsets,
            SIZE_RANGE&& list_sizes,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
        {
            return list_view_array_impl<BIG>::create_proxy(
                std::move(flat_values),
                offset_buffer_type(std::move(list_offsets)),
                size_buffer_type(std::move(list_sizes)),
                nullable,
                name,
                metadata
            );
        }

        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            array&& flat_values,
            offset_buffer_type&& list_offsets,
            size_buffer_type&& list_sizes,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        static constexpr std::size_t OFFSET_BUFFER_INDEX = 1;
        static constexpr std::size_t SIZES_BUFFER_INDEX = 2;
        [[nodiscard]] std::pair<offset_type, offset_type> offset_range(size_type i) const;

        [[nodiscard]] offset_type* make_list_offsets();
        [[nodiscard]] offset_type* make_list_sizes();

        offset_type* p_list_offsets;
        offset_type* p_list_sizes;

        // friend classes
        friend class array_crtp_base<self_type>;
        friend class list_array_crtp_base<self_type>;
    };

    class fixed_sized_list_array final : public list_array_crtp_base<fixed_sized_list_array>
    {
    public:

        using self_type = fixed_sized_list_array;
        using inner_types = array_inner_types<self_type>;
        using base_type = list_array_crtp_base<self_type>;
        using list_size_type = inner_types::list_size_type;
        using size_type = typename base_type::size_type;
        using offset_type = std::uint64_t;

        explicit fixed_sized_list_array(arrow_proxy proxy);

        fixed_sized_list_array(const self_type&) = default;
        fixed_sized_list_array& operator=(const self_type&) = default;

        fixed_sized_list_array(self_type&&) = default;
        fixed_sized_list_array& operator=(self_type&&) = default;

        template <class... ARGS>
            requires(mpl::excludes_copy_and_move_ctor_v<fixed_sized_list_array, ARGS...>)
        fixed_sized_list_array(ARGS&&... args)
            : self_type(create_proxy(std::forward<ARGS>(args)...))
        {
        }

    private:

        template <
            validity_bitmap_input R = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            std::uint64_t list_size,
            array&& flat_values,
            R&& validity_input,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        template <
            validity_bitmap_input R = validity_bitmap,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy(
            std::uint64_t list_size,
            array&& flat_values,
            bool nullable = true,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        [[nodiscard]] static uint64_t list_size_from_format(const std::string_view format);
        [[nodiscard]] std::pair<offset_type, offset_type> offset_range(size_type i) const;

        uint64_t m_list_size;

        // friend classes
        friend class array_crtp_base<self_type>;
        friend class list_array_crtp_base<self_type>;
    };

    /***************************************
     * list_array_crtp_base implementation *
     ***************************************/

    template <class DERIVED>
    list_array_crtp_base<DERIVED>::list_array_crtp_base(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , p_flat_array(make_flat_array())
    {
    }

    template <class DERIVED>
    list_array_crtp_base<DERIVED>::list_array_crtp_base(const self_type& rhs)
        : base_type(rhs)
        , p_flat_array(make_flat_array())
    {
    }

    template <class DERIVED>
    auto list_array_crtp_base<DERIVED>::operator=(const self_type& rhs) -> self_type&
    {
        base_type::operator=(rhs);
        p_flat_array = make_flat_array();
        return *this;
    }

    template <class DERIVED>
    auto list_array_crtp_base<DERIVED>::raw_flat_array() const -> const array_wrapper*
    {
        return p_flat_array.get();
    }

    template <class DERIVED>
    auto list_array_crtp_base<DERIVED>::raw_flat_array() -> array_wrapper*
    {
        return p_flat_array.get();
    }

    template <class DERIVED>
    auto list_array_crtp_base<DERIVED>::value_begin() -> value_iterator
    {
        return value_iterator(detail::layout_value_functor<DERIVED, inner_value_type>(&this->derived_cast()), 0);
    }

    template <class DERIVED>
    auto list_array_crtp_base<DERIVED>::value_end() -> value_iterator
    {
        return value_iterator(
            detail::layout_value_functor<DERIVED, inner_value_type>(&this->derived_cast()),
            this->size()
        );
    }

    template <class DERIVED>
    auto list_array_crtp_base<DERIVED>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator(
            detail::layout_value_functor<const DERIVED, inner_value_type>(&this->derived_cast()),
            0
        );
    }

    template <class DERIVED>
    auto list_array_crtp_base<DERIVED>::value_cend() const -> const_value_iterator
    {
        return const_value_iterator(
            detail::layout_value_functor<const DERIVED, inner_value_type>(&this->derived_cast()),
            this->size()
        );
    }

    template <class DERIVED>
    auto list_array_crtp_base<DERIVED>::value(size_type i) -> inner_reference
    {
        const auto r = this->derived_cast().offset_range(i);
        using st = typename list_value::size_type;
        return list_value{p_flat_array.get(), static_cast<st>(r.first), static_cast<st>(r.second)};
    }

    template <class DERIVED>
    auto list_array_crtp_base<DERIVED>::value(size_type i) const -> inner_const_reference
    {
        const auto r = this->derived_cast().offset_range(i);
        using st = typename list_value::size_type;
        return list_value{p_flat_array.get(), static_cast<st>(r.first), static_cast<st>(r.second)};
    }

    template <class DERIVED>
    cloning_ptr<array_wrapper> list_array_crtp_base<DERIVED>::make_flat_array()
    {
        return array_factory(this->get_arrow_proxy().children()[0].view());
    }

    /**********************************
     * list_array_impl implementation *
     **********************************/

#ifdef __GNUC__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-align"
#endif

    template <bool BIG>
    list_array_impl<BIG>::list_array_impl(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , p_list_offsets(make_list_offsets())
    {
    }

    template <bool BIG>
    template <std::ranges::range SIZES_RANGE>
    auto list_array_impl<BIG>::offset_from_sizes(SIZES_RANGE&& sizes) -> offset_buffer_type
    {
        return detail::offset_buffer_from_sizes<std::remove_const_t<offset_type>>(std::forward<SIZES_RANGE>(sizes
        ));
    }

    template <bool BIG>
    template <validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
    arrow_proxy list_array_impl<BIG>::create_proxy(
        array&& flat_values,
        offset_buffer_type&& list_offsets,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto size = list_offsets.size() - 1;
        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<VB>(validity_input));

        auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(flat_values));

        const auto null_count = vbitmap.null_count();
        const repeat_view<bool> children_ownership{true, 1};

        static const std::optional<std::unordered_set<ArrowFlag>> flags{{ArrowFlag::NULLABLE}};

        ArrowSchema schema = make_arrow_schema(
            BIG ? std::string("+L") : std::string("+l"),                   // format
            name,                                                          // name
            metadata,                                                      // metadata
            flags,                                                         // flags,
            new ArrowSchema*[1]{new ArrowSchema(std::move(flat_schema))},  // children
            children_ownership,                                            // children ownership
            nullptr,                                                       // dictionary
            true                                                           // dictionary ownership

        );
        std::vector<buffer<std::uint8_t>> arr_buffs = {
            std::move(vbitmap).extract_storage(),
            std::move(list_offsets).extract_storage()
        };

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(arr_buffs),
            new ArrowArray*[1]{new ArrowArray(std::move(flat_arr))},  // children
            children_ownership,                                       // children ownership
            nullptr,                                                  // dictionary
            true                                                      // dictionary ownership
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <bool BIG>
    template <validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
    arrow_proxy list_array_impl<BIG>::create_proxy(
        array&& flat_values,
        offset_buffer_type&& list_offsets,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        if (nullable)
        {
            return list_array_impl<BIG>::create_proxy(
                std::move(flat_values),
                std::move(list_offsets),
                validity_bitmap{},
                name,
                metadata
            );
        }
        else
        {
            const auto size = list_offsets.size() - 1;
            auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(flat_values));
            const repeat_view<bool> children_ownership{true, 1};

            ArrowSchema schema = make_arrow_schema(
                BIG ? std::string("+L") : std::string("+l"),                   // format
                name,                                                          // name
                metadata,                                                      // metadata
                std::nullopt,                                                  // flags,
                new ArrowSchema*[1]{new ArrowSchema(std::move(flat_schema))},  // children
                children_ownership,                                            // children ownership
                nullptr,                                                       // dictionary
                true                                                           // dictionary ownership

            );
            std::vector<buffer<std::uint8_t>> arr_buffs = {
                buffer<std::uint8_t>{nullptr, 0},  // no validity bitmap
                std::move(list_offsets).extract_storage()
            };

            ArrowArray arr = make_arrow_array(
                static_cast<std::int64_t>(size),  // length
                0,
                0,  // offset
                std::move(arr_buffs),
                new ArrowArray*[1]{new ArrowArray(std::move(flat_arr))},  // children
                children_ownership,                                       // children ownership
                nullptr,                                                  // dictionary
                true                                                      // dictionary ownership
            );
            return arrow_proxy{std::move(arr), std::move(schema)};
        }
    }

    template <bool BIG>
    list_array_impl<BIG>::list_array_impl(const self_type& rhs)
        : base_type(rhs)
        , p_list_offsets(make_list_offsets())
    {
    }

    template <bool BIG>
    auto list_array_impl<BIG>::operator=(const self_type& rhs) -> self_type&
    {
        if (this != &rhs)
        {
            base_type::operator=(rhs);
            p_list_offsets = make_list_offsets();
        }
        return *this;
    }

    template <bool BIG>
    auto list_array_impl<BIG>::offset_range(size_type i) const -> std::pair<offset_type, offset_type>
    {
        return std::make_pair(p_list_offsets[i], p_list_offsets[i + 1]);
    }

    template <bool BIG>
    auto list_array_impl<BIG>::make_list_offsets() -> offset_type*
    {
        return reinterpret_cast<offset_type*>(
            this->get_arrow_proxy().buffers()[OFFSET_BUFFER_INDEX].data() + this->get_arrow_proxy().offset()
        );
    }

    /***************************************
     * list_view_array_impl implementation *
     ***************************************/

    template <bool BIG>
    inline list_view_array_impl<BIG>::list_view_array_impl(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , p_list_offsets(make_list_offsets())
        , p_list_sizes(make_list_sizes())
    {
    }

    template <bool BIG>
    template <validity_bitmap_input VB, input_metadata_container METADATA_RANGE>
    arrow_proxy list_view_array_impl<BIG>::create_proxy(
        array&& flat_values,
        offset_buffer_type&& list_offsets,
        size_buffer_type&& list_sizes,
        VB&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        SPARROW_ASSERT(list_offsets.size() == list_sizes.size(), "sizes and offset must have the same size");
        const auto size = list_sizes.size();
        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<VB>(validity_input));
        auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(flat_values));
        const auto null_count = vbitmap.null_count();
        const repeat_view<bool> children_ownership{true, 1};
        static const std::optional<std::unordered_set<ArrowFlag>> flags{{ArrowFlag::NULLABLE}};

        ArrowSchema schema = make_arrow_schema(
            BIG ? std::string("+vL") : std::string("+vl"),                 // format
            name,                                                          // name
            metadata,                                                      // metadata
            flags,                                                         // flags,
            new ArrowSchema*[1]{new ArrowSchema(std::move(flat_schema))},  // children
            children_ownership,
            nullptr,  // dictionary
            true
        );
        std::vector<buffer<std::uint8_t>> arr_buffs = {
            std::move(vbitmap).extract_storage(),
            std::move(list_offsets).extract_storage(),
            std::move(list_sizes).extract_storage()
        };

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(arr_buffs),
            new ArrowArray*[1]{new ArrowArray(std::move(flat_arr))},  // children
            children_ownership,
            nullptr,  // dictionary
            true
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <bool BIG>
    template <input_metadata_container METADATA_RANGE>
    arrow_proxy list_view_array_impl<BIG>::create_proxy(
        array&& flat_values,
        offset_buffer_type&& list_offsets,
        size_buffer_type&& list_sizes,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        if (nullable)
        {
            return list_view_array_impl<BIG>::create_proxy(
                std::move(flat_values),
                std::move(list_offsets),
                std::move(list_sizes),
                validity_bitmap{},
                name,
                metadata
            );
        }
        else
        {
            SPARROW_ASSERT(list_offsets.size() == list_sizes.size(), "sizes and offset must have the same size");
            const auto size = list_sizes.size();
            auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(flat_values));
            const repeat_view<bool> children_ownership{true, 1};

            ArrowSchema schema = make_arrow_schema(
                BIG ? std::string("+vL") : std::string("+vl"),                 // format
                name,                                                          // name
                metadata,                                                      // metadata
                std::nullopt,                                                  // flags,
                new ArrowSchema*[1]{new ArrowSchema(std::move(flat_schema))},  // children
                children_ownership,
                nullptr,  // dictionary
                true
            );
            std::vector<buffer<std::uint8_t>> arr_buffs = {
                buffer<std::uint8_t>{nullptr, 0},  // no validity bitmap
                std::move(list_offsets).extract_storage(),
                std::move(list_sizes).extract_storage()
            };

            ArrowArray arr = make_arrow_array(
                static_cast<std::int64_t>(size),  // length
                0,                                // null_count
                0,                                // offset
                std::move(arr_buffs),
                new ArrowArray*[1]{new ArrowArray(std::move(flat_arr))},  // children
                children_ownership,
                nullptr,  // dictionary
                true
            );
            return arrow_proxy{std::move(arr), std::move(schema)};
        }
    }

    template <bool BIG>
    list_view_array_impl<BIG>::list_view_array_impl(const self_type& rhs)
        : base_type(rhs)
        , p_list_offsets(make_list_offsets())
        , p_list_sizes(make_list_sizes())
    {
    }

    template <bool BIG>
    auto list_view_array_impl<BIG>::operator=(const self_type& rhs) -> self_type&
    {
        if (this != &rhs)
        {
            base_type::operator=(rhs);
            p_list_offsets = make_list_offsets();
            p_list_sizes = make_list_sizes();
        }
        return *this;
    }

    template <bool BIG>
    inline auto list_view_array_impl<BIG>::offset_range(size_type i) const
        -> std::pair<offset_type, offset_type>
    {
        const auto offset = p_list_offsets[i];
        return std::make_pair(offset, offset + p_list_sizes[i]);
    }

    template <bool BIG>
    auto list_view_array_impl<BIG>::make_list_offsets() -> offset_type*
    {
        return reinterpret_cast<offset_type*>(
            this->get_arrow_proxy().buffers()[OFFSET_BUFFER_INDEX].data() + this->get_arrow_proxy().offset()
        );
    }

    template <bool BIG>
    auto list_view_array_impl<BIG>::make_list_sizes() -> offset_type*
    {
        return reinterpret_cast<offset_type*>(
            this->get_arrow_proxy().buffers()[SIZES_BUFFER_INDEX].data() + this->get_arrow_proxy().offset()
        );
    }

#ifdef __GNUC__
#    pragma GCC diagnostic pop
#endif

    /*****************************************
     * fixed_sized_list_array implementation *
     *****************************************/

    inline auto fixed_sized_list_array::list_size_from_format(const std::string_view format) -> uint64_t
    {
        SPARROW_ASSERT(format.size() >= 3, "Invalid format string");
        const auto n_digits = format.size() - 3;
        const auto list_size_str = format.substr(3, n_digits);
        return std::stoull(std::string(list_size_str));
    }

    inline fixed_sized_list_array::fixed_sized_list_array(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , m_list_size(fixed_sized_list_array::list_size_from_format(this->get_arrow_proxy().format()))
    {
    }

    inline auto fixed_sized_list_array::offset_range(size_type i) const -> std::pair<offset_type, offset_type>
    {
        const auto offset = i * m_list_size;
        return std::make_pair(offset, offset + m_list_size);
    }

    template <validity_bitmap_input R, input_metadata_container METADATA_RANGE>
    inline arrow_proxy fixed_sized_list_array::create_proxy(
        std::uint64_t list_size,
        array&& flat_values,
        R&& validity_input,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        const auto size = flat_values.size() / static_cast<std::size_t>(list_size);
        auto vbitmap = ensure_validity_bitmap(size, std::forward<R>(validity_input));

        auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(flat_values));

        const auto null_count = vbitmap.null_count();

        const repeat_view<bool> children_ownership{true, 1};

        std::string format = "+w:" + std::to_string(list_size);
        ArrowSchema schema = make_arrow_schema(
            format,
            std::move(name),                                               // name
            std::move(metadata),                                           // metadata
            std::nullopt,                                                  // flags,
            new ArrowSchema*[1]{new ArrowSchema(std::move(flat_schema))},  // children
            children_ownership,                                            // children ownership
            nullptr,                                                       // dictionary
            true                                                           // dictionary ownership

        );
        std::vector<buffer<std::uint8_t>> arr_buffs = {vbitmap.extract_storage()};

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(arr_buffs),
            new ArrowArray*[1]{new ArrowArray(std::move(flat_arr))},  // children
            children_ownership,                                       // children ownership
            nullptr,                                                  // dictionary
            true                                                      // dictionary ownership
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    template <validity_bitmap_input R, input_metadata_container METADATA_RANGE>
    inline arrow_proxy fixed_sized_list_array::create_proxy(
        std::uint64_t list_size,
        array&& flat_values,
        bool nullable,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    )
    {
        if (nullable)
        {
            return fixed_sized_list_array::create_proxy(
                list_size,
                std::move(flat_values),
                validity_bitmap{},
                name,
                metadata
            );
        }
        else
        {
            const auto size = flat_values.size() / static_cast<std::size_t>(list_size);
            auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(flat_values));
            const repeat_view<bool> children_ownership{true, 1};

            std::string format = "+w:" + std::to_string(list_size);
            ArrowSchema schema = make_arrow_schema(
                format,
                std::move(name),                                               // name
                std::move(metadata),                                           // metadata
                std::nullopt,                                                  // flags,
                new ArrowSchema*[1]{new ArrowSchema(std::move(flat_schema))},  // children
                children_ownership,                                            // children ownership
                nullptr,                                                       // dictionary
                true                                                           // dictionary ownership

            );
            std::vector<buffer<std::uint8_t>> arr_buffs = {
                buffer<std::uint8_t>{nullptr, 0}  // no validity bitmap
            };

            ArrowArray arr = make_arrow_array(
                static_cast<std::int64_t>(size),  // length
                0,                                // null_count
                0,                                // offset
                std::move(arr_buffs),
                new ArrowArray*[1]{new ArrowArray(std::move(flat_arr))},  // children
                children_ownership,                                       // children ownership
                nullptr,                                                  // dictionary
                true                                                      // dictionary ownership
            );
            return arrow_proxy{std::move(arr), std::move(schema)};
        }
    }
}
