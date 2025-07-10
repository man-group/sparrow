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

#include "sparrow/layout/array_helper.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/utils/iterator.hpp"

namespace sparrow
{

    class run_end_encoded_array;

    // this iteratas over the **actual** values of the run encoded array
    // Ie nullabes values, not values !!!
    template <bool is_const>
    class run_encoded_array_iterator : public iterator_base<
                                           run_encoded_array_iterator<is_const>,
                                           array_traits::const_reference,
                                           std::bidirectional_iterator_tag,
                                           array_traits::const_reference>
    {
    private:

        using array_ptr_type = std::conditional_t<is_const, const run_end_encoded_array*, run_end_encoded_array*>;

    public:

        run_encoded_array_iterator() = default;
        run_encoded_array_iterator(array_ptr_type array_ptr, std::uint64_t index, std::uint64_t run_end_index);

    private:

        [[nodiscard]] bool equal(const run_encoded_array_iterator& rhs) const;
        void increment();
        void decrement();
        [[nodiscard]] array_traits::const_reference dereference() const;

        array_ptr_type p_array = nullptr;
        array_wrapper* p_encoded_values_array = nullptr;
        std::uint64_t m_index = 0;           // the current index / the index the user sees
        std::uint64_t m_run_end_index = 0;   // the current index in the run ends array
        std::uint64_t m_acc_length_up = 0;   // the accumulated length at m_run_end_index
        std::uint64_t m_acc_length_down = 0; // the accumulated length at m_run_end_index-1

        friend class iterator_access;
    };

    template <bool is_const>
    run_encoded_array_iterator<is_const>::run_encoded_array_iterator(
        array_ptr_type array_ptr,
        std::uint64_t index,
        std::uint64_t run_end_index
    )
        : p_array(array_ptr)
        , p_encoded_values_array(array_ptr->p_encoded_values_array.get())
        , m_index(index)
        , m_run_end_index(run_end_index)
        , m_acc_length_up(m_index < p_array->size() ? p_array->get_acc_length(m_run_end_index) : p_array->m_encoded_length)
        , m_acc_length_down(m_run_end_index == 0 ? 0 : p_array->get_acc_length(m_run_end_index-1))
    {
    }

    template <bool is_const>
    bool run_encoded_array_iterator<is_const>::equal(const run_encoded_array_iterator& rhs) const
    {
        return m_index == rhs.m_index;
    }

    template <bool is_const>
    void run_encoded_array_iterator<is_const>::increment()
    {
        ++m_index;
        if (m_index == 0)
        {
            m_run_end_index = 0;
            m_acc_length_up = p_array->get_acc_length(m_run_end_index);
            m_acc_length_down = 0;
        }
        else if (m_index >= p_array->size())
        {
            m_run_end_index = p_array->m_encoded_length;
        }
        else if (m_index == m_acc_length_up)
        {
            ++m_run_end_index;
            m_acc_length_up = p_array->get_acc_length(m_run_end_index);
            m_acc_length_down = p_array->get_acc_length(m_run_end_index-1);
        }
    }

    template <bool is_const>
    void run_encoded_array_iterator<is_const>::decrement()
    {
        if (m_index == 0)
        {
            m_run_end_index = p_array->m_encoded_length;
        }
        else if (m_index == p_array->size() || m_index == m_acc_length_down)
        {
            --m_run_end_index;
            m_acc_length_up = p_array->get_acc_length(m_run_end_index);
            m_acc_length_down = m_run_end_index == 0 ? 0 : p_array->get_acc_length(m_run_end_index - 1);
        }
        --m_index;
    }

    template <bool is_const>
    typename array_traits::const_reference run_encoded_array_iterator<is_const>::dereference() const
    {
        return array_element(*p_encoded_values_array, static_cast<std::size_t>(m_run_end_index));
    }

}  // namespace sparrow
