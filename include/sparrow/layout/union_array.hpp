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

#include <array>
#include <optional>

#include "sparrow/array_api.hpp"
#include "sparrow/array_factory.hpp"
#include "sparrow/arrow_interface/arrow_flag_utils.hpp"
#include "sparrow/config/config.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/layout/array_helper.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/crtp_base.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/memory.hpp"
#include "sparrow/utils/metadata.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{
    class dense_union_array;
    class sparse_union_array;

    namespace detail
    {
        template <class T>
        struct get_data_type_from_array;

        template <>
        struct get_data_type_from_array<sparrow::dense_union_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::DENSE_UNION;
            }
        };

        template <>
        struct get_data_type_from_array<sparrow::sparse_union_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::SPARSE_UNION;
            }
        };
    }

    /**
     * Checks whether T is a dense_union_array type.
     */
    template <class T>
    constexpr bool is_dense_union_array_v = std::same_as<T, dense_union_array>;

    /**
     * Checks whether T is a sparse_union_array type.
     */
    template <class T>
    constexpr bool is_sparse_union_array_v = std::same_as<T, sparse_union_array>;

    // helper crtp-base to have sparse and dense and dense union share most of their code
    template <class DERIVED>
    class union_array_crtp_base : public crtp_base<DERIVED>
    {
    public:

        using self_type = union_array_crtp_base<DERIVED>;
        using derived_type = DERIVED;
        using inner_value_type = array_traits::inner_value_type;
        using value_type = array_traits::const_reference;
        using const_reference = array_traits::const_reference;
        using functor_type = detail::layout_bracket_functor<derived_type, value_type>;
        using const_functor_type = detail::layout_bracket_functor<const derived_type, value_type>;
        using iterator = functor_index_iterator<functor_type>;
        using const_iterator = functor_index_iterator<const_functor_type>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using size_type = std::size_t;

        using type_id_buffer_type = u8_buffer<std::uint8_t>;

        [[nodiscard]] constexpr std::optional<std::string_view> name() const;
        [[nodiscard]] constexpr std::optional<key_value_view> metadata() const;

        [[nodiscard]] constexpr value_type at(size_type i) const;
        [[nodiscard]] constexpr value_type operator[](size_type i) const;
        [[nodiscard]] constexpr value_type operator[](size_type i);
        [[nodiscard]] constexpr value_type front() const;
        [[nodiscard]] constexpr value_type back() const;

        [[nodiscard]] constexpr bool empty() const;
        [[nodiscard]] constexpr size_type size() const;

        [[nodiscard]] constexpr iterator begin();
        [[nodiscard]] constexpr iterator end();
        [[nodiscard]] constexpr const_iterator begin() const;
        [[nodiscard]] constexpr const_iterator end() const;
        [[nodiscard]] constexpr const_iterator cbegin() const;
        [[nodiscard]] constexpr const_iterator cend() const;

        [[nodiscard]] constexpr const_reverse_iterator rbegin() const;
        [[nodiscard]] constexpr const_reverse_iterator rend() const;

        [[nodiscard]] constexpr const_reverse_iterator crbegin() const;
        [[nodiscard]] constexpr const_reverse_iterator crend() const;

        /**
         * Sets all null values to the specified value.
         * This operation modifies the underlying data values but not the validity bitmap.
         * The bitmap remains unchanged, so the elements will still be considered null.
         *
         * @param value The value to assign to null elements
         */
        constexpr void zero_null_values(const inner_value_type& value)
        {
            sparrow::zero_null_values(*this, value);
        }

    protected:

        using type_id_map = std::array<std::uint8_t, 256>;
        static constexpr type_id_map parse_type_id_map(std::string_view format_string);

        template <std::ranges::input_range R>
        static constexpr type_id_map
        type_id_map_from_child_to_type_id(const std::optional<R>& child_index_to_type_id);

        template <std::ranges::input_range R>
            requires(std::convertible_to<std::ranges::range_value_t<R>, std::uint8_t>)
        static constexpr std::string
        make_format_string(bool dense, std::size_t n, const std::optional<R>& child_index_to_type_id);

        using children_type = std::vector<cloning_ptr<array_wrapper>>;
        constexpr children_type make_children(arrow_proxy& proxy);

        explicit union_array_crtp_base(arrow_proxy proxy);

        constexpr union_array_crtp_base(const self_type& rhs);
        constexpr self_type& operator=(const self_type& rhs);

        constexpr union_array_crtp_base(self_type&& rhs) = default;
        constexpr self_type& operator=(self_type&& rhs) = default;

        [[nodiscard]] constexpr arrow_proxy& get_arrow_proxy();
        [[nodiscard]] constexpr const arrow_proxy& get_arrow_proxy() const;

        arrow_proxy m_proxy;
        const std::uint8_t* p_type_ids;
        children_type m_children;

        // map from type-id to child-index
        std::array<std::uint8_t, 256> m_type_id_map;

        friend class detail::array_access;

#if defined(__cpp_lib_format)
        friend struct std::formatter<DERIVED>;
#endif
    };

    template <class D>
    constexpr bool operator==(const union_array_crtp_base<D>& lhs, const union_array_crtp_base<D>& rhs);

    /**
     * A dense union array implementation.
     * Related Apache Arrow specification: https://arrow.apache.org/docs/dev/format/Columnar.html#dense-union
     */
    class dense_union_array : public union_array_crtp_base<dense_union_array>
    {
    public:

        using base_type = union_array_crtp_base<dense_union_array>;
        using offset_buffer_type = u8_buffer<std::uint32_t>;
        using type_id_buffer_type = typename base_type::type_id_buffer_type;

        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<dense_union_array, Args...>)
        explicit dense_union_array(Args&&... args)
            : dense_union_array(create_proxy(std::forward<Args>(args)...))
        {
        }

        SPARROW_API explicit dense_union_array(arrow_proxy proxy);

        SPARROW_API dense_union_array(const dense_union_array& rhs);
        SPARROW_API dense_union_array& operator=(const dense_union_array& rhs);

        constexpr dense_union_array(dense_union_array&& rhs) = default;
        dense_union_array& operator=(dense_union_array&& rhs) = default;

    private:

        using type_id_map = typename base_type::type_id_map;

        template <
            std::ranges::input_range TYPE_MAPPING = std::vector<std::uint8_t>,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(std::convertible_to<std::ranges::range_value_t<TYPE_MAPPING>, std::uint8_t>)
        [[nodiscard]] static auto create_proxy(
            std::vector<array>&& children,
            type_id_buffer_type&& element_type,
            offset_buffer_type&& offsets,
            std::optional<TYPE_MAPPING>&& type_mapping = std::nullopt,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        template <
            std::ranges::input_range TYPE_ID_BUFFER_RANGE,
            std::ranges::input_range OFFSET_BUFFER_RANGE,
            std::ranges::input_range TYPE_MAPPING = std::vector<std::uint8_t>,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(std::convertible_to<std::ranges::range_value_t<TYPE_MAPPING>, std::uint8_t>)
        [[nodiscard]] static arrow_proxy create_proxy(
            std::vector<array>&& children,
            TYPE_ID_BUFFER_RANGE&& element_type,
            OFFSET_BUFFER_RANGE&& offsets,
            std::optional<TYPE_MAPPING>&& type_mapping = std::nullopt,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
        {
            SPARROW_ASSERT_TRUE(element_type.size() == offsets.size());
            type_id_buffer_type element_type_buffer{std::move(element_type)};
            offset_buffer_type offsets_buffer{std::move(offsets)};
            return dense_union_array::create_proxy(
                std::forward<std::vector<array>>(children),
                std::move(element_type_buffer),
                std::move(offsets_buffer),
                std::move(type_mapping),
                std::forward<std::optional<std::string_view>>(name),
                std::forward<std::optional<METADATA_RANGE>>(metadata)
            );
        }

        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy_impl(
            std::vector<array>&& children,
            type_id_buffer_type&& element_type,
            offset_buffer_type&& offsets,
            std::string&& format,
            type_id_map&& tim,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        );

        template <
            std::ranges::input_range TYPE_ID_BUFFER_RANGE,
            std::ranges::input_range OFFSET_BUFFER_RANGE,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static arrow_proxy create_proxy_impl(
            std::vector<array>&& children,
            TYPE_ID_BUFFER_RANGE&& element_type,
            OFFSET_BUFFER_RANGE&& offsets,
            std::string&& format,
            type_id_map&& tim,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        )
        {
            SPARROW_ASSERT_TRUE(std::ranges::distance(element_type) == std::ranges::distance(offsets));
            SPARROW_ASSERT_TRUE(std::ranges::distance(element_type) == children.size());
            type_id_buffer_type element_type_buffer{std::move(element_type)};
            offset_buffer_type offsets_buffer{std::move(offsets)};
            return dense_union_array::create_proxy_impl(
                std::forward<std::vector<array>>(children),
                std::move(element_type_buffer),
                std::move(offsets_buffer),
                std::forward<std::string>(format),
                std::forward<type_id_map>(tim),
                std::forward<std::optional<std::string_view>>(name),
                std::forward<std::optional<METADATA_RANGE>>(metadata)
            );
        }

        SPARROW_API std::size_t element_offset(std::size_t i) const;

        const std::int32_t* p_offsets;
        friend class union_array_crtp_base<dense_union_array>;
    };

    /**
     * A sparse union array implementation.
     * Related Apache Arrow specification: https://arrow.apache.org/docs/dev/format/Columnar.html#sparse-union
     */
    class sparse_union_array : public union_array_crtp_base<sparse_union_array>
    {
    public:

        using base_type = union_array_crtp_base<sparse_union_array>;
        using type_id_buffer_type = typename base_type::type_id_buffer_type;

        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<sparse_union_array, Args...>)
        explicit sparse_union_array(Args&&... args)
            : sparse_union_array(create_proxy(std::forward<Args>(args)...))
        {
        }

        SPARROW_API explicit sparse_union_array(arrow_proxy proxy);

    private:

        using type_id_map = typename base_type::type_id_map;

        template <
            std::ranges::input_range TYPE_MAPPING = std::vector<std::uint8_t>,
            input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
            requires(std::convertible_to<std::ranges::range_value_t<TYPE_MAPPING>, std::uint8_t>)
        static auto create_proxy(
            std::vector<array>&& children,
            type_id_buffer_type&& element_type,
            std::optional<TYPE_MAPPING>&& type_mapping = std::nullopt,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        template <input_metadata_container METADATA_RANGE>
        static auto create_proxy_impl(
            std::vector<array>&& children,
            type_id_buffer_type&& element_type,
            std::string&& format,
            type_id_map&& tim,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        [[nodiscard]] SPARROW_API std::size_t element_offset(std::size_t i) const;
        friend class union_array_crtp_base<sparse_union_array>;
    };

    /****************************************
     * union_array_crtp_base implementation *
     ****************************************/

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::parse_type_id_map(std::string_view format_string)
        -> type_id_map
    {
        type_id_map ret;
        // remove +du: / +su: prefix
        format_string.remove_prefix(4);

        constexpr std::string_view delim{","};
        std::size_t child_index = 0;
        std::ranges::for_each(
            format_string | std::views::split(delim),
            [&](const auto& s)
            {
                const std::string str(
                    std::string_view{&*std::ranges::begin(s), static_cast<size_t>(std::ranges::distance(s))}
                );
                const auto as_int = std::atoi(str.c_str());
                ret[static_cast<std::size_t>(as_int)] = static_cast<std::uint8_t>(child_index);
                ++child_index;
            }
        );
        return ret;
    }

    template <class DERIVED>
    template <std::ranges::input_range R>
    constexpr auto
    union_array_crtp_base<DERIVED>::type_id_map_from_child_to_type_id(const std::optional<R>& child_index_to_type_id)
        -> type_id_map
    {
        std::array<std::uint8_t, 256> ret;
        if (!child_index_to_type_id.has_value())
        {
            constexpr std::array<std::uint8_t, 256> default_mapping = []
            {
                std::array<std::uint8_t, 256> arr{};
                for (std::size_t i = 0; i < 256; ++i)
                {
                    arr[i] = static_cast<std::uint8_t>(i);
                }
                return arr;
            }();
            return default_mapping;
        }
        else
        {
            const std::size_t n = std::ranges::size(*child_index_to_type_id);
            for (std::size_t i = 0; i < n; ++i)
            {
                ret[(*child_index_to_type_id)[static_cast<std::uint8_t>(i)]] = static_cast<std::uint8_t>(i);
            }
        }
        return ret;
    }

    template <class DERIVED>
    template <std::ranges::input_range R>
        requires(std::convertible_to<std::ranges::range_value_t<R>, std::uint8_t>)
    constexpr std::string
    union_array_crtp_base<DERIVED>::make_format_string(bool dense, const std::size_t n, const std::optional<R>& range)
    {
        const auto range_size = range.has_value() ? std::ranges::size(*range) : 0;
        if (range_size == n || range_size == 0)
        {
            std::string ret = dense ? "+ud:" : "+us:";
            if (range_size == 0)
            {
                for (std::size_t i = 0; i < n; ++i)
                {
                    ret += std::to_string(i) + ",";
                }
            }
            else
            {
                for (const auto& v : *range)
                {
                    ret += std::to_string(v) + ",";
                }
            }
            ret.pop_back();
            return ret;
        }
        else
        {
            throw std::invalid_argument("Invalid type-id map");
        }
    }

    template <class DERIVED>
    constexpr std::optional<std::string_view> union_array_crtp_base<DERIVED>::name() const
    {
        return m_proxy.name();
    }

    template <class DERIVED>
    constexpr std::optional<key_value_view> union_array_crtp_base<DERIVED>::metadata() const
    {
        return m_proxy.metadata();
    }

    template <class DERIVED>
    constexpr arrow_proxy& union_array_crtp_base<DERIVED>::get_arrow_proxy()
    {
        return m_proxy;
    }

    template <class DERIVED>
    constexpr const arrow_proxy& union_array_crtp_base<DERIVED>::get_arrow_proxy() const
    {
        return m_proxy;
    }

    template <class DERIVED>
    union_array_crtp_base<DERIVED>::union_array_crtp_base(arrow_proxy proxy)
        : m_proxy(std::move(proxy))
        , p_type_ids(reinterpret_cast<std::uint8_t*>(m_proxy.buffers()[0 /*index of type-ids*/].data()))
        , m_children(make_children(m_proxy))
        , m_type_id_map(parse_type_id_map(m_proxy.format()))
    {
    }

    template <class DERIVED>
    constexpr union_array_crtp_base<DERIVED>::union_array_crtp_base(const self_type& rhs)
        : self_type(rhs.m_proxy)
    {
    }

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::operator=(const self_type& rhs) -> self_type&
    {
        if (this != &rhs)
        {
            m_proxy = rhs.m_proxy;
            p_type_ids = reinterpret_cast<std::uint8_t*>(m_proxy.buffers()[0 /*index of type-ids*/].data());
            m_children = make_children(m_proxy);
            m_type_id_map = parse_type_id_map(m_proxy.format());
        }
        return *this;
    }

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::operator[](std::size_t i) const -> value_type
    {
        const auto type_id = static_cast<std::size_t>(p_type_ids[i]);
        const auto child_index = m_type_id_map[type_id];
        const auto offset = this->derived_cast().element_offset(i);
        return array_element(*m_children[child_index], static_cast<std::size_t>(offset));
    }

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::operator[](std::size_t i) -> value_type
    {
        return static_cast<const derived_type&>(*this)[i];
    }

    template <class DERIVED>
    constexpr std::size_t union_array_crtp_base<DERIVED>::size() const
    {
        return m_proxy.length();
    }

    template <class DERIVED>
    constexpr bool union_array_crtp_base<DERIVED>::empty() const
    {
        return size() == 0;
    }

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::begin() -> iterator
    {
        return iterator(functor_type{&(this->derived_cast())}, 0);
    }

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::end() -> iterator
    {
        return iterator(functor_type{&(this->derived_cast())}, this->size());
    }

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::begin() const -> const_iterator
    {
        return cbegin();
    }

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::end() const -> const_iterator
    {
        return cend();
    }

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::cbegin() const -> const_iterator
    {
        return const_iterator(const_functor_type{&(this->derived_cast())}, 0);
    }

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::cend() const -> const_iterator
    {
        return const_iterator(const_functor_type{&(this->derived_cast())}, this->size());
    }

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::rbegin() const -> const_reverse_iterator
    {
        return const_reverse_iterator{cend()};
    }

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::rend() const -> const_reverse_iterator
    {
        return const_reverse_iterator{cbegin()};
    }

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::crbegin() const -> const_reverse_iterator
    {
        return rbegin();
    }

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::crend() const -> const_reverse_iterator
    {
        return rend();
    }

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::front() const -> value_type
    {
        return (*this)[0];
    }

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::back() const -> value_type
    {
        return (*this)[this->size() - 1];
    }

    template <class DERIVED>
    constexpr auto union_array_crtp_base<DERIVED>::make_children(arrow_proxy& proxy) -> children_type
    {
        children_type children(proxy.children().size(), nullptr);
        for (std::size_t i = 0; i < children.size(); ++i)
        {
            children[i] = array_factory(proxy.children()[i].view());
        }
        return children;
    }

    template <class D>
    constexpr bool operator==(const union_array_crtp_base<D>& lhs, const union_array_crtp_base<D>& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }

    /************************************
     * dense_union_array implementation *
     ************************************/

    template <std::ranges::input_range TYPE_MAPPING, input_metadata_container METADATA_RANGE>
        requires(std::convertible_to<std::ranges::range_value_t<TYPE_MAPPING>, std::uint8_t>)
    auto dense_union_array::create_proxy(
        std::vector<array>&& children,
        type_id_buffer_type&& element_type,
        offset_buffer_type&& offsets,
        std::optional<TYPE_MAPPING>&& child_index_to_type_id,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        SPARROW_ASSERT_TRUE(element_type.size() == offsets.size());
        const auto n_children = children.size();

        // inverse type mapping (type_id -> child_index)
        auto type_id_to_child_index = type_id_map_from_child_to_type_id(child_index_to_type_id);

        std::string format = make_format_string(true /*dense union*/, n_children, child_index_to_type_id);

        return create_proxy_impl(
            std::move(children),
            std::move(element_type),
            std::move(offsets),
            std::move(format),
            std::move(type_id_to_child_index),
            std::move(name),
            std::move(metadata)
        );
    }

    template <input_metadata_container METADATA_RANGE>
    auto dense_union_array::create_proxy_impl(
        std::vector<array>&& children,
        type_id_buffer_type&& element_type,
        offset_buffer_type&& offsets,
        std::string&& format,
        type_id_map&& tim,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        SPARROW_ASSERT_TRUE(element_type.size() == offsets.size());
        const auto n_children = children.size();
        ArrowSchema** child_schemas = new ArrowSchema*[n_children];
        ArrowArray** child_arrays = new ArrowArray*[n_children];
        const auto size = element_type.size();

        // count nulls (expensive!)
        int64_t null_count = 0;
        for (std::size_t i = 0; i < size; ++i)
        {
            // child_id from type_id
            const auto type_id = static_cast<std::uint8_t>(element_type[i]);
            const auto child_index = tim[type_id];
            const auto offset = static_cast<std::size_t>(offsets[i]);
            // check if child is null
            if (!children[child_index][offset].has_value())
            {
                ++null_count;
            }
        }

        for (std::size_t i = 0; i < n_children; ++i)
        {
            auto& child = children[i];
            auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(child));
            child_arrays[i] = new ArrowArray(std::move(flat_arr));
            child_schemas[i] = new ArrowSchema(std::move(flat_schema));
        }

        const bool is_nullable = std::all_of(
            child_schemas,
            child_schemas + n_children,
            [](const ArrowSchema* schema)
            {
                return to_set_of_ArrowFlags(schema->flags).contains(ArrowFlag::NULLABLE);
            }
        );

        const std::optional<std::unordered_set<sparrow::ArrowFlag>>
            flags = is_nullable
                        ? std::make_optional(std::unordered_set<sparrow::ArrowFlag>{ArrowFlag::NULLABLE})
                        : std::nullopt;

        ArrowSchema schema = make_arrow_schema(
            std::move(format),
            std::move(name),                      // name
            std::move(metadata),                  // metadata
            flags,                                // flags,
            child_schemas,                        // children
            repeat_view<bool>(true, n_children),  // children_ownership
            nullptr,                              // dictionary,
            true                                  // dictionary ownership
        );

        std::vector<buffer<std::uint8_t>> arr_buffs = {
            std::move(element_type).extract_storage(),
            std::move(offsets).extract_storage()
        };

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<std::int64_t>(null_count),
            0,  // offset
            std::move(arr_buffs),
            child_arrays,                         // children
            repeat_view<bool>(true, n_children),  // children_ownership
            nullptr,                              // dictionary,
            true
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    /*************************************
     * sparse_union_array implementation *
     *************************************/

    template <std::ranges::input_range TYPE_MAPPING, input_metadata_container METADATA_RANGE>
        requires(std::convertible_to<std::ranges::range_value_t<TYPE_MAPPING>, std::uint8_t>)
    auto sparse_union_array::create_proxy(
        std::vector<array>&& children,
        type_id_buffer_type&& element_type,
        std::optional<TYPE_MAPPING>&& child_index_to_type_id,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        const auto n_children = children.size();
        if (child_index_to_type_id.has_value())
        {
            SPARROW_ASSERT_TRUE((*child_index_to_type_id).size() == n_children);
        }

        // inverse type mapping (type_id -> child_index)
        auto type_id_to_child_index = type_id_map_from_child_to_type_id(child_index_to_type_id);

        std::string format = make_format_string(false /*is dense union*/, n_children, child_index_to_type_id);

        return create_proxy_impl(
            std::move(children),
            std::move(element_type),
            std::move(format),
            std::move(type_id_to_child_index),
            std::move(name),
            std::move(metadata)
        );
    }

    template <input_metadata_container METADATA_RANGE>
    auto sparse_union_array::create_proxy_impl(
        std::vector<array>&& children,
        type_id_buffer_type&& element_type,
        std::string&& format,
        type_id_map&& tim,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        for (const auto& child : children)
        {
            SPARROW_ASSERT_TRUE(child.size() == element_type.size());
        }
        const auto n_children = children.size();
        ArrowSchema** child_schemas = new ArrowSchema*[n_children];
        ArrowArray** child_arrays = new ArrowArray*[n_children];
        const auto size = element_type.size();

        // count nulls (expensive!)
        int64_t null_count = 0;
        for (std::size_t i = 0; i < size; ++i)
        {
            // child_id from type_id
            const auto type_id = static_cast<std::uint8_t>(element_type[i]);
            const auto child_index = tim[type_id];
            // check if child is null
            if (!children[child_index][i].has_value())
            {
                ++null_count;
            }
        }

        for (std::size_t i = 0; i < n_children; ++i)
        {
            auto& child = children[i];
            auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(child));
            child_arrays[i] = new ArrowArray(std::move(flat_arr));
            child_schemas[i] = new ArrowSchema(std::move(flat_schema));
        }

        const bool is_nullable = std::all_of(
            child_schemas,
            child_schemas + n_children,
            [](const ArrowSchema* schema)
            {
                return to_set_of_ArrowFlags(schema->flags).contains(ArrowFlag::NULLABLE);
            }
        );

        const std::optional<std::unordered_set<sparrow::ArrowFlag>>
            flags = is_nullable
                        ? std::make_optional(std::unordered_set<sparrow::ArrowFlag>{ArrowFlag::NULLABLE})
                        : std::nullopt;

        ArrowSchema schema = make_arrow_schema(
            std::move(format),
            std::move(name),                      // name
            std::move(metadata),                  // metadata
            flags,                                // flags,
            child_schemas,                        // children
            repeat_view<bool>(true, n_children),  // children_ownership
            nullptr,                              // dictionary,
            true                                  // dictionary ownership
        );

        std::vector<buffer<std::uint8_t>> arr_buffs = {std::move(element_type).extract_storage()};

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<std::int64_t>(null_count),
            0,  // offset
            std::move(arr_buffs),
            child_arrays,                         // children
            repeat_view<bool>(true, n_children),  // children_ownership
            nullptr,                              // dictionary
            true
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }
}

#if defined(__cpp_lib_format)

template <typename U>
    requires std::derived_from<U, sparrow::union_array_crtp_base<U>>
struct std::formatter<U>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const U& ar, std::format_context& ctx) const
    {
        if constexpr (std::is_same_v<U, sparrow::dense_union_array>)
        {
            std::format_to(ctx.out(), "DenseUnion");
        }
        else if constexpr (std::is_same_v<U, sparrow::sparse_union_array>)
        {
            std::format_to(ctx.out(), "SparseUnion");
        }
        else
        {
            static_assert(sparrow::mpl::dependent_false<U>::value, "Unknown union array type");
            sparrow::mpl::unreachable();
        }
        const auto& proxy = ar.get_arrow_proxy();
        std::format_to(ctx.out(), " [name={} | size={}] <", proxy.name().value_or("nullptr"), proxy.length());

        std::for_each(
            ar.cbegin(),
            std::prev(ar.cend()),
            [&ctx](const auto& value)
            {
                std::format_to(ctx.out(), "{}, ", value);
            }
        );

        return std::format_to(ctx.out(), "{}>", ar.back());
    }
};

namespace sparrow
{
    template <typename U>
        requires std::derived_from<U, sparrow::union_array_crtp_base<U>>
    std::ostream& operator<<(std::ostream& os, const U& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

#endif
