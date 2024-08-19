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
#include "sparrow/utils/algorithm.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{


    template <class CHILD_LAYOUT, class OFFSET_TYPE, bool IS_CONST>
    class list_proxy
    {
    public:
        using child_layout_type = CHILD_LAYOUT;
        using self_type = list_proxy<CHILD_LAYOUT, OFFSET_TYPE, IS_CONST>;
        using offset_type = OFFSET_TYPE;
    
        using value_type = typename child_layout_type::value_type;
        using reference = std::conditional_t<IS_CONST, typename child_layout_type::const_reference, typename child_layout_type::reference>;
        using const_reference = typename child_layout_type::const_reference;

        using iterator = std::conditional_t<IS_CONST, typename child_layout_type::const_iterator, typename child_layout_type::iterator>;
        using const_iterator = typename child_layout_type::const_iterator;


        using size_type = offset_type;
    

        list_proxy(child_layout_type* layout, size_type index, size_type size)
            : p_child_layout(layout), m_child_offset(index), m_size(size)
        {
    
        }
        list_proxy(const list_proxy&) = default;
        list_proxy(list_proxy&&) = default;

        // template <std::ranges::range T>
        // self_type& operator=(T&& rhs)
        // {
        //     if(rhs.size() != m_size)
        //     {
        //         throw std::invalid_argument("cannot assign rhs to range: size mismatch");
        //     }
        //     std::ranges::copy(rhs, this.begin());
        // }

        size_type size() const
        {
            return this->m_size;
        }
        iterator begin(){
            return this->p_child_layout->begin() + this->m_child_offset;
        }
        iterator end(){
            return this->p_child_layout->begin() + this->m_child_offset + this->m_size;
        }
        const_iterator begin() const{
            return this->p_child_layout->begin() + this->m_child_offset;
        }
        const_iterator end() const{
            return this->p_child_layout->begin() + this->m_child_offset + this->m_size;
        }
        const_iterator cbegin() const{
            return this->p_child_layout->begin() + this->m_child_offset;
        }
        const_iterator cend() const{
            return this->p_child_layout->begin() + this->m_child_offset + this->m_size;
        }

        reference operator[](size_type i){
            return this->p_child_layout->operator[](this->m_child_offset + i);
        }
        const_reference operator[](size_type i) const{
            return this->p_child_layout->operator[](this->m_child_offset + i);
        }

    private:

        child_layout_type* p_child_layout = nullptr;

        size_type m_child_offset = size_type(0);
        size_type m_size = size_type(0);
    };


    template<class LIST_LAYOUT_TYPE, class CHILD_LAYOUT_TYPE, class OFFSET_TYPE, bool IS_CONST>
    class list_layout_value_iterator: public iterator_base<
            // derived
            list_layout_value_iterator<LIST_LAYOUT_TYPE, CHILD_LAYOUT_TYPE, OFFSET_TYPE , IS_CONST>, 
            // element
            list_proxy<CHILD_LAYOUT_TYPE, OFFSET_TYPE, IS_CONST>,
            // category
            std::random_access_iterator_tag,
            // REFERENCE
            list_proxy<CHILD_LAYOUT_TYPE, OFFSET_TYPE, IS_CONST> 
        >
    {
        public:
            using self_type = list_layout_value_iterator<LIST_LAYOUT_TYPE, CHILD_LAYOUT_TYPE, OFFSET_TYPE, IS_CONST>;
            using list_proxy_type = list_proxy<CHILD_LAYOUT_TYPE, OFFSET_TYPE, IS_CONST>;
            using size_type = typename LIST_LAYOUT_TYPE::size_type;

            list_layout_value_iterator(
                LIST_LAYOUT_TYPE* layout, size_type index)
                : p_layout(layout), m_index(index)
            {
            }

            list_layout_value_iterator() = default;
            list_layout_value_iterator(const self_type& rhs) = default;
            list_layout_value_iterator(self_type&& rhs) = default;
            self_type& operator=(const self_type& rhs) = default;


            list_proxy_type dereference() const
            {
                return list_proxy_type(&(p_layout->m_child_layout), p_layout->element_offset(m_index),  p_layout->element_length(m_index));
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
                m_index += n;
            }
            std::ptrdiff_t distance_to(const self_type& rhs) const
            {
                return rhs.m_index - m_index;
            }
            bool less_than(const self_type& rhs) const
            {
                return m_index < rhs.m_index;
            }
            


        private:
            using list_layout_type = LIST_LAYOUT_TYPE;

            list_layout_type* p_layout = nullptr;
            size_type m_index = size_type(0);

    };


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
        using inner_value_type = list_proxy<child_layout_type, offset_type, true>;
        using inner_reference = list_proxy<child_layout_type, offset_type, false>;
        using inner_const_reference = list_proxy<child_layout_type, offset_type, true>;

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
