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

#include <variant>

#include "sparrow/array/data_type.hpp"
#include "sparrow/array/array_common.hpp"

namespace sparrow
{
    class array
    {
    public:

        using traits_type = array_traits<array_data>;
        using value_type = traits_type::value_type;
        using reference = traits_type::reference;
        using const_reference = traits_type::const_reference;
        using size_type = std::size_t;

        using iterator = array_iterator<false>;
        using const_iterator = array_iterator<true>;

        explicit array(array_data data);

        bool empty() const;
        size_type size() const;

        reference at(size_type i);
        const_reference at(size_type i) const;

        reference operator[](size_type i);
        const_reference operator[](size_type i) const;

        reference front();
        const_reference front() const;

        reference back();
        const_reference back() const;

        iterator begin();
        iterator end();

        const_iterator begin() const;
        const_iterator end() const;

        const_iterator cbegin() const;
        const_iterator cend() const;

        template <class T>
        using as_reference = typed_array<T>::reference;

        template <class T>
        using as_const_reference = typed_array<T>::const_reference;

        template <class T>
        as_reference<T> get(size_type i);

        template <class T>
        as_const_reference<T> get(size_type i) const;

    private:

        using array_variant = traits_type::array_variant;
        array_variant m_array;
    };

    /************************
     * array implementation *
     ************************/

    inline array::array(array_data data)
        : m_array(build_array_variant(std::move(data)))
    {
    }

    inline bool array::empty() const
    {
        return std::visit(
            [](auto&& arg)
            {
                return arg.empty();
            },
            m_array
        );
    }

    inline auto array::size() const -> size_type
    {
        return std::visit(
            [](auto&& arg)
            {
                return arg.size();
            },
            m_array
        );
    }

    inline auto array::at(size_type i) -> reference
    {
        if (i >= size())
        {
            // TODO: Use our own format function
            throw std::out_of_range(
                "sparrow::array::at: index out of range for array of size " + std::to_string(size()) + " at index "
                + std::to_string(i)
            );
        }
        return (*this)[i];
    }
    
    inline auto array::at(size_type i) const -> const_reference
    {
        if (i >= size())
        {
            // TODO: Use our own format function
            throw std::out_of_range(
                "sparrow::array::at: index out of range for array of size " + std::to_string(size()) + " at index "
                + std::to_string(i)
            );
        }
        return (*this)[i];
    }

    inline auto array::operator[](size_type i) -> reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return std::visit(
            [i](auto&& arg)
            {
                return reference(arg[i]);
            },
            m_array
        );
    }
    
    inline auto array::operator[](size_type i) const -> const_reference
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

    inline auto array::front() -> reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return (*this)[0];
    }
    
    inline auto array::front() const -> const_reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return (*this)[0];
    }

    inline auto array::back() -> reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return (*this)[size() - 1];
    }
    
    inline auto array::back() const -> const_reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return (*this)[size() - 1];
    }

    inline auto array::begin() -> iterator
    {
        return std::visit(
            [](auto&& arg)
            {
                return iterator(arg.begin());
            },
            m_array
        );
    }

    inline auto array::end() -> iterator
    {
        return std::visit(
            [](auto&& arg)
            {
                return iterator(arg.end());
            },
            m_array
        );
    }
    
    inline auto array::begin() const -> const_iterator
    {
        return cbegin();
    }

    inline auto array::end() const -> const_iterator
    {
        return cend();
    }

    inline auto array::cbegin() const -> const_iterator
    {
        return std::visit(
            [](auto&& arg)
            {
                return const_iterator(arg.cbegin());
            },
            m_array
        );
    }

    inline auto array::cend() const -> const_iterator
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
    inline auto array::get(size_type i) -> as_reference<T> 
    {
        return std::get<as_reference<T>>(this->operator[](i));
    }

    template <class T>
    inline auto array::get(size_type i) const -> as_const_reference<T>
    {
        return std::get<as_const_reference<T>>(this->operator[](i));
    }
}
