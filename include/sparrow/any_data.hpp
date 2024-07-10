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

#include <any>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>

#include "sparrow/any_data_utils.hpp"


namespace sparrow
{
    /// A class that can own or not any object and expose it as a raw pointer.
    template <typename T>
    class any_data
    {
    public:

        explicit any_data(T* data);

        template <mpl::unique_ptr_or_derived U>
            requires std::same_as<typename U::element_type, T>
        explicit any_data(U&& data);

        template <mpl::shared_ptr_or_derived U>
            requires std::same_as<typename U::element_type, T>
        explicit any_data(U data);


        explicit any_data(T data)
            requires(!mpl::smart_ptr_and_derived<T>);

        T* get();
        const T* get() const;

        // Performs type-safe access to the contained object.
        template <class U>
        U get_data();

        template <class U>
        const U get_data() const;

        [[nodiscard]] bool owns_data() const noexcept;

        std::type_index type_id() const noexcept;

    private:

        std::any m_owner;
        T* m_raw_ptr = nullptr;
    };

    /**
     * @brief A class that can own or not a container of objects and expose them as raw pointers.
     *
     * @tparam T The type of the pointers.
     */
    template <typename T>
    class any_data_container
    {
    public:

        template <typename Tuple>
            requires mpl::is_type_instance_of_v<Tuple, std::tuple>
        explicit any_data_container(Tuple container);

        explicit any_data_container(T** container);

        explicit any_data_container(std::vector<T*> container);

        // In case where `container` is a range, raw pointers or shared pointers.
        template <std::ranges::input_range C>
            requires std::is_same_v<std::ranges::range_value_t<C>, T>
                     || std::is_same_v<std::ranges::range_value_t<C>, T*>
                     || std::ranges::input_range<std::ranges::range_value_t<C>>
                     || mpl::shared_ptr_or_derived<std::ranges::range_value_t<C>>
        explicit any_data_container(C container);

        // In the case where `container` is a range of unique pointers, we have to transform them to
        // value_ptr.
        template <std::ranges::input_range C>
            requires mpl::unique_ptr_or_derived<std::ranges::range_value_t<C>>
        explicit any_data_container(C container);

        any_data_container(const any_data_container<T>&) = delete;
        any_data_container& operator=(const any_data_container<T>&) = delete;

        any_data_container(any_data_container<T>&& other) noexcept;

        any_data_container& operator=(any_data_container<T>&& other) noexcept;

        ~any_data_container() = default;

        [[nodiscard]] std::vector<T*>& get_pointers_vec() noexcept;

        [[nodiscard]] std::span<const T* const> get_pointers_vec() const noexcept;

        [[nodiscard]] T** get() noexcept;

        [[nodiscard]] const T** get() const noexcept;

        template <class U>
        [[nodiscard]] U get_data();

        template <class U>
        [[nodiscard]] const U get_data() const;

        [[nodiscard]] bool owns_data() const noexcept;

        std::type_index type_id() const noexcept;

    private:

        std::any m_owner;
        std::vector<T*> m_pointers_vec;
        T** m_raw_pointers = nullptr;
    };

    template <typename T>
    any_data<T>::any_data(T* data)
        : m_raw_ptr(data)
    {
    }

    template <typename T>
    template <mpl::unique_ptr_or_derived U>
        requires std::same_as<typename U::element_type, T>
    any_data<T>::any_data(U&& data)
        : m_owner(value_ptr<typename U::element_type, typename U::deleter_type>(std::forward<U>(data)))
        , m_raw_ptr(std::any_cast<value_ptr<typename U::element_type, typename U::deleter_type>&>(m_owner).get())
    {
    }

    template <typename T>
    template <mpl::shared_ptr_or_derived U>
        requires std::same_as<typename U::element_type, T>
    any_data<T>::any_data(U data)
        : m_owner(std::move(data))
        , m_raw_ptr(std::any_cast<U&>(m_owner).get())
    {
    }

    template <typename T>
    any_data<T>::any_data(T data)
        requires(!mpl::smart_ptr_and_derived<T>)
        : m_owner(std::move(data))
        , m_raw_ptr(&std::any_cast<T&>(m_owner))
    {
    }

    template <typename T>
    T* any_data<T>::get()
    {
        return m_raw_ptr;
    }

    template <typename T>
    const T* any_data<T>::get() const
    {
        return m_raw_ptr;
    }

    template <typename T>
    template <class U>
    U any_data<T>::get_data()
    {
        return std::any_cast<U>(m_owner);
    }

    template <typename T>
    template <class U>
    const U any_data<T>::get_data() const
    {
        return std::any_cast<U>(m_owner);
    }

    template <typename T>
    bool any_data<T>::owns_data() const noexcept
    {
        return m_owner.has_value();
    }

    template <typename T>
    std::type_index any_data<T>::type_id() const noexcept
    {
        return m_owner.type();
    }

    /**
     * Convert a std::tuple to another. They must have the same size and be constructible with the same
     * values.
     *
     * @tparam Tuple The type of the tuple to convert.
     * @tparam NewTuple The type of the new tuple.
     * @param tuple The tuple to convert.
     * @return The new tuple.
     */
    template <class Tuple, class NewTuple = replace_unique_ptrs_by_value_ptrs_t<Tuple>>
        requires(std::tuple_size_v<Tuple> == std::tuple_size_v<NewTuple>)
    NewTuple convert_tuple(Tuple&& tuple)
    {
        NewTuple tuple_without_unique_ptr_instance{};
        std::apply(
            [&tuple_without_unique_ptr_instance](auto&&... args)
            {
                tuple_without_unique_ptr_instance = NewTuple{std::move(args)...};
            },
            tuple
        );
        return tuple_without_unique_ptr_instance;
    }

    template <typename T>
    template <typename Tuple>
        requires mpl::is_type_instance_of_v<Tuple, std::tuple>
    any_data_container<T>::any_data_container(Tuple tuple)
        : m_owner(convert_tuple(std::move(tuple)))
        , m_pointers_vec(to_raw_ptr_vec<T>(std::any_cast<replace_unique_ptrs_by_value_ptrs_t<Tuple>&>(m_owner)))
        , m_raw_pointers(m_pointers_vec.data())
    {
    }

    template <typename T>
    any_data_container<T>::any_data_container(T** container)
        : m_raw_pointers(container)
    {
    }

    template <typename T>
    any_data_container<T>::any_data_container(std::vector<T*> container)
        : m_pointers_vec(std::move(container))
        , m_raw_pointers(m_pointers_vec.data())
    {
    }

    template <typename T>
    template <std::ranges::input_range C>
        requires std::is_same_v<std::ranges::range_value_t<C>, T>
                     || std::is_same_v<std::ranges::range_value_t<C>, T*>
                     || std::ranges::input_range<std::ranges::range_value_t<C>>
                     || mpl::shared_ptr_or_derived<std::ranges::range_value_t<C>>
    any_data_container<T>::any_data_container(C container)
        : m_owner(std::move(container))
        , m_pointers_vec(to_raw_ptr_vec<T>(std::any_cast<C&>(m_owner)))
        , m_raw_pointers(m_pointers_vec.data())
    {
    }

    template <typename T>
    template <std::ranges::input_range C>
        requires mpl::unique_ptr_or_derived<std::ranges::range_value_t<C>>
    any_data_container<T>::any_data_container(C container)
        : m_owner(range_of_unique_ptr_to_vec_of_value_ptr(container))
        , m_pointers_vec(to_raw_ptr_vec<T>(std::any_cast<std::vector<sparrow::value_ptr<
                                               typename std::ranges::range_value_t<C>::element_type,
                                               typename std::ranges::range_value_t<C>::deleter_type>>&>(m_owner)))
        , m_raw_pointers(m_pointers_vec.data())
    {
    }

    template <typename T>
    any_data_container<T>::any_data_container(any_data_container<T>&& other) noexcept
        : m_owner(std::move(other.m_owner))
        , m_pointers_vec(std::move(other.m_pointers_vec))
        , m_raw_pointers(other.m_raw_pointers)
    {
        other.m_raw_pointers = nullptr;
    }

    template <typename T>
    any_data_container<T>& any_data_container<T>::operator=(any_data_container<T>&& other) noexcept
    {
        if (this != &other)
        {
            m_owner = std::move(other.m_owner);
            m_pointers_vec = std::move(other.m_pointers_vec);
            m_raw_pointers = other.m_raw_pointers;
            other.raw_pointers = nullptr;
        }
        return *this;
    }

    template <typename T>
    std::vector<T*>& any_data_container<T>::get_pointers_vec() noexcept
    {
        return m_pointers_vec;
    }

    template <typename T>
    std::span<const T* const> any_data_container<T>::get_pointers_vec() const noexcept
    {
        return m_pointers_vec;
    }

    template <typename T>
    T** any_data_container<T>::get() noexcept
    {
        return m_raw_pointers;
    }

    template <typename T>
    const T** any_data_container<T>::get() const noexcept
    {
        return m_raw_pointers;
    }

    template <typename T>
    template <typename U>
    U any_data_container<T>::get_data()
    {
        return std::any_cast<U>(m_owner);
    }

    template <typename T>
    template <typename U>
    const U any_data_container<T>::get_data() const
    {
        return std::any_cast<U>(m_owner);
    }

    template <typename T>
    bool any_data_container<T>::owns_data() const noexcept
    {
        return m_owner.has_value();
    }

    template <typename T>
    std::type_index any_data_container<T>::type_id() const noexcept
    {
        return m_owner.type();
    }
}
