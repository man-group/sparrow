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

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/buffer/buffer_adaptor.hpp"
#include "sparrow/layout/array_access.hpp"

namespace sparrow
{
    template <typename T>
    concept trivial_copyable_type = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

    namespace details
    {
        /**
         * @brief Data access class for trivial copyable types.
         * FOR INTERNAL USE ONLY
         * @tparam T Type of the data.
         */
        template <trivial_copyable_type T>
        class primitive_data_access
        {
        public:

            using inner_value_type = T;
            using inner_reference = T&;
            using inner_const_reference = const T&;
            using inner_pointer = inner_value_type*;
            using inner_const_pointer = const inner_value_type*;

            using value_iterator = pointer_iterator<inner_pointer>;
            using const_value_iterator = pointer_iterator<inner_const_pointer>;

            /**
             * @brief Constructor for primitive_data_access.
             * @param layout Pointer to the layout object.
             * @param data_buffer_index Index of the data buffer.
             */
            primitive_data_access(arrow_proxy& proxy, size_t data_buffer_index);

            // This class is meant to be use as a private member of array classes,
            // and holds a inner_pointer to the arrow_proxy of the array. Therefore we
            // forbid the copy and the move semantics to:
            // - force the array constructors to call the primitive_data_access
            // constructor taking an arrow_proxy
            // - force the arra assignment operators to call the reset_proxy
            // method.
            primitive_data_access(const primitive_data_access&) = delete;
            primitive_data_access& operator=(const primitive_data_access&) = delete;
            primitive_data_access(primitive_data_access&&) = delete;
            primitive_data_access& operator=(primitive_data_access&&) = delete;

            [[nodiscard]] constexpr inner_pointer data();
            [[nodiscard]] constexpr inner_const_pointer data() const;

            [[nodiscard]] constexpr inner_reference value(size_t i);
            [[nodiscard]] constexpr inner_const_reference value(size_t i) const;

            [[nodiscard]] constexpr value_iterator value_begin();
            [[nodiscard]] constexpr value_iterator value_end();

            [[nodiscard]] constexpr const_value_iterator value_cbegin() const;
            [[nodiscard]] constexpr const_value_iterator value_cend() const;

            constexpr void resize_values(size_t new_length, const T& value);

            constexpr value_iterator insert_value(const_value_iterator pos, T value, size_t count);
            constexpr value_iterator insert_value(size_t idx, T value, size_t count);

            // Template parameter InputIt must be an value_iterator type that iterates over elements of type T
            template <mpl::iterator_of_type<T> InputIt>
            constexpr value_iterator insert_values(const_value_iterator pos, InputIt first, InputIt last);

            template <mpl::iterator_of_type<T> InputIt>
            constexpr value_iterator insert_values(size_t idx, InputIt first, InputIt last);

            constexpr value_iterator erase_values(const_value_iterator pos, size_t count);
            constexpr value_iterator erase_values(size_t idx, size_t count);

            void reset_proxy(arrow_proxy& proxy);

        private:

            [[nodiscard]] constexpr buffer_adaptor<T, buffer<uint8_t>&> get_data_buffer();

            [[nodiscard]] arrow_proxy& get_proxy();
            [[nodiscard]] const arrow_proxy& get_proxy() const;

            arrow_proxy* p_proxy;
            size_t m_data_buffer_index;
        };

        /****************************************
         * primitiva_data_access implementation *
         ****************************************/

        template <trivial_copyable_type T>
        primitive_data_access<T>::primitive_data_access(arrow_proxy& proxy, size_t data_buffer_index)
            : p_proxy(&proxy)
            , m_data_buffer_index(data_buffer_index)
        {
        }

        template <trivial_copyable_type T>
        [[nodiscard]] constexpr auto primitive_data_access<T>::data() -> inner_pointer
        {
            return get_proxy().buffers()[m_data_buffer_index].template data<T>()
                   + static_cast<size_t>(get_proxy().offset());
        }

        template <trivial_copyable_type T>
        [[nodiscard]] constexpr auto primitive_data_access<T>::data() const -> inner_const_pointer
        {
            return get_proxy().buffers()[m_data_buffer_index].template data<T>()
                   + static_cast<size_t>(get_proxy().offset());
        }

        template <trivial_copyable_type T>
        [[nodiscard]] constexpr auto primitive_data_access<T>::value(size_t i) -> inner_reference
        {
            SPARROW_ASSERT_TRUE(i < get_proxy().length());
            return data()[i];
        }

        template <trivial_copyable_type T>
        [[nodiscard]] constexpr auto primitive_data_access<T>::value(size_t i) const -> inner_const_reference
        {
            SPARROW_ASSERT_TRUE(i < get_proxy().length());
            return data()[i];
        }

        template <trivial_copyable_type T>
        [[nodiscard]] constexpr auto primitive_data_access<T>::value_begin() -> value_iterator
        {
            return value_iterator{data()};
        }

        template <trivial_copyable_type T>
        [[nodiscard]] constexpr auto primitive_data_access<T>::value_end() -> value_iterator
        {
            return sparrow::next(value_begin(), get_proxy().length());
        }

        template <trivial_copyable_type T>
        [[nodiscard]] constexpr auto primitive_data_access<T>::value_cbegin() const -> const_value_iterator
        {
            return const_value_iterator{data()};
        }

        template <trivial_copyable_type T>
        [[nodiscard]] constexpr auto primitive_data_access<T>::value_cend() const -> const_value_iterator
        {
            return sparrow::next(value_cbegin(), get_proxy().length());
        }

        template <trivial_copyable_type T>
        constexpr void primitive_data_access<T>::resize_values(size_t new_length, const T& value)
        {
            const size_t new_size = new_length + static_cast<size_t>(get_proxy().offset());
            get_data_buffer().resize(new_size, value);
        }

        template <trivial_copyable_type T>
        constexpr auto primitive_data_access<T>::insert_value(const_value_iterator pos, T value, size_t count)
            -> value_iterator
        {
            const const_value_iterator value_cbegin{data()};
            const const_value_iterator value_cend{sparrow::next(value_cbegin, get_proxy().length())};
            SPARROW_ASSERT_TRUE(value_cbegin <= pos);
            SPARROW_ASSERT_TRUE(pos <= value_cend);
            const auto distance = std::distance(value_cbegin, sparrow::next(pos, get_proxy().offset()));
            get_data_buffer().insert(pos, count, value);
            const value_iterator value_begin{data()};
            return sparrow::next(value_begin, distance);
        }

        template <trivial_copyable_type T>
        constexpr auto primitive_data_access<T>::insert_value(size_t idx, T value, size_t count)
            -> value_iterator
        {
            SPARROW_ASSERT_TRUE(idx <= get_proxy().length());
            const const_value_iterator begin{data()};
            const const_value_iterator it = sparrow::next(begin, idx);
            return insert_value(it, value, count);
        }

        // Template parameter InputIt must be an value_iterator type that iterates over elements of type T
        template <trivial_copyable_type T>
        template <mpl::iterator_of_type<T> InputIt>
        constexpr auto
        primitive_data_access<T>::insert_values(const_value_iterator pos, InputIt first, InputIt last)
            -> value_iterator
        {
            const const_value_iterator value_cbegin{data()};
            const const_value_iterator value_cend{sparrow::next(value_cbegin, get_proxy().length())};
            SPARROW_ASSERT_TRUE(value_cbegin <= pos);
            SPARROW_ASSERT_TRUE(pos <= value_cend);
            const auto distance = std::distance(value_cbegin, sparrow::next(pos, get_proxy().offset()));
            get_data_buffer().insert(pos, first, last);
            const value_iterator value_begin{data()};
            return sparrow::next(value_begin, distance);
        }

        template <trivial_copyable_type T>
        template <mpl::iterator_of_type<T> InputIt>
        constexpr auto primitive_data_access<T>::insert_values(size_t idx, InputIt first, InputIt last)
            -> value_iterator
        {
            SPARROW_ASSERT_TRUE(idx <= get_proxy().length());
            const const_value_iterator begin{data()};
            const const_value_iterator it = sparrow::next(begin, idx);
            return insert_values(it, first, last);
        }

        template <trivial_copyable_type T>
        constexpr auto primitive_data_access<T>::erase_values(const_value_iterator pos, size_t count)
            -> value_iterator
        {
            const const_value_iterator value_cbegin{data()};
            const const_value_iterator value_cend{sparrow::next(value_cbegin, get_proxy().length())};
            SPARROW_ASSERT_TRUE(value_cbegin <= pos);
            SPARROW_ASSERT_TRUE(pos < value_cend);
            const auto distance = static_cast<size_t>(
                std::distance(value_cbegin, sparrow::next(pos, get_proxy().offset()))
            );
            auto data_buffer = get_data_buffer();
            const auto first = sparrow::next(data_buffer.cbegin(), distance);
            const auto last = sparrow::next(first, count);
            data_buffer.erase(first, last);
            const value_iterator value_begin{data()};
            return sparrow::next(value_begin, distance);
        }

        template <trivial_copyable_type T>
        constexpr auto primitive_data_access<T>::erase_values(size_t idx, size_t count) -> value_iterator
        {
            SPARROW_ASSERT_TRUE(idx <= get_proxy().length());
            const const_value_iterator cbegin{data()};
            const const_value_iterator it = sparrow::next(cbegin, idx);
            erase_values(it, count);
            return sparrow::next(value_iterator{data()}, idx);
        }

        template <trivial_copyable_type T>
        void primitive_data_access<T>::reset_proxy(arrow_proxy& proxy)
        {
            p_proxy = &proxy;
        }

        template <trivial_copyable_type T>
        [[nodiscard]] constexpr buffer_adaptor<T, buffer<uint8_t>&> primitive_data_access<T>::get_data_buffer()
        {
            auto& buffers = get_proxy().get_array_private_data()->buffers();
            return make_buffer_adaptor<T>(buffers[m_data_buffer_index]);
        }

        template <trivial_copyable_type T>
        [[nodiscard]] arrow_proxy& primitive_data_access<T>::get_proxy()
        {
            return *p_proxy;
        }

        template <trivial_copyable_type T>
        [[nodiscard]] const arrow_proxy& primitive_data_access<T>::get_proxy() const
        {
            return *p_proxy;
        }
    }
}
