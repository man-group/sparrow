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

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/layout/mutable_array_base.hpp"

namespace sparrow
{
    /**
     * Base class for arrays using a validity buffer for
     * defining their bitmap.
     *
     * @tparam D The derived type, i.e. the inheriting class for which
     *           array_bitmap_base_impl provides the interface.
     * @tparam is_mutable A boolean indicating whether the validity buffer
     *                    is mutable.
     */
    template <class D, bool is_mutable>
    class array_bitmap_base_impl
        : public std::conditional_t<is_mutable, mutable_array_base<D>, array_crtp_base<D>>
    {
    public:

        using base_type = std::conditional_t<is_mutable, mutable_array_base<D>, array_crtp_base<D>>;

        using size_type = std::size_t;  // typename base_type::size_type;

        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_iterator = typename base_type::bitmap_iterator;
        using const_bitmap_iterator = typename base_type::const_bitmap_iterator;

        using bitmap_const_reference = typename base_type::bitmap_const_reference;
        using difference_type = typename base_type::difference_type;

        using const_bitmap_range = typename base_type::const_bitmap_range;

        using iterator_tag = typename base_type::iterator_tag;

    protected:

        array_bitmap_base_impl(arrow_proxy);

        array_bitmap_base_impl(const array_bitmap_base_impl&);
        array_bitmap_base_impl& operator=(const array_bitmap_base_impl&);

        array_bitmap_base_impl(array_bitmap_base_impl&&) noexcept = default;
        array_bitmap_base_impl& operator=(array_bitmap_base_impl&&) noexcept = default;

        bitmap_type& get_bitmap()
            requires is_mutable;
        const bitmap_type& get_bitmap() const;

        void resize_bitmap(size_type new_length)
            requires is_mutable;

        bitmap_iterator insert_bitmap(const_bitmap_iterator pos, bool value, size_type count)
            requires is_mutable;

        template <std::input_iterator InputIt>
            requires std::same_as<typename std::iterator_traits<InputIt>::value_type, bool>
        bitmap_iterator insert_bitmap(const_bitmap_iterator pos, InputIt first, InputIt last)
            requires is_mutable;

        bitmap_iterator erase_bitmap(const_bitmap_iterator pos, size_type count)
            requires is_mutable;

        void update()
            requires is_mutable;

        non_owning_dynamic_bitset<uint8_t> get_non_owning_dynamic_bitset();

        bitmap_type make_bitmap();

    private:

        bitmap_type m_bitmap;

        friend array_crtp_base<D>;
        friend mutable_array_base<D>;
    };

    /**
     * Convenient typedef to be used as a crtp base class for
     * arrays using an immutable validity buffer.
     *
     * @tparam D The derived type,
     */
    template <class D>
    using array_bitmap_base = array_bitmap_base_impl<D, false>;

    /**
     * Convenient typedef to be used as a crtp base class for
     * arrays using a mutable validity buffer.
     *
     * @tparam D The derived type,
     */
    template <class D>
    using mutable_array_bitmap_base = array_bitmap_base_impl<D, true>;

    /************************************
     * array_bitmap_base implementation *
     ************************************/

    template <class D, bool is_mutable>
    array_bitmap_base_impl<D, is_mutable>::array_bitmap_base_impl(arrow_proxy proxy_param)
        : base_type(std::move(proxy_param))
        , m_bitmap(make_bitmap())
    {
    }

    template <class D, bool is_mutable>
    array_bitmap_base_impl<D, is_mutable>::array_bitmap_base_impl(const array_bitmap_base_impl& rhs)
        : base_type(rhs)
        , m_bitmap(make_bitmap())
    {
    }

    template <class D, bool is_mutable>
    array_bitmap_base_impl<D, is_mutable>&
    array_bitmap_base_impl<D, is_mutable>::operator=(const array_bitmap_base_impl& rhs)
    {
        base_type::operator=(rhs);
        m_bitmap = make_bitmap();
        return *this;
    }

    template <class D, bool is_mutable>
    auto array_bitmap_base_impl<D, is_mutable>::get_bitmap() -> bitmap_type&
        requires is_mutable
    {
        return m_bitmap;
    }

    template <class D, bool is_mutable>
    auto array_bitmap_base_impl<D, is_mutable>::get_bitmap() const -> const bitmap_type&
    {
        return m_bitmap;
    }

    template <class D, bool is_mutable>
    auto array_bitmap_base_impl<D, is_mutable>::make_bitmap() -> bitmap_type
    {
        static constexpr size_t bitmap_buffer_index = 0;
        arrow_proxy& arrow_proxy = this->get_arrow_proxy();
        SPARROW_ASSERT_TRUE(arrow_proxy.buffers().size() > bitmap_buffer_index);
        const auto bitmap_size = arrow_proxy.length() + arrow_proxy.offset();
        return bitmap_type(arrow_proxy.buffers()[bitmap_buffer_index].data(), bitmap_size);
    }

    template <class D, bool is_mutable>
    void array_bitmap_base_impl<D, is_mutable>::resize_bitmap(size_type new_length)
        requires is_mutable
    {
        arrow_proxy& arrow_proxy = this->get_arrow_proxy();
        const size_t new_size = new_length + arrow_proxy.offset();
        arrow_proxy.resize_bitmap(new_size);
    }

    template <class D, bool is_mutable>
    auto
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
                 auto array_bitmap_base_impl<D, is_mutable>::insert_bitmap(
                     const_bitmap_iterator pos,
                     InputIt first,
                     InputIt last
                 ) -> bitmap_iterator
                     requires is_mutable
    {
        SPARROW_ASSERT_TRUE(this->bitmap_cbegin() <= pos)
        SPARROW_ASSERT_TRUE(pos <= this->bitmap_cend());
        SPARROW_ASSERT_TRUE(first <= last);
        const auto distance = static_cast<size_t>(std::distance(this->bitmap_cbegin(), pos));
        const auto idx = this->get_arrow_proxy().insert_bitmap(distance, std::ranges::subrange(first, last));
        return sparrow::next(this->bitmap_begin(), idx);
    }

    template <class D, bool is_mutable>
    auto
    array_bitmap_base_impl<D, is_mutable>::erase_bitmap(const_bitmap_iterator pos, size_type count) -> bitmap_iterator
        requires is_mutable
    {
        SPARROW_ASSERT_TRUE(this->bitmap_cbegin() <= pos)
        SPARROW_ASSERT_TRUE(pos < this->bitmap_cend())
        const auto pos_idx = static_cast<size_t>(std::distance(this->bitmap_cbegin(), pos));
        const auto idx = this->get_arrow_proxy().erase_bitmap(pos_idx, count);
        return sparrow::next(this->bitmap_begin(), idx);
    }

    template <class D, bool is_mutable>
    void array_bitmap_base_impl<D, is_mutable>::update()
        requires is_mutable
    {
        m_bitmap = make_bitmap();
    }
}
