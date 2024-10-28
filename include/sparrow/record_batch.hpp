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

#include <algorithm>
#include <initializer_list>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

#include "sparrow/array.hpp"
#include "sparrow/utils/contracts.hpp"

namespace sparrow
{
    /**
     * Table-like data structure.
     *
     * A record batch is a collection of equal-length arrays mapped to
     * names.
     *
     * @tparm T the type of the names
     */
    template <class T = std::string>
    class record_batch
    {
    public:

        using name_type = T;
        using size_type = std::size_t;
        using initializer_type = std::initializer_list<std::pair<T, array>>;

        using name_range = std::ranges::ref_view<const std::vector<T>>;
        using column_range = std::ranges::ref_view<const std::vector<array>>;

        template <std::ranges::input_range NR, std::ranges::input_range CR>
            requires (std::convertible_to<std::ranges::range_value_t<NR>, T>
                    and std::same_as<std::ranges::range_value_t<CR>, array>)
        record_batch(NR&& names, CR&& columns);

        explicit record_batch(initializer_type init);

        record_batch(const record_batch&);
        record_batch& operator=(const record_batch&);

        record_batch(record_batch&&) = default;
        record_batch& operator=(record_batch&&) = default;

        size_type nb_columns() const;
        size_type nb_rows() const;

        bool contains_column(const name_type& key) const;
        const name_type& get_column_name(size_type index) const;
        const array& get_column(const name_type& key) const;
        const array& get_column(size_type index) const;

        name_range names() const;
        column_range columns() const;

    private:

        template <class U, class R>
        std::vector<U> to_vector(R&& range) const;

        void init_array_map();

        bool check_consistency() const;

        std::vector<T> m_name_list;
        std::vector<array> m_array_list;
        std::unordered_map<T, array*> m_array_map;
    };

    template <class T>
    bool operator==(const record_batch<T>& lhs, const record_batch<T>& rhs);

    /*******************************
     * record_batch implementation *
     *******************************/

    template <class T>
    template <std::ranges::input_range NR, std::ranges::input_range CR>
        requires (std::convertible_to<std::ranges::range_value_t<NR>, T>
                and std::same_as<std::ranges::range_value_t<CR>, array>)
    record_batch<T>::record_batch(NR&& names, CR&& columns)
        : m_name_list(to_vector<name_type>(std::move(names)))
        , m_array_list(to_vector<array>(std::move(columns)))
    {
        init_array_map();
        SPARROW_ASSERT_TRUE(check_consistency());
    }

    template <class T>
    record_batch<T>::record_batch(initializer_type init)
    {
        m_name_list.reserve(init.size());
        m_array_list.reserve(init.size());

        for(auto& entry: init)
        {
            m_name_list.push_back(std::move(entry.first));
            m_array_list.push_back(std::move(entry.second));
        }

        init_array_map();
        SPARROW_ASSERT_TRUE(check_consistency());
    }

    template <class T>
    record_batch<T>::record_batch(const record_batch& rhs)
        : m_name_list(rhs.m_name_list)
        , m_array_list(rhs.m_array_list)
    {
        init_array_map();
    }

    template <class T>
    record_batch<T>& record_batch<T>::operator=(const record_batch& rhs)
    {
        m_name_list = rhs.m_name_list;
        m_array_list = rhs.m_array_list;
        init_array_map();
        return *this;
    }
    
    template <class T>
    auto record_batch<T>::nb_columns() const -> size_type
    {
        return m_array_list.size();
    }

    template <class T>
    auto record_batch<T>::nb_rows() const -> size_type
    {
        return m_array_list.empty() ? size_type(0) : m_array_list.front().size();
    }
    
    template <class T>
    bool record_batch<T>::contains_column(const name_type& name) const
    {
        return m_array_map.contains(name);
    }

    template <class T>
    auto record_batch<T>::get_column_name(size_type index) const -> const name_type&
    {
        return m_name_list[index];
    }
    
    template <class T>
    const array& record_batch<T>::get_column(const name_type& name) const
    {
        auto iter = m_array_map.find(name);
        if (iter == m_array_map.end())
        {
            throw std::out_of_range("Column's name not found in record batch");
        }
        return *(iter->second);
    }

    template <class T>
    const array& record_batch<T>::get_column(size_type index) const
    {
        return m_array_list[index];
    }

    template <class T>
    auto record_batch<T>::names() const -> name_range 
    {
        return std::ranges::ref_view(m_name_list);
    }

    template <class T>
    auto record_batch<T>::columns() const -> column_range
    {
        return std::ranges::ref_view(m_array_list);
    }
    
    template <class T>
    template <class U, class R>
    std::vector<U> record_batch<T>::to_vector(R&& range) const
    {
        std::vector<U> v;
        if constexpr(std::ranges::sized_range<decltype(range)>)
        {
            v.reserve(std::ranges::size(range));
        }
        std::ranges::move(range, std::back_inserter(v));
        return v;
    }

    template <class T>
    void record_batch<T>::init_array_map()
    {
        for (size_t i = 0; i < m_name_list.size(); ++i)
        {
            m_array_map.insert({m_name_list[i], &(m_array_list[i])});
        }
    }

    template <class T>
    bool record_batch<T>::check_consistency() const
    {
        SPARROW_ASSERT(m_name_list.size() == m_array_list.size(),
            "The size of the names and of the array list must be the same");
        bool res = true;
        if (!m_array_list.empty())
        {
            size_type size = m_array_list[0].size();
            for (size_type i = 1u; i < m_array_list.size(); ++i)
            {
                SPARROW_ASSERT(m_array_list[i].size() == size,
                    "The arrays of a record batch must have the same size");
            }
        }
        return res;
    }

    template <class T>
    bool operator==(const record_batch<T>& lhs, const record_batch<T>& rhs)
    {
        return std::ranges::equal(lhs.names(), rhs.names()) &&
            std::ranges::equal(lhs.columns(), rhs.columns());
    }
}

