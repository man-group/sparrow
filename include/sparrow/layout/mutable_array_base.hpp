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

#include <concepts>

#include "sparrow/layout/array_base.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    /**
     * Base class definining common interface for arrays
     * with a bitmap.
     *
     * This class is a CRTP base class that defines and implements
     * common interface for arrays with a bitmap. The immutable
     * interface is inherited from \ref array_crtp_base.
     *
     * @tparam D The derived type, i.e. the inheriting class for which
     *           mutable_array_base provides the interface.
     */
    template <class D>
    class mutable_array_base : public array_crtp_base<D>
    {
    public:

        using self_type = mutable_array_base<D>;
        using base_type = array_crtp_base<D>;
        using derived_type = D;
        using inner_types = array_inner_types<derived_type>;

        using size_type = base_type::size_type;
        using difference_type = base_type::difference_type;

        using bitmap_type = typename inner_types::bitmap_type;
        using bitmap_reference = bitmap_type::reference;
        using bitmap_const_reference = bitmap_type::const_reference;
        using bitmap_iterator = bitmap_type::iterator;
        using bitmap_range = std::ranges::subrange<bitmap_iterator>;
        using const_bitmap_range = base_type::const_bitmap_range;

        using inner_value_type = typename base_type::inner_value_type;
        using value_type = typename base_type::value_type;

        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename base_type::inner_const_reference;

        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = base_type::const_reference;

        using value_iterator = typename inner_types::value_iterator;

        using iterator_tag = base_type::iterator_tag;

        struct iterator_types
        {
            using value_type = self_type::value_type;
            using reference = self_type::reference;
            using value_iterator = self_type::value_iterator;
            using bitmap_iterator = self_type::bitmap_iterator;
            using iterator_tag = self_type::iterator_tag;
        };

        using iterator = layout_iterator<iterator_types>;
        using const_iterator = base_type::const_iterator;

        reference operator[](size_type i);
        using base_type::operator[];

        iterator begin();
        iterator end();

        using base_type::begin;
        using base_type::end;

        void resize(size_type new_size, const value_type& value);

        iterator insert(const_iterator pos, const value_type& value);
        iterator insert(const_iterator pos, const value_type& value, size_type count);
        iterator insert(const_iterator pos, std::initializer_list<value_type> values);

        template <mpl::iterator_of_type<value_type> InputIt>
        iterator insert(const_iterator pos, InputIt first, InputIt last)
        {
            SPARROW_ASSERT_TRUE(pos >= this->cbegin())
            SPARROW_ASSERT_TRUE(pos <= this->cend());
            SPARROW_ASSERT_TRUE(first <= last);
            const difference_type distance = std::distance(this->cbegin(), pos);
            const auto validity_range = std::ranges::subrange(first, last)
                                        | std::views::transform(
                                            [](const value_type& obj)
                                            {
                                                return obj.has_value();
                                            }
                                        );
            auto& derived = this->derived_cast();
            derived.insert_bitmap(
                sparrow::next(this->bitmap_cbegin(), distance),
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
            derived.insert_values(
                sparrow::next(derived.value_cbegin(), distance),
                value_range.begin(),
                value_range.end()
            );
            const difference_type count = std::distance(first, last);
            // The following must be done after modifying the bitmap and values
            this->get_arrow_proxy().set_length(this->size() + static_cast<size_t>(count));

            derived.update();
            return sparrow::next(this->begin(), distance);
        }

        template <std::ranges::input_range R>
            requires std::same_as<std::ranges::range_value_t<R>, value_type>
        iterator insert(const_iterator pos, const R& range)
        {
            return insert(pos, std::ranges::begin(range), std::ranges::end(range));
        }

        iterator erase(const_iterator pos);
        iterator erase(const_iterator first, const_iterator last);

        void push_back(const value_type& value);
        void pop_back();

    protected:

        mutable_array_base(arrow_proxy);
        mutable_array_base(const mutable_array_base&) = default;
        mutable_array_base& operator=(const mutable_array_base&) = default;

        mutable_array_base(mutable_array_base&&) = default;
        mutable_array_base& operator=(mutable_array_base&&) = default;

        bitmap_reference has_value(size_type i);
        using base_type::has_value;

        bitmap_iterator bitmap_begin();
        bitmap_iterator bitmap_end();

        friend class layout_iterator<iterator_types>;
    };

    /*************************************
     * mutable_array_base implementation *
     *************************************/

    template <class D>
    mutable_array_base<D>::mutable_array_base(arrow_proxy proxy)
        : array_crtp_base<D>(std::forward<arrow_proxy>(proxy))
    {
    }

    /**
     * Returns an iterator to the first element of the array.
     */
    template <class D>
    auto mutable_array_base<D>::begin() -> iterator
    {
        return iterator(this->derived_cast().value_begin(), this->derived_cast().bitmap_begin());
    }

    /**
     * Returns a iterator to the element following the last
     * element of the array.
     */
    template <class D>
    auto mutable_array_base<D>::end() -> iterator
    {
        return iterator(this->derived_cast().value_end(), this->derived_cast().bitmap_end());
    }

    /**
     * Returns a reference to the element at the specified position
     * in the array.
     * @param i the index of the element in the array.
     */
    template <class D>
    auto mutable_array_base<D>::operator[](size_type i) -> reference
    {
        SPARROW_ASSERT_TRUE(i < this->size());
        return reference(inner_reference(this->derived_cast().value(i)), this->derived_cast().has_value(i));
    }

    template <class D>
    auto mutable_array_base<D>::has_value(size_type i) -> bitmap_reference
    {
        SPARROW_ASSERT_TRUE(i < this->size());
        return *sparrow::next(bitmap_begin(), i);
    }

    template <class D>
    auto mutable_array_base<D>::bitmap_begin() -> bitmap_iterator
    {
        return sparrow::next(this->derived_cast().get_bitmap().begin(), this->get_arrow_proxy().offset());
    }

    template <class D>
    auto mutable_array_base<D>::bitmap_end() -> bitmap_iterator
    {
        return sparrow::next(bitmap_begin(), this->size());
    }

    /**
     * Resizes the array to contain \c new_length elements, does nothing if <tt>new_length == size()</tt>.
     * If the current size is greater than \c new_length, the array is reduced to its first \c new_length
     * elements. If the current size is less than \c new_length, additional copies of \c values are appended.
     *
     * @param new_length The new size of the array.
     * @param value The value to initialize the new elements with.
     */
    template <class D>
    void mutable_array_base<D>::resize(size_type new_length, const value_type& value)
    {
        auto& derived = this->derived_cast();
        derived.resize_bitmap(new_length);
        derived.resize_values(new_length, value.get());
        this->get_arrow_proxy().set_length(new_length);  // Must be done after resizing the bitmap and values
        derived.update();
    }

    /**
     * Inserts a copy of \c value before \c pos in the array.
     *
     * @param pos The iterator before which the element will be inserted (\c pos may be the end() iterator).
     * @param value The element to insert.
     * @return An iterator pointing to the inserted value.
     */
    template <class D>
    auto mutable_array_base<D>::insert(const_iterator pos, const value_type& value) -> iterator
    {
        return insert(pos, value, 1);
    }

    /**
     * Inserts \c count copies of \c value before \c pos in the array.
     *
     * @param pos The iterator before which the elements will be inserted (\c pos may be the end() iterator).
     * @param value The element to insert.
     * @param count The number of elements to insert.
     * @return An iterator pointing to the first element inserted, or \c pos if <tt>count == 0</tt>.
     */
    template <class D>
    auto mutable_array_base<D>::insert(const_iterator pos, const value_type& value, size_type count) -> iterator
    {
        SPARROW_ASSERT_TRUE(pos >= this->cbegin());
        SPARROW_ASSERT_TRUE(pos <= this->cend());
        const size_t distance = static_cast<size_t>(std::distance(this->cbegin(), pos));
        auto& derived = this->derived_cast();
        derived.insert_bitmap(sparrow::next(this->bitmap_cbegin(), distance), value.has_value(), count);
        derived.insert_value(sparrow::next(derived.value_cbegin(), distance), value.get(), count);
        this->get_arrow_proxy().set_length(this->size() + count);  // Must be done after resizing the bitmap
                                                                   // and values
        derived.update();
        return sparrow::next(this->begin(), distance);
    }

    /**
     * Inserts elements from initializer list \c values before \c pos in the array.
     *
     * @param pos The iterator before which the elements will be inserted (\c pos may be the end() iterator).
     * @param values The <tt>std::initializer_list</tt> to insert the values from.
     * @return An iterator pointing to the first element inserted, or \c pos if \c values is empty.
     */
    template <class D>
    auto mutable_array_base<D>::insert(const_iterator pos, std::initializer_list<value_type> values) -> iterator
    {
        return insert(pos, values.begin(), values.end());
    }

    /**
     * Inserts elements from range [\c first , \c last ) before \c pos in the array.
     *
     * @param pos The iterator before which the elements will be inserted (\c pos may be the end() iterator).
     * @param first The iterator to the first element to insert.
     * @param last The iterator to the element following the last element to insert.
     * @return An iterator pointing to the first element inserted, or \c pos if <tt>first == last</tt>.
     */
    // template <class D>
    // template <mpl::iterator_of_type<typename mutable_array_base<D>::value_type> InputIt>
    // auto mutable_array_base<D>::insert(const_iterator pos, InputIt first, InputIt last) -> iterator
    // {
    //     SPARROW_ASSERT_TRUE(pos >= this->cbegin())
    //     SPARROW_ASSERT_TRUE(pos <= this->cend());
    //     SPARROW_ASSERT_TRUE(first <= last);
    //     const difference_type distance = std::distance(this->cbegin(), pos);
    //     const auto validity_range = std::ranges::subrange(first, last)
    //                                 | std::views::transform(
    //                                     [](const value_type& obj)
    //                                     {
    //                                         return obj.has_value();
    //                                     }
    //                                 );
    //     auto& derived = this->derived_cast();
    //     derived.insert_bitmap(
    //         sparrow::next(this->bitmap_cbegin(), distance),
    //         validity_range.begin(),
    //         validity_range.end()
    //     );

    //     const auto value_range = std::ranges::subrange(first, last)
    //                              | std::views::transform(
    //                                  [](const value_type& obj)
    //                                  {
    //                                      return obj.get();
    //                                  }
    //                              );
    //     derived.insert_values(
    //         sparrow::next(derived.value_cbegin(), distance),
    //         value_range.begin(),
    //         value_range.end()
    //     );
    //     const difference_type count = std::distance(first, last);
    //     // The following must be done after modifying the bitmap and values
    //     this->get_arrow_proxy().set_length(this->size() + static_cast<size_t>(count));

    //     derived.update();
    //     return sparrow::next(begin(), distance);
    // }

    /**
     * Inserts elements from range \c range before \c pos in the array.
     *
     * @tparam R the type of range to insert.
     * @param pos The iterator before which the elements will be inserted (\c pos may be the end() iterator).
     * @param range The range of values to insert.
     * @return An iterator pointing to the first element inserted, or \c pos if \c range is empty.
     */
    // template <class D>
    // template <std::ranges::input_range R>
    //     requires std::same_as<std::ranges::range_value_t<R>, typename mutable_array_base<D>::value_type>
    // auto mutable_array_base<D>::insert(const_iterator pos, const R& range) -> iterator
    // {
    //     return insert(pos, std::ranges::begin(range), std::ranges::end(range));
    // }

    /**
     * Removes the element at \c pos from the array.
     *
     * @param pos The iterator to the element to remove.
     * @return The iterator following the last element removed.
     */
    template <class D>
    auto mutable_array_base<D>::erase(const_iterator pos) -> iterator
    {
        SPARROW_ASSERT_TRUE(this->cbegin() <= pos)
        SPARROW_ASSERT_TRUE(pos < this->cend());
        return erase(pos, pos + 1);
    }

    /**
     * Removes the elemens in the range [\cfirst , \clast ) from the array.
     *
     * @param first The iterator to the first element to remove.
     * @param last The iterator to the element following the last element to remove.
     * @return The iterator following the last element removed.
     */
    template <class D>
    auto mutable_array_base<D>::erase(const_iterator first, const_iterator last) -> iterator
    {
        SPARROW_ASSERT_TRUE(first < last);
        SPARROW_ASSERT_TRUE(this->cbegin() <= first)
        SPARROW_ASSERT_TRUE(last <= this->cend());
        const difference_type first_index = std::distance(this->cbegin(), first);
        if (first == last)
        {
            return sparrow::next(begin(), first_index);
        }
        const auto count = static_cast<size_t>(std::distance(first, last));
        auto& derived = this->derived_cast();
        derived.erase_bitmap(sparrow::next(this->bitmap_cbegin(), first_index), count);
        derived.erase_values(sparrow::next(derived.value_cbegin(), first_index), count);
        this->get_arrow_proxy().set_length(this->size() - count);  // Must be done after modifying the bitmap
                                                                   // and values
        derived.update();
        return sparrow::next(begin(), first_index);
    }

    /**
     * Appends a copy of \c value to the end of the array.
     *
     * @param value The value o the element to append.
     */
    template <class D>
    void mutable_array_base<D>::push_back(const value_type& value)
    {
        insert(this->cend(), value);
    }

    /**
     * Removes the last element of the array.
     */
    template <class D>
    void mutable_array_base<D>::pop_back()
    {
        erase(std::prev(this->cend()));
    }
}
