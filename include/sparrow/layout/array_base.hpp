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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <utility>

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset_view.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/layout/layout_iterator.hpp"
#include "sparrow/utils/crtp_base.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{
    /**
     * Base class for array_inner_types specialization
     *
     * It defines common types used in the array implementation
     * classes.
     * */
    struct array_inner_types_base
    {
        using bitmap_type = dynamic_bitset_view<std::uint8_t>;
    };

    /**
     * Traits class that must be specialized by array
     * classes inheriting from array_crtp_base.
     *
     * @tparam D the class inheriting from array_crtp_base.
     */
    template <class D>
    struct array_inner_types;

    /**
     * Base class defining common immutable interface for arrays
     * with a bitmap.
     *
     * This class is a CRTP base class that defines and implements
     * common immutable interface for arrays with a bitmap. These
     * arrays hold nullable elements.
     *
     * @tparam D The derived type, i.e. the inheriting class for which
     *           array_crtp_base provides the interface.
     * @see nullable
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
        using bitmap_const_reference = bitmap_type::const_reference;
        using bitmap_iterator = bitmap_type::iterator;
        using const_bitmap_iterator = bitmap_type::const_iterator;
        using const_bitmap_range = std::ranges::subrange<const_bitmap_iterator>;

        using inner_value_type = typename inner_types::inner_value_type;
        using value_type = nullable<inner_value_type>;

        using inner_const_reference = typename inner_types::inner_const_reference;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;

        using const_value_iterator = typename inner_types::const_value_iterator;
        using const_value_range = std::ranges::subrange<const_value_iterator>;

        using iterator_tag = typename inner_types::iterator_tag;

        struct iterator_types
        {
            using value_type = self_type::value_type;
            using reference = self_type::const_reference;
            using value_iterator = self_type::const_value_iterator;
            using bitmap_iterator = self_type::const_bitmap_iterator;
            using iterator_tag = self_type::iterator_tag;
        };

        using const_iterator = layout_iterator<iterator_types>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        std::optional<std::string_view> name() const;
        std::optional<std::string_view> metadata() const;

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

        const_reverse_iterator rbegin() const;
        const_reverse_iterator rend() const;

        const_reverse_iterator crbegin() const;
        const_reverse_iterator crend() const;

        const_bitmap_range bitmap() const;
        const_value_range values() const;

        /**
         * Slices the array to keep only the elements between the given \p start and \p end.
         * A copy of the \ref array is modified. The data is not modified, only the ArrowArray.offset and
         * ArrowArray.length are updated. If \p end is greater than the size of the buffers, the following
         * elements will be invalid.
         *
         * @param start The index of the first element to keep. Must be less than \p end.
         * @param end The index of the first element to discard. Must be less than the size of the buffers.
         */
        D slice(size_type start, size_type end) const;

        /**
         * Slices the array to keep only the elements between the given \p start and \p end.
         * A view of the \ref array is returned. The data is not modified, only the ArrowArray.offset and
         * ArrowArray.length are updated. If \p end is greater than the size of the buffers, the following
         * elements will be invalid.
         *
         * @param start The index of the first element to keep. Must be less than \p end.
         * @param end The index of the first element to discard. Must be less than the size of the buffers.
         */
        D slice_view(size_type start, size_type end) const;

    protected:

        explicit array_crtp_base(arrow_proxy);

        array_crtp_base(const array_crtp_base&) = default;
        array_crtp_base& operator=(const array_crtp_base&) = default;

        array_crtp_base(array_crtp_base&&) = default;
        array_crtp_base& operator=(array_crtp_base&&) = default;

        [[nodiscard]] arrow_proxy& get_arrow_proxy();
        [[nodiscard]] const arrow_proxy& get_arrow_proxy() const;

        bitmap_const_reference has_value(size_type i) const;

        const_bitmap_iterator bitmap_begin() const;
        const_bitmap_iterator bitmap_end() const;

        const_bitmap_iterator bitmap_cbegin() const;
        const_bitmap_iterator bitmap_cend() const;

    private:

        arrow_proxy m_proxy;

        // friend classes
        friend class layout_iterator<iterator_types>;
        friend class detail::array_access;
#if defined(__cpp_lib_format)
        friend struct std::formatter<D>;
#endif
    };

    template <class D>
    bool operator==(const array_crtp_base<D>& lhs, const array_crtp_base<D>& rhs);

    /**********************************
     * array_crtp_base implementation *
     **********************************/

    template <class D>
    std::optional<std::string_view> array_crtp_base<D>::name() const
    {
        return get_arrow_proxy().name();
    }

    template <class D>
    std::optional<std::string_view> array_crtp_base<D>::metadata() const
    {
        return get_arrow_proxy().metadata();
    }

    /**
     * Checks if the array has no element, i.e. whether begin() == end().
     */
    template <class D>
    bool array_crtp_base<D>::empty() const
    {
        return size() == size_type(0);
    }

    /**
     * Returns the number of elements in the array.
     */
    template <class D>
    auto array_crtp_base<D>::size() const -> size_type
    {
        return static_cast<size_type>(get_arrow_proxy().length());
    }

    /**
     * Returns a constant reference to the element at the specified position
     * in the array with bounds checking.
     * @param i the index of the element in the array.
     * @throw std::out_of_range if \c i is not within the range of the container.
     */
    template <class D>
    auto array_crtp_base<D>::at(size_type i) const -> const_reference
    {
        if (i >= size())
        {
            std::ostringstream oss117;
            oss117 << "Index " << i << "is greater or equal to size of array (" << size() << ")";
            throw std::out_of_range(oss117.str());
        }
        return (*this)[i];
    }

    /**
     * Returns a constant reference to the element at the specified position
     * in the array.
     * @param i the index of the element in the array.
     */
    template <class D>
    auto array_crtp_base<D>::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < this->derived_cast().size());
        return const_reference(
            inner_const_reference(this->derived_cast().value(i)),
            this->derived_cast().has_value(i)
        );
    }

    /**
     * Returns a constant reference to the first element in the container.
     * Calling `front` on an empty container causes undefined behavior.
     */
    template <class D>
    auto array_crtp_base<D>::front() const -> const_reference
    {
        SPARROW_ASSERT_TRUE(!empty());
        return (*this)[size_type(0)];
    }

    /**
     * Returns a constant reference to the last element in the container.
     * Calling `back` on an empty container causes undefined behavior.
     */
    template <class D>
    auto array_crtp_base<D>::back() const -> const_reference
    {
        SPARROW_ASSERT_TRUE(!empty());
        return (*this)[size() - 1];
    }

    /**
     * Returns a constant iterator to the first element of the array.
     */
    template <class D>
    auto array_crtp_base<D>::begin() const -> const_iterator
    {
        return cbegin();
    }

    /**
     * Returns a constant iterator to the element following the last
     * element of the array.
     */
    template <class D>
    auto array_crtp_base<D>::end() const -> const_iterator
    {
        return cend();
    }

    /**
     * Returns a constant iterator to the first element of the array.
     * This method ensures that a constant iterator is returned, even
     * when called on a non-const array.
     */
    template <class D>
    auto array_crtp_base<D>::cbegin() const -> const_iterator
    {
        return const_iterator(this->derived_cast().value_cbegin(), bitmap_begin());
    }

    /**
     * Returns a constant iterator to the element following the last
     * element of the array. This method ensures that a constant iterator
     * is returned, even when called on a non-const array.
     */
    template <class D>
    auto array_crtp_base<D>::cend() const -> const_iterator
    {
        return const_iterator(this->derived_cast().value_cend(), bitmap_end());
    }

    /**
     * Returns a constant reverse iterator to the first element of the
     * reversed array. It corresponds to the last element of the non-
     * reversed array.
     */
    template <class D>
    auto array_crtp_base<D>::rbegin() const -> const_reverse_iterator
    {
        return crbegin();
    }

    /**
     * Returns a reverse iterator to the element following the last
     * element of the reversed array. It corresponds to the element
     * preceding the first element of the non-reversed array.
     */
    template <class D>
    auto array_crtp_base<D>::rend() const -> const_reverse_iterator
    {
        return crend();
    }

    /**
     * Returns a constant reverse iterator to the first element of the
     * reversed array. It corresponds to the last element of the non-
     * reversed array. This method ensures that a constant reverse
     * iterator is returned, even when called on a non-const array.
     */
    template <class D>
    auto array_crtp_base<D>::crbegin() const -> const_reverse_iterator
    {
        return const_reverse_iterator(cend());
    }

    /**
     * Returns a reverse iterator to the element following the last
     * element of the reversed array. It corresponds to the element
     * preceding the first element of the non-reversed array. This
     * method ensures that a constant reverse iterator is returned,
     * even when called on a non-const array.
     */
    template <class D>
    auto array_crtp_base<D>::crend() const -> const_reverse_iterator
    {
        return const_reverse_iterator(cbegin());
    }

    /**
     * Returns the validity bitmap of the array (i.e. the "has_value" part of the
     * nullable elements) as a constant range.
     */
    template <class D>
    auto array_crtp_base<D>::bitmap() const -> const_bitmap_range
    {
        return const_bitmap_range(bitmap_begin(), bitmap_end());
    }

    /**
     * Returns the raw values of the array (i.e. the "value" part og the nullable
     * elements) as a constant range.
     */
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
    auto array_crtp_base<D>::get_arrow_proxy() -> arrow_proxy&
    {
        return m_proxy;
    }

    template <class D>
    auto array_crtp_base<D>::get_arrow_proxy() const -> const arrow_proxy&
    {
        return m_proxy;
    }

    template <class D>
    auto array_crtp_base<D>::has_value(size_type i) const -> bitmap_const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return *sparrow::next(bitmap_begin(), i);
    }

    template <class D>
    auto array_crtp_base<D>::bitmap_begin() const -> const_bitmap_iterator
    {
        return sparrow::next(this->derived_cast().get_bitmap().cbegin(), get_arrow_proxy().offset());
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
    D array_crtp_base<D>::slice(size_type start, size_type end) const
    {
        SPARROW_ASSERT_TRUE(start <= end);
        return D{get_arrow_proxy().slice(start, end)};
    }

    template <class D>
    D array_crtp_base<D>::slice_view(size_type start, size_type end) const
    {
        SPARROW_ASSERT_TRUE(start <= end);
        return D{get_arrow_proxy().slice_view(start, end)};
    }

    /**
     * Checks if the contents of lhs and rhs are equal, that is, they have the same
     * number of elements and each element in lhs compares equal with the element
     * in rhs at the same position.
     *
     * @param lhs the first array to compare.
     * @param rhs the second array to compare.
     */
    template <class D>
    bool operator==(const array_crtp_base<D>& lhs, const array_crtp_base<D>& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }
}

#if defined(__cpp_lib_format)

template <typename D>
    requires std::derived_from<D, sparrow::array_crtp_base<D>>
struct std::formatter<D>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const D& ar, std::format_context& ctx) const
    {
        const auto& proxy = ar.get_arrow_proxy();
        std::string type;
        if (proxy.dictionary())
        {
            std::format_to(ctx.out(), "Dictionary<{}>", proxy.dictionary()->data_type());
        }
        else
        {
            std::format_to(ctx.out(), "{}", proxy.data_type());
        }
        std::format_to(ctx.out(), " [name={} | size={}] <", ar.name().value_or("nullptr"), proxy.length());

        std::for_each(
            ar.cbegin(),
            std::prev(ar.cend()),
            [&ctx](const auto& value)
            {
                std::format_to(ctx.out(), "{}, ", value);
            }
        );
        return std::format_to(ctx.out(), "{}>", ar.back());
    }
};

template <typename D>
    requires std::derived_from<D, sparrow::array_crtp_base<D>>
std::ostream& operator<<(std::ostream& os, const D& value)
{
    os << std::format("{}", value);
    return os;
}

#endif
