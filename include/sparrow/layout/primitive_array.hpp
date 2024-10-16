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
#include "sparrow/layout/array_base.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{   
    template <class T>
    class primitive_array;

    template <class T>
    struct array_inner_types<primitive_array<T>> : array_inner_types_base
    {
        using array_type = primitive_array<T>;

        using inner_value_type = T;
        using inner_reference = T&;
        using inner_const_reference = const T&;
        using pointer = inner_value_type*;
        using const_pointer = const inner_value_type*;

        using value_iterator = pointer_iterator<pointer>;
        using const_value_iterator = pointer_iterator<const_pointer>;

        using iterator_tag = std::random_access_iterator_tag;
    };

    template <class T>
    class primitive_array final : public array_crtp_base<primitive_array<T>>
    {
    public:

        using self_type = primitive_array<T>;
        using base_type = array_crtp_base<self_type>;
        using inner_types = array_inner_types<self_type>;
        using inner_value_type = typename inner_types::inner_value_type;
        using inner_reference = typename inner_types::inner_reference;
        using inner_const_reference = typename inner_types::inner_const_reference;
        using bitmap_type = typename base_type::bitmap_type;
        using bitmap_reference = typename base_type::bitmap_reference;
        using bitmap_const_reference = typename base_type::bitmap_const_reference;
        using value_type = nullable<inner_value_type>;
        using reference = nullable<inner_reference, bitmap_reference>;
        using const_reference = nullable<inner_const_reference, bitmap_const_reference>;
        using pointer = typename inner_types::pointer;
        using const_pointer = typename inner_types::const_pointer;
        using size_type = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using iterator_tag = typename base_type::iterator_tag;

        using value_iterator = typename base_type::value_iterator;
        using bitmap_range = typename base_type::bitmap_range;
        using const_value_iterator = typename base_type::const_value_iterator;
        using const_bitmap_range = typename base_type::const_bitmap_range;

        explicit primitive_array(arrow_proxy);

        using base_type::size;

    private:

        using base_type::storage;

        pointer data();
        const_pointer data() const;

        inner_reference value(size_type i);
        inner_const_reference value(size_type i) const;

        value_iterator value_begin();
        value_iterator value_end();

        const_value_iterator value_cbegin() const;
        const_value_iterator value_cend() const;

        static constexpr size_type DATA_BUFFER_INDEX = 1;

        friend class array_crtp_base<self_type>;
        friend class run_end_encoded_array;
    };

    /**********************************
     * primitive_array implementation *
     **********************************/

    namespace detail
    {
        inline bool check_primitive_data_type(data_type dt)
        {
            constexpr std::array<data_type, 14> dtypes = {
                data_type::BOOL,
                data_type::UINT8,
                data_type::INT8,
                data_type::UINT16,
                data_type::INT16,
                data_type::UINT32,
                data_type::INT32,
                data_type::UINT64,
                data_type::INT64,
                data_type::HALF_FLOAT,
                data_type::FLOAT,
                data_type::DOUBLE,
                data_type::FIXED_SIZE_BINARY,
                data_type::TIMESTAMP
            };
            return std::find(dtypes.cbegin(), dtypes.cend(), dt) != dtypes.cend();
        }
    }

    template <class T>
    primitive_array<T>::primitive_array(arrow_proxy proxy)
        : base_type(std::move(proxy))
    {
        SPARROW_ASSERT_TRUE(detail::check_primitive_data_type(storage().data_type()));
    }

    template <class T>
    auto primitive_array<T>::data() -> pointer
    {
        return storage().buffers()[DATA_BUFFER_INDEX].template data<inner_value_type>()
               + static_cast<size_type>(storage().offset());
    }

    template <class T>
    auto primitive_array<T>::data() const -> const_pointer
    {
        return storage().buffers()[DATA_BUFFER_INDEX].template data<const inner_value_type>()
               + static_cast<size_type>(storage().offset());
    }

    template <class T>
    auto primitive_array<T>::value(size_type i) -> inner_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return data()[i];
    }

    template <class T>
    auto primitive_array<T>::value(size_type i) const -> inner_const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return data()[i];
    }

    template <class T>
    auto primitive_array<T>::value_begin() -> value_iterator
    {
        return value_iterator{data()};
    }

    template <class T>
    auto primitive_array<T>::value_end() -> value_iterator
    {
        return sparrow::next(value_begin(), size());
    }

    template <class T>
    auto primitive_array<T>::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator{data()};
    }

    template <class T>
    auto primitive_array<T>::value_cend() const -> const_value_iterator
    {
        return sparrow::next(value_cbegin(), size());
    }
}
