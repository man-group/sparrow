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

#include "sparrow/utils/functor_index_iterator.hpp"

namespace sparrow::detail
{
    
    template<class LAYOUT_TYPE>
    class layout_functor_base
    {
        public:
        using layout_type = LAYOUT_TYPE;
        constexpr layout_functor_base() = default;
        constexpr layout_functor_base& operator=(layout_functor_base&&) = default;
        constexpr layout_functor_base(const layout_functor_base&) = default;
        constexpr layout_functor_base(layout_functor_base&&) = default;
        constexpr layout_functor_base& operator=(const layout_functor_base&) = default;

        constexpr layout_functor_base(layout_type * layout)
        : p_layout(layout)
        {
        }

        protected:
        layout_type * p_layout = nullptr;
    };


    // Functor to get the value of the layout at index i.
    //
    // This is usefull to create a iterator over the values of a layout.
    // This functor will be passed to the functor_index_iterator.
    template<class LAYOUT_TYPE, class VALUE_TYPE>
    class layout_value_functor : public layout_functor_base<LAYOUT_TYPE>
    {
        public:
        using base_type = layout_functor_base<LAYOUT_TYPE>;
        using base_type::base_type;
        using base_type::operator=;
        using value_type = VALUE_TYPE;

        value_type operator()(std::size_t i) const
        {
            return this->p_layout->value(i);
        }
    };


    // Functor to get the optional-value of the layout at index i.
    //
    // This is usefull to create a iterator over the nullable-values of a layout.
    // This functor will be passed to the functor_index_iterator.
    template<class LAYOUT_TYPE, class VALUE_TYPE>
    class layout_bracket_functor : public layout_functor_base<LAYOUT_TYPE>
    {
    public:
        using base_type = layout_functor_base<LAYOUT_TYPE>;
        using base_type::base_type;
        using base_type::operator=;
        using value_type = VALUE_TYPE;

        value_type operator()(std::size_t i) const
        {
            return this->p_layout->operator[](i);
        }
    };

}; // namespace sparrow::detail
