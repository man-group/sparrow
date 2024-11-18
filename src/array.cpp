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

    enum data_type array::data_type() const
    {
        return p_array->data_type();
    }

    bool array::empty() const
    {
        return size() == size_type(0);
    }

    array::size_type array::size() const
    {
        return array_size(*p_array);
    }

    array::const_reference array::at(size_type index) const
    {
        if (index >= size())
        {
            std::ostringstream oss117;
            oss117 << "Index " << index << "is greater or equal to size of array ("
                << size() << ")";
            throw std::out_of_range(oss117.str());
        }
        return array_element(*p_array, index);
    }

    array::const_reference array::operator[](size_type index) const
    {
        return array_element(*p_array, index);
    }

    array::const_reference array::front() const
    {
        SPARROW_ASSERT_TRUE(!empty());
        return (*this)[0];
    }

    array::const_reference array::back() const
    {
        SPARROW_ASSERT_TRUE(!empty());
        return (*this)[size() - 1];
    }

    arrow_proxy& array::get_arrow_proxy()
    {
        return p_array->get_arrow_proxy();
    }

    const arrow_proxy& array::get_arrow_proxy() const
    {
        return p_array->get_arrow_proxy();
    }

    bool operator==(const array& lhs, const array& rhs)
    {
        return lhs.visit([&rhs](const auto& typed_lhs) -> bool
        {
            return rhs.visit([&typed_lhs](const auto& typed_rhs) -> bool
            {
                if constexpr (!std::same_as<decltype(typed_lhs), decltype(typed_rhs)>)
                {
                    return false;
                }
                else
                {
                    return typed_lhs == typed_rhs;
                }
            });
        });
    }
}

