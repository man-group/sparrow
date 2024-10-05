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
#include "sparrow/array/data_traits.hpp"
#include "sparrow/layout/array_base.hpp"
#include "sparrow/layout/layout_iterator.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/utils/memory.hpp"

namespace sparrow
{
    template <bool BIG>
    class list_array_impl;

    using list_array = list_array_impl<false>;
    using big_list_array = list_array_impl<true>;

    template <bool BIG>
    struct array_inner_types<list_array_impl<BIG>> : array_inner_types_base
    {
        using array_type = list_array_impl<BIG>;
        using inner_value_type = list_value2;
        using inner_reference  = list_value2;
        using inner_const_reference = list_value2;
        using value_iterator = functor_index_iterator<detail::layout_value_functor<array_type, inner_value_type>>;
        using const_value_iterator = functor_index_iterator<detail::layout_value_functor<const array_type, inner_value_type>>;
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

        explicit list_array_impl(arrow_proxy proxy);
        virtual ~list_array_impl() = default;
        list_array_impl(const list_array_impl& rhs) = default;
        list_array_impl* clone_impl() const override;
        const array_base * raw_flat_array() const;
        array_base * raw_flat_array();

        
    private:
        constexpr static std::size_t OFFSET_BUFFER_INDEX = 1;
        value_iterator value_begin();
        value_iterator value_end();
        const_value_iterator value_cbegin() const;
        const_value_iterator value_cend() const;

        inner_reference value(size_type i);
        inner_const_reference value(size_type i) const;
        
        // data members
        flat_array_offset_type * p_list_offsets;
        cloning_ptr<array_base>  p_flat_array;

        // friend classes
        friend class array_crtp_base<self_type>;

        // needs access to this->value(i)
        friend class detail::layout_value_functor<self_type, inner_value_type>;
        friend class detail::layout_value_functor<const self_type, inner_value_type>;
    };


    template <bool BIG>
    list_array_impl<BIG>::list_array_impl(arrow_proxy proxy)
    :   array_base(proxy.data_type()),
        base_type(std::move(proxy)),
        p_list_offsets(reinterpret_cast<flat_array_offset_type*>(this->storage().buffers()[OFFSET_BUFFER_INDEX].data() + this->storage().offset())),
        p_flat_array(std::move(array_factory(this->storage().children()[0].view())))
    {
    }
    
    template <bool BIG>
    auto list_array_impl<BIG>::clone_impl() const -> list_array_impl* 
    {
        return new list_array_impl(*this);
    }

    template <bool BIG>
    auto list_array_impl<BIG>::raw_flat_array() const -> const array_base * 
    {
        return p_flat_array.get();
    }
    template <bool BIG>
    auto list_array_impl<BIG>::raw_flat_array()-> array_base * 
    {
        return p_flat_array.get();
    }
    
    template <bool BIG>
    auto list_array_impl<BIG>::value_begin() -> value_iterator 
    {
        return value_iterator(detail::layout_value_functor<self_type, inner_value_type>(this), 0);
    }
    
    template <bool BIG>
    auto list_array_impl<BIG>::value_end() -> value_iterator
    {
        return value_iterator(detail::layout_value_functor<self_type, inner_value_type>(this), this->size());
    }

    template <bool BIG>
    auto list_array_impl<BIG>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator(detail::layout_value_functor<const self_type, inner_value_type>(this), 0);
    }

    template <bool BIG>
    auto list_array_impl<BIG>::value_cend() const -> const_value_iterator
    {
        return const_value_iterator(detail::layout_value_functor<const self_type, inner_value_type>(this), this->size());
    }

    template <bool BIG>
    auto list_array_impl<BIG>::value(size_type i) -> inner_reference
    {
        return  list_value2{p_flat_array.get(), p_list_offsets[i], p_list_offsets[i + 1]};
    }

    template <bool BIG>
    auto list_array_impl<BIG>::value(size_type i) const -> inner_const_reference
    {
        return list_value2{p_flat_array.get(), p_list_offsets[i], p_list_offsets[i + 1]};
    }
        
}
