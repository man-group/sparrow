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
#include <cstddef>
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
    /// If a raw pointer is passed to the constructor, the class does not own the object.
    /// If a rvalue is passed, the class takes the ownership of the object.
    /// In any other case, the class copies the object.
    class any_data
    {
    public:

        explicit any_data() noexcept = default;

        explicit any_data(std::nullptr_t) noexcept;

        template <class T>
        explicit any_data(T* data);

        template <mpl::unique_ptr_or_derived T>
        explicit any_data(T&& data);

        template <mpl::shared_ptr_or_derived T>
        explicit any_data(T data);

        template <class T>
        explicit any_data(T data)
            requires(!mpl::smart_ptr_and_derived<T>);

        template <class T>
        T* get();
        template <class T>
        const T* get() const;

        // Performs type-safe access to the contained object.
        template <class T>
        T value();

        template <class T>
        const T value() const;

        [[nodiscard]] bool owns_data() const noexcept;

        /// Return the type_index of the contained object.
        std::type_index type_id() const noexcept;

    private:

        std::any m_owner;
        void* m_raw_ptr = nullptr;
    };

    /// Stores or refers to a container and expose it's elements through raw pointers.
    class any_data_container
    {
    public:

        template <typename Tuple>
            requires mpl::is_type_instance_of_v<Tuple, std::tuple>
        explicit any_data_container(Tuple container);

        template <class T>
        explicit any_data_container(T** pointers);

        template <class T>
        explicit any_data_container(const std::vector<T*>& container);

        // In case where `container` is a range, raw pointers or shared pointers.
        template <std::ranges::input_range C>
            requires std::ranges::input_range<std::ranges::range_value_t<C>>
                     || mpl::shared_ptr_or_derived<std::ranges::range_value_t<C>>
        explicit any_data_container(C container);

        // In the case where `container` is a range of unique pointers, we have to transform them to
        // value_ptr.
        template <std::ranges::input_range C>
            requires mpl::unique_ptr_or_derived<std::ranges::range_value_t<C>>
        explicit any_data_container(C container);


        any_data_container(const any_data_container&) = delete;
        any_data_container& operator=(const any_data_container&) = delete;

        any_data_container(any_data_container&& other) noexcept;

        any_data_container& operator=(any_data_container&& other) noexcept;

        ~any_data_container() = default;


        [[nodiscard]] std::vector<void*>& get_pointers_vec() noexcept;

        [[nodiscard]] std::span<const void* const> get_pointers_vec() const noexcept;

        template <class T>
        [[nodiscard]] std::vector<T*> get_pointers_vec() noexcept;

        template <class T>
        [[nodiscard]] std::vector<const T*> get_pointers_vec() const noexcept;

        template <class T>
        [[nodiscard]] T** get() noexcept;

        template <class T>
        [[nodiscard]] const T** get() const noexcept;

        template <class T>
        [[nodiscard]] T value();

        template <class T>
        [[nodiscard]] const T value() const;

        [[nodiscard]] bool owns_data() const noexcept;

        /// Return the type_index of the container.
        std::type_index type_id() const noexcept;

    private:

        std::any m_owner;
        std::vector<void*> m_pointers_vec;
        void** m_raw_pointers = nullptr;
    };

    any_data::any_data(std::nullptr_t) noexcept
    {
    }

    template <typename T>
    any_data::any_data(T* data)
        : m_raw_ptr(data)
    {
    }

    template <mpl::unique_ptr_or_derived U>
    any_data::any_data(U&& data)
        : m_owner(value_ptr<typename U::element_type, typename U::deleter_type>(std::forward<U>(data)))
        , m_raw_ptr(std::any_cast<value_ptr<typename U::element_type, typename U::deleter_type>&>(m_owner).get())
    {
    }

    template <mpl::shared_ptr_or_derived U>
    any_data::any_data(U data)
        : m_owner(std::move(data))
        , m_raw_ptr(std::any_cast<U&>(m_owner).get())
    {
    }

    template <typename T>
    any_data::any_data(T data)
        requires(!mpl::smart_ptr_and_derived<T>)
        : m_owner(std::move(data))
        , m_raw_ptr(&std::any_cast<T&>(m_owner))
    {
    }

    template <typename T>
    T* any_data::get()
    {
        return static_cast<T*>(m_raw_ptr);
    }

    template <typename T>
    const T* any_data::get() const
    {
        return static_cast<T*>(m_raw_ptr);
    }

    template <class U>
    U any_data::value()
    {
        return std::any_cast<U>(m_owner);
    }

    template <class U>
    const U any_data::value() const
    {
        return std::any_cast<U>(m_owner);
    }

    bool any_data::owns_data() const noexcept
    {
        return m_owner.has_value();
    }

    std::type_index any_data::type_id() const noexcept
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

    template <typename Tuple>
        requires mpl::is_type_instance_of_v<Tuple, std::tuple>
    any_data_container::any_data_container(Tuple tuple)
        : m_owner(convert_tuple(std::move(tuple)))
        , m_pointers_vec(
              to_raw_ptr_vec<void>(std::any_cast<replace_unique_ptrs_by_value_ptrs_t<Tuple>&>(m_owner))
          )
        , m_raw_pointers(m_pointers_vec.data())
    {
    }

    template <typename T>
    any_data_container::any_data_container(T** pointer)
        : m_raw_pointers(reinterpret_cast<void**>(pointer))
    {
    }

    template <typename T>
    any_data_container::any_data_container(const std::vector<T*>& container)
        : m_pointers_vec(std::vector<void*>{std::begin(container), std::end(container)})
        , m_raw_pointers(m_pointers_vec.data())
    {
    }

    template <std::ranges::input_range C>
        requires std::ranges::input_range<std::ranges::range_value_t<C>>
                     || mpl::shared_ptr_or_derived<std::ranges::range_value_t<C>>
    any_data_container::any_data_container(C container)
        : m_owner(std::move(container))
        , m_pointers_vec(to_raw_ptr_vec<void>(std::any_cast<C&>(m_owner)))
        , m_raw_pointers(m_pointers_vec.data())
    {
    }

    template <std::ranges::input_range C>
        requires mpl::unique_ptr_or_derived<std::ranges::range_value_t<C>>
    any_data_container::any_data_container(C container)
        : m_owner(range_of_unique_ptr_to_vec_of_value_ptr(container))
        , m_pointers_vec(
              to_raw_ptr_vec<void>(std::any_cast<std::vector<sparrow::value_ptr<
                                       typename std::ranges::range_value_t<C>::element_type,
                                       typename std::ranges::range_value_t<C>::deleter_type>>&>(m_owner))
          )
        , m_raw_pointers(m_pointers_vec.data())
    {
    }

    any_data_container::any_data_container(any_data_container&& other) noexcept
        : m_owner(std::move(other.m_owner))
        , m_pointers_vec(std::move(other.m_pointers_vec))
        , m_raw_pointers(other.m_raw_pointers)
    {
        other.m_raw_pointers = nullptr;
    }

    any_data_container& any_data_container::operator=(any_data_container&& other) noexcept
    {
        if (this != &other)
        {
            m_owner = std::move(other.m_owner);
            m_pointers_vec = std::move(other.m_pointers_vec);
            m_raw_pointers = other.m_raw_pointers;
            other.m_raw_pointers = nullptr;
        }
        return *this;
    }

    std::vector<void*>& any_data_container::get_pointers_vec() noexcept
    {
        return m_pointers_vec;
    }

    std::span<const void* const> any_data_container::get_pointers_vec() const noexcept
    {
        return m_pointers_vec;
    }

    template <class T>
    std::vector<T*> any_data_container::get_pointers_vec() noexcept
    {
        std::vector<T*> pointers_vec;
        pointers_vec.reserve(m_pointers_vec.size());
        for (auto ptr : m_pointers_vec)
        {
            pointers_vec.emplace_back(static_cast<T*>(ptr));
        }
        return pointers_vec;
    }

    template <class T>
    std::vector<const T*> any_data_container::get_pointers_vec() const noexcept
    {
        std::vector<const T*> pointers_vec;
        pointers_vec.reserve(m_pointers_vec.size());
        for (auto ptr : m_pointers_vec)
        {
            pointers_vec.emplace_back(static_cast<const T*>(ptr));
        }
        return pointers_vec;
    }

    template <typename T>
    T** any_data_container::get() noexcept
    {
        return reinterpret_cast<T**>(m_raw_pointers);
    }

    template <typename T>
    const T** any_data_container::get() const noexcept
    {
        return reinterpret_cast<T**>(m_raw_pointers);
    }

    template <typename T>
    T any_data_container::value()
    {
        return std::any_cast<T>(m_owner);
    }

    template <typename T>
    const T any_data_container::value() const
    {
        return std::any_cast<T>(m_owner);
    }

    bool any_data_container::owns_data() const noexcept
    {
        return m_owner.has_value();
    }

    std::type_index any_data_container::type_id() const noexcept
    {
        return m_owner.type();
    }
}
