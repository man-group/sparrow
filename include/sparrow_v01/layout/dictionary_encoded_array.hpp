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

#include <memory>

#include "sparrow/array/data_type.hpp"
#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/utils/contracts.hpp"

#include "sparrow_v01/layout/array_base.hpp"
#include "sparrow_v01/layout/dictionary_encoded_array/dictionary_encoded_array_bitmap.hpp"
#include "sparrow_v01/layout/dictionary_encoded_array/dictionary_encoded_array_iterator.hpp"
#include "sparrow_v01/layout/primitive_array.hpp"

namespace sparrow
{
    /*
     * Traits class for the iterator over the data values
     * of a dictionary encoded layout.
     *
     * @tparam L the layout type
     * @tparam IC a constnat indicating whether the inner types
     * must be defined for a constant iterator.
     */
    template <class L, bool IC>
    struct dictionary_value_traits
    {
        using layout_type = L;
        using value_type = typename L::inner_value_type;
        using tag = std::random_access_iterator_tag;
        using const_reference = typename L::inner_const_reference;
        static constexpr bool is_value = true;
        static constexpr bool is_const = IC;
    };


    template <std::integral IT, class SL, layout_offset OT = std::int64_t>
    class dictionary_encoded_array;

    template <std::integral IT, class SL, layout_offset OT>
    struct array_inner_types<dictionary_encoded_array<IT, SL, OT>>
    {
        using array_type = dictionary_encoded_array<IT, SL, OT>;

        using keys_layout = primitive_array<IT>;
        using values_layout = SL;

        using inner_value_type = SL::inner_value_type;
        using inner_reference = SL::inner_reference;
        using inner_const_reference = SL::inner_const_reference;

        using value_iterator = dictionary_iterator<dictionary_value_traits<array_inner_types<array_type>, false>>;
        using const_value_iterator = dictionary_iterator<dictionary_value_traits<array_inner_types<array_type>, true>>;

        using iterator_tag = std::random_access_iterator_tag;

        using iterator = layout_iterator<array_type, false>;
        using const_iterator = layout_iterator<array_type, true>;

        using bitmap_type = dictionary_bitmap<keys_layout, typename values_layout::const_bitmap_range>;
    };

    template <std::integral IT, class SL, layout_offset OT>
    class dictionary_encoded_array final : public array_base,
                                           public array_crtp_base<dictionary_encoded_array<IT, SL, OT>>
    {
    public:

        using self_type = dictionary_encoded_array<IT, SL, OT>;
        using index_type = IT;
        using values_layout = SL;
        using base_type = array_crtp_base<self_type>;
        using inner_types = array_inner_types<self_type>;
        using inner_value_type = typename inner_types::inner_value_type;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;
        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;
        using value_type = SL::value_type;
        using reference = SL::reference;
        using const_reference = SL::const_reference;
        using size_type = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using keys_layout = typename inner_types::keys_layout;
        using iterator_tag =  typename base_type::iterator_tag;

        using value_iterator = typename base_type::value_iterator;
        using const_value_iterator = typename base_type::const_value_iterator;

        using const_bitmap_range = typename base_type::const_bitmap_range;

        explicit dictionary_encoded_array(arrow_proxy);
        ~dictionary_encoded_array() override = default;

        using base_type::size;

        reference operator[](size_type i);
        const_reference operator[](size_type i) const;

    private:

        using base_type::bitmap_begin;
        using base_type::bitmap_end;
        using base_type::has_value;
        using base_type::storage;

        // inner_reference value(size_type i);
        inner_const_reference value(size_type i) const;

        // value_iterator value_begin();
        // value_iterator value_end();

        const_value_iterator value_cbegin() const;
        const_value_iterator value_cend() const;

        dictionary_encoded_array(const dictionary_encoded_array&) = default;
        dictionary_encoded_array* clone_impl() const override;

        keys_layout m_keys_layout;
        values_layout m_values_layout;
        bitmap_type m_bitmap;

        bitmap_type& get_bitmap();
        const bitmap_type& get_bitmap() const;

        static const const_reference& dummy_const_reference();
        static keys_layout create_keys_layout(arrow_proxy& proxy);
        static values_layout create_values_layout(arrow_proxy& proxy);
        bitmap_type make_bitmap(keys_layout& keys, values_layout& values);

        friend class array_crtp_base<self_type>;
        friend class dictionary_iterator<dictionary_value_traits<inner_types, true>>;
        friend class dictionary_iterator<dictionary_value_traits<inner_types, false>>;
    };

    /*******************************************
     * dictionary_encoded_array implementation *
     *******************************************/

    template <std::integral IT, class SL, layout_offset OT>
    dictionary_encoded_array<IT, SL, OT>::dictionary_encoded_array(arrow_proxy proxy)
        : array_base(proxy.data_type())
        , base_type(std::move(proxy))
        , m_keys_layout(create_keys_layout(storage()))
        , m_values_layout(create_values_layout(storage()))
        , m_bitmap(make_bitmap(m_keys_layout, m_values_layout))
    {
        SPARROW_ASSERT_TRUE(data_type_is_integer(storage().data_type()));
    }

    // template <std::integral IT, class SL, layout_offset OT>
    // auto dictionary_encoded_array<IT, SL, OT>::operator[](size_type i) -> reference
    // {
    //     SPARROW_ASSERT_TRUE(i < size());
    //     return reference(value(i), has_value(i));
    // }

    template <std::integral IT, class SL, layout_offset OT>
    auto dictionary_encoded_array<IT, SL, OT>::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        const auto index = m_keys_layout[i];
        if (index.has_value())
        {
            return m_values_layout[index.value()];
        }
        else
        {
            return dummy_const_reference();
        }
    }

    template <std::integral IT, class SL, layout_offset OT>
    auto dictionary_encoded_array<IT, SL, OT>::value(size_type i) const -> inner_const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        const auto index = m_keys_layout[i];
        if (index.has_value())
        {
            auto ref = m_values_layout[index.value()];
            if (ref.has_value())
            {
                return ref.value();
            }
        }

        return inner_const_reference();
    }

    // template <std::integral IT, class SL, layout_offset OT>
    // auto dictionary_encoded_array<IT, SL, OT>::value_begin() -> value_iterator
    // {
    //     return value_iterator{m_keys_layout.cbegin(), m_values_layout};
    // }

    // template <std::integral IT, class SL, layout_offset OT>
    // auto dictionary_encoded_array<IT, SL, OT>::value_end() -> value_iterator
    // {
    //     return sparrow::next(value_begin(), size());
    // }

    template <std::integral IT, class SL, layout_offset OT>
    auto dictionary_encoded_array<IT, SL, OT>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator{m_keys_layout.cbegin(), m_values_layout};
    }

    template <std::integral IT, class SL, layout_offset OT>
    auto dictionary_encoded_array<IT, SL, OT>::value_cend() const -> const_value_iterator
    {
        return sparrow::next(value_cbegin(), size());
    }

    template <std::integral IT, class SL, layout_offset OT>
    dictionary_encoded_array<IT, SL, OT>* dictionary_encoded_array<IT, SL, OT>::clone_impl() const
    {
        arrow_proxy copy = storage();
        return new dictionary_encoded_array<IT, SL, OT>(std::move(copy));
    }

    template <std::integral T, class SL, layout_offset OT>
    typename dictionary_encoded_array<T, SL, OT>::values_layout
    dictionary_encoded_array<T, SL, OT>::create_values_layout(arrow_proxy& proxy)
    {
        const auto& dictionary = proxy.dictionary();
        SPARROW_ASSERT_TRUE(dictionary);
        arrow_proxy ar_dictionary{&(dictionary->array()), &(dictionary->schema())};
        return values_layout{std::move(ar_dictionary)};
    }

    template <std::integral IT, class SL, layout_offset OT>
    auto dictionary_encoded_array<IT, SL, OT>::get_bitmap() -> bitmap_type&
    {
        return m_bitmap;
    }

    template <std::integral IT, class SL, layout_offset OT>
    auto dictionary_encoded_array<IT, SL, OT>::get_bitmap() const -> const bitmap_type&
    {
        return m_bitmap;
    }

    template <std::integral IT, class SL, layout_offset OT>
    auto dictionary_encoded_array<IT, SL, OT>::dummy_const_reference() -> const const_reference&
    {
        static const typename values_layout::inner_value_type dummy_inner_value;
        static const const_reference instance(dummy_inner_value, false);
        return instance;
    }

    template <std::integral IT, class SL, layout_offset OT>
    auto dictionary_encoded_array<IT, SL, OT>::create_keys_layout(arrow_proxy& proxy) -> keys_layout
    {
        return keys_layout{arrow_proxy{&proxy.array(), &proxy.schema()}};
    }

    template <std::integral IT, class SL, layout_offset OT>
    auto
    dictionary_encoded_array<IT, SL, OT>::make_bitmap(keys_layout& keys, values_layout& values) -> bitmap_type
    {
        return bitmap_type{keys, values.bitmap()};
    }


}