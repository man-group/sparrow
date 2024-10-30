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

#include "sparrow/array_api.hpp"
#include "sparrow/layout/dispatch.hpp"

namespace sparrow
{
    template <layout A>
    requires (not std::is_lvalue_reference_v<A>)
    array::array(A&& a)
        : p_array(new array_wrapper_impl<A>(std::move(a)))
    {
    }

    template <layout A>
    array::array(A* a)
        : p_array(new array_wrapper_impl<A>(a))
    {
    }

    template <layout A>
    array::array(std::shared_ptr<A> a)
        : p_array(new array_wrapper_impl<A>(a))
    {
    }

    template <class F>
    array::visit_result_t<F> array::visit(F&& func) const
    {
        return sparrow::visit(std::forward<F>(func), *p_array);
    }
}
