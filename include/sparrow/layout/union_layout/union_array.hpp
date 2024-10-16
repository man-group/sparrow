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
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/memory.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/layout/array_helper.hpp"
#include "sparrow/utils/crtp_base.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"




namespace sparrow
{   

    
    // helper crtp-base to have sparse and dense and dense union in the same file
    template<class DERIVED>
    class union_array_crtp_base : public crtp_base<DERIVED>
    {
    public: 
        using derived_type = DERIVED;
        using value_type = array_traits::const_reference;
        using iterator = functor_index_iterator<detail::layout_bracket_functor<derived_type, value_type>>;
        using const_iterator = functor_index_iterator<detail::layout_bracket_functor<const derived_type, value_type>>;

        explicit union_array_crtp_base(arrow_proxy proxy);
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
        static type_id_map parse_type_id_map(std::string_view format_string)
        {
            type_id_map ret;
            // remove +du: / +su: prefix
            format_string.remove_prefix(4); 

            // const std::string_view const_format_string = format_string;

            constexpr std::string_view delim { "," };
            std::size_t child_index = 0;    
            std::ranges::for_each(format_string | std::views::split(delim), [&](const auto& s) { 
                // convert s to uint8_t number
                const auto as_int = std::atoi(std::string(s.begin(), s.end()).c_str());
                ret[static_cast<std::size_t>(as_int)] = static_cast<std::uint8_t>(child_index);
                ++child_index;
            });
            return ret;
        }

        arrow_proxy m_proxy;
        const std::uint8_t * p_type_ids;
        std::vector<cloning_ptr<array_wrapper>> m_children;

        // map from type-id to child-index
        std::array<std::uint8_t, 256> m_type_id_map;
        
    };  

    class dense_union_array : public union_array_crtp_base<dense_union_array>
    {
    public:
        explicit dense_union_array(arrow_proxy proxy);
    private:
        std::size_t element_offset(std::size_t i) const;
        const std::int32_t *  p_offsets;
        friend class union_array_crtp_base<dense_union_array>;
    };

    class sparse_union_array : public union_array_crtp_base<sparse_union_array>
    {
    public:
        using union_array_crtp_base<sparse_union_array>::union_array_crtp_base;
    private:
        std::size_t element_offset(std::size_t i) const;
        friend class union_array_crtp_base<sparse_union_array>;
    };

    template <class DERIVED>
    union_array_crtp_base<DERIVED>::union_array_crtp_base(arrow_proxy proxy)
    :   m_proxy(std::move(proxy)),
        p_type_ids(reinterpret_cast<std::uint8_t*>(m_proxy.buffers()[0].data())),
        m_children(m_proxy.children().size(), nullptr),
        m_type_id_map(parse_type_id_map(m_proxy.format()))
    {
        for (std::size_t i = 0; i < m_children.size(); ++i)
        {
            m_children[i] = array_factory(m_proxy.children()[i].view());
        }
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
        return iterator(detail::layout_bracket_functor<derived_type, value_type>{this}, 0);
    }

    template <class DERIVED>
    auto union_array_crtp_base<DERIVED>::end() -> iterator
    {
        return iterator(detail::layout_bracket_functor<derived_type, value_type>{this}, this->size());
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
        return const_iterator(detail::layout_bracket_functor<const derived_type, value_type>{this}, 0);
    }

    template <class DERIVED>
    auto union_array_crtp_base<DERIVED>::cend() const  -> const_iterator
    {
        return const_iterator(detail::layout_bracket_functor<const derived_type, value_type>{this}, this->size());
    }

    inline dense_union_array::dense_union_array(arrow_proxy proxy)
    :   union_array_crtp_base(std::move(proxy)),
        p_offsets(reinterpret_cast<std::int32_t*>(m_proxy.buffers()[1].data()))
    {
    }
    inline std::size_t dense_union_array::element_offset(std::size_t i) const
    {
        return static_cast<std::size_t>(p_offsets[i]) + static_cast<std::size_t>(m_proxy.offset());
    }

    inline std::size_t sparse_union_array::element_offset(std::size_t i) const
    {
        return i + static_cast<std::size_t>(m_proxy.offset());
    }
}