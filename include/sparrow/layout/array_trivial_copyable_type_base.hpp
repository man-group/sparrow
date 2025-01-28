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

#include "sparrow/buffer/buffer_adaptor.hpp"
#include "sparrow/layout/array_bitmap_base.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    template <typename D>
    class array_trivial_copyable_type_base_impl;

    template <typename D>
    struct array_inner_types<array_trivial_copyable_type_base_impl<D>> : array_inner_types_base
    {
        using array_type = array_trivial_copyable_type_base_impl<D>;

        using derived_type = D;

        using inner_types = array_inner_types<D>;
        using inner_value_type = inner_types::inner_value_type;
        using inner_reference = inner_types::inner_reference;
        using inner_const_reference = inner_types::inner_const_reference;
        using pointer = inner_value_type*;
        using const_pointer = const inner_value_type*;

        using value_iterator = pointer_iterator<pointer>;
        using const_value_iterator = pointer_iterator<const_pointer>;

        // using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using iterator_tag = std::random_access_iterator_tag;
    };

    template <typename D>
    class array_trivial_copyable_type_base_impl : public mutable_array_bitmap_base<D>
    {
    public:

        using base_type = mutable_array_bitmap_base<D>;
        using size_type = std::size_t;

        using derived_type = D;
        using inner_types = array_inner_types<derived_type>;

        using inner_value_type = typename inner_types::inner_value_type;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;

        using pointer = typename inner_types::pointer;
        using const_pointer = typename inner_types::const_pointer;

        using value_iterator = typename base_type::value_iterator;
        using const_value_iterator = typename base_type::const_value_iterator;

    protected:

        array_trivial_copyable_type_base_impl(arrow_proxy);

        array_trivial_copyable_type_base_impl(const array_trivial_copyable_type_base_impl&);
        array_trivial_copyable_type_base_impl& operator=(const array_trivial_copyable_type_base_impl&);

        array_trivial_copyable_type_base_impl(array_trivial_copyable_type_base_impl&&) noexcept = default;
        array_trivial_copyable_type_base_impl&
        operator=(array_trivial_copyable_type_base_impl&&) noexcept = default;

        pointer data();
        const_pointer data() const;

        inner_reference value(size_type i);
        inner_const_reference value(size_type i) const;

        value_iterator value_begin();
        value_iterator value_end();

        const_value_iterator value_cbegin() const;
        const_value_iterator value_cend() const;

        // Modifiers

        void resize_values(size_type new_length, inner_value_type value);

        value_iterator insert_value(const_value_iterator pos, inner_value_type value, size_type count);

        template <mpl::iterator_of_type<inner_value_type> InputIt>
        value_iterator insert_values(const_value_iterator pos, InputIt first, InputIt last)
        {
            SPARROW_ASSERT_TRUE(value_cbegin() <= pos)
            SPARROW_ASSERT_TRUE(pos <= value_cend());
            const auto distance = std::distance(
                value_cbegin(),
                sparrow::next(pos, this->get_arrow_proxy().offset())
            );
            get_data_buffer().insert(pos, first, last);
            return sparrow::next(this->value_begin(), distance);
        }

        value_iterator erase_values(const_value_iterator pos, size_type count);

        buffer_adaptor<inner_value_type, buffer<uint8_t>&> get_data_buffer();

        static constexpr size_type DATA_BUFFER_INDEX = 1;

        friend base_type;
        friend base_type::base_type;
        friend base_type::base_type::base_type;
    };

    /********************************************************
     * array_trivial_copyable_type_base_impl implementation *
     ********************************************************/

    template <typename D>
    array_trivial_copyable_type_base_impl<D>::array_trivial_copyable_type_base_impl(arrow_proxy proxy_param)
        : base_type(std::move(proxy_param))
    {
    }

    template <typename D>
    array_trivial_copyable_type_base_impl<D>::array_trivial_copyable_type_base_impl(
        const array_trivial_copyable_type_base_impl& rhs
    )
        : base_type(rhs)
    {
    }

    template <typename D>
    array_trivial_copyable_type_base_impl<D>&
    array_trivial_copyable_type_base_impl<D>::operator=(const array_trivial_copyable_type_base_impl& rhs)
    {
        base_type::operator=(rhs);
        return *this;
    }

    template <typename D>
    auto array_trivial_copyable_type_base_impl<D>::data() -> pointer
    {
        return this->get_arrow_proxy().buffers()[DATA_BUFFER_INDEX].template data<inner_value_type>()
               + static_cast<size_type>(this->get_arrow_proxy().offset());
    }

    template <typename D>
    auto array_trivial_copyable_type_base_impl<D>::data() const -> const_pointer
    {
        return this->get_arrow_proxy().buffers()[DATA_BUFFER_INDEX].template data<const inner_value_type>()
               + static_cast<size_type>(this->get_arrow_proxy().offset());
    }

    template <typename D>
    auto array_trivial_copyable_type_base_impl<D>::value(size_type i) -> inner_reference
    {
        SPARROW_ASSERT_TRUE(i < this->size());
        return data()[i];
    }

    template <typename D>
    auto array_trivial_copyable_type_base_impl<D>::value(size_type i) const -> inner_const_reference
    {
        SPARROW_ASSERT_TRUE(i < this->size());
        return data()[i];
    }

    template <typename D>
    auto array_trivial_copyable_type_base_impl<D>::value_begin() -> value_iterator
    {
        return value_iterator{data()};
    }

    template <typename D>
    auto array_trivial_copyable_type_base_impl<D>::value_end() -> value_iterator
    {
        return sparrow::next(value_begin(), this->size());
    }

    template <typename D>
    auto array_trivial_copyable_type_base_impl<D>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator{data()};
    }

    template <typename D>
    auto array_trivial_copyable_type_base_impl<D>::value_cend() const -> const_value_iterator
    {
        return sparrow::next(value_cbegin(), this->size());
    }

    template <typename D>
    auto array_trivial_copyable_type_base_impl<D>::get_data_buffer()
        -> buffer_adaptor<inner_value_type, buffer<uint8_t>&>
    {
        auto& buffers = this->get_arrow_proxy().get_array_private_data()->buffers();
        return make_buffer_adaptor<inner_value_type>(buffers[DATA_BUFFER_INDEX]);
    }

    template <typename D>
    void array_trivial_copyable_type_base_impl<D>::resize_values(size_type new_length, inner_value_type value)
    {
        const size_t new_size = new_length + static_cast<size_t>(this->get_arrow_proxy().offset());
        get_data_buffer().resize(new_size, value);
    }

    template <typename D>
    auto array_trivial_copyable_type_base_impl<D>::insert_value(
        const_value_iterator pos,
        inner_value_type value,
        size_type count
    ) -> value_iterator
    {
        SPARROW_ASSERT_TRUE(value_cbegin() <= pos)
        SPARROW_ASSERT_TRUE(pos <= value_cend());
        const auto distance = std::distance(value_cbegin(), sparrow::next(pos, this->get_arrow_proxy().offset()));
        get_data_buffer().insert(pos, count, value);
        return sparrow::next(this->value_begin(), distance);
    }

    template <typename D>
    auto array_trivial_copyable_type_base_impl<D>::erase_values(const_value_iterator pos, size_type count)
        -> value_iterator
    {
        SPARROW_ASSERT_TRUE(this->value_cbegin() <= pos)
        SPARROW_ASSERT_TRUE(pos < this->value_cend());
        const auto distance = static_cast<size_t>(
            std::distance(this->value_cbegin(), sparrow::next(pos, this->get_arrow_proxy().offset()))
        );
        auto data_buffer = get_data_buffer();
        const auto first = sparrow::next(data_buffer.cbegin(), distance);
        const auto last = sparrow::next(first, count);
        data_buffer.erase(first, last);
        return sparrow::next(this->value_begin(), distance);
    }
}