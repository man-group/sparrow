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

#include <climits>
#include <cstdint>
#include <vector>

#include "sparrow/buffer.hpp"
#include "sparrow/dynamic_bitset.hpp"
#include "sparrow/data_type.hpp"

namespace sparrow
{
    struct array_data
    {
        data_descriptor type;
        std::int64_t length = 0;
        std::int64_t offset = 0;
        using block_type = std::uint8_t;
        using bitmap_type = dynamic_bitset<block_type>; 
        // bitmap buffer and null_count
        bitmap_type bitmap;
        // Other buffers
        std::vector<buffer<block_type>> buffers;
        std::vector<array_data> child_data;
    };

    struct null_type
    {
    };
    constexpr null_type null;

    template <class T>
    class reference_proxy
    {
    public:

        using self_type = reference_proxy<T>;
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using size_type = std::size_t;

        reference_proxy(reference ref, size_type index, array_data& ad);
        reference_proxy(const self_type&) = default;
        reference_proxy(self_type&&) = default;

        self_type& operator=(const self_type&);
        self_type& operator=(self_type&&);
        self_type& operator=(null_type);

        template <class U>
        self_type& operator=(const U&);

        template <class U>
        self_type& operator+=(const U&);

        template <class U>
        self_type& operator-=(const U&);

        template <class U>
        self_type& operator*=(const U&);

        template <class U>
        self_type& operator/=(const U&);

        operator const_reference() const;

    private:

        void update_value(value_type value);
        void update_value(null_type);

        pointer p_ref;
        size_type m_index;
        array_data* p_array_data;
    };

    /**********************************
     * reference_proxy implementation *
     **********************************/

    template <class T>
    reference_proxy<T>::reference_proxy(reference ref, size_type index, array_data& ad)
        : p_ref(&ref)
        , m_index(index)
        , p_array_data(&ad)
    {
    }

    template <class T>
    auto reference_proxy<T>::operator=(const self_type& rhs) -> self_type&
    {
        update_value(*rhs.p_ref);
        return *this;
    }

    template <class T>
    auto reference_proxy<T>::operator=(self_type&& rhs) -> self_type&
    {
        update_value(*rhs.p_ref);
        return *this;
    }

    template <class T>
    auto reference_proxy<T>::operator=(null_type rhs) -> self_type&
    {
        update_value(std::move(rhs));
        return *this;
    }

    template <class T>
    template <class U>
    auto reference_proxy<T>::operator=(const U& u) -> self_type& 
    {
        update_value(u);
        return *this;
    }

    template <class T>
    template <class U>
    auto reference_proxy<T>::operator+=(const U& u) -> self_type& 
    {
        update_value(*p_ref + u);
        return *this;
    }


    template <class T>
    template <class U>
    auto reference_proxy<T>::operator-=(const U& u) -> self_type& 
    {
        update_value(*p_ref - u);
        return *this;
    }

    template <class T>
    template <class U>
    auto reference_proxy<T>::operator*=(const U& u) -> self_type& 
    {
        update_value(*p_ref * u);
        return *this;
    }

    template <class T>
    template <class U>
    auto reference_proxy<T>::operator/=(const U& u) -> self_type& 
    {
        update_value(*p_ref / u);
        return *this;
    }

    template <class T>
    reference_proxy<T>::operator const_reference() const
    {
        return *p_ref;
    }

    template <class T>
    void reference_proxy<T>::update_value(value_type value)
    {
        auto& bitmap = p_array_data->bitmap;
        bitmap.set(m_index, true);
        *p_ref = value;
    }

    template <class T>
    void reference_proxy<T>::update_value(null_type)
    {
        auto& bitmap = p_array_data->bitmap;
        bitmap.set(m_index, false);
    }
}

