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

#include "sparrow/config/config.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/array_factory.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/utils/memory.hpp"
#include "sparrow/layout/array_helper.hpp"
#include "sparrow/utils/crtp_base.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/array_api.hpp"

namespace sparrow
{   
    class dense_union_array;
    class sparse_union_array;

    namespace detail
    {
        template<class T>
        struct get_data_type_from_array;

        template<>
        struct get_data_type_from_array<sparrow::dense_union_array>
        {
            constexpr static sparrow::data_type get()
            {
                return sparrow::data_type::DENSE_UNION;
            }
        };
        template<>
        struct get_data_type_from_array<sparrow::sparse_union_array>
        {
            constexpr static sparrow::data_type get()
            {
                return sparrow::data_type::SPARSE_UNION;
            }
        };
    }

    // helper crtp-base to have sparse and dense and dense union share most of their code
    template<class DERIVED>
    class union_array_crtp_base : public crtp_base<DERIVED>
    {
    public: 

        using self_type = union_array_crtp_base<DERIVED>;
        using derived_type = DERIVED;
        using inner_value_type = array_traits::inner_value_type;
        using value_type = array_traits::const_reference;
        using functor_type = detail::layout_bracket_functor<derived_type, value_type>;
        using const_functor_type = detail::layout_bracket_functor<const derived_type, value_type>;
        using iterator = functor_index_iterator<functor_type>;
        using const_iterator = functor_index_iterator<const_functor_type>;

        using type_id_buffer_type  = u8_buffer<std::uint8_t>;

        value_type operator[](std::size_t i) const;
        value_type operator[](std::size_t i);

        std::size_t size() const;

        iterator begin();
        iterator end();
        const_iterator begin() const;
        const_iterator end() const;
        const_iterator cbegin() const;
        const_iterator cend() const;

    protected:

        using type_id_map = std::array<std::uint8_t, 256>;
        static type_id_map parse_type_id_map(std::string_view format_string);

        template <std::ranges::input_range R>
        requires(std::convertible_to<std::ranges::range_value_t<R>, std::uint8_t>)
        static std::string make_format_string(bool dense, std::size_t n, R&& range);

        using children_type = std::vector<cloning_ptr<array_wrapper>>;
        children_type make_children(arrow_proxy& proxy);

        explicit union_array_crtp_base(arrow_proxy proxy);

        union_array_crtp_base(const self_type& rhs);
        self_type& operator=(const self_type& rhs);

        union_array_crtp_base(self_type&& rhs) = default;
        self_type& operator=(self_type&& rhs) = default;

        [[nodiscard]] arrow_proxy& get_arrow_proxy();
        [[nodiscard]] const arrow_proxy& get_arrow_proxy() const;

        arrow_proxy m_proxy;
        const std::uint8_t * p_type_ids;
        children_type m_children;

        // map from type-id to child-index
        std::array<std::uint8_t, 256> m_type_id_map;

        friend class detail::array_access;
    };  

    template <class D>
    bool operator==(const union_array_crtp_base<D>& lhs, const union_array_crtp_base<D>& rhs);

    class dense_union_array : public union_array_crtp_base<dense_union_array>
    {
    public:

        using base_type = union_array_crtp_base<dense_union_array>;
        using offset_buffer_type  = u8_buffer<std::uint32_t>;
        using type_id_buffer_type  = typename base_type::type_id_buffer_type;

        template <class ... Args>
        requires(mpl::excludes_copy_and_move_ctor_v<dense_union_array, Args...>)
        explicit dense_union_array(Args&& ... args)
            : dense_union_array(create_proxy(std::forward<Args>(args) ...))
        {}

        explicit dense_union_array(arrow_proxy proxy);

        dense_union_array(const dense_union_array& rhs);
        dense_union_array& operator=(const dense_union_array& rhs);

        dense_union_array(dense_union_array&& rhs) = default;
        dense_union_array& operator=(dense_union_array&& rhs) = default;

    private:

        template <
            std::ranges::input_range TYPE_MAPPING  = std::vector<std::uint8_t>
        >
        requires(std::convertible_to<std::ranges::range_value_t<TYPE_MAPPING>, std::uint8_t>)
        static auto create_proxy(
            std::vector<array> && children,
            type_id_buffer_type && element_type,    
            offset_buffer_type && offsets,
            TYPE_MAPPING && type_mapping = TYPE_MAPPING{}
        ) -> arrow_proxy;

        std::size_t element_offset(std::size_t i) const;
        const std::int32_t *  p_offsets;
        friend class union_array_crtp_base<dense_union_array>;
    };

    class sparse_union_array : public union_array_crtp_base<sparse_union_array>
    {
    public:
        
        using base_type = union_array_crtp_base<sparse_union_array>;
        using type_id_buffer_type  = typename base_type::type_id_buffer_type;

        template <class ... Args>
        requires(mpl::excludes_copy_and_move_ctor_v<sparse_union_array, Args...>)
        explicit sparse_union_array(Args&& ... args)
            : sparse_union_array(create_proxy(std::forward<Args>(args) ...))
        {}

        explicit sparse_union_array(arrow_proxy proxy);

        template <
            std::ranges::input_range TYPE_MAPPING  = std::vector<std::uint8_t>
        >
        requires(std::convertible_to<std::ranges::range_value_t<TYPE_MAPPING>, std::uint8_t>)
        static auto create_proxy(
            std::vector<array> && children,
            type_id_buffer_type && element_type,
            TYPE_MAPPING && type_mapping = TYPE_MAPPING{}
        ) -> arrow_proxy;


    private:
        std::size_t element_offset(std::size_t i) const;
        friend class union_array_crtp_base<sparse_union_array>;
    };

    template <class DERIVED>
    auto union_array_crtp_base<DERIVED>::parse_type_id_map(std::string_view format_string) -> type_id_map
    {
        type_id_map ret;
        // remove +du: / +su: prefix
        format_string.remove_prefix(4); 
 
        constexpr std::string_view delim { "," };
        std::size_t child_index = 0;    
        std::ranges::for_each(format_string | std::views::split(delim), [&](const auto& s) { 
            const auto as_int = std::atoi(std::string(s.begin(), s.end()).c_str());
            ret[static_cast<std::size_t>(as_int)] = static_cast<std::uint8_t>(child_index);
            ++child_index;
        });
        return ret;
    }

    template <class DERIVED>
    template <std::ranges::input_range R>
    requires(std::convertible_to<std::ranges::range_value_t<R>, std::uint8_t>)
    std::string union_array_crtp_base<DERIVED>::make_format_string(bool dense, const std::size_t n, R&& range)
    {
        const auto range_size = std::ranges::size(range);
        if(range_size == n || range_size == 0)
        {   
            std::string ret = dense ? "+ud:" : "+us:";
            if(range_size == 0)
            {
                for(std::size_t i = 0; i < n; ++i)
                {
                    ret += std::to_string(i) + ",";
                }
            }
            else
            {
                for (const auto& v : range)
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


    /****************************************
     * union_array_crtp_base implementation *
     ****************************************/

    template <class DERIVED>
    arrow_proxy& union_array_crtp_base<DERIVED>::get_arrow_proxy()
    {
        return m_proxy;
    }

    template <class DERIVED>
    const arrow_proxy& union_array_crtp_base<DERIVED>::get_arrow_proxy() const
    {
        return m_proxy;
    }

    template <class DERIVED>
    union_array_crtp_base<DERIVED>::union_array_crtp_base(arrow_proxy proxy)
        : m_proxy(std::move(proxy))
        , p_type_ids(reinterpret_cast<std::uint8_t*>(m_proxy.buffers()[0/*index of type-ids*/].data()))
        , m_children(make_children(m_proxy))
        , m_type_id_map(parse_type_id_map(m_proxy.format()))
    {
    }

    template <class DERIVED>
    union_array_crtp_base<DERIVED>::union_array_crtp_base(const self_type& rhs)
        : self_type(rhs.m_proxy)
    {
    }

    template <class DERIVED>
    auto union_array_crtp_base<DERIVED>::operator=(const self_type& rhs) -> self_type&
    {
        if (this != &rhs)
        {
            m_proxy = rhs.m_proxy;
            p_type_ids = reinterpret_cast<std::uint8_t*>(m_proxy.buffers()[0/*index of type-ids*/].data());
            m_children = make_children(m_proxy);
            m_type_id_map = parse_type_id_map(m_proxy.format());
        }
        return *this;
    }

    template <class DERIVED>
    auto union_array_crtp_base<DERIVED>::operator[](std::size_t i) const -> value_type
    {   
        const auto type_id = static_cast<std::size_t>(p_type_ids[i]);
        const auto child_index = m_type_id_map[type_id];
        const auto offset = this->derived_cast().element_offset(i);
        return array_element(*m_children[child_index], static_cast<std::size_t>(offset));
    }

    template <class DERIVED>
    auto union_array_crtp_base<DERIVED>::operator[](std::size_t i) -> value_type
    {   
        return static_cast<const derived_type&>(*this)[i];
    }

    template <class DERIVED>
    std::size_t union_array_crtp_base<DERIVED>::size() const
    {
        return m_proxy.length();
    }

    template <class DERIVED>
    auto union_array_crtp_base<DERIVED>::begin() -> iterator
    {
        return iterator(functor_type{&(this->derived_cast())}, 0);
    }

    template <class DERIVED>
    auto union_array_crtp_base<DERIVED>::end() -> iterator
    {
        return iterator(functor_type{&(this->derived_cast())}, this->size());
    }

    template <class DERIVED>
    auto union_array_crtp_base<DERIVED>::begin() const -> const_iterator    
    {
        return cbegin();
    }

    template <class DERIVED>
    auto union_array_crtp_base<DERIVED>::end() const -> const_iterator   
    {
        return cend();
    }

    template <class DERIVED>
    auto union_array_crtp_base<DERIVED>::cbegin() const -> const_iterator   
    {
        return const_iterator(const_functor_type{&(this->derived_cast())}, 0);
    }

    template <class DERIVED>
    auto union_array_crtp_base<DERIVED>::cend() const  -> const_iterator
    {
        return const_iterator(const_functor_type{&(this->derived_cast())}, this->size());
    }

    template <class DERIVED>
    auto union_array_crtp_base<DERIVED>::make_children(arrow_proxy& proxy) -> children_type
    {
        children_type children(proxy.children().size(), nullptr);
        for (std::size_t i = 0; i < children.size(); ++i)
        {
            children[i] = array_factory(proxy.children()[i].view());
        }
        return children;
    }

    template <class D>
    bool operator==(const union_array_crtp_base<D>& lhs, const union_array_crtp_base<D>& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }

    /************************************
     * dense_union_array implementation *
     ************************************/     

    #ifdef __GNUC__
    #    pragma GCC diagnostic push
    #    pragma GCC diagnostic ignored "-Wcast-align"
    #endif
    inline dense_union_array::dense_union_array(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , p_offsets(reinterpret_cast<std::int32_t*>(m_proxy.buffers()[1/*index of offsets*/].data()))
    {
    }

    inline dense_union_array::dense_union_array(const dense_union_array& rhs)
        : dense_union_array(rhs.m_proxy)
    {
    }

    inline dense_union_array& dense_union_array::operator=(const dense_union_array& rhs)
    {
        if (this !=&rhs)
        {
            base_type::operator=(rhs);
            p_offsets = reinterpret_cast<std::int32_t*>(m_proxy.buffers()[1/*index of offsets*/].data());
        }
        return *this;
    }

    template <
        std::ranges::input_range TYPE_MAPPING
    >
    requires(std::convertible_to<std::ranges::range_value_t<TYPE_MAPPING>, std::uint8_t>)
    auto dense_union_array::create_proxy(
        std::vector<array> && children,
        type_id_buffer_type && element_type,    
        offset_buffer_type && offsets,
        TYPE_MAPPING && type_mapping
    ) -> arrow_proxy
    {
        const auto n_children = children.size();
        ArrowSchema** child_schemas = new ArrowSchema*[n_children];
        ArrowArray** child_arrays = new ArrowArray*[n_children];
        const auto size = element_type.size();

        // count nulls (expensive!)
        int64_t null_count = 0;
        for(std::size_t i = 0; i < size; ++i)
        {
            // child_id from type_id
            const auto type_id = static_cast<std::uint8_t>(element_type[i]);
            const auto child_index = std::ranges::size(type_mapping) == 0 ? type_id : type_mapping[type_id];
            const auto offset = static_cast<std::size_t>(offsets[i]);
            // check if child is null
            if (!children[child_index][offset].has_value())
            {
                ++null_count;
            }
        }

        for(std::size_t i=0; i<n_children; ++i)
        {
            auto & child = children[i];
            auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(child));
            child_arrays[i] = new ArrowArray(std::move(flat_arr));
            child_schemas[i] = new ArrowSchema(std::move(flat_schema));
        }

        std::string format = make_format_string(true /*dense union*/, n_children, std::forward<TYPE_MAPPING>(type_mapping));

        ArrowSchema schema = make_arrow_schema(
            format,
            std::nullopt, // name
            std::nullopt, // metadata
            std::nullopt, // flags,
            static_cast<int64_t>(n_children),
            child_schemas, // children
            nullptr // dictionary
        );

        std::vector<buffer<std::uint8_t>> arr_buffs = {
            std::move(element_type).extract_storage(),
            std::move(offsets).extract_storage()
        };

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size), // length
            static_cast<std::int64_t>(null_count),
            0, // offset
            std::move(arr_buffs),
            static_cast<std::size_t>(n_children), // n_children
            child_arrays, // children
            nullptr // dictionary
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    #ifdef __GNUC__
    #    pragma GCC diagnostic pop
    #endif

    inline std::size_t dense_union_array::element_offset(std::size_t i) const
    {
        return static_cast<std::size_t>(p_offsets[i]) + m_proxy.offset();
    }

    /*************************************
     * sparse_union_array implementation *
     *************************************/

    inline sparse_union_array::sparse_union_array(arrow_proxy proxy)
        : base_type(std::move(proxy))
    {
    }

    template <
        std::ranges::input_range TYPE_MAPPING
    >
    requires(std::convertible_to<std::ranges::range_value_t<TYPE_MAPPING>, std::uint8_t>)
    auto sparse_union_array::create_proxy(
        std::vector<array> && children,
        type_id_buffer_type && element_type,
        TYPE_MAPPING && type_mapping
    ) -> arrow_proxy
    {
        const auto n_children = children.size();
        ArrowSchema** child_schemas = new ArrowSchema*[n_children];
        ArrowArray** child_arrays = new ArrowArray*[n_children];
        const auto size = element_type.size();

        // count nulls (expensive!)
        int64_t null_count = 0;
        for(std::size_t i = 0; i < size; ++i)
        {
            // child_id from type_id
            const auto type_id = static_cast<std::uint8_t>(element_type[i]);
            const auto child_index = std::ranges::size(type_mapping) == 0 ? type_id : type_mapping[type_id];
            // check if child is null
            if (!children[child_index][i].has_value())
            {
                ++null_count;
            }
        }

        for(std::size_t i=0; i<n_children; ++i)
        {
            auto & child = children[i];
            auto [flat_arr, flat_schema] = extract_arrow_structures(std::move(child));
            child_arrays[i] = new ArrowArray(std::move(flat_arr));
            child_schemas[i] = new ArrowSchema(std::move(flat_schema));
        }

        std::string format = make_format_string(false /*is dense union*/, n_children, std::forward<TYPE_MAPPING>(type_mapping));

        ArrowSchema schema = make_arrow_schema(
            format,
            std::nullopt, // name
            std::nullopt, // metadata
            std::nullopt, // flags,
            static_cast<int64_t>(n_children),
            child_schemas, // children
            nullptr // dictionary
        );

        std::vector<buffer<std::uint8_t>> arr_buffs = {
            std::move(element_type).extract_storage()
        };

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size), // length
            static_cast<std::int64_t>(null_count),
            0, // offset
            std::move(arr_buffs),
            static_cast<std::size_t>(n_children), // n_children
            child_arrays, // children
            nullptr // dictionary
        );
        return arrow_proxy{std::move(arr), std::move(schema)};
    }

    inline std::size_t sparse_union_array::element_offset(std::size_t i) const
    {
        return i + m_proxy.offset();
    }
}
