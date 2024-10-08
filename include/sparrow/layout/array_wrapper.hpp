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

#include <memory>
#include <variant>

#include "sparrow/types/data_type.hpp"
#include "sparrow/utils/memory.hpp"

namespace sparrow
{
    /**
     * Base class for array type erasure
     */
    class array_wrapper_base
    {
    public:

        virtual ~array_wrapper_base() = default;

        array_wrapper_base(array_wrapper_base&&) = delete;
        array_wrapper_base& operator=(const array_wrapper_base&) = delete;
        array_wrapper_base& operator=(array_wrapper_base&&) = delete;

        array_wrapper_base* clone() const;

        enum data_type data_type() const;

    protected:

        array_wrapper_base(enum data_type dt);
        array_wrapper_base(const array_wrapper_base&) = default;

    private:

        enum data_type m_data_type;
        virtual array_wrapper_base* clone_impl() const = 0;
    };

    template <class T>
    class array_wrapper : public array_wrapper_base
    {
    public:

        array_wrapper(T&& ar);
        array_wrapper(T* ar);
        array_wrapper(std::shared_ptr<T> ar);

        virtual ~array_wrapper() = default;

        T& get_wrapped();
        const T& get_wrapped() const;
        
    private:

        array_wrapper(const array_wrapper&);
        array_wrapper* clone_impl() const override;

        using storage_type = std::variant<value_ptr<T>, std::shared_ptr<T>, T*>;
        storage_type m_storage;
        T* p_array;
    };

    template <class T>
    T& unwrap_array(array_wrapper_base&);

    template <class T>
    const T& unwrap_array(const array_wrapper_base&);

    /*************************************
     * array_wrapper_base implementation *
     *************************************/

    inline array_wrapper_base* array_wrapper_base::clone() const
    {
        return clone_impl();
    }

    inline enum data_type array_wrapper_base::data_type() const
    {
        return m_data_type;
    }

    inline array_wrapper_base::array_wrapper_base(enum data_type dt)
        : m_data_type(dt)
    {
    }

    /********************************
     * array_wrapper implementation *
     ********************************/

    template <class T>
    array_wrapper<T>::array_wrapper(T&& ar)
        : array_wrapper_base(ar.data_type())
        , m_storage(value_ptr<T>(std::move(ar)))
        , p_array(std::get<value_ptr<T>>(m_storage).get())
    {
    }

    template <class T>
    array_wrapper<T>::array_wrapper(T* ar)
        : array_wrapper_base(ar->data_type())
        , m_storage(ar)
        , p_array(ar)
    {
    }

    template <class T>
    array_wrapper<T>::array_wrapper(std::shared_ptr<T> ar)
        : array_wrapper_base(ar->data_type())
        , m_storage(ar)
        , p_array(ar.get())
    {
    }

    template <class T>
    T& array_wrapper<T>::get_wrapped()
    {
        return *p_array;
    }

    template <class T>
    const T& array_wrapper<T>::get_wrapped() const
    {
        return *p_array;
    }

    template <class T>
    array_wrapper<T>::array_wrapper(const array_wrapper& rhs)
        : array_wrapper_base(rhs)
        , m_storage(rhs.m_storage)
    {
        p_array = std::visit([](auto&& arg)
        {
            using U = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<U, T*>)
                return arg;
            else
                return arg.get();
        }, m_storage);
    }

    template <class T>
    array_wrapper<T>* array_wrapper<T>::clone_impl() const
    {
        return new array_wrapper<T>(*this);
    }

    template <class T>
    T& unwrap_array(array_wrapper_base& ar)
    {
        return static_cast<array_wrapper<T>&>(ar).get_wrapped();
    }

    template <class T>
    const T& unwrap_array(const array_wrapper_base& ar)
    {
        return static_cast<const array_wrapper<T>&>(ar).get_wrapped();
    }
}

