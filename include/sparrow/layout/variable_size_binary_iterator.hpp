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
        : public pointer_index_iterator_base<
              variable_size_binary_value_iterator<Layout, Iterator_types>,
              mpl::constify_t<Layout, true>,
              typename Iterator_types::value_type,
              typename Iterator_types::reference,
              typename Iterator_types::iterator_tag,
              std::ptrdiff_t>
    {
    public:

        using self_type = variable_size_binary_value_iterator<Layout, Iterator_types>;
        using base_type = pointer_index_iterator_base<
            self_type,
            mpl::constify_t<Layout, true>,
            typename Iterator_types::value_type,
            typename Iterator_types::reference,
            typename Iterator_types::iterator_tag,
            std::ptrdiff_t>;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;
        using layout_type = mpl::constify_t<Layout, true>;
        using size_type = std::size_t;
        using value_type = typename Iterator_types::value_type;

        constexpr variable_size_binary_value_iterator() noexcept = default;
        constexpr variable_size_binary_value_iterator(layout_type* layout, size_type index);

    private:

        [[nodiscard]] constexpr reference dereference() const;

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
        : base_type(layout, static_cast<std::ptrdiff_t>(index))
    {
    }

    template <class Layout, iterator_types Iterator_types>
    constexpr auto variable_size_binary_value_iterator<Layout, Iterator_types>::dereference() const -> reference
    {
        if constexpr (std::same_as<reference, typename Layout::inner_const_reference>)
        {
            return this->p_object->value(static_cast<size_type>(this->m_index));
        }
        else
        {
            return reference(const_cast<Layout*>(this->p_object), static_cast<size_type>(this->m_index));
        }
    }
}
