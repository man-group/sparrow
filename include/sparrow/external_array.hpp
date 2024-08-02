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

#include <concepts>
#include <sstream>
#include <variant>

#include "sparrow/array/array_common.hpp"
#include "sparrow/array/data_type.hpp"
#include "sparrow/array/external_array_data.hpp"

namespace sparrow
{
    /**
     * Holds and provide a c++ interface for raw Arrow data allocated outside of this library.
     * Usually constructed using `ArrayArray` and `ArrowSchema` C structures
     * (see `arrow_interface/c_interface.hpp` for details).
     *
     * Data held by this type will not be modifiable but ownership will
     * be preserved according to the requested behavior specified at construction.
     *
     */
    class external_array
    {
    public:

        using traits_type = array_traits<external_array_data>;
        using value_type = traits_type::value_type;
        using const_reference = traits_type::const_reference;
        using size_type = std::size_t;

        using const_iterator = external_array_iterator<true>;

        /**
         * Constructor acquiring data from `ArrowArray` and `ArrowSchema` C structures.
         * Ownership for either is specified through parameter `ownership`.
         * As per Arrow's format specification, if the data is own, the provided release functions
         * which are part of the provided structures will be used and must exist in that case.
         *
         * @param aschema `ArrowSchema` object passed by reference, value or pointer, that
         *                this object will handle.
         * @param aarray `ArrowArray` object passed by reference, value or pointer, that
         *                this object will handle.
         * @param ownership Specifies ownership of the data provided through the
                            `ArrowSchema` and `ArrowArray`. By default, if not specified,
                            we assume that the ownership of these data is transfered to this object.
         */
        template <arrow_schema_or_ptr S, arrow_array_or_ptr A>
        external_array(S&& aschema, A&& aarray, arrow_data_ownership ownership = owns_arrow_data);

        bool empty() const;
        size_type size() const;

        const_reference at(size_type i) const;
        const_reference operator[](size_type i) const;

        const_reference front() const;
        const_reference back() const;

        const_iterator begin() const;
        const_iterator end() const;

        const_iterator cbegin() const;
        const_iterator cend() const;

        template <class T>
        using as_const_reference = typed_array<T>::const_reference;

        template <class T>
        as_const_reference<T> get(size_type i) const;

    private:

        using array_variant = traits_type::array_variant;
        array_variant m_array;
    };

    /*********************************
     * external_array implementation *
     *********************************/

    template <arrow_schema_or_ptr S, arrow_array_or_ptr A>
    external_array::external_array(S&& aschema, A&& aarray, arrow_data_ownership ownership)
        : m_array(build_array_variant(
            external_array_data(std::forward<S>(aschema), std::forward<A>(aarray), ownership)
        )
        )
    {
    }

    inline bool external_array::empty() const
    {
        return std::visit(
            [](auto&& arg)
            {
                return arg.empty();
            },
            m_array
        );
    }

    inline auto external_array::size() const -> size_type
    {
        return std::visit(
            [](auto&& arg)
            {
                return arg.size();
            },
            m_array
        );
    }

    inline auto external_array::at(size_type i) const -> const_reference
    {
        if (i >= size())
        {
            // TODO: Use our own format function
            throw std::out_of_range(
                "sparrow::array::at: index out of range for array of size " + std::to_string(size())
                + " at index " + std::to_string(i)
            );
        }
        return (*this)[i];
    }

    inline auto external_array::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return std::visit(
            [i](auto&& arg)
            {
                return const_reference(arg[i]);
            },
            m_array
        );
    }

    inline auto external_array::front() const -> const_reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return (*this)[0];
    }

    inline auto external_array::back() const -> const_reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return (*this)[size() - 1];
    }

    inline auto external_array::begin() const -> const_iterator
    {
        return cbegin();
    }

    inline auto external_array::end() const -> const_iterator
    {
        return cend();
    }

    inline auto external_array::cbegin() const -> const_iterator
    {
        return std::visit(
            [](auto&& arg)
            {
                return const_iterator(arg.cbegin());
            },
            m_array
        );
    }

    inline auto external_array::cend() const -> const_iterator
    {
        return std::visit(
            [](auto&& arg)
            {
                return const_iterator(arg.cend());
            },
            m_array
        );
    }

    template <class T>
    inline auto external_array::get(size_type i) const -> as_const_reference<T>
    {
        return std::get<as_const_reference<T>>(this->operator[](i));
    }
}
