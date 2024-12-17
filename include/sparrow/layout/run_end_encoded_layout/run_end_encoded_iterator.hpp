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
#include "sparrow/config/config.hpp"
#include "sparrow/layout/array_helper.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/layout_utils.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/memory.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{

    class run_end_encoded_array;

    // this iteratas over the **actual** values of the run encoded array
    // Ie nullabes values, not values !!!
    template <bool CONST>
    class run_encoded_array_iterator : public iterator_base<
                                           run_encoded_array_iterator<CONST>,
                                           array_traits::const_reference,
                                           std::forward_iterator_tag,
                                           array_traits::const_reference>
    {
    private:

        using array_ptr_type = std::conditional_t<CONST, const run_end_encoded_array*, run_end_encoded_array*>;

    public:

        run_encoded_array_iterator() = default;
        run_encoded_array_iterator(array_ptr_type array_ptr, std::uint64_t index, std::uint64_t run_end_index);

    private:

        bool equal(const run_encoded_array_iterator& rhs) const;
        void increment();
        array_traits::const_reference dereference() const;
        array_ptr_type p_array = nullptr;
        array_wrapper* p_encoded_values_array = nullptr;
        std::uint64_t m_index = 0;          // the current index / the index the user sees
        std::uint64_t m_run_end_index = 0;  // the current index in the run ends array
        std::uint64_t m_runs_left = 0;      // the number of runs left in the current run

        friend class iterator_access;
    };

    template <bool CONST>
    run_encoded_array_iterator<CONST>::run_encoded_array_iterator(
        array_ptr_type array_ptr,
        std::uint64_t index,
        std::uint64_t run_end_index
    )
        : p_array(array_ptr)
        , p_encoded_values_array(array_ptr->p_encoded_values_array.get())
        , m_index(index)
        , m_run_end_index(run_end_index)
        , m_runs_left(array_ptr->get_run_length(index))
    {
    }

    template <bool CONST>
    bool run_encoded_array_iterator<CONST>::equal(const run_encoded_array_iterator& rhs) const
    {
        return m_index == rhs.m_index;
    }

    template <bool CONST>
    void run_encoded_array_iterator<CONST>::increment()
    {
        ++m_index;
        --m_runs_left;
        if (m_runs_left == 0 && m_index < p_array->size())
        {
            ++m_run_end_index;
            m_runs_left = p_array->get_run_length(m_run_end_index);
        }
    }

    template <bool CONST>
    typename array_traits::const_reference run_encoded_array_iterator<CONST>::dereference() const
    {
        return array_element(*p_encoded_values_array, static_cast<std::size_t>(m_run_end_index));
    }

}  // namespace sparrow
