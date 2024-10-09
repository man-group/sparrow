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

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <utility>

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset_view.hpp"
#include "sparrow/layout/layout_iterator.hpp"
#include "sparrow/utils/crtp_base.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{
    /**
     * Make a simple bitmap from an arrow proxy.
     */
    [[nodiscard]] inline dynamic_bitset_view<uint8_t> make_simple_bitmap(arrow_proxy& arrow_proxy)
    {
        constexpr size_t bitmap_buffer_index = 0;
        SPARROW_ASSERT_TRUE(arrow_proxy.buffers().size() > bitmap_buffer_index);
        const auto bitmap_size = arrow_proxy.length() + arrow_proxy.offset();
        return {arrow_proxy.buffers()[bitmap_buffer_index].data(), bitmap_size};
    }

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
    class array_crtp_base : public crtp_base<D>
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
        using bitmap_range = std::ranges::subrange<bitmap_iterator>;
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

        [[nodiscard]] size_type size() const;

        reference operator[](size_type i);
        const_reference operator[](size_type i) const;

        iterator begin();
        iterator end();

        const_iterator begin() const;
        const_iterator end() const;

        const_iterator cbegin() const;
        const_iterator cend() const;

        const_bitmap_range bitmap() const;
        const_value_range values() const;

        void resize(size_type new_size, const value_type& value);

        iterator insert(const_iterator pos, const value_type& value);
        iterator insert(const_iterator pos, const value_type& value, size_type count);
        iterator insert(const_iterator pos, std::initializer_list<value_type> values);
        template <typename InputIt>
        iterator insert(const_iterator pos, InputIt first, InputIt last);
        template <std::ranges::input_range R>
        iterator insert(const_iterator pos, const R& range);

        iterator erase(const_iterator pos);
        iterator erase(const_iterator first, const_iterator last);

        void push_back(const value_type& value);
        void pop_back();

    protected:

        array_crtp_base(arrow_proxy);

        array_crtp_base(const array_crtp_base&) = default;
        array_crtp_base& operator=(const array_crtp_base&) = default;

        array_crtp_base(array_crtp_base&&) = default;
        array_crtp_base& operator=(array_crtp_base&&) = default;

        const arrow_proxy& storage() const;
        arrow_proxy& storage();

        bitmap_reference has_value(size_type i);
        bitmap_const_reference has_value(size_type i) const;

        bitmap_iterator bitmap_begin();
        bitmap_iterator bitmap_end();

        const_bitmap_iterator bitmap_begin() const;
        const_bitmap_iterator bitmap_end() const;

        const_bitmap_iterator bitmap_cbegin() const;
        const_bitmap_iterator bitmap_cend() const;

    private:

        arrow_proxy& get_arrow_proxy();

        arrow_proxy m_proxy;

        // friend classes
        friend class layout_iterator<self_type, false>;
        friend class layout_iterator<self_type, true>;
        template <class T>
        friend class array_wrapper_impl;
    };

    template <class D>
    bool operator==(const array_crtp_base<D>& lhs, const array_crtp_base<D>& rhs);

    /*
     * Base class for arrays using a validity buffer for
     * defining their bitmap.
     */
    template <class D>
    class array_bitmap_base : public array_crtp_base<D>
    {
    public:

        using base_type = array_crtp_base<D>;
        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_iterator = typename base_type::bitmap_iterator;
        using const_bitmap_iterator = typename base_type::const_bitmap_iterator;
        using size_type = typename base_type::size_type;

    protected:

        array_bitmap_base(arrow_proxy);

        array_bitmap_base(const array_bitmap_base&);
        array_bitmap_base& operator=(const array_bitmap_base&);

        array_bitmap_base(array_bitmap_base&&) = default;
        array_bitmap_base& operator=(array_bitmap_base&&) = default;

        bitmap_type& get_bitmap();
        const bitmap_type& get_bitmap() const;

        void resize_bitmap(size_type new_length);

        bitmap_iterator insert_bitmap(const_bitmap_iterator pos, bool value, size_type count);

        template <std::input_iterator InputIt>
            requires std::same_as<typename std::iterator_traits<InputIt>::value_type, bool>
        bitmap_iterator insert_bitmap(const_bitmap_iterator pos, InputIt first, InputIt last);

        bitmap_iterator erase_bitmap(const_bitmap_iterator pos, size_type count);

        void update();

    private:

        non_owning_dynamic_bitset<uint8_t> get_non_owning_dynamic_bitset();

        bitmap_type make_bitmap();
        bitmap_type m_bitmap;
    };

    /**********************************
     * array_crtp_base implementation *
     **********************************/

    template <class D>
    auto array_crtp_base<D>::size() const -> size_type
    {
        return static_cast<size_type>(storage().length());
    }

    template <class D>
    auto array_crtp_base<D>::operator[](size_type i) -> reference
    {
        SPARROW_ASSERT_TRUE(i < this->derived_cast().size());
        return reference(inner_reference(this->derived_cast().value(i)), this->derived_cast().has_value(i));
    }

    template <class D>
    auto array_crtp_base<D>::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < this->derived_cast().size());
        return const_reference(
            inner_const_reference(this->derived_cast().value(i)),
            this->derived_cast().has_value(i)
        );
    }

    template <class D>
    auto array_crtp_base<D>::begin() -> iterator
    {
        return iterator(this->derived_cast().value_begin(), this->derived_cast().bitmap_begin());
    }

    template <class D>
    auto array_crtp_base<D>::end() -> iterator
    {
        return iterator(this->derived_cast().value_end(), this->derived_cast().bitmap_end());
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
        return const_iterator(this->derived_cast().value_cbegin(), bitmap_begin());
    }

    template <class D>
    auto array_crtp_base<D>::cend() const -> const_iterator
    {
        return const_iterator(this->derived_cast().value_cend(), bitmap_end());
    }

    template <class D>
    auto array_crtp_base<D>::bitmap() const -> const_bitmap_range
    {
        return const_bitmap_range(bitmap_begin(), bitmap_end());
    }

    template <class D>
    auto array_crtp_base<D>::values() const -> const_value_range
    {
        return const_value_range(this->derived_cast().value_cbegin(), this->derived_cast().value_cend());
    }

    template <class D>
    array_crtp_base<D>::array_crtp_base(arrow_proxy proxy)
        : m_proxy(std::move(proxy))
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
        return *sparrow::next(bitmap_begin(), i);
    }

    template <class D>
    auto array_crtp_base<D>::has_value(size_type i) const -> bitmap_const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return *sparrow::next(bitmap_begin(), i);
    }

    template <class D>
    auto array_crtp_base<D>::bitmap_begin() -> bitmap_iterator
    {
        return sparrow::next(this->derived_cast().get_bitmap().begin(), storage().offset());
    }

    template <class D>
    auto array_crtp_base<D>::bitmap_end() -> bitmap_iterator
    {
        return sparrow::next(bitmap_begin(), size());
    }

    template <class D>
    auto array_crtp_base<D>::bitmap_begin() const -> const_bitmap_iterator
    {
        return sparrow::next(this->derived_cast().get_bitmap().cbegin(), storage().offset());
    }

    template <class D>
    auto array_crtp_base<D>::bitmap_end() const -> const_bitmap_iterator
    {
        return sparrow::next(bitmap_begin(), size());
    }

    template <class D>
    auto array_crtp_base<D>::bitmap_cbegin() const -> const_bitmap_iterator
    {
        return bitmap_begin();
    }

    template <class D>
    auto array_crtp_base<D>::bitmap_cend() const -> const_bitmap_iterator
    {
        return bitmap_end();
    }

    template <class D>
    auto array_crtp_base<D>::get_arrow_proxy() -> arrow_proxy&
    {
        return m_proxy;
    }

    template <class D>
    bool operator==(const array_crtp_base<D>& lhs, const array_crtp_base<D>& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }

    template <class D>
    void array_crtp_base<D>::resize(size_type new_length, const value_type& value)
    {
        this->derived_cast().resize_bitmap(new_length);
        this->derived_cast().resize_values(new_length, value.get());
        m_proxy.set_length(new_length); // Must be done after resizing the bitmap and values
        this->derived_cast().update();
    }

    template <class D>
    auto array_crtp_base<D>::insert(const_iterator pos, const value_type& value) -> iterator
    {
        return insert(pos, value, 1);
    }

    template <class D>
    auto array_crtp_base<D>::insert(const_iterator pos, const value_type& value, size_type count) -> iterator
    {
        SPARROW_ASSERT_TRUE(pos >= cbegin());
        SPARROW_ASSERT_TRUE(pos <= cend());
        const size_t distance = static_cast<size_t>(std::distance(cbegin(), pos));
        this->derived_cast().insert_bitmap(sparrow::next(this->bitmap_cbegin(), distance), value.has_value(), count);
        this->derived_cast()
            .insert_value(sparrow::next(this->derived_cast().value_cbegin(), distance), value.get(), count);
        m_proxy.set_length(size() + count); // Must be done after resizing the bitmap and values
        this->derived_cast().update();
        return sparrow::next(begin(), distance);
    }

    template <class D>
    auto array_crtp_base<D>::insert(const_iterator pos, std::initializer_list<value_type> values) -> iterator
    {
        return insert(pos, values.begin(), values.end());
    }

    template <class D>
    template <typename InputIt>
    auto array_crtp_base<D>::insert(const_iterator pos, InputIt first, InputIt last) -> iterator
    {
        SPARROW_ASSERT_TRUE(pos >= cbegin())
        SPARROW_ASSERT_TRUE(pos <= cend());
        SPARROW_ASSERT_TRUE(first <= last);
        const difference_type distance = std::distance(cbegin(), pos);
        const auto validity_range = std::ranges::subrange(first, last)
                                    | std::views::transform(
                                        [](const value_type& obj)
                                        {
                                            return obj.has_value();
                                        }
                                    );
        this->derived_cast().insert_bitmap(
            sparrow::next(bitmap_cbegin(), distance),
            validity_range.begin(),
            validity_range.end()
        );

        const auto value_range = std::ranges::subrange(first, last)
                                 | std::views::transform(
                                     [](const value_type& obj)
                                     {
                                         return obj.get();
                                     }
                                 );
        this->derived_cast().insert_values(
            sparrow::next(this->derived_cast().value_cbegin(), distance),
            value_range.begin(),
            value_range.end()
        );
        const difference_type count = std::distance(first, last);
        m_proxy.set_length(size() + static_cast<size_t>(count)); // Must be done after modifying the bitmap and values
        this->derived_cast().update();
        return sparrow::next(begin(), distance);
    }

    template <class D>
    template <std::ranges::input_range R>
    auto array_crtp_base<D>::insert(const_iterator pos, const R& range) -> iterator
    {
        return insert(pos, std::ranges::begin(range), std::ranges::end(range));
    }

    template <class D>
    auto array_crtp_base<D>::erase(const_iterator pos) -> iterator
    {
        SPARROW_ASSERT_TRUE(cbegin() <= pos)
        SPARROW_ASSERT_TRUE(pos < cend());
        return erase(pos, pos + 1);
    }

    template <class D>
    auto array_crtp_base<D>::erase(const_iterator first, const_iterator last) -> iterator
    {
        SPARROW_ASSERT_TRUE(first < last);
        SPARROW_ASSERT_TRUE(cbegin() <= first)
        SPARROW_ASSERT_TRUE(last <= cend());
        const difference_type first_index = std::distance(cbegin(), first);
        if (first == last)
        {
            return sparrow::next(begin(), first_index);
        }
        const auto count = static_cast<size_t>(std::distance(first, last));
        this->derived_cast().erase_bitmap(sparrow::next(bitmap_cbegin(), first_index), count);
        this->derived_cast().erase_values(sparrow::next(this->derived_cast().value_cbegin(), first_index), count);
        m_proxy.set_length(size() - count); // Must be done after modifying the bitmap and values
        this->derived_cast().update();
        return sparrow::next(begin(), first_index);
    }

    template <class D>
    void array_crtp_base<D>::push_back(const value_type& value)
    {
        insert(cend(), value);
    }

    template <class D>
    void array_crtp_base<D>::pop_back()
    {
        erase(std::prev(cend()));
    }

    /************************************
     * array_bitmap_base implementation *
     ************************************/

    template <class D>
    array_bitmap_base<D>::array_bitmap_base(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , m_bitmap(make_bitmap())
    {
    }

    template <class D>
    array_bitmap_base<D>::array_bitmap_base(const array_bitmap_base& rhs)
        : base_type(rhs)
        , m_bitmap(make_bitmap())
    {
    }

    template <class D>
    array_bitmap_base<D>& array_bitmap_base<D>::operator=(const array_bitmap_base& rhs)
    {
        base_type::operator=(rhs);
        m_bitmap = make_bitmap();
        return *this;
    }

    template <class D>
    auto array_bitmap_base<D>::get_bitmap() -> bitmap_type&
    {
        return m_bitmap;
    }

    template <class D>
    auto array_bitmap_base<D>::get_bitmap() const -> const bitmap_type&
    {
        return m_bitmap;
    }

    template <class D>
    auto array_bitmap_base<D>::make_bitmap() -> bitmap_type
    {
        static constexpr size_t bitmap_buffer_index = 0;
        SPARROW_ASSERT_TRUE(this->storage().buffers().size() > bitmap_buffer_index);
        const auto bitmap_size = static_cast<std::size_t>(this->storage().length() + this->storage().offset());
        return bitmap_type(this->storage().buffers()[bitmap_buffer_index].data(), bitmap_size);
    }

    template <class D>
    void array_bitmap_base<D>::resize_bitmap(size_type new_length)
    {
        const size_t new_size = new_length + static_cast<size_t>(this->storage().offset());
        this->storage().resize_bitmap(new_size);
    }

    template <class D>
    auto
    array_bitmap_base<D>::insert_bitmap(const_bitmap_iterator pos, bool value, size_type count) -> bitmap_iterator
    {
        SPARROW_ASSERT_TRUE(this->bitmap_cbegin() <= pos)
        SPARROW_ASSERT_TRUE(pos <= this->bitmap_cend())
        const auto pos_index = static_cast<size_t>(std::distance(this->bitmap_cbegin(), pos));
        const auto idx = this->storage().insert_bitmap(pos_index, value, count);
        return sparrow::next(this->bitmap_begin(), idx);
    }

    template <class D>
    template <std::input_iterator InputIt>
        requires std::same_as<typename std::iterator_traits<InputIt>::value_type, bool>
    auto
    array_bitmap_base<D>::insert_bitmap(const_bitmap_iterator pos, InputIt first, InputIt last) -> bitmap_iterator
    {
        SPARROW_ASSERT_TRUE(this->bitmap_cbegin() <= pos)
        SPARROW_ASSERT_TRUE(pos <= this->bitmap_cend());
        SPARROW_ASSERT_TRUE(first <= last);
        const auto distance = static_cast<size_t>(std::distance(this->bitmap_cbegin(), pos));
        const auto idx = this->storage().insert_bitmap(distance, first, last);
        return sparrow::next(this->bitmap_begin(), idx);
    }

    template <class D>
    auto array_bitmap_base<D>::erase_bitmap(const_bitmap_iterator pos, size_type count) -> bitmap_iterator
    {
        SPARROW_ASSERT_TRUE(this->bitmap_cbegin() <= pos)
        SPARROW_ASSERT_TRUE(pos < this->bitmap_cend())
        const auto pos_idx = static_cast<size_t>(std::distance(this->bitmap_cbegin(), pos));
        const auto idx = this->storage().erase_bitmap(pos_idx, count);
        return sparrow::next(this->bitmap_begin(), idx);
    }

    template <class D>
    auto array_bitmap_base<D>::update() -> void
    {
        m_bitmap = make_bitmap();
    }
}
