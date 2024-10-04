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

#include <ranges>
#include <string>
#include <string_view>

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/layout/layout_iterator.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    /**
     * Base class for array type erasure
     */
    class array_base
    {
    public:

        virtual ~array_base() = default;

        array_base(array_base&&) = delete;
        array_base& operator=(const array_base&) = delete;
        array_base& operator=(array_base&&) = delete;

        array_base* clone() const;

        enum data_type data_type() const;

    protected:

        array_base(enum data_type dt);
        array_base(const array_base&) = default;

    private:

        enum data_type m_data_type;
        virtual array_base* clone_impl() const = 0;
    };

    /**
     * Base class for array_inner_types specialization
     *
     * It defines common typs used in the array implementation
     * classes.
     * */
    struct array_inner_types_base
    {
        using bitmap_type = dynamic_bitset_view<std::uint8_t>;
    };

    /**
     * traits class that must be specialized by array
     * classes inheriting from array_crtp_base.
     */
    template <class D>
    struct array_inner_types;

    /**
     * Base class defining common interface for arrays.
     *
     * This class is a CRTP base class that defines and
     * implements comme interface for arrays with a bitmap.
     */
    template <class D>
    class array_crtp_base
    {
    public:
        using self_type = array_crtp_base<D>;
        using derived_type = D;
        using inner_types = array_inner_types<derived_type>;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using bitmap_type = typename inner_types::bitmap_type;
        using bitmap_reference = bitmap_type::reference;
        using bitmap_const_reference = bitmap_type::const_reference;
        using bitmap_iterator = bitmap_type::iterator;
        using const_bitmap_iterator = bitmap_type::const_iterator;
        using const_bitmap_range = std::ranges::subrange<const_bitmap_iterator>;

        using inner_value_type = typename inner_types::inner_value_type;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;
        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;

        using value_type = nullable<inner_value_type>;
        using iterator_tag = typename inner_types::iterator_tag;

        using iterator = layout_iterator<self_type, false>;
        using const_iterator = layout_iterator<self_type, true>;
         
        using value_iterator = typename inner_types::value_iterator;
        using const_value_iterator = typename inner_types::const_value_iterator;

        using const_value_range = std::ranges::subrange<const_value_iterator>;

        size_type size() const;

        iterator begin();
        iterator end();

        const_iterator begin() const;
        const_iterator end() const;

        const_iterator cbegin() const;
        const_iterator cend() const;

        const_bitmap_range bitmap() const;
        const_value_range values() const;
        reference operator[](size_type i);
        const_reference operator[](size_type i) const;

    protected:

        array_crtp_base(arrow_proxy);
        array_crtp_base(const array_crtp_base&);

        const arrow_proxy& storage() const;
        arrow_proxy& storage();

        bitmap_reference has_value(size_type i);
        bitmap_const_reference has_value(size_type i) const;

        bitmap_iterator bitmap_begin();
        bitmap_iterator bitmap_end();

        const_bitmap_iterator bitmap_begin() const;
        const_bitmap_iterator bitmap_end() const;

    private:

        static constexpr std::size_t m_bitmap_buffer_index = 0;

        bitmap_type make_bitmap();

        derived_type& derived_cast();
        const derived_type& derived_cast() const;

        arrow_proxy m_proxy;
        bitmap_type m_bitmap;

        // friend classes
        friend class layout_iterator<self_type, false>;
        friend class layout_iterator<self_type, true>;
    };

    /*****************************
     * array_base implementation *
     *****************************/

    inline array_base* array_base::clone() const
    {
        return clone_impl();
    }

    inline enum data_type array_base::data_type() const
    {
        return m_data_type;
    }

    inline array_base::array_base(enum data_type dt)
        : m_data_type(dt)
    {
    }

    /**********************************
     * array_crtp_base implementation *
     **********************************/

    template <class D>
    auto array_crtp_base<D>::size() const -> size_type
    {
        return static_cast<size_type>(storage().length());
    }

    template <class D>
    auto array_crtp_base<D>::begin() -> iterator
    {
        return iterator(derived_cast().value_begin(), derived_cast().bitmap_begin());
    }

    template <class D>
    auto array_crtp_base<D>::end() -> iterator
    {
        return iterator(derived_cast().value_end(), derived_cast().bitmap_end());
    }

    template <class D>
    auto array_crtp_base<D>::begin() const -> const_iterator
    {
        return cbegin();
    }

    template <class D>
    auto array_crtp_base<D>::end() const -> const_iterator
    {
        return cend();
    }

    template <class D>
    auto array_crtp_base<D>::cbegin() const -> const_iterator
    {
        return const_iterator(derived_cast().value_cbegin(), derived_cast().bitmap_begin());
    }

    template <class D>
    auto array_crtp_base<D>::cend() const -> const_iterator
    {
        return const_iterator(derived_cast().value_cend(), derived_cast().bitmap_end());
    }

    template <class D>
    auto array_crtp_base<D>::bitmap() const -> const_bitmap_range
    {
        return const_bitmap_range(bitmap_begin(), bitmap_end());
    }

    template <class D>
    auto array_crtp_base<D>::values() const -> const_value_range
    {
        return const_value_range(derived_cast().value_cbegin(), derived_cast().value_cend());
    }

    template <class D>
    auto array_crtp_base<D>::operator[](size_type i) -> reference
    {
        return reference(
            inner_reference(derived_cast().value(i)),
            derived_cast().has_value(i)
        );
    }

    template <class D>
    auto array_crtp_base<D>::operator[](size_type i) const -> const_reference
    {
        return const_reference(
            inner_const_reference(derived_cast().value(i)),
            derived_cast().has_value(i)
        );
    }

    template <class D>
    array_crtp_base<D>::array_crtp_base(arrow_proxy proxy)
        : m_proxy(std::move(proxy))
        , m_bitmap(make_bitmap())
    {
    }

    template <class D>
    array_crtp_base<D>::array_crtp_base(const array_crtp_base& rhs)
        : m_proxy(rhs.m_proxy)
        , m_bitmap(make_bitmap())
    {
    }

    template <class D>
    auto array_crtp_base<D>::storage() -> arrow_proxy&
    {
        return m_proxy;
    }

    template <class D>
    auto array_crtp_base<D>::storage() const -> const arrow_proxy&
    {
        return m_proxy;
    }

    template <class D>
    auto array_crtp_base<D>::has_value(size_type i) -> bitmap_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return m_bitmap[i + static_cast<size_type>(storage().offset())];
    }

    template <class D>
    auto array_crtp_base<D>::has_value(size_type i) const -> bitmap_const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return m_bitmap[i + static_cast<size_type>(storage().offset())];
    }

    template <class D>
    auto array_crtp_base<D>::bitmap_begin() -> bitmap_iterator
    {
        return sparrow::next(m_bitmap.begin(), storage().offset());
    }

    template <class D>
    auto array_crtp_base<D>::bitmap_end() -> bitmap_iterator
    {
        return sparrow::next(bitmap_begin(), size());
    }

    template <class D>
    auto array_crtp_base<D>::bitmap_begin() const -> const_bitmap_iterator
    {
        return sparrow::next(m_bitmap.cbegin(), storage().offset());
    }

    template <class D>
    auto array_crtp_base<D>::bitmap_end() const -> const_bitmap_iterator
    {
        return sparrow::next(bitmap_begin(), size());
    }

    template <class D>
    auto array_crtp_base<D>::make_bitmap() -> bitmap_type
    {
        SPARROW_ASSERT_TRUE(storage().buffers().size() > m_bitmap_buffer_index);
        const auto bitmap_size = static_cast<std::size_t>(storage().length() + storage().offset());
        return bitmap_type(storage().buffers()[m_bitmap_buffer_index].data(), bitmap_size);
    }

    template <class D>
    auto array_crtp_base<D>::derived_cast() -> derived_type&
    {
        return *static_cast<derived_type*>(this);
    }

    template <class D>
    auto array_crtp_base<D>::derived_cast() const -> const derived_type&
    {
        return *static_cast<const derived_type*>(this);
    }
}
