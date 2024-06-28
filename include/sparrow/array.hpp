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

#include "sparrow/data_type.hpp"
#include "sparrow/mp_utils.hpp"
#include "sparrow/typed_array.hpp"

namespace sparrow
{
    template <class T>
    using make_typed_array_t = typed_array<T>;

    using all_array_types_t = mpl::transform<make_typed_array_t, all_base_types_t>;

    struct array_traits
    {
        using array_variant = mpl::rename<all_array_types_t, std::variant>;

        using value_type = mpl::transform<array_value_type_t, array_variant>;
        using reference = mpl::transform<array_reference_t, array_variant>;
        using const_reference = mpl::transform<array_const_reference_t, array_variant>;

        using inner_iterator = mpl::transform<array_iterator_t, array_variant>;
        using inner_const_iterator = mpl::transform<array_const_iterator_t, array_variant>;
    };

    template <bool is_const>
    class array_iterator
        : public iterator_base<
              array_iterator<is_const>,
              array_traits::value_type,
              std::random_access_iterator_tag,
              std::conditional_t<is_const, array_traits::const_reference, array_traits::reference>>
    {
    public:

        using self_type = array_iterator<is_const>;
        using base_type = iterator_base<
            array_iterator<is_const>,
            array_traits::value_type,
            std::random_access_iterator_tag,
            std::conditional_t<is_const, array_traits::const_reference, array_traits::reference>>;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;

        array_iterator() = default;

        template <class It>
            requires(!std::same_as<std::decay_t<It>, array_iterator<is_const>>)
        array_iterator(It&& iter)
            : m_iter(std::forward<It>(iter))
        {
        }

    private:

        std::string get_type_name() const;
        std::string build_mismatch_message(std::string_view method, const self_type& rhs) const;

        reference dereference() const;
        void increment();
        void decrement();
        void advance(difference_type n);
        difference_type distance_to(const self_type& rhs) const;
        bool equal(const self_type& rhs) const;
        bool less_than(const self_type& rhs) const;

        using iterator_storage = std::conditional_t<
            is_const,
            mpl::transform<array_iterator_t, array_traits::array_variant>,
            mpl::transform<array_const_iterator_t, array_traits::array_variant>>;

        using inner_iterator = std::conditional_t<is_const, array_traits::inner_const_iterator, array_traits::inner_iterator>;
        inner_iterator m_iter;

        friend class iterator_access;
    };

    class array
    {
    public:

        using value_type = array_traits::value_type;
        using reference = array_traits::reference;
        using const_reference = array_traits::const_reference;
        using size_type = std::size_t;

        using const_iterator = array_iterator<true>;

        array(data_descriptor dd, array_data data);

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

    private:

        using array_variant = array_traits::array_variant;
        array_variant build_array(const data_descriptor& d, array_data&& data) const;

        data_descriptor m_data_descriptor;
        array_variant m_array;
    };

    /*********************************
     * array_iterator implementation *
     *********************************/

    template <bool IC>
    std::string array_iterator<IC>::get_type_name() const
    {
        return std::visit(
            [](auto&& arg)
            {
                return typeid(std::decay_t<decltype(arg)>).name();
            },
            m_iter
        );
    }

    template <bool IC>
    std::string array_iterator<IC>::build_mismatch_message(std::string_view method, const self_type& rhs) const
    {
        std::ostringstream oss117;
        oss117 << method << ": iterators must have the same type, got " << get_type_name() << " and "
               << rhs.get_type_name();
        return oss117.str();
    }

    template <bool IC>
    auto array_iterator<IC>::dereference() const -> reference
    {
        return std::visit(
            [](auto&& arg)
            {
                return reference(*arg);
            },
            m_iter
        );
    }

    template <bool IC>
    void array_iterator<IC>::increment()
    {
        std::visit(
            [](auto&& arg)
            {
                ++arg;
            },
            m_iter
        );
    }

    template <bool IC>
    void array_iterator<IC>::decrement()
    {
        std::visit(
            [](auto&& arg)
            {
                --arg;
            },
            m_iter
        );
    }

    template <bool IC>
    void array_iterator<IC>::advance(difference_type n)
    {
        std::visit(
            [n](auto&& arg)
            {
                arg += n;
            },
            m_iter
        );
    }

    template <bool IC>
    auto array_iterator<IC>::distance_to(const self_type& rhs) const -> difference_type
    {
        if (m_iter.index() != rhs.m_iter.index())
        {
            throw std::invalid_argument(build_mismatch_message("array_iterator::distance_to", rhs));
        }

        return std::visit(
            [&rhs](auto&& arg)
            {
                return std::get<std::decay_t<decltype(arg)>>(rhs.m_iter) - arg;
            },
            m_iter
        );
    }

    template <bool IC>
    bool array_iterator<IC>::equal(const self_type& rhs) const
    {
        if (m_iter.index() != rhs.m_iter.index())
        {
            return false;
        }

        return std::visit(
            [&rhs](auto&& arg)
            {
                return arg == std::get<std::decay_t<decltype(arg)>>(rhs.m_iter);
            },
            m_iter
        );
    }

    template <bool IC>
    bool array_iterator<IC>::less_than(const self_type& rhs) const
    {
        if (m_iter.index() != rhs.m_iter.index())
        {
            throw std::invalid_argument(build_mismatch_message("array_iterator::less_than", rhs));
        }

        return std::visit(
            [&rhs](auto&& arg)
            {
                return arg <= std::get<std::decay_t<decltype(arg)>>(rhs.m_iter);
            },
            m_iter
        );
    }

    /************************
     * array implementation *
     ************************/

    inline auto array::build_array(const data_descriptor& dd, array_data&& data) const -> array_variant
    {
        switch (dd.id())
        {
            case data_type::NA:
                return typed_array<null_type>(std::move(data));
            case data_type::BOOL:
                return typed_array<bool>(std::move(data));
            case data_type::UINT8:
                return typed_array<std::uint8_t>(std::move(data));
            case data_type::INT8:
                return typed_array<std::int8_t>(std::move(data));
            case data_type::UINT16:
                return typed_array<std::uint16_t>(std::move(data));
            case data_type::INT16:
                return typed_array<std::int16_t>(std::move(data));
            case data_type::UINT32:
                return typed_array<std::uint32_t>(std::move(data));
            case data_type::INT32:
                return typed_array<std::int32_t>(std::move(data));
            case data_type::UINT64:
                return typed_array<std::uint64_t>(std::move(data));
            case data_type::INT64:
                return typed_array<std::int64_t>(std::move(data));
            case data_type::HALF_FLOAT:
                return typed_array<float16_t>(std::move(data));
            case data_type::FLOAT:
                return typed_array<float32_t>(std::move(data));
            case data_type::DOUBLE:
                return typed_array<float64_t>(std::move(data));
            case data_type::STRING:
            case data_type::FIXED_SIZE_BINARY:
                return typed_array<std::string>(std::move(data));
            case data_type::TIMESTAMP:
                return typed_array<sparrow::timestamp>(std::move(data));
            default:
                // TODO: implement other data types, remove the default use case
                // and throw from outside of the switch
                throw std::invalid_argument("not supported yet");
        }
    }

    inline array::array(data_descriptor dd, array_data data)
        : m_data_descriptor(std::move(dd))
        , m_array(build_array(m_data_descriptor, std::move(data)))
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

    inline auto array::at(size_type i) const -> const_reference
    {
        if (i >= size())
        {
            // TODO: Use our own format function
            throw std::out_of_range(
                "array::at: index out of range for array of size " + std::to_string(size()) + " at index "
                + std::to_string(i)
            );
        }
        return (*this)[i];
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

    inline auto array::front() const -> const_reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return (*this)[0];
    }

    inline auto array::back() const -> const_reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return (*this)[size() - 1];
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
}
