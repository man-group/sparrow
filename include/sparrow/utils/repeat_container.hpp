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

#include <functional>
#include <ranges>
#include <type_traits>
#include <utility>

#include "sparrow/utils/iterator.hpp"

namespace sparrow
{
    namespace detail
    {
        template <typename T>
        struct repeat_view_storage_traits
        {
            using value_type = T;
            using storage_type = T;
            using constructor_reference = const value_type&;

            [[nodiscard]] static constexpr const value_type& get(const storage_type& value) noexcept
            {
                return value;
            }
        };

        template <typename T>
        struct repeat_view_storage_traits<T&>
        {
            using value_type = std::remove_reference_t<T>;
            using storage_type = std::reference_wrapper<value_type>;
            using constructor_reference = value_type&;

            [[nodiscard]] static constexpr const value_type& get(const storage_type& value) noexcept
            {
                return value.get();
            }
        };

        template <typename T>
        struct repeat_view_storage_traits<const T&>
        {
            using value_type = std::remove_cv_t<T>;
            using storage_type = std::reference_wrapper<const std::remove_reference_t<T>>;
            using constructor_reference = const std::remove_reference_t<T>&;

            [[nodiscard]] static constexpr const value_type& get(const storage_type& value) noexcept
            {
                return value.get();
            }
        };
    }

    template <typename T>
    class repeat_view_iterator
        : public sparrow::iterator_base<repeat_view_iterator<T>, const T, std::random_access_iterator_tag>
    {
    public:

        using self_type = repeat_view_iterator<T>;
        using base_type = sparrow::iterator_base<self_type, const T, std::random_access_iterator_tag>;

        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;

        constexpr repeat_view_iterator() = default;

        /**
         * Constructs a repeat_view_iterator.
         * @param value The value to repeat
         * @param index The index of the iterator, representing the current position in the repeated sequence
         */
        constexpr repeat_view_iterator(const T& value, size_t index);

    private:

        [[nodiscard]] constexpr reference dereference() const noexcept;
        constexpr void increment() noexcept;
        constexpr void decrement() noexcept;
        constexpr void advance(difference_type n) noexcept;
        [[nodiscard]] constexpr difference_type distance_to(const self_type& rhs) const noexcept;
        [[nodiscard]] constexpr bool equal(const self_type& rhs) const noexcept;
        [[nodiscard]] constexpr bool less_than(const self_type& rhs) const noexcept;

        const T* m_value = nullptr;
        size_t m_index = 0;

        friend class iterator_access;
    };

    /**
     * A view that repeats a value a given number of times.
     */
    template <typename T>
    class repeat_view : public std::ranges::view_interface<repeat_view<T>>
    {
    public:

        using storage_traits = detail::repeat_view_storage_traits<T>;
        using value_type = typename storage_traits::value_type;
        using self_type = repeat_view<T>;
        using const_iterator = repeat_view_iterator<value_type>;

        /**
         * Constructs a repeat_view.
         * @param value The value to repeat
         * @param count The number of times to repeat the value
         */
        constexpr repeat_view(typename storage_traits::constructor_reference value, size_t count) noexcept(
            std::is_nothrow_constructible_v<typename storage_traits::storage_type, typename storage_traits::constructor_reference>
        );

        constexpr repeat_view(value_type&& value, size_t count) noexcept(
            std::is_nothrow_move_constructible_v<value_type>
        )
            requires(!std::is_reference_v<T>);

        constexpr const_iterator begin() const noexcept;
        constexpr const_iterator end() const noexcept;
        constexpr const_iterator cbegin() const noexcept;
        constexpr const_iterator cend() const noexcept;
        [[nodiscard]] constexpr size_t size() const noexcept;

    private:

        [[nodiscard]] constexpr const value_type& stored_value() const noexcept;

        typename storage_traits::storage_type m_value;
        size_t m_count = 0;
    };

    template <typename T>
    repeat_view(T&, size_t) -> repeat_view<T&>;

    template <typename T>
    repeat_view(T&&, size_t) -> repeat_view<std::remove_cvref_t<T>>;

    template <typename T>
    constexpr repeat_view_iterator<T>::repeat_view_iterator(const T& value, size_t index)
        : m_value(&value)
        , m_index(index)
    {
    }

    template <typename T>
    constexpr auto repeat_view_iterator<T>::dereference() const noexcept -> reference
    {
        return *m_value;
    }

    template <typename T>
    constexpr void repeat_view_iterator<T>::increment() noexcept
    {
        ++m_index;
    }

    template <typename T>
    constexpr void repeat_view_iterator<T>::decrement() noexcept
    {
        --m_index;
    }

    template <typename T>
    constexpr void repeat_view_iterator<T>::advance(difference_type n) noexcept
    {
        m_index += n;
    }

    template <typename T>
    constexpr auto repeat_view_iterator<T>::distance_to(const self_type& rhs) const noexcept -> difference_type
    {
        return static_cast<difference_type>(rhs.m_index - m_index);
    }

    template <typename T>
    constexpr bool repeat_view_iterator<T>::equal(const self_type& rhs) const noexcept
    {
        return m_index == rhs.m_index;
    }

    template <typename T>
    constexpr bool repeat_view_iterator<T>::less_than(const self_type& rhs) const noexcept
    {
        return m_index < rhs.m_index;
    }

    template <typename T>
    constexpr repeat_view<T>::repeat_view(typename storage_traits::constructor_reference value, size_t count) noexcept(
        std::is_nothrow_constructible_v<typename storage_traits::storage_type, typename storage_traits::constructor_reference>
    )
        : m_value(value)
        , m_count(count)
    {
    }

    template <typename T>
    constexpr repeat_view<T>::repeat_view(value_type&& value, size_t count) noexcept(
        std::is_nothrow_move_constructible_v<value_type>
    )
        requires(!std::is_reference_v<T>)
        : m_value(std::move(value))
        , m_count(count)
    {
    }

    template <typename T>
    [[nodiscard]] constexpr const typename repeat_view<T>::value_type&
    repeat_view<T>::stored_value() const noexcept
    {
        return storage_traits::get(m_value);
    }

    template <typename T>
    constexpr auto repeat_view<T>::begin() const noexcept -> const_iterator
    {
        return const_iterator(stored_value(), 0);
    }

    template <typename T>
    constexpr auto repeat_view<T>::end() const noexcept -> const_iterator
    {
        return const_iterator(stored_value(), m_count);
    }

    template <typename T>
    constexpr auto repeat_view<T>::cbegin() const noexcept -> const_iterator
    {
        return const_iterator(stored_value(), 0);
    }

    template <typename T>
    constexpr auto repeat_view<T>::cend() const noexcept -> const_iterator
    {
        return const_iterator(stored_value(), m_count);
    }

    template <typename T>
    [[nodiscard]] constexpr size_t repeat_view<T>::size() const noexcept
    {
        return m_count;
    }

}
