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

#include "sparrow/record_batch.hpp"

#include <unordered_set>

#include "sparrow/utils/contracts.hpp"

namespace sparrow
{
    record_batch::record_batch(initializer_type init)
    {
        m_name_list.reserve(init.size());
        m_array_list.reserve(init.size());

        for (auto& [name, array] : init)
        {
            m_name_list.push_back(std::move(name));
            m_array_list.push_back(std::move(array));
        }

        init_array_map();
        SPARROW_ASSERT_TRUE(check_consistency());
    }

    record_batch::record_batch(const record_batch& rhs)
        : m_name_list(rhs.m_name_list)
        , m_array_list(rhs.m_array_list)
    {
        init_array_map();
    }

    record_batch& record_batch::operator=(const record_batch& rhs)
    {
        m_name_list = rhs.m_name_list;
        m_array_list = rhs.m_array_list;
        init_array_map();
        return *this;
    }

    auto record_batch::nb_columns() const -> size_type
    {
        return m_array_list.size();
    }

    auto record_batch::nb_rows() const -> size_type
    {
        return m_array_list.empty() ? size_type(0) : m_array_list.front().size();
    }

    bool record_batch::contains_column(const name_type& name) const
    {
        return m_array_map.contains(name);
    }

    auto record_batch::get_column_name(size_type index) const -> const name_type&
    {
        SPARROW_ASSERT_TRUE(index < nb_columns());
        return m_name_list[index];
    }

    const array& record_batch::get_column(const name_type& name) const
    {
        const auto iter = m_array_map.find(name);
        if (iter == m_array_map.end())
        {
            throw std::out_of_range("Column's name not found in record batch");
        }
        return *(iter->second);
    }

    const array& record_batch::get_column(size_type index) const
    {
        SPARROW_ASSERT_TRUE(index < nb_columns());
        return m_array_list[index];
    }

    auto record_batch::names() const -> name_range
    {
        return std::ranges::ref_view(m_name_list);
    }

    auto record_batch::columns() const -> column_range
    {
        return std::ranges::ref_view(m_array_list);
    }

    void record_batch::init_array_map()
    {
        m_array_map.clear();
        m_array_map.reserve(m_name_list.size());
        for (size_t i = 0; i < m_name_list.size(); ++i)
        {
            m_array_map.try_emplace(m_name_list[i], &(m_array_list[i]));
        }
    }

    bool record_batch::check_consistency() const
    {
        SPARROW_ASSERT(
            m_name_list.size() == m_array_list.size(),
            "The size of the names and of the array list must be the same"
        );

        const auto unique_names = std::unordered_set<name_type>(m_name_list.begin(), m_name_list.end());
        SPARROW_ASSERT(unique_names.size() == m_name_list.size(), "The names of the columns must be unique");

        if (!m_array_list.empty())
        {
            const size_type size = m_array_list[0].size();
            for (size_type i = 1u; i < m_array_list.size(); ++i)
            {
                const bool same_size = m_array_list[i].size() == size;
                SPARROW_ASSERT(same_size, "The arrays of a record batch must have the same size");
                if (!same_size)
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool operator==(const record_batch& lhs, const record_batch& rhs)
    {
        return std::ranges::equal(lhs.names(), rhs.names()) && std::ranges::equal(lhs.columns(), rhs.columns());
    }
}
