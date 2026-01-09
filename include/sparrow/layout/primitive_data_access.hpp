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

#include <type_traits>

#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/buffer/buffer_adaptor.hpp"
#include "sparrow/buffer/bit_vector/bit_vector_view.hpp"
#include "sparrow/buffer/bit_vector/non_owning_bit_vector.hpp"
#include "sparrow/u8_buffer.hpp"

namespace sparrow
{
    template <typename T>
    concept trivial_copyable_type = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

    namespace details
    {
        /**
         * @class primitive_data_access
         * @brief Provides access to primitive data stored in Arrow format buffers.
         *
         * This class template manages access to primitive data types stored in an Arrow proxy's
         * data buffer. It provides a type-safe interface for accessing, modifying, and iterating
         * over the underlying buffer data.
         *
         * The class is designed to be used as a private member of array classes and enforces
         * strict ownership semantics by deleting copy and move operations. This ensures that
         * array classes explicitly manage their Arrow proxy relationships through constructor
         * calls and reset_proxy() method.
         *
         * @tparam T The outer value type (trivial copyable type)
         * @tparam T2 The inner value type stored in the buffer (trivial copyable type, defaults to T)
         *
         * @note This class holds a pointer to an arrow_proxy and is not copyable or movable.
         *       Array classes using this must explicitly call the constructor with an arrow_proxy
         *       or use reset_proxy() for assignment operations.
         *
         * Key features:
         * - Direct access to underlying buffer data via data() methods
         * - Element access through value() methods with bounds checking capabilities
         * - Iterator support for range-based operations
         * - Buffer manipulation operations (resize, insert, erase)
         * - Static factory methods for buffer creation
         *
         * @see arrow_proxy
         * @see pointer_iterator
         * @see buffer_adaptor
         */
        template <trivial_copyable_type T, trivial_copyable_type T2 = T>
        class primitive_data_access
        {
        public:

            using inner_value_type = T2;
            using inner_reference = T2&;
            using inner_const_reference = std::conditional_t<std::is_same_v<T2, bool>, T2, const T2&>;
            using inner_pointer = inner_value_type*;
            using inner_const_pointer = const inner_value_type*;

            using value_iterator = pointer_iterator<inner_pointer>;
            using const_value_iterator = pointer_iterator<inner_const_pointer>;

            /**
             * @brief Constructor for primitive_data_access.
             * @param proxy Arrow proxy object that holds the data buffer.
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

            constexpr void resize_values(size_t new_length, const T2& value);

            constexpr value_iterator insert_value(const_value_iterator pos, T2 value, size_t count);
            constexpr value_iterator insert_value(size_t idx, T2 value, size_t count);

            // Template parameter InputIt must be an value_iterator type that iterates over elements of type T
            template <mpl::iterator_of_type<T2> InputIt>
            constexpr value_iterator insert_values(const_value_iterator pos, InputIt first, InputIt last);

            template <mpl::iterator_of_type<T2> InputIt>
            constexpr value_iterator insert_values(size_t idx, InputIt first, InputIt last);

            constexpr value_iterator erase_values(const_value_iterator pos, size_t count);
            constexpr value_iterator erase_values(size_t idx, size_t count);

            constexpr void reset_proxy(arrow_proxy& proxy);

            template <std::ranges::input_range RANGE>
            [[nodiscard]] static constexpr u8_buffer<T2> make_data_buffer(RANGE&& r);

            [[nodiscard]] static constexpr u8_buffer<T2> make_data_buffer(size_t n, const T2& value);

        private:

            [[nodiscard]] constexpr buffer_adaptor<T2, buffer<uint8_t>&> get_data_buffer();

            [[nodiscard]] arrow_proxy& get_proxy();
            [[nodiscard]] const arrow_proxy& get_proxy() const;

            arrow_proxy* p_proxy;
            size_t m_data_buffer_index;
        };

        template <>
        class primitive_data_access<bool>
        {
        public:

            using bitset_view = bit_vector_view<std::uint8_t>;
            using inner_value_type = typename bitset_view::value_type;
            using inner_reference = typename bitset_view::reference;
            using inner_const_reference = typename bitset_view::const_reference;
            using inner_pointer = inner_value_type*;
            using inner_const_pointer = const inner_value_type*;

            using value_iterator = typename bitset_view::iterator;
            using const_value_iterator = typename bitset_view::const_iterator;

            primitive_data_access(arrow_proxy& proxy, size_t data_buffer_index);

            primitive_data_access(const primitive_data_access&) = delete;
            primitive_data_access& operator=(const primitive_data_access&) = delete;
            primitive_data_access(primitive_data_access&&) = delete;
            primitive_data_access& operator=(primitive_data_access&&) = delete;

            [[nodiscard]] inner_reference value(size_t i);
            [[nodiscard]] inner_const_reference value(size_t i) const;

            [[nodiscard]] value_iterator value_begin();
            [[nodiscard]] value_iterator value_end();

            [[nodiscard]] const_value_iterator value_cbegin() const;
            [[nodiscard]] const_value_iterator value_cend() const;

            void resize_values(size_t new_length, bool value);

            value_iterator insert_value(const_value_iterator pos, bool value, size_t count);
            value_iterator insert_value(size_t idx, bool value, size_t count);

            // Template parameter InputIt must be an value_iterator type that iterates over elements of type T
            template <mpl::iterator_of_type<bool> InputIt>
            constexpr value_iterator insert_values(const_value_iterator pos, InputIt first, InputIt last);

            template <mpl::iterator_of_type<bool> InputIt>
            constexpr value_iterator insert_values(size_t idx, InputIt first, InputIt last);

            value_iterator erase_values(const_value_iterator pos, size_t count);
            value_iterator erase_values(size_t idx, size_t count);

            void reset_proxy(arrow_proxy& proxy);

            template <std::ranges::input_range RANGE>
            [[nodiscard]] static u8_buffer<bool> make_data_buffer(RANGE&& r);

            [[nodiscard]] static u8_buffer<bool> make_data_buffer(size_t size, bool value);

        private:

            using bitset_adaptor = non_owning_bit_vector<std::uint8_t>;
            using difference_type = typename bitset_adaptor::difference_type;
            using adaptor_iterator = typename bitset_adaptor::iterator;
            using const_adaptor_iterator = typename bitset_adaptor::const_iterator;

            template <class F>
            [[nodiscard]] static u8_buffer<bool> make_data_buffer(size_t size, F init_func);

            [[nodiscard]] size_t get_offset(size_t i) const;

            [[nodiscard]] adaptor_iterator adaptor_begin();
            [[nodiscard]] adaptor_iterator adaptor_end();

            [[nodiscard]] const_adaptor_iterator adaptor_cbegin() const;
            [[nodiscard]] const_adaptor_iterator adaptor_cend() const;

            [[nodiscard]] arrow_proxy& get_proxy();
            [[nodiscard]] const arrow_proxy& get_proxy() const;

            [[nodiscard]] bitset_view get_data_view();
            [[nodiscard]] bitset_adaptor get_data_adaptor();

            void update_data_view();

            arrow_proxy* p_proxy;
            size_t m_data_buffer_index;
            bitset_view m_view;
            buffer<std::uint8_t> m_dummy_buffer;
            bitset_adaptor m_adaptor;
        };

        /****************************************
         * primitive_data_access implementation *
         ****************************************/

        template <trivial_copyable_type T, trivial_copyable_type T2>
        primitive_data_access<T, T2>::primitive_data_access(arrow_proxy& proxy, size_t data_buffer_index)
            : p_proxy(&proxy)
            , m_data_buffer_index(data_buffer_index)
        {
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        [[nodiscard]] constexpr auto primitive_data_access<T, T2>::data() -> inner_pointer
        {
            return get_proxy().buffers()[m_data_buffer_index].template data<T2>()
                   + static_cast<size_t>(get_proxy().offset());
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        [[nodiscard]] constexpr auto primitive_data_access<T, T2>::data() const -> inner_const_pointer
        {
            return get_proxy().buffers()[m_data_buffer_index].template data<T2>()
                   + static_cast<size_t>(get_proxy().offset());
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        [[nodiscard]] constexpr auto primitive_data_access<T, T2>::value(size_t i) -> inner_reference
        {
            SPARROW_ASSERT_TRUE(i < get_proxy().length());
            return data()[i];
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        [[nodiscard]] constexpr auto primitive_data_access<T, T2>::value(size_t i) const
            -> inner_const_reference
        {
            SPARROW_ASSERT_TRUE(i < get_proxy().length());
            return data()[i];
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        [[nodiscard]] constexpr auto primitive_data_access<T, T2>::value_begin() -> value_iterator
        {
            return value_iterator{data()};
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        [[nodiscard]] constexpr auto primitive_data_access<T, T2>::value_end() -> value_iterator
        {
            return sparrow::next(value_begin(), get_proxy().length());
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        [[nodiscard]] constexpr auto primitive_data_access<T, T2>::value_cbegin() const -> const_value_iterator
        {
            return const_value_iterator{data()};
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        [[nodiscard]] constexpr auto primitive_data_access<T, T2>::value_cend() const -> const_value_iterator
        {
            return sparrow::next(value_cbegin(), get_proxy().length());
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        constexpr void primitive_data_access<T, T2>::resize_values(size_t new_length, const T2& value)
        {
            const size_t new_size = new_length + static_cast<size_t>(get_proxy().offset());
            get_data_buffer().resize(new_size, value);
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        constexpr auto
        primitive_data_access<T, T2>::insert_value(const_value_iterator pos, T2 value, size_t count)
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

        template <trivial_copyable_type T, trivial_copyable_type T2>
        constexpr auto primitive_data_access<T, T2>::insert_value(size_t idx, T2 value, size_t count)
            -> value_iterator
        {
            SPARROW_ASSERT_TRUE(idx <= get_proxy().length());
            const const_value_iterator begin{data()};
            const const_value_iterator it = sparrow::next(begin, idx);
            return insert_value(it, value, count);
        }

        // Template parameter InputIt must be an value_iterator type that iterates over elements of type T
        template <trivial_copyable_type T, trivial_copyable_type T2>
        template <mpl::iterator_of_type<T2> InputIt>
        constexpr auto
        primitive_data_access<T, T2>::insert_values(const_value_iterator pos, InputIt first, InputIt last)
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

        template <trivial_copyable_type T, trivial_copyable_type T2>
        template <mpl::iterator_of_type<T2> InputIt>
        constexpr auto primitive_data_access<T, T2>::insert_values(size_t idx, InputIt first, InputIt last)
            -> value_iterator
        {
            SPARROW_ASSERT_TRUE(idx <= get_proxy().length());
            const const_value_iterator begin{data()};
            const const_value_iterator it = sparrow::next(begin, idx);
            return insert_values(it, first, last);
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        constexpr auto primitive_data_access<T, T2>::erase_values(const_value_iterator pos, size_t count)
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

        template <trivial_copyable_type T, trivial_copyable_type T2>
        constexpr auto primitive_data_access<T, T2>::erase_values(size_t idx, size_t count) -> value_iterator
        {
            SPARROW_ASSERT_TRUE(idx <= get_proxy().length());
            const const_value_iterator cbegin{data()};
            const const_value_iterator it = sparrow::next(cbegin, idx);
            erase_values(it, count);
            return sparrow::next(value_iterator{data()}, idx);
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        constexpr void primitive_data_access<T, T2>::reset_proxy(arrow_proxy& proxy)
        {
            p_proxy = &proxy;
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        template <std::ranges::input_range RANGE>
        [[nodiscard]] constexpr u8_buffer<T2> primitive_data_access<T, T2>::make_data_buffer(RANGE&& r)
        {
            return u8_buffer<T2>(std::forward<RANGE>(r));
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        [[nodiscard]] constexpr u8_buffer<T2>
        primitive_data_access<T, T2>::make_data_buffer(size_t size, const T2& value)
        {
            return u8_buffer<T2>(size, value);
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        [[nodiscard]] constexpr buffer_adaptor<T2, buffer<uint8_t>&>
        primitive_data_access<T, T2>::get_data_buffer()
        {
            auto& buffers = get_proxy().get_array_private_data()->buffers();
            return make_buffer_adaptor<T2>(buffers[m_data_buffer_index]);
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        [[nodiscard]] arrow_proxy& primitive_data_access<T, T2>::get_proxy()
        {
            return *p_proxy;
        }

        template <trivial_copyable_type T, trivial_copyable_type T2>
        [[nodiscard]] const arrow_proxy& primitive_data_access<T, T2>::get_proxy() const
        {
            return *p_proxy;
        }

        /**********************************************
         * primitive_data_access<bool> implementation *
         **********************************************/

        inline primitive_data_access<bool>::primitive_data_access(arrow_proxy& proxy, size_t data_buffer_index)
            : p_proxy(&proxy)
            , m_data_buffer_index(data_buffer_index)
            , m_view(get_data_view())
            , m_dummy_buffer()
            , m_adaptor(get_data_adaptor())
        {
        }

        [[nodiscard]] inline auto primitive_data_access<bool>::value(size_t i) -> inner_reference
        {
            return m_view[get_offset(i)];
        }

        [[nodiscard]] inline auto primitive_data_access<bool>::value(size_t i) const -> inner_const_reference
        {
            return m_view[get_offset(i)];
        }

        [[nodiscard]] inline auto primitive_data_access<bool>::value_begin() -> value_iterator
        {
            return sparrow::next(m_view.begin(), get_offset(0u));
        }

        [[nodiscard]] inline auto primitive_data_access<bool>::value_end() -> value_iterator
        {
            return m_view.end();
        }

        [[nodiscard]] inline auto primitive_data_access<bool>::value_cbegin() const -> const_value_iterator
        {
            return sparrow::next(m_view.cbegin(), get_offset(0u));
        }

        [[nodiscard]] inline auto primitive_data_access<bool>::value_cend() const -> const_value_iterator
        {
            return m_view.cend();
        }

        inline void primitive_data_access<bool>::resize_values(size_t new_length, bool value)
        {
            m_adaptor.resize(get_offset(new_length), value);
            update_data_view();
        }

        inline auto
        primitive_data_access<bool>::insert_value(const_value_iterator pos, bool value, size_t count)
            -> value_iterator
        {
            auto ins_iter = sparrow::next(adaptor_cbegin(), std::distance(value_cbegin(), pos));
            auto res = m_adaptor.insert(ins_iter, count, value);
            update_data_view();
            return sparrow::next(value_begin(), std::distance(adaptor_begin(), res));
        }

        inline auto primitive_data_access<bool>::insert_value(size_t idx, bool value, size_t count)
            -> value_iterator
        {
            auto iter = sparrow::next(adaptor_cbegin(), static_cast<difference_type>(idx));
            auto res = m_adaptor.insert(iter, count, value);
            update_data_view();
            return sparrow::next(value_begin(), std::distance(adaptor_begin(), res));
        }

        // Template parameter InputIt must be an value_iterator type that iterates over elements of type T
        template <mpl::iterator_of_type<bool> InputIt>
        constexpr auto
        primitive_data_access<bool>::insert_values(const_value_iterator pos, InputIt first, InputIt last)
            -> value_iterator
        {
            auto ins_iter = sparrow::next(adaptor_cbegin(), std::distance(value_cbegin(), pos));
            auto res = m_adaptor.insert(ins_iter, first, last);
            update_data_view();
            return sparrow::next(value_begin(), std::distance(adaptor_begin(), res));
        }

        template <mpl::iterator_of_type<bool> InputIt>
        constexpr auto primitive_data_access<bool>::insert_values(size_t idx, InputIt first, InputIt last)
            -> value_iterator
        {
            auto iter = sparrow::next(adaptor_cbegin(), static_cast<difference_type>(idx));
            auto res = m_adaptor.insert(iter, first, last);
            update_data_view();
            return sparrow::next(value_begin(), std::distance(adaptor_begin(), res));
        }

        inline auto primitive_data_access<bool>::erase_values(const_value_iterator pos, size_t count)
            -> value_iterator
        {
            auto iter = sparrow::next(adaptor_cbegin(), std::distance(value_cbegin(), pos));
            auto iter_end = sparrow::next(iter, count);
            auto res = m_adaptor.erase(iter, iter_end);
            update_data_view();
            return sparrow::next(value_begin(), std::distance(adaptor_begin(), res));
        }

        inline auto primitive_data_access<bool>::erase_values(size_t idx, size_t count) -> value_iterator
        {
            auto iter = sparrow::next(adaptor_cbegin(), idx);
            auto iter_end = sparrow::next(iter, count);
            auto res = m_adaptor.erase(iter, iter_end);
            update_data_view();
            return sparrow::next(value_begin(), std::distance(adaptor_begin(), res));
        }

        inline void primitive_data_access<bool>::reset_proxy(arrow_proxy& proxy)
        {
            p_proxy = &proxy;
            m_view = get_data_view();
            m_adaptor = get_data_adaptor();
        }

        template <std::ranges::input_range RANGE>
        [[nodiscard]] u8_buffer<bool> primitive_data_access<bool>::make_data_buffer(RANGE&& r)
        {
            auto size = static_cast<size_t>(std::ranges::distance(r));
            auto init_func = [&r](bitset_view& v)
            {
                std::copy(r.begin(), r.end(), v.begin());
            };
            return make_data_buffer(size, init_func);
        }

        [[nodiscard]] inline u8_buffer<bool>
        primitive_data_access<bool>::make_data_buffer(size_t size, bool value)
        {
            auto init_func = [&value](bitset_view& v)
            {
                std::fill(v.begin(), v.end(), value);
            };
            return make_data_buffer(size, init_func);
        }

        template <class F>
        [[nodiscard]] inline u8_buffer<bool>
        primitive_data_access<bool>::make_data_buffer(size_t size, F init_func)
        {
            std::size_t block_nb = size / 8;
            if (block_nb * 8 < size)
            {
                ++block_nb;
            }
            u8_buffer<bool> res(block_nb);
            std::uint8_t* buffer = reinterpret_cast<std::uint8_t*>(res.data());
            bitset_view v(buffer, size);
            init_func(v);
            return res;
        }

        [[nodiscard]] inline size_t primitive_data_access<bool>::get_offset(size_t i) const
        {
            return i + get_proxy().offset();
        }

        [[nodiscard]] inline auto primitive_data_access<bool>::adaptor_begin() -> adaptor_iterator
        {
            return sparrow::next(m_adaptor.begin(), get_offset(0u));
        }

        [[nodiscard]] inline auto primitive_data_access<bool>::adaptor_end() -> adaptor_iterator
        {
            return m_adaptor.end();
        }

        [[nodiscard]] inline auto primitive_data_access<bool>::adaptor_cbegin() const -> const_adaptor_iterator
        {
            return sparrow::next(m_adaptor.cbegin(), get_offset(0u));
        }

        [[nodiscard]] inline auto primitive_data_access<bool>::adaptor_cend() const -> const_adaptor_iterator
        {
            return m_adaptor.cend();
        }

        [[nodiscard]] inline arrow_proxy& primitive_data_access<bool>::get_proxy()
        {
            return *p_proxy;
        }

        [[nodiscard]] inline const arrow_proxy& primitive_data_access<bool>::get_proxy() const
        {
            return *p_proxy;
        }

        [[nodiscard]] inline auto primitive_data_access<bool>::get_data_view() -> bitset_view
        {
            auto& proxy = get_proxy();
            size_t size = proxy.length() + proxy.offset();
            return bitset_view(proxy.buffers()[m_data_buffer_index].data(), size);
        }

        [[nodiscard]] inline auto primitive_data_access<bool>::get_data_adaptor() -> bitset_adaptor
        {
            auto& proxy = get_proxy();
            if (proxy.is_created_with_sparrow())
            {
                size_t size = proxy.length() + proxy.offset();
                return bitset_adaptor(&(proxy.get_array_private_data()->buffers()[m_data_buffer_index]), size);
            }
            else
            {
                return bitset_adaptor(&m_dummy_buffer, 0u);
            }
        }

        inline void primitive_data_access<bool>::update_data_view()
        {
            m_view = bitset_view(get_proxy().buffers()[m_data_buffer_index].data(), m_adaptor.size());
        }
    }
}
