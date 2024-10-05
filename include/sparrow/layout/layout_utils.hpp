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
    // Functor to get the value of the layout at index i.
    //
    // This is usefull to create a iterator over the values of a layout.
    // This functor will be passed to the functor_index_iterator.
    template<class LAYOUT_TYPE, class VALUE_TYPE>
    class layout_value_functor
    {
        public:
        using layout_type = LAYOUT_TYPE;
        using value_type = VALUE_TYPE;
        constexpr layout_value_functor() = default;
        constexpr layout_value_functor& operator=(layout_value_functor&&) = default;
        constexpr layout_value_functor(const layout_value_functor&) = default;
        constexpr layout_value_functor(layout_value_functor&&) = default;
        constexpr layout_value_functor& operator=(const layout_value_functor&) = default;

        constexpr layout_value_functor(layout_type * layout)
        : p_layout(layout)
        {
        }

        value_type operator()(std::size_t i) const
        {
            return p_layout->value(i);
        }
        private:
        layout_type * p_layout = nullptr;
    };

}; // namespace sparrow::detail
