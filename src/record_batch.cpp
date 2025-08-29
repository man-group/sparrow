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

        update_array_map_cache();
    }

    record_batch::record_batch(ArrowArray&& arr, ArrowSchema&& sch)
    {
        partial_init_from_schema(sch);

        std::size_t column_size = m_name_list.capacity();
        for (std::size_t i = 0; i < column_size; ++i)
        {
            m_name_list.emplace_back(sch.children[i]->name);
            m_array_list.emplace_back(std::move(*(arr.children[i])), std::move(*(sch.children[i])));
            *(arr.children[i]) = make_empty_arrow_array();
            *(sch.children[i]) = make_empty_arrow_schema();
        }
        arr.release(&arr);
        sch.release(&sch);

        update_array_map_cache();
    }

    record_batch::record_batch(ArrowArray&& arr, ArrowSchema* sch)
    {
        init(std::move(arr), sch);
    }

    record_batch::record_batch(ArrowArray&& arr, const ArrowSchema* sch)
    {
        init(std::move(arr), sch);
    }

    record_batch::record_batch(ArrowArray* arr, ArrowSchema* sch)
    {
        init(arr, sch);
    }

    record_batch::record_batch(const ArrowArray* arr, const ArrowSchema* sch)
    {
        init(arr, sch);
    }

    record_batch::record_batch(struct_array&& arr)
    {
        SPARROW_ASSERT_TRUE(owns_arrow_array(arr));
        SPARROW_ASSERT_TRUE(owns_arrow_schema(arr));

        auto [struct_arr, struct_sch] = extract_arrow_structures(std::move(arr));
        auto n_children = static_cast<std::size_t>(struct_arr.n_children);
        m_name_list.reserve(n_children);
        m_array_list.reserve(n_children);
        for (std::size_t i = 0; i < n_children; ++i)
        {
            array arr(move_array(*(struct_arr.children[i])), move_schema(*(struct_sch.children[i])));
            m_name_list.push_back(std::string(arr.name().value()));
            m_array_list.push_back(std::move(arr));
        }
        update_array_map_cache();
        struct_arr.release(&struct_arr);
        struct_sch.release(&struct_sch);
    }

    record_batch::record_batch(const record_batch& rhs)
        : m_name_list(rhs.m_name_list)
        , m_array_list(rhs.m_array_list)
    {
        update_array_map_cache();
    }

    record_batch& record_batch::operator=(const record_batch& rhs)
    {
        m_name_list = rhs.m_name_list;
        m_array_list = rhs.m_array_list;
        update_array_map_cache();
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
        update_array_map_cache();
        return m_array_map.contains(name);
    }

    auto record_batch::get_column_name(size_type index) const -> const name_type&
    {
        SPARROW_ASSERT_TRUE(index < nb_columns());
        return m_name_list[index];
    }

    const array& record_batch::get_column(const name_type& name) const
    {
        return const_cast<record_batch&>(*this).get_column(name);
    }

    array& record_batch::get_column(const name_type& name)
    {
        update_array_map_cache();
        const auto iter = m_array_map.find(name);
        if (iter == m_array_map.end())
        {
            throw std::out_of_range("Column's name not found in record batch");
        }
        return *(iter->second);
    }

    array& record_batch::get_column(size_type index)
    {
        SPARROW_ASSERT_TRUE(index < nb_columns());
        return m_array_list[index];
    }

    const array& record_batch::get_column(size_type index) const
    {
        SPARROW_ASSERT_TRUE(index < nb_columns());
        return m_array_list[index];
    }

    auto record_batch::name() const -> const std::optional<name_type>&
    {
        return m_name;
    }

    auto record_batch::names() const -> name_range
    {
        return std::ranges::ref_view(m_name_list);
    }

    auto record_batch::columns() const -> column_range
    {
        return std::ranges::ref_view(std::as_const(m_array_list));
    }

    struct_array record_batch::extract_struct_array()
    {
        for (std::size_t i = 0; i < m_name_list.size(); ++i)
        {
            m_array_list[i].set_name(m_name_list[i]);
        }
        m_array_map.clear();
        return struct_array(std::move(m_array_list), false, m_name, std::move(m_metadata));
    }

    void record_batch::add_column(name_type name, array column)
    {
        m_name_list.push_back(std::move(name));
        m_array_list.push_back(std::move(column));
        m_dirty_map = true;
    }

    void record_batch::add_column(array column)
    {
        auto opt_col_name = column.name();
        SPARROW_ASSERT_TRUE(opt_col_name.has_value());
        std::string name(opt_col_name.value());
        add_column(std::move(name), std::move(column));
    }

    void record_batch::partial_init_from_schema(const ArrowSchema& sch)
    {
        if (sch.name)
        {
            m_name = std::string(sch.name);
        }

        if (sch.metadata)
        {
            auto rg = key_value_view(sch.metadata);
            m_metadata = metadata_type(rg.begin(), rg.end());
        }

        std::size_t column_size = static_cast<std::size_t>(sch.n_children);
        m_name_list.reserve(column_size);
        m_array_list.reserve(column_size);
    }

    void record_batch::update_array_map_cache() const
    {
        if (!m_dirty_map)
        {
            return;
        }

        // Columns can only be appened, so update the map
        // in reverse order and stops when it finds a name
        // already contained in it.
        for (std::size_t i = m_name_list.size(); i != 0; --i)
        {
            const auto& name = m_name_list[i - 1];
            array* ar = const_cast<array*>(&(m_array_list[i - 1]));
            if (!m_array_map.try_emplace(name, ar).second)
            {
                break;
            }
        }
        m_dirty_map = false;
        check_consistency();
    }

    void record_batch::check_consistency() const
    {
        SPARROW_ASSERT(
            m_name_list.size() == m_array_list.size(),
            "The size of the names and of the array list must be the same"
        );

        auto iter = std::find(m_name_list.begin(), m_name_list.end(), "");
        SPARROW_ASSERT(iter == m_name_list.end(), "A column can not have an empty name");

        if (!m_array_list.empty())
        {
            const size_type size = m_array_list[0].size();
            for (size_type i = 1u; i < m_array_list.size(); ++i)
            {
                const bool same_size = m_array_list[i].size() == size;

                if (!same_size)
                {
                    const std::string error = "The size of the array at index " + std::to_string(i) + " is "
                                              + std::to_string(m_array_list[i].size())
                                              + ", but the size of the first array is " + std::to_string(size);
                    SPARROW_ASSERT(same_size, error.c_str());
                }
            }
        }
    }

    bool operator==(const record_batch& lhs, const record_batch& rhs)
    {
        return std::ranges::equal(lhs.names(), rhs.names()) && std::ranges::equal(lhs.columns(), rhs.columns());
    }
}
