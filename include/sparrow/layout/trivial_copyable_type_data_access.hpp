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
         * @tparam L Type of the layout.
         */
        template <trivial_copyable_type T, typename L>
        class trivial_copyable_type_data_access
        {
        public:

            using inner_value_type = T;
            using inner_reference = T&;
            using inner_const_reference = const T&;
            using pointer = inner_value_type*;
            using const_pointer = const inner_value_type*;

            using value_iterator = pointer_iterator<pointer>;
            using const_value_iterator = pointer_iterator<const_pointer>;

            /**
             * @brief Constructor for trivial_copyable_type_data_access.
             * @param layout Pointer to the layout object.
             * @param data_buffer_index Index of the data buffer.
             */
            trivial_copyable_type_data_access(L* layout, size_t data_buffer_index)
                : p_layout(layout)
                , m_data_buffer_index(data_buffer_index)
            {
            }

            constexpr T* data()
            {
                return get_arrow_proxy().buffers()[m_data_buffer_index].template data<T>()
                       + static_cast<size_t>(get_arrow_proxy().offset());
            }

            constexpr const T* data() const
            {
                return get_arrow_proxy().buffers()[m_data_buffer_index].template data<T>()
                       + static_cast<size_t>(get_arrow_proxy().offset());
            }

            constexpr T& value(size_t i)
            {
                SPARROW_ASSERT_TRUE(i < get_arrow_proxy().length());
                return data()[i];
            }

            constexpr const T& value(size_t i) const
            {
                SPARROW_ASSERT_TRUE(i < get_arrow_proxy().length());
                return data()[i];
            }

            constexpr buffer_adaptor<T, buffer<uint8_t>&> get_data_buffer()
            {
                auto& buffers = get_arrow_proxy().get_array_private_data()->buffers();
                return make_buffer_adaptor<T>(buffers[m_data_buffer_index]);
            }

            constexpr void resize_values(size_t new_length, const T& value)
            {
                const size_t new_size = new_length + static_cast<size_t>(get_arrow_proxy().offset());
                get_data_buffer().resize(new_size, value);
            }

            constexpr value_iterator insert_value(const_value_iterator pos, T value, size_t count)
            {
                const const_value_iterator value_cbegin{data()};
                const const_value_iterator value_cend{sparrow::next(value_cbegin, get_arrow_proxy().length())};
                SPARROW_ASSERT_TRUE(value_cbegin <= pos);
                SPARROW_ASSERT_TRUE(pos <= value_cend);
                const auto distance = std::distance(value_cbegin, sparrow::next(pos, get_arrow_proxy().offset()));
                get_data_buffer().insert(pos, count, value);
                const value_iterator value_begin{data()};
                return sparrow::next(value_begin, distance);
            }

            constexpr value_iterator insert_value(size_t idx, T value, size_t count)
            {
                SPARROW_ASSERT_TRUE(idx <= get_arrow_proxy().length());
                const const_value_iterator begin{data()};
                const const_value_iterator it = sparrow::next(begin, idx);
                return insert_value(it, value, count);
            }

            // Template parameter InputIt must be an iterator type that iterates over elements of type T
            template <mpl::iterator_of_type<T> InputIt>
            constexpr value_iterator insert_values(const_value_iterator pos, InputIt first, InputIt last)
            {
                const const_value_iterator value_cbegin{data()};
                const const_value_iterator value_cend{sparrow::next(value_cbegin, get_arrow_proxy().length())};
                SPARROW_ASSERT_TRUE(value_cbegin <= pos);
                SPARROW_ASSERT_TRUE(pos <= value_cend);
                const auto distance = std::distance(value_cbegin, sparrow::next(pos, get_arrow_proxy().offset()));
                get_data_buffer().insert(pos, first, last);
                const value_iterator value_begin{data()};
                return sparrow::next(value_begin, distance);
            }

            template <mpl::iterator_of_type<T> InputIt>
            constexpr value_iterator insert_values(size_t idx, InputIt first, InputIt last)
            {
                SPARROW_ASSERT_TRUE(idx <= get_arrow_proxy().length());
                const const_value_iterator begin{data()};
                const const_value_iterator it = sparrow::next(begin, idx);
                return insert_values(it, first, last);
            }

            constexpr value_iterator erase_values(const_value_iterator pos, size_t count)
            {
                const const_value_iterator value_cbegin{data()};
                const const_value_iterator value_cend{sparrow::next(value_cbegin, get_arrow_proxy().length())};
                SPARROW_ASSERT_TRUE(value_cbegin <= pos);
                SPARROW_ASSERT_TRUE(pos < value_cend);
                const auto distance = static_cast<size_t>(
                    std::distance(value_cbegin, sparrow::next(pos, get_arrow_proxy().offset()))
                );
                auto data_buffer = get_data_buffer();
                const auto first = sparrow::next(data_buffer.cbegin(), distance);
                const auto last = sparrow::next(first, count);
                data_buffer.erase(first, last);
                const value_iterator value_begin{data()};
                return sparrow::next(value_begin, distance);
            }

            constexpr value_iterator erase_values(size_t idx, size_t count)
            {
                SPARROW_ASSERT_TRUE(idx <= get_arrow_proxy().length());
                const const_value_iterator cbegin{data()};
                const const_value_iterator it = sparrow::next(cbegin, idx);
                erase_values(it, count);
                return sparrow::next(value_iterator{data()}, idx);
            }

        private:

            arrow_proxy& get_arrow_proxy()
            {
                return detail::array_access::get_arrow_proxy(*p_layout);
            }

            const arrow_proxy& get_arrow_proxy() const
            {
                return detail::array_access::get_arrow_proxy(*p_layout);
            }

            L* p_layout;
            size_t m_data_buffer_index;
        };
    }
}
