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

#include "sparrow/layout/array_helper.hpp"

#include "sparrow/layout/dispatch.hpp"

namespace sparrow
{
    std::size_t array_size(const array_wrapper& ar)
    {
        return visit(
            [](const auto& impl)
            {
                return impl.size();
            },
            ar
        );
    }

    bool array_has_value(const array_wrapper& ar, std::size_t index)
    {
        return visit(
            [index](const auto& impl)
            {
                return impl[index].has_value();
            },
            ar
        );
    }

    array_traits::const_reference array_element(const array_wrapper& ar, std::size_t index)
    {
        using return_type = array_traits::const_reference;
        return visit(
            [index](const auto& impl) -> return_type
            {
                return return_type(impl[index]);
            },
            ar
        );
    }

    array_traits::inner_value_type array_default_element_value(const array_wrapper& ar)
    {
        using return_type = array_traits::inner_value_type;
        return visit(
            [](const auto& impl) -> return_type
            {
                using value_type = typename std::decay_t<decltype(impl)>::inner_value_type;
                return value_type();
            },
            ar
        );
    }
}
