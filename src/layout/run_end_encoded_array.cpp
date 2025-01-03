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

#include "sparrow/layout/run_end_encoded_layout/run_end_encoded_array.hpp"

namespace sparrow
{
    run_end_encoded_array::run_end_encoded_array(arrow_proxy proxy)
        : m_proxy(std::move(proxy))
        , m_encoded_length(m_proxy.children()[0].length())
        , p_acc_lengths_array(array_factory(m_proxy.children()[0].view()))
        , p_encoded_values_array(array_factory(m_proxy.children()[1].view()))
        , m_acc_lengths(run_end_encoded_array::get_acc_lengths_ptr(*p_acc_lengths_array))
    {
    }

    run_end_encoded_array::run_end_encoded_array(const self_type& rhs)
        : run_end_encoded_array(rhs.m_proxy)
    {
    }

    auto run_end_encoded_array::operator=(const self_type& rhs) -> self_type&
    {
        if (this != &rhs)
        {
            m_proxy = rhs.m_proxy;
            m_encoded_length = rhs.m_encoded_length;
            p_acc_lengths_array = array_factory(m_proxy.children()[0].view());
            p_encoded_values_array = array_factory(m_proxy.children()[1].view());
            m_acc_lengths = run_end_encoded_array::get_acc_lengths_ptr(*p_acc_lengths_array);
        }
        return *this;
    }

    auto run_end_encoded_array::size() const -> size_type
    {
        return m_proxy.length();
    }

    auto run_end_encoded_array::empty() const -> bool
    {
        return size() == 0;
    }

    std::optional<std::string_view> run_end_encoded_array::name() const
    {
        return m_proxy.name();
    }

    std::optional<std::string_view> run_end_encoded_array::metadata() const
    {
        return m_proxy.metadata();
    }

    auto run_end_encoded_array::get_run_length(std::uint64_t run_index) const -> std::uint64_t
    {
        auto ret = std::visit(
            [run_index](auto&& acc_lengths_ptr) -> std::uint64_t
            {
                if (run_index == 0)
                {
                    return static_cast<std::uint64_t>(acc_lengths_ptr[run_index]);
                }
                else
                {
                    return static_cast<std::uint64_t>(
                        acc_lengths_ptr[run_index] - acc_lengths_ptr[run_index - 1]
                    );
                }
            },
            m_acc_lengths
        );
        return ret;
    }

    arrow_proxy& run_end_encoded_array::get_arrow_proxy()
    {
        return m_proxy;
    }

    const arrow_proxy& run_end_encoded_array::get_arrow_proxy() const
    {
        return m_proxy;
    }

    auto run_end_encoded_array::operator[](std::uint64_t i) -> array_traits::const_reference
    {
        return static_cast<const run_end_encoded_array*>(this)->operator[](i);
    }

    auto run_end_encoded_array::begin() -> iterator
    {
        return iterator(this, 0, 0);
    }

    auto run_end_encoded_array::end() -> iterator
    {
        return iterator(this, size(), 0);
    }

    auto run_end_encoded_array::begin() const -> const_iterator
    {
        return this->cbegin();
    }

    auto run_end_encoded_array::end() const -> const_iterator
    {
        return this->cend();
    }

    auto run_end_encoded_array::cbegin() const -> const_iterator
    {
        return const_iterator(this, 0, 0);
    }

    auto run_end_encoded_array::cend() const -> const_iterator
    {
        return const_iterator(this, size(), 0);
    }

    auto run_end_encoded_array::front() const -> array_traits::const_reference
    {
        return operator[](0);
    }

    auto run_end_encoded_array::back() const -> array_traits::const_reference
    {
        return operator[](size() - 1);
    }

    bool operator==(const run_end_encoded_array& lhs, const run_end_encoded_array& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }
}
