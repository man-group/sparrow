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

#include <iterator>

namespace sparrow
{
    template <class T>
    class repeat_view
    {
    public:

        class iterator
        {
        public:

            using iterator_category = std::random_access_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = const T*;
            using reference = const T&;

            iterator() = default;

            iterator(const T& value, size_t index)
                : m_value(&value)
                , m_index(index)
            {
            }

            reference operator*() const
            {
                return *m_value;
            }

            pointer operator->() const
            {
                return m_value;
            }

            iterator& operator++()
            {
                ++m_index;
                return *this;
            }

            iterator operator++(int)
            {
                iterator tmp = *this;
                ++m_index;
                return tmp;
            }

            iterator& operator--()
            {
                --m_index;
                return *this;
            }

            iterator operator--(int)
            {
                iterator tmp = *this;
                --m_index;
                return tmp;
            }

            iterator& operator+=(difference_type n)
            {
                m_index += n;
                return *this;
            }

            iterator operator+(difference_type n) const
            {
                return iterator(*m_value, m_index + n);
            }

            iterator& operator-=(difference_type n)
            {
                m_index -= n;
                return *this;
            }

            iterator operator-(difference_type n) const
            {
                return iterator(*m_value, m_index - n);
            }

            difference_type operator-(const iterator& other) const
            {
                return static_cast<difference_type>(m_index - other.m_index);
            }

            bool operator==(const iterator& other) const
            {
                return m_index == other.m_index;
            }

            auto operator<=>(const iterator& other) const
            {
                return m_index <=> other.m_index;
            }

        private:

            const T* m_value = nullptr;
            size_t m_index = 0;
        };

        repeat_view(const T& value, size_t count)
            : m_value(value)
            , m_count(count)
        {
        }

        iterator begin() const
        {
            return iterator(m_value, 0);
        }

        iterator end() const
        {
            return iterator(m_value, m_count);
        }

        size_t size() const
        {
            return m_count;
        }

    private:

        T m_value;
        size_t m_count;
    };
}
