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

#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/layout/mutable_array_base.hpp"

namespace sparrow
{
    /**
     * @brief Base class template for arrays using a validity bitmap to track null values.
     *
     * This CRTP base class provides common functionality for array implementations that use
     * an Arrow-compatible validity bitmap to track which elements are null. The bitmap is
     * stored as the first buffer in the Arrow array layout and uses one bit per element,
     * where 1 indicates a valid (non-null) value and 0 indicates a null value.
     *
     * The class supports both mutable and immutable bitmap operations based on the template
     * parameter, allowing for efficient read-only access or full modification capabilities.
     *
     * @tparam D The derived type (CRTP pattern) - the actual array implementation class
     * @tparam is_mutable Boolean flag indicating whether the validity buffer should be mutable
     *
     * @pre D must be a complete array implementation type
     * @pre Arrow proxy must contain valid bitmap buffer at index 0
     * @post Maintains Arrow array layout compatibility
     * @post Provides efficient bitmap operations with O(1) access patterns
     * @post Thread-safe for read operations, requires external synchronization for writes
     *
     * @example
     * ```cpp
     * // For immutable bitmap
     * class my_array : public array_bitmap_base<my_array> { ... };
     *
     * // For mutable bitmap
     * class my_mutable_array : public mutable_array_bitmap_base<my_mutable_array> { ... };
     * ```
     */
    template <class D, bool is_mutable>
    class array_bitmap_base_impl
        : public std::conditional_t<is_mutable, mutable_array_base<D>, array_crtp_base<D>>
    {
    public:

        using base_type = std::conditional_t<is_mutable, mutable_array_base<D>, array_crtp_base<D>>;

        using size_type = std::size_t;

        using bitmap_type = typename base_type::bitmap_type;
        using const_bitmap_type = typename base_type::const_bitmap_type;
        using bitmap_iterator = typename base_type::bitmap_iterator;
        using const_bitmap_iterator = typename base_type::const_bitmap_iterator;

        using bitmap_const_reference = typename base_type::bitmap_const_reference;
        using difference_type = typename base_type::difference_type;

        using const_bitmap_range = typename base_type::const_bitmap_range;

        using iterator_tag = typename base_type::iterator_tag;

    protected:

        /**
         * @brief Constructs array bitmap base from Arrow proxy.
         *
         * @param proxy Arrow proxy containing array data and schema with validity bitmap
         *
         * @pre proxy must contain valid Arrow array and schema
         * @pre proxy.buffers().size() must be > 0 (validity bitmap at index 0)
         * @pre proxy validity bitmap must be properly sized for the array length
         * @post Array is initialized with data from proxy
         * @post Bitmap is constructed from the validity buffer
         * @post Base class is properly initialized
         *
         * @note Internal assertion: SPARROW_ASSERT_TRUE(arrow_proxy.buffers().size() > bitmap_buffer_index)
         */
        array_bitmap_base_impl(arrow_proxy proxy);

        /**
         * @brief Copy constructor.
         *
         * @param rhs Source array to copy from
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post Bitmap is reconstructed from copied Arrow data
         * @post Base class is properly copied
         */
        constexpr array_bitmap_base_impl(const array_bitmap_base_impl&);

        /**
         * @brief Copy assignment operator.
         *
         * @param rhs Source array to copy from
         * @return Reference to this array
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post Previous data is properly released
         * @post Bitmap is reconstructed from copied Arrow data
         */
        constexpr array_bitmap_base_impl& operator=(const array_bitmap_base_impl&);

        constexpr array_bitmap_base_impl(array_bitmap_base_impl&&) noexcept = default;
        constexpr array_bitmap_base_impl& operator=(array_bitmap_base_impl&&) noexcept = default;

        /**
         * @brief Gets mutable reference to the validity bitmap.
         *
         * @return Mutable reference to the bitmap
         *
         * @pre Array must be constructed with is_mutable = true
         * @post Returns valid reference to the bitmap for modification
         * @post Bitmap modifications affect null status of array elements
         *
         * @note Only available when is_mutable is true
         */
        [[nodiscard]] constexpr bitmap_type& get_bitmap()
            requires is_mutable;

        /**
         * @brief Gets const reference to the validity bitmap.
         *
         * @return Const reference to the bitmap
         *
         * @post Returns valid const reference to the bitmap
         * @post Bitmap can be read but not modified through this reference
         */
        [[nodiscard]] constexpr const const_bitmap_type& get_bitmap() const;

        /**
         * @brief Resizes the validity bitmap to accommodate new array length.
         *
         * @param new_length New length for the array
         * @param value Default validity value for new elements (true = valid, false = null)
         *
         * @pre Array must be constructed with is_mutable = true
         * @pre new_length must be a valid array size
         * @post Bitmap is resized to accommodate new_length + offset elements
         * @post New elements (if any) have validity set to value parameter
         * @post Array offset is preserved during resize operation
         *
         * @note Only available when is_mutable is true
         */
        constexpr void resize_bitmap(size_type new_length, bool value)
            requires is_mutable;

        /**
         * @brief Inserts validity bits at specified position.
         *
         * @param pos Iterator position where to insert validity bits
         * @param value Validity value for inserted bits (true = valid, false = null)
         * @param count Number of bits to insert
         * @return Iterator pointing to first inserted bit
         *
         * @pre Array must be constructed with is_mutable = true
         * @pre pos must be valid iterator within [bitmap_cbegin(), bitmap_cend()]
         * @pre count must be >= 0
         * @post count validity bits with value are inserted at pos
         * @post Bitmap size increases by count
         * @post Array offset is properly handled during insertion
         * @post Returns iterator to first inserted element
         *
         * @note Internal assertions:
         *   - SPARROW_ASSERT_TRUE(this->bitmap_cbegin() <= pos)
         *   - SPARROW_ASSERT_TRUE(pos <= this->bitmap_cend())
         * @note Only available when is_mutable is true
         */
        constexpr bitmap_iterator insert_bitmap(const_bitmap_iterator pos, bool value, size_type count)
            requires is_mutable;

        /**
         * @brief Inserts range of validity bits at specified position.
         *
         * @tparam InputIt Input iterator type for boolean values
         * @param pos Iterator position where to insert validity bits
         * @param first Iterator to beginning of range to insert
         * @param last Iterator to end of range to insert
         * @return Iterator pointing to first inserted bit
         *
         * @pre Array must be constructed with is_mutable = true
         * @pre InputIt must be input iterator with value_type of bool
         * @pre pos must be valid iterator within [bitmap_cbegin(), bitmap_cend()]
         * @pre [first, last) must be valid range
         * @post All validity bits in [first, last) are inserted at pos
         * @post Bitmap size increases by distance(first, last)
         * @post Array offset is properly handled during insertion
         * @post Returns iterator to first inserted element
         *
         * @note Internal assertions:
         *   - SPARROW_ASSERT_TRUE(this->bitmap_cbegin() <= pos)
         *   - SPARROW_ASSERT_TRUE(pos <= this->bitmap_cend())
         *   - SPARROW_ASSERT_TRUE(first <= last)
         * @note Only available when is_mutable is true
         */
        template <std::input_iterator InputIt>
            requires std::same_as<typename std::iterator_traits<InputIt>::value_type, bool>
        constexpr bitmap_iterator insert_bitmap(const_bitmap_iterator pos, InputIt first, InputIt last)
            requires is_mutable;

        /**
         * @brief Erases validity bits starting at specified position.
         *
         * @param pos Iterator position where to start erasing validity bits
         * @param count Number of bits to erase
         * @return Iterator pointing to bit after the erased range
         *
         * @pre Array must be constructed with is_mutable = true
         * @pre pos must be valid iterator within [bitmap_cbegin(), bitmap_cend())
         * @pre count must be >= 0
         * @pre pos + count must not exceed bitmap_cend()
         * @post count validity bits starting at pos are removed
         * @post Bitmap size decreases by count
         * @post Array offset is properly handled during erasure
         * @post Returns iterator to element after erased range
         *
         * @note Internal assertions:
         *   - SPARROW_ASSERT_TRUE(this->bitmap_cbegin() <= pos)
         *   - SPARROW_ASSERT_TRUE(pos < this->bitmap_cend())
         * @note Only available when is_mutable is true
         */
        constexpr bitmap_iterator erase_bitmap(const_bitmap_iterator pos, size_type count)
            requires is_mutable;

    private:

        friend array_crtp_base<D>;
        friend mutable_array_base<D>;
    };

    /**
     * @brief Convenient alias for arrays with immutable validity bitmaps.
     *
     * This alias creates an array_bitmap_base_impl with is_mutable = false,
     * providing read-only access to the validity bitmap.
     *
     * @tparam D The derived array type
     */
    template <class D>
    using array_bitmap_base = array_bitmap_base_impl<D, false>;

    /**
     * @brief Convenient alias for arrays with mutable validity bitmaps.
     *
     * This alias creates an array_bitmap_base_impl with is_mutable = true,
     * providing full read-write access to the validity bitmap including
     * resize, insert, and erase operations.
     *
     * @tparam D The derived array type
     */
    template <class D>
    using mutable_array_bitmap_base = array_bitmap_base_impl<D, true>;

    /************************************
     * array_bitmap_base implementation *
     ************************************/

    template <class D, bool is_mutable>
    array_bitmap_base_impl<D, is_mutable>::array_bitmap_base_impl(arrow_proxy proxy_param)
        : base_type(std::move(proxy_param))
    {
    }

    template <class D, bool is_mutable>
    constexpr array_bitmap_base_impl<D, is_mutable>::array_bitmap_base_impl(const array_bitmap_base_impl& rhs)
        : base_type(rhs)
    {
    }

    template <class D, bool is_mutable>
    constexpr array_bitmap_base_impl<D, is_mutable>&
    array_bitmap_base_impl<D, is_mutable>::operator=(const array_bitmap_base_impl& rhs)
    {
        base_type::operator=(rhs);
        return *this;
    }

    template <class D, bool is_mutable>
    constexpr auto array_bitmap_base_impl<D, is_mutable>::get_bitmap() -> bitmap_type&
        requires is_mutable
    {
        arrow_proxy& arrow_proxy = this->get_arrow_proxy();
        SPARROW_ASSERT_TRUE(arrow_proxy.bitmap().has_value());
        return *arrow_proxy.bitmap();
    }

    template <class D, bool is_mutable>
    constexpr auto array_bitmap_base_impl<D, is_mutable>::get_bitmap() const -> const const_bitmap_type&
    {
        const arrow_proxy& proxy = this->get_arrow_proxy();
        SPARROW_ASSERT_TRUE(proxy.const_bitmap().has_value());
        return *proxy.const_bitmap();
    }

    template <class D, bool is_mutable>
    constexpr void array_bitmap_base_impl<D, is_mutable>::resize_bitmap(size_type new_length, bool value)
        requires is_mutable
    {
        this->get_arrow_proxy().resize_bitmap(new_length, value);
    }

    template <class D, bool is_mutable>
    constexpr auto
    array_bitmap_base_impl<D, is_mutable>::insert_bitmap(const_bitmap_iterator pos, bool value, size_type count)
        -> bitmap_iterator
        requires is_mutable
    {
        SPARROW_ASSERT_TRUE(this->bitmap_cbegin() <= pos)
        SPARROW_ASSERT_TRUE(pos <= this->bitmap_cend())
        const auto pos_index = static_cast<size_t>(std::distance(this->bitmap_cbegin(), pos));
        const auto idx = this->get_arrow_proxy().insert_bitmap(pos_index, value, count);
        return sparrow::next(this->bitmap_begin(), idx);
    }

    template <class D, bool is_mutable>
    template <std::input_iterator InputIt>
        requires std::same_as<typename std::iterator_traits<InputIt>::value_type, bool>
    constexpr auto
    array_bitmap_base_impl<D, is_mutable>::insert_bitmap(const_bitmap_iterator pos, InputIt first, InputIt last)
        -> bitmap_iterator
        requires is_mutable
    {
        SPARROW_ASSERT_TRUE(this->bitmap_cbegin() <= pos)
        SPARROW_ASSERT_TRUE(pos <= this->bitmap_cend());
        SPARROW_ASSERT_TRUE(first <= last);
        const auto pos_index = static_cast<size_t>(std::distance(this->bitmap_cbegin(), pos));
        const auto idx = this->get_arrow_proxy().insert_bitmap(pos_index, std::ranges::subrange(first, last));
        return sparrow::next(this->bitmap_begin(), idx);
    }

    template <class D, bool is_mutable>
    constexpr auto
    array_bitmap_base_impl<D, is_mutable>::erase_bitmap(const_bitmap_iterator pos, size_type count)
        -> bitmap_iterator
        requires is_mutable
    {
        SPARROW_ASSERT_TRUE(this->bitmap_cbegin() <= pos)
        SPARROW_ASSERT_TRUE(pos < this->bitmap_cend())
        arrow_proxy& arrow_proxy = this->get_arrow_proxy();
        const auto pos_idx = static_cast<size_t>(std::distance(this->bitmap_cbegin(), pos));
        const auto idx = arrow_proxy.erase_bitmap(pos_idx, count);
        return sparrow::next(this->bitmap_begin(), idx);
    }
}
