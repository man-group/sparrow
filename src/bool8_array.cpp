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

#include "sparrow/bool8_array.hpp"

#include "sparrow/layout/array_access.hpp"

namespace sparrow
{
    // Constructor from arrow_proxy
    bool8_array::bool8_array(arrow_proxy proxy)
        : primitive_base(std::move(proxy))
    {
        init_extension_metadata(detail::array_access::get_arrow_proxy(*this));
    }

    // Data type accessor
    data_type bool8_array::data_type() const
    {
        return data_type::INT8;
    }

    // Value accessor
    bool8_array::inner_const_reference bool8_array::value(size_type index) const
    {
        return this->primitive_base::operator[](index).value();
    }

    // Element access operators
    bool8_array::reference bool8_array::operator[](size_type index)
    {
        return reference(this->primitive_base::operator[](index));
    }

    bool8_array::const_reference bool8_array::operator[](size_type index) const
    {
        return const_reference(this->primitive_base::operator[](index));
    }

    // Iterator accessors
    bool8_array::iterator bool8_array::begin()
    {
        return iterator(this->primitive_base::begin());
    }

    bool8_array::iterator bool8_array::end()
    {
        return iterator(this->primitive_base::end());
    }

    bool8_array::const_iterator bool8_array::begin() const
    {
        return const_iterator(this->primitive_base::begin());
    }

    bool8_array::const_iterator bool8_array::end() const
    {
        return const_iterator(this->primitive_base::end());
    }

    bool8_array::const_iterator bool8_array::cbegin() const
    {
        return const_iterator(this->primitive_base::cbegin());
    }

    bool8_array::const_iterator bool8_array::cend() const
    {
        return const_iterator(this->primitive_base::cend());
    }

    // Comparison operator
    bool operator==(const bool8_array& lhs, const bool8_array& rhs)
    {
        return static_cast<const bool8_array::primitive_base&>(lhs)
               == static_cast<const bool8_array::primitive_base&>(rhs);
    }

    // Arrow proxy access methods

    arrow_proxy& bool8_array::get_arrow_proxy()
    {
        return detail::array_access::get_arrow_proxy(static_cast<primitive_base&>(*this));
    }

    const arrow_proxy& bool8_array::get_arrow_proxy() const
    {
        return detail::array_access::get_arrow_proxy(static_cast<const primitive_base&>(*this));
    }
}
