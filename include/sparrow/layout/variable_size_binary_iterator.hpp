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

#include "sparrow/layout/layout_iterator.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    /**
     * Iterator over the data values of a variable size binary layout.
     *
     * @tparam L the layout type
     * @tparam is_const a boolean flag specifying whether this iterator is const.
     */
    template <class Layout, iterator_types Iterator_types>
    class variable_size_binary_value_iterator
        : public iterator_base<
              variable_size_binary_value_iterator<Layout, Iterator_types>,
              typename Iterator_types::value_type,
              typename Iterator_types::iterator_tag,
              typename Iterator_types::reference>
    {
    public:

        using self_type = variable_size_binary_value_iterator<Layout, Iterator_types>;
        using base_type = iterator_base<
            self_type,
            typename Iterator_types::value_type,
            typename Iterator_types::iterator_tag,
            typename Iterator_types::reference>;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;
        using layout_type = mpl::constify_t<Layout, true>;
        using size_type = size_t;
        using value_type = base_type::value_type;

        constexpr variable_size_binary_value_iterator() noexcept = default;
        constexpr variable_size_binary_value_iterator(layout_type* layout, size_type index);

    private:

        [[nodiscard]] constexpr reference dereference() const;

        constexpr void increment() noexcept;
        constexpr void decrement() noexcept;
        constexpr void advance(difference_type n) noexcept;
        [[nodiscard]] constexpr difference_type distance_to(const self_type& rhs) const noexcept;
        [[nodiscard]] constexpr bool equal(const self_type& rhs) const noexcept;
        [[nodiscard]] constexpr bool less_than(const self_type& rhs) const noexcept;

        layout_type* p_layout = nullptr;
        difference_type m_index;

        friend class iterator_access;
    };

    /******************************************************
     * variable_size_binary_value_iterator implementation *
     ******************************************************/

    template <class Layout, iterator_types Iterator_types>
    constexpr variable_size_binary_value_iterator<Layout, Iterator_types>::variable_size_binary_value_iterator(
        layout_type* layout,
        size_type index
    )
        : p_layout(layout)
        , m_index(static_cast<difference_type>(index))
    {
    }

    template <class Layout, iterator_types Iterator_types>
    constexpr auto variable_size_binary_value_iterator<Layout, Iterator_types>::dereference() const -> reference
    {
        if constexpr (std::same_as<reference, typename Layout::inner_const_reference>)
        {
            return p_layout->value(static_cast<size_type>(m_index));
        }
        else
        {
            return reference(const_cast<Layout*>(p_layout), static_cast<size_type>(m_index));
        }
    }

    template <class Layout, iterator_types Iterator_types>
    constexpr void variable_size_binary_value_iterator<Layout, Iterator_types>::increment() noexcept
    {
        ++m_index;
    }

    template <class Layout, iterator_types Iterator_types>
    constexpr void variable_size_binary_value_iterator<Layout, Iterator_types>::decrement() noexcept
    {
        --m_index;
    }

    template <class Layout, iterator_types Iterator_types>
    constexpr void
    variable_size_binary_value_iterator<Layout, Iterator_types>::advance(difference_type n) noexcept
    {
        m_index += n;
    }

    template <class Layout, iterator_types Iterator_types>
    constexpr auto
    variable_size_binary_value_iterator<Layout, Iterator_types>::distance_to(const self_type& rhs) const noexcept
        -> difference_type
    {
        return rhs.m_index - m_index;
    }

    template <class Layout, iterator_types Iterator_types>
    constexpr bool
    variable_size_binary_value_iterator<Layout, Iterator_types>::equal(const self_type& rhs) const noexcept
    {
        return (p_layout == rhs.p_layout) && (m_index == rhs.m_index);
    }

    template <class Layout, iterator_types Iterator_types>
    constexpr bool
    variable_size_binary_value_iterator<Layout, Iterator_types>::less_than(const self_type& rhs) const noexcept
    {
        return (p_layout == rhs.p_layout) && (m_index < rhs.m_index);
    }
}
