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

#include "sparrow/array_factory.hpp"
#include "sparrow/layout/array_base.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/memory.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{
    class run_end_encoded_array;



    // this iteratas over the **actual** values of the run encoded array
    // Ie nullabes values, not values.
    template<bool CONST>
    class run_encoded_array_iterator : public iterator_base<
        run_encoded_array_iterator<CONST>,
        array_traits::const_reference,
        std::forward_iterator_tag,
        array_traits::const_reference
    >
    {   
        private:
        using array_ptr_type = std::conditional_t<CONST, const run_end_encoded_array *, run_end_encoded_array*>;
        public:
        run_encoded_array_iterator() = default;


        run_encoded_array_iterator(array_ptr_type array_ptr, std::uint64_t index, std::uint64_t run_end_index)
        : 
            p_array(array_ptr),
            p_encoded_values_array(array_ptr->p_encoded_values_array.get()),
            m_index(index), 
            m_run_end_index(run_end_index),
            m_runs_left(array_ptr->get_run_length(index))
        {
        }
        
        private:

        bool equal(const run_encoded_array_iterator& rhs) const
        {
            return m_index == rhs.m_index;
        }

        void increment()
        {
            std::cout<<"incrementig from "<<m_index<<" to "<<m_index+1<<" runs_left: "<<m_runs_left<<std::endl;



            ++m_index;
            --m_runs_left;
            if(m_runs_left == 0)
            {
                ++m_run_end_index;
                m_runs_left = p_array->get_run_length(m_index);
            }
        }

        array_traits::const_reference dereference() const
        {
            return array_element(*p_encoded_values_array, m_run_end_index);
        }

        array_ptr_type p_array = nullptr;
        array_wrapper * p_encoded_values_array = nullptr;
        std::uint64_t m_index = 0 ;          // the current index / the index the user sees
        std::uint64_t m_run_end_index = 0;  // the current index in the run ends array
        std::uint64_t m_runs_left = 0;      // the number of runs left in the current run

        friend class iterator_access;
    };
    









    

    // template <>
    // struct array_inner_types<run_end_encoded_array> : array_inner_types_base
    // {

    //     using iterator_tag = std::forward_iterator_tag;
    // };

    class run_end_encoded_array final //: public array_crtp_base<run_end_encoded_array>
    {
    public:
        using self_type = run_end_encoded_array;
        using base_type = array_crtp_base<self_type>;
        using size_type = std::size_t;
        using iterator = run_encoded_array_iterator<false>;
        using const_iterator = run_encoded_array_iterator<true>;

        
        explicit run_end_encoded_array(arrow_proxy proxy);

        array_traits::const_reference operator[](std::uint64_t i);
        array_traits::const_reference operator[](std::uint64_t i) const;

        iterator begin();
        iterator end();

        const_iterator begin() const;
        const_iterator end() const;

        const_iterator cbegin() const;
        const_iterator cend() const;

        size_type size() const;

    private:    
        using acc_length_ptr_variant_type = std::variant< const std::uint16_t*, const std::uint32_t*,const std::uint64_t*> ;

        static acc_length_ptr_variant_type get_acc_lengths_ptr(const array_wrapper& ar);
        std::uint64_t get_run_length(std::uint64_t run_index) const;

        arrow_proxy m_proxy;
        std::uint64_t m_encoded_length;
        
        cloning_ptr<array_wrapper> p_acc_lengths_array;
        cloning_ptr<array_wrapper> p_encoded_values_array;
        acc_length_ptr_variant_type m_acc_lengths;

        // friend classes
        friend class array_crtp_base<self_type>;
        friend class run_encoded_array_iterator<false>;
        friend class run_encoded_array_iterator<true>;
    };

    run_end_encoded_array::run_end_encoded_array(arrow_proxy proxy) 
    :   m_proxy(std::move(proxy)),
        m_encoded_length(m_proxy.children()[0].length()),
        p_acc_lengths_array(array_factory(m_proxy.children()[0].view())),
        p_encoded_values_array(array_factory(m_proxy.children()[1].view())),
        m_acc_lengths(self_type::get_acc_lengths_ptr(*p_acc_lengths_array))
    {
    }
    inline auto run_end_encoded_array::size() const -> size_type
    {
        return static_cast<size_type>(m_proxy.length());
    }

    inline auto run_end_encoded_array::get_acc_lengths_ptr(const array_wrapper& ar) -> acc_length_ptr_variant_type
    {
        return visit(
            [](const auto& ar) -> acc_length_ptr_variant_type
            {
                using array_type = std::decay_t<decltype(ar)>;
                using value_type = typename array_type::value_type;
                using raw_inner_value_type = typename value_type::value_type;
                using inner_value_type = std::decay_t<raw_inner_value_type>;
                if constexpr(std::is_same_v<inner_value_type, std::uint16_t>)
                {
                    return ar.data();
                }
                else if constexpr(std::is_same_v<inner_value_type, std::uint32_t>)
                {
                    return ar.data();
                }
                else if constexpr(std::is_same_v<inner_value_type, std::uint64_t>)
                {
                    return ar.data();
                }
                else
                {
                    std::cout<<"type with name "<<typeid(inner_value_type).name()<<" not supported"<<std::endl;
                    throw std::invalid_argument("array type not supported");
                }

            },
            ar
        );
    }

    auto run_end_encoded_array::get_run_length(std::uint64_t run_index) const -> std::uint64_t
    {

        auto ret =  std::visit(
            [run_index](auto&& acc_lengths_ptr) -> std::uint64_t
            {
                if(run_index == 0)
                {   
                    return static_cast<std::uint64_t>(acc_lengths_ptr[run_index]);
                }
                else
                {
                    return static_cast<std::uint64_t>(acc_lengths_ptr[run_index] - acc_lengths_ptr[run_index - 1]);
                }
            },
            m_acc_lengths
        );
        return ret;
    }

    inline auto run_end_encoded_array::operator[](std::uint64_t i) const -> array_traits::const_reference
    {
        // visit the variant
        auto ret =visit(
            [i, this](const auto& acc_lengths_ptr) -> array_traits::const_reference
            {
                
                auto it = std::upper_bound(
                    acc_lengths_ptr,
                    acc_lengths_ptr + this->m_encoded_length,
                    i
                );
                // std::lower_bound returns an iterator, so we need to convert it to an index
                const auto index = static_cast<std::uint64_t>(std::distance(acc_lengths_ptr, it));
                std::cout<<"i "<<i<<" index "<<index<<std::endl;
                return array_element(*p_encoded_values_array, index);
            },
            m_acc_lengths
        );
        
        return ret;
    }
    
    inline auto run_end_encoded_array::operator[](std::uint64_t i) -> array_traits::const_reference
    {
        return static_cast<const run_end_encoded_array*>(this)->operator[](i);
    }

    auto run_end_encoded_array::begin() -> iterator
    {
        return iterator(this, 0, 0);
    }

    auto run_end_encoded_array::end() -> iterator
    {
        return iterator(this, size(), 0);
    }

    auto run_end_encoded_array::begin() const -> const_iterator
    {
        return this->cbegin();
    }

    auto run_end_encoded_array::end() const -> const_iterator
    {
        return this->cend();
    }

    auto run_end_encoded_array::cbegin() const -> const_iterator
    {
        return const_iterator(this, 0, 0);
    }

    auto run_end_encoded_array::cend() const -> const_iterator
    {
        return const_iterator(this, size(), 0);
    }





} // namespace sparrow