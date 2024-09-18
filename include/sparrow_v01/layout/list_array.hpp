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

#include "sparrow_v01/layout/array_base.hpp"
#include "sparrow/layout/layout_iterator.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow_v01/array_factory.hpp"

namespace sparrow
{

    namespace detail{

    template<class FUNCTOR>
    class functor_index_iterator : public iterator_base<
        functor_index_iterator<FUNCTOR>,   // Derived
        std::invoke_result_t<FUNCTOR, std::size_t>,  // Element
        std::random_access_iterator_tag,
        std::invoke_result_t<FUNCTOR, std::size_t> & // Reference
    >
    {
      public:
        friend class iterator_access;
        
        using result_type = std::invoke_result_t<FUNCTOR, std::size_t>;
        using self_type = functor_index_iterator<FUNCTOR>;
        using difference_type = std::ptrdiff_t;


        // copy constructor
        functor_index_iterator(const self_type& other) = default;

        // copy assignment
        self_type& operator=(const self_type& other) = default;

        // move constructor
        functor_index_iterator(self_type&& other) = default;

        // move assignment
        self_type& operator=(self_type&& other) = default;


        functor_index_iterator(FUNCTOR functor, std::size_t index)
            : m_functor(functor)
            , m_index(index)
        {
        }
      private:

        const result_type &  dereference()
        {
            m_value =  m_functor(m_index);
            return m_value;
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
        void advance(difference_type n)
        {
            m_index += n;
        }
        difference_type distance_to(const self_type& rhs) const
        {
            return rhs.m_index - m_index;
        }

        bool less_than(const self_type& rhs) const
        {
            return m_index < rhs.m_index;
        }
        FUNCTOR m_functor;
        std::size_t m_index;

        private:
        result_type m_value;
    };

    }










    template <bool BIG>
    class list_array_impl;

    using list_array = list_array_impl<false>;
    using big_list_array = list_array_impl<true>;




    struct NullableReferenceVariantPlaceholder{};
    struct NullableConstReferenceVariantPlaceholder{};
    struct ReferenceVariantPlaceholder{};
    struct ConstReferenceVariantPlaceholder{};
    

    template <bool CONST>
    class ListValue
    {
        private:
        using array_base_ptr_type = std::conditional_t<CONST, const array_base*, array_base*>;
        using value_type = std::conditional_t<CONST, const NullableConstReferenceVariantPlaceholder, NullableReferenceVariantPlaceholder>;
        
        
        public:

        ListValue(array_base_ptr_type flat_array, std::int64_t flat_begin, std::int64_t flat_end)
        :   p_flat_array(flat_array),
            m_flat_begin(flat_begin),
            m_flat_end(flat_end)
            {
            }
        
        std::int64_t size() const
        {
            return m_flat_end - m_flat_begin;
        }

        value_type operator[](std::int64_t i) const
        {
            return value_type{};
            // ** CANNOT DO THIS BECAUSE OF LACK OF APIs ** //
            //return p_array->get_value(m_flat_begin + i);
        }

        private:
        array_base_ptr_type p_flat_array;
        std::int64_t m_flat_begin;
        std::int64_t m_flat_end;
    };


    

    template<bool BIG, bool CONST>
    class ListArrayValueIteratorFunctor
    {
        public:
        using list_array_ptr = std::conditional_t<CONST, const list_array_impl<BIG>*, list_array_impl<BIG>*>;


        ListArrayValueIteratorFunctor(list_array_ptr list_array)
        : p_list_array(list_array)
        {
        }

        NullableReferenceVariantPlaceholder operator()(std::size_t i) const
        {
            return p_list_array->value(i);
        }

        private:
        list_array_ptr p_list_array;
    };

    template <bool BIG>
    struct array_inner_types<list_array_impl<BIG>> : array_inner_types_base
    {
        using inner_value_type = ListValue</*const*/ true>;
        using inner_reference  = ListValue</*const*/ false>;
        using inner_const_reference = ListValue</*const*/ true>;

        using value_iterator = detail::functor_index_iterator<ListArrayValueIteratorFunctor<BIG, false>>;
        using const_value_iterator = detail::functor_index_iterator<ListArrayValueIteratorFunctor<BIG, true>>;
        
        using iterator_tag = std::random_access_iterator_tag;
    };



    template <bool BIG>
    class list_array_impl : public array_base,
                            public array_crtp_base<list_array_impl<BIG>>
    {
        public:
        using self_type = list_array_impl<BIG>;
        using base_type = array_crtp_base<self_type>;
        using inner_types = array_inner_types<self_type>;
        using size_type = typename base_type::size_type;
        
        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_reference = typename base_type::bitmap_reference;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;

        using inner_value_type = typename inner_types::inner_value_type;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;


        using value_type = nullable<inner_value_type>;
        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using iterator_tag = std::contiguous_iterator_tag;


        using flat_array_offset_type = std::conditional_t<BIG, std::uint64_t, std::uint32_t>;
        using list_size_type = flat_array_offset_type;
        using flat_array_offset_pointer = flat_array_offset_type*;



        list_array_impl(arrow_proxy proxy)
        :   m_proxy(proxy),
            p_list_offsets(reinterpret_cast<flat_array_offset_type*>(proxy.buffers()[1].data() + proxy.offset())),
            p_flat_array(std::move(array_factory(proxy.children()[0])))
        {
        }
        
        private:


        // get the raw value
        inner_reference value(size_type i){
            return  ListValue<false>{p_flat_array.get(), offset_begin(i), offset_end(i)};
        }

        inner_const_reference value(size_type i) const{
            return inner_const_reference{};
        }
        
        // helper functions
        list_size_type offset_begin(size_type i) const{
            return p_list_offsets[i];
        }
        list_size_type offset_end(size_type i) const{
            return p_list_offsets[i + 1];
        }
        list_size_type list_size(size_type i) const{
            return offset_end(i) - offset_begin(i);
        }

        // data members
        arrow_proxy m_proxy;
        flat_array_offset_type * p_list_offsets;

        std::unique_ptr<array_base>  p_flat_array;

        // friend class(es)
        friend class array_crtp_base<self_type>;

        template<bool CONST_VALUE>
        friend class ListValue;

        template<bool BIG_LIST, bool CONST_ITER>
        friend class ListArrayValueIteratorFunctor;
    };


   
}
