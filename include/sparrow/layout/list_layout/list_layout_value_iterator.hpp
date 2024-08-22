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

#include <functional>
#include <ranges>
#include <string_view>

#include "sparrow/array/array_data_concepts.hpp"
#include "sparrow/array/array_data.hpp"
#include "sparrow/layout/layout_iterator.hpp"
#include "sparrow/layout/list_layout/list_value.hpp"
#include "sparrow/utils/algorithm.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{
    template<class CHILD_LAYOUT,  bool IS_CONST>
    using list_value_t  = list_value<std::conditional_t<IS_CONST, typename CHILD_LAYOUT::const_iterator, typename CHILD_LAYOUT::iterator>, IS_CONST>;

    template<class LIST_LAYOUT_TYPE, class CHILD_LAYOUT_TYPE, class OFFSET_TYPE, bool IS_CONST>
    class list_layout_value_iterator: public iterator_base<
            // derived
            list_layout_value_iterator<LIST_LAYOUT_TYPE, CHILD_LAYOUT_TYPE, OFFSET_TYPE , IS_CONST>, 
            // element
            list_value_t<CHILD_LAYOUT_TYPE, IS_CONST>,
            // category
            std::random_access_iterator_tag,
            // REFERENCE
            list_value_t<CHILD_LAYOUT_TYPE, IS_CONST> 
        >
    {
        public:
            using self_type = list_layout_value_iterator<LIST_LAYOUT_TYPE, CHILD_LAYOUT_TYPE, OFFSET_TYPE, IS_CONST>;
            using layout_ptr_type = std::conditional_t<IS_CONST, const LIST_LAYOUT_TYPE*, LIST_LAYOUT_TYPE*>;
            using child_layout_reference = std::conditional_t<IS_CONST, const  CHILD_LAYOUT_TYPE &, CHILD_LAYOUT_TYPE &>;

            using list_value_type =list_value_t<CHILD_LAYOUT_TYPE, IS_CONST>;


            using size_type = typename LIST_LAYOUT_TYPE::size_type;

            list_layout_value_iterator(
                layout_ptr_type layout, size_type index)
                : p_layout(layout), m_index(index)
            {
            }

            list_layout_value_iterator() = default;
            list_layout_value_iterator(const self_type& rhs) = default;
            list_layout_value_iterator(self_type&& rhs) = default;
            self_type& operator=(const self_type& rhs) = default;

            list_value_type dereference() const
            {   
                child_layout_reference child_layout = p_layout->m_child_layout;
                const auto offset = p_layout->element_offset(m_index);
                const auto length = p_layout->element_length(m_index);

                
                return list_value_type(
                    child_layout.begin() + offset,
                    child_layout.begin() + offset + length);
            }

            bool equal(const self_type& rhs) const
            {
                return m_index == rhs.m_index;
            }

            void increment()
            {
                ++m_index;
            }
            void decrement()
            {
                --m_index; 
            }
            void advance(std::ptrdiff_t n)
            {
                m_index += static_cast<size_type>(n);
            }
            std::ptrdiff_t distance_to(const self_type& rhs) const
            {
                return static_cast<ptrdiff_t>(rhs.m_index) - static_cast<std::ptrdiff_t>(m_index);
            }
            bool less_than(const self_type& rhs) const
            {
                return m_index < rhs.m_index;
            }
            
        private:
            using list_layout_type = LIST_LAYOUT_TYPE;

            layout_ptr_type  p_layout = nullptr;
            size_type m_index = size_type(0);

    };
}
