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
#include "sparrow/layout/list_layout/list_layout_value_iterator.hpp"
#include "sparrow/utils/algorithm.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{

    template <class CHILD_LAYOUT, data_storage DS = array_data, layout_offset OT = std::int64_t>
    class list_layout
    {
    public:

        using self_type = list_layout<CHILD_LAYOUT, DS, OT>;
        using child_layout_type = CHILD_LAYOUT;

        using child_layout_reference = typename child_layout_type::reference;
        using child_layout_const_reference = typename child_layout_type::const_reference;

        using data_storage_type = DS;
        using offset_type = OT;
        using inner_value_type = list_value_t<child_layout_type, true>;
        using inner_reference = list_value_t<child_layout_type, false>;
        using inner_const_reference = list_value_t<child_layout_type, true>;

        using bitmap_type = typename data_storage_type::bitmap_type;
        using bitmap_reference = typename bitmap_type::reference;
        using bitmap_const_reference = typename bitmap_type::const_reference;
        using value_type = nullable<inner_value_type>;
        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using iterator_tag = std::random_access_iterator_tag;



        using const_value_iterator = list_layout_value_iterator<self_type, child_layout_type,offset_type, true>;
        using const_bitmap_iterator = data_storage_type::bitmap_type::const_iterator;

        using value_iterator = list_layout_value_iterator<self_type, child_layout_type,offset_type, false>;
        using bitmap_iterator = data_storage_type::bitmap_type::iterator;

        using iterator = layout_iterator<self_type, false>;
        using const_iterator = layout_iterator<self_type, true>;

        using const_value_range = std::ranges::subrange<const_value_iterator>;
        using const_bitmap_range = std::ranges::subrange<const_bitmap_iterator>;

        explicit list_layout(data_storage_type& data)
        : m_data(data),
          m_child_layout(data.child_data[0])
        {
        }

        void rebind_data(data_storage_type& data)
        {
            m_data = data;  
            m_child_layout = data.child_data[0];
        }

        list_layout(const self_type&) = delete;
        self_type& operator=(const self_type&) = delete;
        list_layout(self_type&&) = delete;
        self_type& operator=(self_type&&) = delete;

        size_type size() const
        {
            const auto  _length = sparrow::length(storage());
            const auto  _offset = sparrow::offset(storage());
            return static_cast<size_type>(_length - _offset);
        }

        reference operator[](size_type i)
        {
            auto child_index = sparrow::offset(m_data.get()) + i;
        }
        const_reference operator[](size_type i) const;

        iterator begin(){
            return iterator(value_begin(), bitmap_begin());
        }
        iterator end() {
            return iterator(value_end(), bitmap_end());
        }

        const_iterator cbegin() const{
            return const_iterator(value_cbegin(), bitmap_cbegin());
        }
        const_iterator cend() const{
            return const_iterator(value_cend(), bitmap_cend());
        }

        const_value_range values() const{
            return const_value_range(value_cbegin(), value_cend());
        }
        const_bitmap_range bitmap() const{
            return const_bitmap_range(bitmap_cbegin(), bitmap_cend());
        }

    private:
     

        value_iterator value_begin()
        {
            return value_iterator(this, offset(storage()));
        }
        value_iterator value_end()
        {
            // with an offset we get a smaller array in total
            return value_iterator(this, sparrow::length(storage()));
        }

        const_value_iterator value_cbegin() const
        {
            return const_value_iterator(this, offset(storage()));
        }
        const_value_iterator value_cend() const
        {
            return const_value_iterator(this, sparrow::length(storage()));
        }

        bitmap_iterator bitmap_begin(){
            return sparrow::bitmap(storage()).begin() + sparrow::offset(storage());
        }

        bitmap_iterator bitmap_end(){
            return sparrow::bitmap(storage()).end();
        }

        const_bitmap_iterator bitmap_cbegin() const{
            return sparrow::bitmap(storage()).cbegin() + sparrow::offset(storage());
        }
        const_bitmap_iterator bitmap_cend() const {
            return sparrow::bitmap(storage()).cend();
        }

    
        offset_type element_offset(size_type i)const
        {
            const auto j = sparrow::offset(storage()) + i;
            return buffer_at(storage(), 0u).template data<const offset_type>()[j]; 
        }
            
        offset_type element_length(size_type i)const
        {
            
            // delta between the offsets of the current and the next element
            const auto j = sparrow::offset(storage()) + i;
            const auto offset_ptr = buffer_at(storage(), 0u).template data<const offset_type>();
            const auto size = offset_ptr[j + 1] - offset_ptr[j];
            return size;
        }
        data_storage_type& storage()
        {
            return m_data;
        }
        const data_storage_type& storage() const
        {
            return m_data;
        }

        std::reference_wrapper<data_storage_type> m_data;
        child_layout_type m_child_layout;

        friend class list_layout_value_iterator<self_type, child_layout_type, offset_type, true>;
        friend class list_layout_value_iterator<self_type, child_layout_type, offset_type, false>;

    };
}
