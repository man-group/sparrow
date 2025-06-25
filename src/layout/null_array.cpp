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

#include "sparrow/layout/null_array.hpp"

namespace sparrow
{
    null_array::null_array(arrow_proxy proxy)
        : m_proxy(std::move(proxy))
    {
        SPARROW_ASSERT_TRUE(m_proxy.data_type() == data_type::NA);
    }

    std::optional<std::string_view> null_array::name() const
    {
        return m_proxy.name();
    }

    std::optional<key_value_view> null_array::metadata() const
    {
        return m_proxy.metadata();
    }

    auto null_array::size() const -> size_type
    {
        return m_proxy.length();
    }

    auto null_array::operator[](size_type i) -> reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return *(begin());
    }

    auto null_array::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        return *(cbegin());
    }

    auto null_array::begin() -> iterator
    {
        return iterator(0);
    }

    auto null_array::end() -> iterator
    {
        return iterator(ssize());
    }

    auto null_array::rbegin() -> reverse_iterator
    {
        return reverse_iterator(end());
    }

    auto null_array::rend() -> reverse_iterator
    {
        return reverse_iterator(begin());
    }

    auto null_array::begin() const -> const_iterator
    {
        return cbegin();
    }

    auto null_array::end() const -> const_iterator
    {
        return cend();
    }

    auto null_array::rbegin() const -> const_reverse_iterator
    {
        return const_reverse_iterator(cend());
    }

    auto null_array::rend() const -> const_reverse_iterator
    {
        return const_reverse_iterator(cbegin());
    }

    auto null_array::cbegin() const -> const_iterator
    {
        return const_iterator(0);
    }

    auto null_array::cend() const -> const_iterator
    {
        return const_iterator(ssize());
    }

    auto null_array::crbegin() const -> const_reverse_iterator
    {
        return const_reverse_iterator(cend());
    }

    auto null_array::crend() const -> const_reverse_iterator
    {
        return const_reverse_iterator(cbegin());
    }

    auto null_array::front() -> reference
    {
        return *begin();
    }

    auto null_array::front() const -> const_reference
    {
        return *cbegin();
    }

    auto null_array::back() -> reference
    {
        return *(end() - 1);
    }

    auto null_array::back() const -> const_reference
    {
        return *(cend() - 1);
    }

    auto null_array::values() const -> const_value_range
    {
        return std::ranges::subrange(const_value_iterator(0), const_value_iterator(ssize()));
    }

    auto null_array::bitmap() const -> const_bitmap_range
    {
        return std::ranges::subrange(const_bitmap_iterator(0), const_bitmap_iterator(ssize()));
    }

    auto null_array::ssize() const -> difference_type
    {
        return static_cast<difference_type>(size());
    }

    arrow_proxy& null_array::get_arrow_proxy()
    {
        return m_proxy;
    }

    const arrow_proxy& null_array::get_arrow_proxy() const
    {
        return m_proxy;
    }

    bool operator==(const null_array& lhs, const null_array& rhs)
    {
        return lhs.size() == rhs.size();
    }
}
