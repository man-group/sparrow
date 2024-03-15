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

#include <cassert>
#include <compare>
#include <concepts>
#include <optional>
#include <vector>

#include "sparrow/buffer.hpp"
#include "sparrow/data_type.hpp"
#include "sparrow/dynamic_bitset.hpp"

namespace sparrow
{
    /**
     * Structure holding the raw data.
     *
     * array_data is meant to be used by the
     * different layout classes to implement
     * the array API, based on the type
     * specified in the type attribute.
     */
    struct array_data
    {
        using block_type = std::uint8_t;
        using bitmap_type = dynamic_bitset<block_type>;
        using buffer_type = buffer<block_type>;

        data_descriptor type;
        std::int64_t length = 0;
        std::int64_t offset = 0;
        // bitmap buffer and null_count
        bitmap_type bitmap;
        // Other buffers
        std::vector<buffer_type> buffers;
        std::vector<array_data> child_data;
    };

    /**
     * CRTP base class for reference types used in
     * layout classes. The reference proxy classes
     * inheriting from this class provide a similar
     * behavior as std::optional, but as reference on
     * data in an array_data.
     */
    template <class D>
    class reference_proxy_base
    {
    public:

        using self_type = reference_proxy_base<D>;
        using derived_type = D;

        derived_type& derived_cast();
        const derived_type& derived_cast() const;

    protected:

        reference_proxy_base() = default;
        ~reference_proxy_base() = default;

        reference_proxy_base(const reference_proxy_base&) = default;
        reference_proxy_base& operator=(const reference_proxy_base&) = default;

        reference_proxy_base(reference_proxy_base&&) = default;
        reference_proxy_base& operator=(reference_proxy_base&&) = default;
    };

    template <class T>
    concept ref_proxy = std::derived_from<T, reference_proxy_base<T>>;

    template <class T>
    concept not_ref_proxy = (!ref_proxy<T>);

    template <class D1, class D2>
    bool operator==(const reference_proxy_base<D1>& lhs, const reference_proxy_base<D2>& rhs);

    template <class D, not_ref_proxy T>
    bool operator==(const reference_proxy_base<D>& lhs, const T& rhs);

    template <class D>
    bool operator==(const reference_proxy_base<D>& lhs, std::nullopt_t);

    template <class D1, class D2>
    std::strong_ordering operator<=>(const reference_proxy_base<D1>& lhs, const reference_proxy_base<D2>& rhs);

    template <class D, not_ref_proxy T>
    std::strong_ordering operator<=>(const reference_proxy_base<D>& lhs, const T& rhs);

    template <class D>
    std::strong_ordering operator<=>(const reference_proxy_base<D>& lhs, std::nullopt_t);

    /**
     * Default const reference proxy class
     */
    template <class L>
    class const_reference_proxy : public reference_proxy_base<const_reference_proxy<L>>
    {
    public:

        using self_type = const_reference_proxy<L>;
        using base_type = reference_proxy_base<self_type>;
        using layout_type = L;
        using value_type = typename L::inner_value_type;
        using size_type = typename L::size_type;

        const_reference_proxy(const layout_type& l, size_type index);
        ~const_reference_proxy() = default;

        const_reference_proxy(const self_type&) = default;
        const_reference_proxy(self_type&&) = default;

        bool has_value() const;
        explicit operator bool() const;

        const value_type& value() const;

    private:

        const layout_type* p_layout;
        size_type m_index;
    };

    /**
     * Default reference proxy class.
     */
    template <class L>
    class reference_proxy : public reference_proxy_base<reference_proxy<L>>
    {
    public:

        using self_type = reference_proxy<L>;
        using base_type = reference_proxy_base<self_type>;
        using layout_type = L;
        using value_type = typename L::inner_value_type;
        using size_type = typename L::size_type;

        reference_proxy(layout_type& l, size_type index);
        ~reference_proxy() = default;

        reference_proxy(const self_type&) = default;
        reference_proxy(self_type&&) = default;

        bool has_value() const;
        explicit operator bool() const;

        value_type& value();
        const value_type& value() const;

        self_type& operator=(const self_type& rhs);
        self_type& operator=(self_type&& rhs);
        self_type& operator=(std::nullopt_t);

        template <std::convertible_to<typename L::inner_value_type> U>
        self_type& operator=(const std::optional<U>& rhs);

        template <std::convertible_to<typename L::inner_value_type> U>
        self_type& operator=(std::optional<U>&& rhs);

        template <std::convertible_to<typename L::inner_value_type> U>
        self_type& operator=(U&& value);

        void swap(self_type& rhs);
        
    private:

        void reset();

        template <class U>
        void update(U&& u);

        template <class U>
        void update_value(U&& u);

        layout_type* p_layout;
        size_type m_index;
    };

    template <class L>
    void swap(reference_proxy<L>& lhs, reference_proxy<L>& rhs);

    /***************************************
     * reference_proxy_base implementation *
     ***************************************/

    template <class D>
    auto reference_proxy_base<D>::derived_cast() -> derived_type&
    {
        return *static_cast<derived_type*>(this);
    }

    template <class D>
    auto reference_proxy_base<D>::derived_cast() const -> const derived_type&
    {
        return *static_cast<const derived_type*>(this);
    }

    template <class D1, class D2>
    bool operator==(const reference_proxy_base<D1>& lhs, const reference_proxy_base<D2>& rhs)
    {
        const D1& dlhs = lhs.derived_cast();
        const D2& drhs = rhs.derived_cast();
        return (dlhs && drhs && (dlhs.value() == drhs.value())) || (!dlhs && !drhs);

    }

    template <class D, not_ref_proxy T>
    bool operator==(const reference_proxy_base<D>& lhs, const T& rhs)
    {
        return lhs.derived_cast() && (lhs.derived_cast().value() == rhs);
    }

    template <class D>
    bool operator==(const reference_proxy_base<D>& lhs, std::nullopt_t)
    {
        return !lhs.derived_cast();
    }

    template <class D1, class D2>
    std::strong_ordering operator<=>(const reference_proxy_base<D1>& lhs, const reference_proxy_base<D2>& rhs)
    {
        const D1& dlhs = lhs.derived_cast();
        const D2& drhs = rhs.derived_cast();
        return (dlhs && drhs) ? (dlhs.value() <=> drhs.value()) : (dlhs.has_value() <=> drhs.has_value());
    }

    template <class D, not_ref_proxy T>
    std::strong_ordering operator<=>(const reference_proxy_base<D>& lhs, const T& rhs)
    {
        return lhs.derived_cast() ? (lhs.derived_cast().value() <=> rhs) : std::strong_ordering::less;
    }

    template <class D>
    std::strong_ordering operator<=>(const reference_proxy_base<D>& lhs, std::nullopt_t)
    {
        return lhs.derived_cast().has_value() <=> false;
    }

    /****************************************
     * const_reference_proxy implementation *
     ****************************************/

    template <class L>
    const_reference_proxy<L>::const_reference_proxy(const layout_type& l, size_type index)
        : p_layout(&l)
        , m_index(index)
    {
    }

    template <class L>
    bool const_reference_proxy<L>::has_value() const
    {
        return p_layout->has_value(m_index);
    }

    template <class L>
    const_reference_proxy<L>::operator bool() const
    {
        return has_value();
    }

    template <class L>
    auto const_reference_proxy<L>::value() const -> const value_type&
    {
        assert(has_value());
        return p_layout->value(m_index);
    }

    /**********************************
     * reference_proxy implementation *
     **********************************/

    template <class L>
    reference_proxy<L>::reference_proxy(layout_type& l, size_type index)
        : p_layout(&l)
        , m_index(index)
    {
    }

    template <class L>
    bool reference_proxy<L>::has_value() const
    {
        return p_layout->has_value(m_index);
    }

    template <class L>
    reference_proxy<L>::operator bool() const
    {
        return has_value();
    }

    template <class L>
    auto reference_proxy<L>::value() -> value_type&
    {
        assert(has_value());
        return p_layout->value(m_index);
    }

    template <class L>
    auto reference_proxy<L>::value() const -> const value_type&
    {
        assert(has_value());
        return p_layout->value(m_index);
    }

    template <class L>
    auto reference_proxy<L>::operator=(const self_type& rhs) -> self_type&
    {
        update(rhs);
        return *this;
    }

    template <class L>
    auto reference_proxy<L>::operator=(self_type&& rhs) -> self_type&
    {
        update(std::move(rhs));
        return *this;
    }

    template <class L>
    auto reference_proxy<L>::operator=(std::nullopt_t) -> self_type&
    {
        reset();
        return *this;
    }

    template <class L>
    template <std::convertible_to<typename L::inner_value_type> U>
    auto reference_proxy<L>::operator=(const std::optional<U>& rhs) -> self_type& 
    {
        update(rhs);
        return *this;
    }

    template <class L>
    template <std::convertible_to<typename L::inner_value_type> U>
    auto reference_proxy<L>::operator=(std::optional<U>&& rhs) -> self_type& 
    {
        update(std::move(rhs));
        return *this;
    }

    template <class L>
    template <std::convertible_to<typename L::inner_value_type> U>
    auto reference_proxy<L>::operator=(U&& value) -> self_type&
    {
        update_value(std::forward<U>(value));
        return *this;
    }

    template <class L>
    void reference_proxy<L>::reset()
    {
        p_layout->reset(m_index);
    }

    template <class L>
    void reference_proxy<L>::swap(self_type& rhs)
    {
        const bool this_has_value = has_value();
        const bool rhs_has_value = rhs.has_value();
        using std::swap;
        if (this_has_value)
        {
            if (rhs_has_value)
            {
                swap(p_layout->value(m_index), rhs.p_layout->value(rhs.m_index));
            }
            else
            {
                rhs.update_value(value());
                reset();
            }
        }
        else if (rhs_has_value)
        {
            update_value(rhs.value());
            rhs.reset();
        }
        // If none has value, nothing to do
    }

    template <class L>
    template <class U>
    void reference_proxy<L>::update(U&& u)
    {
        if (u.has_value())
        {
            update_value(std::forward<U>(u).value());
        }
        else
        {
            reset();
        }
    }

    template <class L>
    template <class U>
    void reference_proxy<L>::update_value(U&& u)
    {
        p_layout->update(m_index, std::forward<U>(u));
    }

    template <class L>
    void swap(reference_proxy<L>& lhs, reference_proxy<L>& rhs)
    {
        lhs.swap(rhs);
    }
}
