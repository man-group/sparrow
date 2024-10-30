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

#include "sparrow/c_interface.hpp"
#include "sparrow/config/config.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/layout/null_array.hpp"
#include "sparrow/types/data_traits.hpp"


namespace sparrow
{
    class array
    {
    public:

        using size_type = std::size_t;
        using value_type = array_traits::value_type;
        using const_reference = array_traits::const_reference;
 
        SPARROW_API array() = default;

        template <layout A>
        requires (not std::is_lvalue_reference_v<A>)
        array(A&& a);

        template <layout A>
        array(A* a);

        template <layout A>
        array(std::shared_ptr<A> a);

        SPARROW_API array(ArrowArray&& array, ArrowSchema&& schema);
        SPARROW_API array(ArrowArray&& array, ArrowSchema* schema);
        SPARROW_API array(ArrowArray* array, ArrowSchema* schema);
        
        SPARROW_API size_type size() const;
        SPARROW_API const_reference operator[](size_type) const;

        template <class F>
        using visit_result_t = std::invoke_result_t<F, null_array>;
        
        template <class F>
        visit_result_t<F> visit(F&& func) const;

    private:

        SPARROW_API arrow_proxy& get_arrow_proxy();
        SPARROW_API const arrow_proxy& get_arrow_proxy() const;

        cloning_ptr<array_wrapper> p_array = nullptr;

        friend class detail::array_access;
    };

    SPARROW_API
    bool operator==(const array& lhs, const array& rhs);

    template <class A>
    concept layout_or_array = layout<A> or std::same_as<A, array>;

    template <layout_or_array A>
    bool owns_arrow_array(const A& a);

    template <layout_or_array A>
    bool owns_arrow_schema(const A& a);

    template <layout_or_array A>
    ArrowArray* get_arrow_array(A& a);

    template <layout_or_array A>
    ArrowSchema* get_arrow_schema(A& a);

    template <layout_or_array A>
    std::pair<ArrowArray*, ArrowSchema*> get_arrow_structures(A& a);

    template <layout_or_array A>
    ArrowArray extract_arrow_array(A&& a);

    template <layout_or_array A>
    ArrowSchema extract_arrow_schema(A&& a);

    template <layout_or_array A>
    std::pair<ArrowArray, ArrowSchema> extract_arrow_structures(A&& a);
}

