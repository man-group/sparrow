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
        requires(not std::is_lvalue_reference_v<A>)
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

    template <layout_or_array A>
    bool owns_arrow_array(const A& a)
    {
        return detail::array_access::get_arrow_proxy(a).owns_array();
    }

    template <layout_or_array A>
    bool owns_arrow_schema(const A& a)
    {
        return detail::array_access::get_arrow_proxy(a).owns_schema();
    }

    template <layout_or_array A>
    ArrowArray* get_arrow_array(A& a)
    {
        return &(detail::array_access::get_arrow_proxy(a).array());
    }

    template <layout_or_array A>
    ArrowSchema* get_arrow_schema(A& a)
    {
        return &(detail::array_access::get_arrow_proxy(a).schema());
    }

    template <layout_or_array A>
    std::pair<ArrowArray*, ArrowSchema*> get_arrow_structures(A& a)
    {
        arrow_proxy& proxy = detail::array_access::get_arrow_proxy(a);
        return std::make_pair(&(proxy.array()), &(proxy.schema()));
    }

    template <layout_or_array A>
    ArrowArray extract_arrow_array(A&& a)
    {
        return detail::array_access::get_arrow_proxy(a).extract_array();
    }

    template <layout_or_array A>
    ArrowSchema extract_arrow_schema(A&& a)
    {
        return detail::array_access::get_arrow_proxy(a).extract_schema();
    }

    template <layout_or_array A>
    std::pair<ArrowArray, ArrowSchema> extract_arrow_structures(A&& a)
    {
        arrow_proxy& proxy = detail::array_access::get_arrow_proxy(a);
        return std::make_pair(proxy.extract_array(), proxy.extract_schema());
    }
}

#if defined(__cpp_lib_format)

template <>
struct std::formatter<sparrow::array>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::array& ar, std::format_context& ctx) const
    {
        return ar.visit(
            [&ctx](const auto& layout)
            {
                return std::format_to(ctx.out(), "{}", layout);
            }
        );
    }
};

inline std::ostream& operator<<(std::ostream& os, const sparrow::array& value)
{
    os << std::format("{}", value);
    return os;
}

#endif
