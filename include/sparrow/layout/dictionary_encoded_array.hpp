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

#include "sparrow/array_factory.hpp"
#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/layout/array_base.hpp"
#include "sparrow/layout/array_helper.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/layout/primitive_array.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/memory.hpp"
#include "sparrow/layout/array_access.hpp"

namespace sparrow
{
    template <class Layout, bool is_const>
    class layout_element_functor
    {
    public:

        using layout_type = Layout;
        using storage_type = std::conditional_t<is_const, const layout_type*, layout_type>;
        using return_type = std::conditional_t<is_const, typename layout_type::const_reference, typename layout_type::reference>;

        constexpr layout_element_functor() = default;

        constexpr explicit layout_element_functor(storage_type layout)
            : p_layout(layout)
        {
        }
        
        return_type operator()(std::size_t i) const
        {
            return p_layout->operator[](i);
        }

    private:

        storage_type p_layout;
    };

    template <std::integral IT>
    class dictionary_encoded_array;

    namespace detail
    {
        template<class T>
        struct get_data_type_from_array;

        template<std::integral IT>
        struct get_data_type_from_array<sparrow::dictionary_encoded_array<IT>>
        {
            constexpr static sparrow::data_type get()
            {
                return arrow_traits<typename primitive_array<IT>::inner_value_type>::type_id;
            }
        };

        template <std::integral IT>
        struct is_dictionary_encoded_array<sparrow::dictionary_encoded_array<IT>>
        {
            constexpr static bool get()
            {
                return true;
            }
        };
    }

    template <std::integral IT>
    class dictionary_encoded_array
    {
    public:

        using self_type = dictionary_encoded_array<IT>;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        using inner_value_type = array_traits::inner_value_type;

        using value_type = array_traits::value_type;
        using reference = array_traits::const_reference;
        using const_reference = array_traits::const_reference;

        using functor_type = layout_element_functor<self_type, true>;
        using const_functor_type = layout_element_functor<self_type, true>;

        using iterator = functor_index_iterator<functor_type>;
        using const_iterator = functor_index_iterator<const_functor_type>;

        explicit dictionary_encoded_array(arrow_proxy);

        dictionary_encoded_array(const self_type&);
        self_type& operator=(const self_type&);

        dictionary_encoded_array(self_type&&);
        self_type& operator=(self_type&&);

        size_type size() const;

        const_reference operator[](size_type i) const;

        iterator begin();
        iterator end();

        const_iterator begin() const;
        const_iterator end() const;

        const_iterator cbegin() const;
        const_iterator cend() const;

    private:

        using keys_layout = primitive_array<IT>;
        using values_layout = cloning_ptr<array_wrapper>;

        const inner_value_type& dummy_inner_value() const;
        //inner_const_reference dummy_inner_const_reference() const;
        const_reference dummy_const_reference() const;

        static keys_layout create_keys_layout(arrow_proxy& proxy);
        static values_layout create_values_layout(arrow_proxy& proxy);

        [[nodiscard]] arrow_proxy& get_arrow_proxy();
        [[nodiscard]] const arrow_proxy& get_arrow_proxy() const;

        arrow_proxy m_proxy;
        keys_layout m_keys_layout;
        values_layout p_values_layout;

        friend class detail::array_access;
    };

    template <class IT>
    bool operator==(const dictionary_encoded_array<IT>& lhs, const dictionary_encoded_array<IT>& rhs);

    /*******************************************
     * dictionary_encoded_array implementation *
     *******************************************/

    template <std::integral IT>
    dictionary_encoded_array<IT>::dictionary_encoded_array(arrow_proxy proxy)
        : m_proxy(std::move(proxy))
        , m_keys_layout(create_keys_layout(m_proxy))
        , p_values_layout(create_values_layout(m_proxy))
    {
        SPARROW_ASSERT_TRUE(data_type_is_integer(m_proxy.data_type()));
    }

    template <std::integral IT>
    dictionary_encoded_array<IT>::dictionary_encoded_array(const self_type& rhs)
        : m_proxy(rhs.m_proxy)
        , m_keys_layout(create_keys_layout(m_proxy))
        , p_values_layout(create_values_layout(m_proxy))
    {
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::operator=(const self_type& rhs) -> self_type&
    {
        if (this != &rhs)
        {
            m_proxy = rhs.m_proxy;
            m_keys_layout = create_keys_layout(m_proxy);
            p_values_layout = create_values_layout(m_proxy);
        }
        return *this;
    }

    template <std::integral IT>
    dictionary_encoded_array<IT>::dictionary_encoded_array(self_type&& rhs)
        : m_proxy(std::move(rhs.m_proxy))
        , m_keys_layout(create_keys_layout(m_proxy))
        , p_values_layout(create_values_layout(m_proxy))
    {
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::operator=(self_type&& rhs) -> self_type&
    {
        if (this != &rhs)
        {
            using std::swap;
            swap(m_proxy, rhs.m_proxy);
            m_keys_layout = create_keys_layout(m_proxy);
            p_values_layout = create_values_layout(m_proxy);
        }
        return *this;
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::size() const -> size_type
    {
        return m_proxy.length();
    }
    
    template <std::integral IT>
    auto dictionary_encoded_array<IT>::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        const auto index = m_keys_layout[i];
        if (index.has_value())
        {
            return array_element(*p_values_layout, index.value());
        }
        else
        {
            return dummy_const_reference();
        }
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::begin() -> iterator
    {
        return iterator(functor_type(this), 0u);
    }
    
    template <std::integral IT>
    auto dictionary_encoded_array<IT>::end() -> iterator
    {
        return iterator(functor_type(this), size());
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::begin() const -> const_iterator
    {
        return cbegin();
    }
    
    template <std::integral IT>
    auto dictionary_encoded_array<IT>::end() const -> const_iterator
    {
        return cend();
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::cbegin() const -> const_iterator
    {
        return const_iterator(const_functor_type(this), 0u);
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::cend() const -> const_iterator
    {
        return const_iterator(const_functor_type(this), size());
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::dummy_inner_value() const -> const inner_value_type&
    {
        static const inner_value_type instance = array_default_element_value(*p_values_layout);
        return instance;
    }

    /*template <std::integral IT>
    auto dictionary_encoded_array<IT>::dummy_inner_const_reference() const -> inner_const_reference
    {
        static const inner_const_reference instance = 
            std::visit([](const auto& val) -> inner_const_reference { return val; }, dummy_inner_value());
        return instance;
    }*/

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::dummy_const_reference() const -> const_reference
    {
        static const const_reference instance = 
            std::visit([](const auto& val) -> const_reference {
                using inner_ref = typename arrow_traits<std::decay_t<decltype(val)>>::const_reference;
                return nullable<inner_ref>(inner_ref(val), false);
            },
            dummy_inner_value());
        return instance;
    }

    template <std::integral IT>
    typename dictionary_encoded_array<IT>::values_layout
    dictionary_encoded_array<IT>::create_values_layout(arrow_proxy& proxy)
    {
        const auto& dictionary = proxy.dictionary();
        SPARROW_ASSERT_TRUE(dictionary);
        arrow_proxy ar_dictionary{&(dictionary->array()), &(dictionary->schema())};
        return array_factory(std::move(ar_dictionary));
    }
    
    template <std::integral IT>
    auto dictionary_encoded_array<IT>::create_keys_layout(arrow_proxy& proxy) -> keys_layout
    {
        return keys_layout{arrow_proxy{&proxy.array(), &proxy.schema()}};
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::get_arrow_proxy() -> arrow_proxy&
    {
        return m_proxy;
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::get_arrow_proxy() const -> const arrow_proxy&
    {
        return m_proxy;
    }

    template <class IT>
    bool operator==(const dictionary_encoded_array<IT>& lhs, const dictionary_encoded_array<IT>& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }
}
