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

#include "sparrow/array.hpp"
#include "sparrow/array_factory.hpp"
#include "sparrow/layout/array_helper.hpp"

namespace sparrow
{
    array::array(ArrowArray&& array, ArrowSchema&& schema)
        : p_array(array_factory(arrow_proxy(std::move(array), std::move(schema))))
    {
    }

    array::array(ArrowArray&& array, ArrowSchema* schema)
        : p_array(array_factory(arrow_proxy(std::move(array), schema)))
    {
    }

    array::array(ArrowArray* array, ArrowSchema* schema)
        : p_array(array_factory(arrow_proxy(array, schema)))
    {
    }

    bool array::owns_arrow_array() const
    {
        return p_array->get_arrow_proxy().owns_array();
    }

    array& array::get_arrow_array(ArrowArray*& dst)
    {
        dst = &(p_array->get_arrow_proxy().array());
        return *this;
    }

    array&& array::extract_arrow_array(ArrowArray& dst) &&
    {
        dst = p_array->get_arrow_proxy().extract_array();
        return std::move(*this);
    }

    bool array::owns_arrow_schema() const
    {
        return p_array->get_arrow_proxy().owns_schema();
    }

    array& array::get_arrow_schema(ArrowSchema*& dst)
    {
        dst = &(p_array->get_arrow_proxy().schema());
        return *this;
    }

    array&& array::extract_arrow_schema(ArrowSchema& dst) &&
    {
        dst = p_array->get_arrow_proxy().extract_schema();
        return std::move(*this);
    }

    array::size_type array::size() const
    {
        return array_size(*p_array);
    }

    array::const_reference array::operator[](size_type index) const
    {
        return array_element(*p_array, index);
    }

    cloning_ptr<array_wrapper> && array::extract_array_wrapper() &&
    {
        return std::move(p_array);
    }
}

