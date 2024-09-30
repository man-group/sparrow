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
#include "sparrow_v01/layout/list_layout/list_value.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/array/data_traits.hpp"
#include "sparrow_v01/array_factory.hpp"
#include "sparrow_v01/utils/memory.hpp"
#include "sparrow_v01/utils/functor_index_iterator.hpp"

namespace sparrow
{
    template <bool BIG>
    class list_array_impl;

    using list_array = list_array_impl<false>;
    using big_list_array = list_array_impl<true>;

    namespace detail{
        template<bool BIG, bool CONST>
        class ListArrayValueIteratorFunctor
        {
            // the value type of a list
            using value_type = list_value2;

            public:
            using list_array_ptr = std::conditional_t<CONST, const ::sparrow::list_array_impl<BIG>*, ::sparrow::list_array_impl<BIG>*>;

            constexpr ListArrayValueIteratorFunctor(list_array_ptr list_array)
            : p_list_array(list_array)
            {
            }
            value_type operator()(std::size_t i) const
            {
                return p_list_array->value(i);
            }
            private:
            list_array_ptr p_list_array;
        };
    }

    template <bool BIG>
    struct array_inner_types<list_array_impl<BIG>> : array_inner_types_base
    {
        using inner_value_type = list_value2;
        using inner_reference  = list_value2;
        using inner_const_reference = list_value2;

        using value_iterator = functor_index_iterator<detail::ListArrayValueIteratorFunctor<BIG, false>>;
        using const_value_iterator = functor_index_iterator<detail::ListArrayValueIteratorFunctor<BIG, true>>;
        
        using iterator_tag = std::random_access_iterator_tag;
    };

    template <bool BIG>
    class list_array_impl final : public array_base,
                                  public array_crtp_base<list_array_impl<BIG>>
    {
        public:
        using self_type = list_array_impl<BIG>;
        using base_type = array_crtp_base<self_type>;
        using inner_types = array_inner_types<self_type>;
        using value_iterator = typename inner_types::value_iterator;
        using const_value_iterator = typename inner_types::const_value_iterator;
        using size_type = typename base_type::size_type;
        
        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_reference = typename base_type::bitmap_reference;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;

        using inner_value_type =  list_value2;
        using inner_reference =  list_value2;
        using inner_const_reference =  list_value2;


        using value_type = nullable<inner_value_type>;
        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using iterator_tag = std::contiguous_iterator_tag;


        using flat_array_offset_type = std::conditional_t<BIG, std::uint64_t, std::uint32_t>;
        using list_size_type = flat_array_offset_type;
        using flat_array_offset_pointer = flat_array_offset_type*;


        explicit list_array_impl(arrow_proxy proxy)
        :   array_base(proxy.data_type()),
            base_type(std::move(proxy)),
            p_list_offsets(reinterpret_cast<flat_array_offset_type*>(this->storage().buffers()[1].data() + this->storage().offset())),
            p_flat_array(std::move(array_factory(this->storage().children()[0].view())))
        {
        }
        
        virtual ~list_array_impl() = default;
        list_array_impl(const list_array_impl& rhs) = default;

        list_array_impl* clone_impl() const override{
            return new list_array_impl(*this);
        }


        const array_base * raw_flat_array() const{
            return p_flat_array.get();
        }
        array_base * raw_flat_array(){
            return p_flat_array.get();
        }
        
        private:

        value_iterator value_begin(){
            return value_iterator(detail::ListArrayValueIteratorFunctor<BIG, false>(this), 0);
        }
        
        value_iterator value_end(){
            return value_iterator(detail::ListArrayValueIteratorFunctor<BIG, false>(this), this->size());
        }

        const_value_iterator value_cbegin() const{
            return const_value_iterator(detail::ListArrayValueIteratorFunctor<BIG, true>(this), 0);
        }

        const_value_iterator value_cend() const{
            return const_value_iterator(detail::ListArrayValueIteratorFunctor<BIG, true>(this), this->size());
        }

        // get the raw value
        inner_reference value(size_type i){
            return  list_value2{p_flat_array.get(), p_list_offsets[i], p_list_offsets[i + 1]};
        }

        inner_const_reference value(size_type i) const{
            return list_value2{p_flat_array.get(), p_list_offsets[i], p_list_offsets[i + 1]};
        }
        
        // data members
        flat_array_offset_type * p_list_offsets;

        cloning_ptr<array_base>  p_flat_array;

        // friend classes
        friend class array_crtp_base<self_type>;

        template<bool BIG_LIST, bool CONST_ITER>
        friend class detail::ListArrayValueIteratorFunctor;
    };
}
