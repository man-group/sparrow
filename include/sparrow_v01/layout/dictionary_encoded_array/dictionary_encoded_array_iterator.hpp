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

#include <concepts>
#include <optional>

#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    template <class T>
    concept dictionary_iterator_traits = requires {
        typename T::layout_type;
        typename T::value_type;
        typename T::tag;
        typename T::const_reference;
        { T::is_value } -> std::same_as<const bool&>;
        { T::is_const } -> std::same_as<const bool&>;
    };

    /**
     * Iterator over the values or the bitmap elements of a dictionary layout.
     *
     * @tparam Traits the traits defining the inner types of the iterator.
     */
    template <dictionary_iterator_traits Traits>
    class dictionary_iterator
        : public iterator_base<dictionary_iterator<Traits>, typename Traits::value_type, typename Traits::tag, typename Traits::const_reference>

    {
    public:

        using self_type = dictionary_iterator<Traits>;
        using base_type = iterator_base<
            dictionary_iterator<Traits>,
            typename Traits::value_type,
            typename Traits::tag,
            typename Traits::const_reference>;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;

        using layout_type = typename Traits::layout_type;
        using keys_layout = typename layout_type::keys_layout;
        using key_iterator = std::
            conditional_t<Traits::is_const, typename keys_layout::const_iterator, typename keys_layout::iterator>;
        using values_layout = typename layout_type::values_layout;
        using values_layout_storage = mpl::constify_t<values_layout, Traits::is_const>;
        using values_layout_reference = values_layout_storage&;

        // `dictionary_iterator` needs to be default constructible
        // to satisfy `dictionary_encoded_layout::const_value_range`'s
        // and `dicitonary_encoded_layout::const_bitmap_range`'s
        // constraints.
        dictionary_iterator() noexcept = default;
        dictionary_iterator(key_iterator index_it, values_layout_reference values_layout_reference);

    private:

        using sub_reference = std::
            conditional_t<Traits::is_const, typename values_layout::const_reference, typename values_layout::reference>;

        sub_reference get_subreference() const;

        reference dereference() const;
        void increment();
        void decrement();
        void advance(difference_type n);
        difference_type distance_to(const self_type& rhs) const;
        bool equal(const self_type& rhs) const;
        bool less_than(const self_type& rhs) const;

        key_iterator m_index_it;
        // Use std::optional because of default constructor.
        std::optional<std::reference_wrapper<values_layout_storage>> m_values_layout_reference;

        friend class iterator_access;
    };

    /**************************************
     * dictionary_iterator implementation *
     **************************************/

    template <dictionary_iterator_traits Traits>
    dictionary_iterator<Traits>::dictionary_iterator(key_iterator index_it, values_layout_reference values_layout_ref)
        : m_index_it(index_it)
        , m_values_layout_reference(values_layout_ref)
    {
    }

    template <dictionary_iterator_traits Traits>
    auto dictionary_iterator<Traits>::get_subreference() const -> sub_reference
    {
        return (*m_values_layout_reference).get()[m_index_it->value()];
    }

    template <dictionary_iterator_traits Traits>
    auto dictionary_iterator<Traits>::dereference() const -> reference
    {
        SPARROW_ASSERT_TRUE(m_values_layout_reference.has_value());
        if constexpr (Traits::is_value)
        {
            if (m_index_it->has_value() && get_subreference().has_value())
            {
                return get_subreference().get();
            }

            return layout_type::array_type::dummy_const_reference().get();
        }
        else
        {
            return m_index_it->has_value() && get_subreference().has_value();
        }
    }

    template <dictionary_iterator_traits Traits>
    void dictionary_iterator<Traits>::increment()
    {
        ++m_index_it;
    }

    template <dictionary_iterator_traits Traits>
    void dictionary_iterator<Traits>::decrement()
    {
        --m_index_it;
    }

    template <dictionary_iterator_traits Traits>
    void dictionary_iterator<Traits>::advance(difference_type n)
    {
        m_index_it += n;
    }

    template <dictionary_iterator_traits Traits>
    auto dictionary_iterator<Traits>::distance_to(const self_type& rhs) const -> difference_type
    {
        m_index_it.distance_to(rhs.m_index_it);
    }

    template <dictionary_iterator_traits Traits>
    bool dictionary_iterator<Traits>::equal(const self_type& rhs) const
    {
        return m_index_it == rhs.m_index_it;
    }

    template <dictionary_iterator_traits Traits>
    bool dictionary_iterator<Traits>::less_than(const self_type& rhs) const
    {
        return m_index_it < rhs.m_index_it;
    }

}
