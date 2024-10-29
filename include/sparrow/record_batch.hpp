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
     */
    class record_batch
    {
    public:

        using name_type = std::string;
        using size_type = std::size_t;
        using initializer_type = std::initializer_list<std::pair<name_type, array>>;

        using name_range = std::ranges::ref_view<const std::vector<name_type>>;
        using column_range = std::ranges::ref_view<const std::vector<array>>;

        template <std::ranges::input_range NR, std::ranges::input_range CR>
            requires (std::convertible_to<std::ranges::range_value_t<NR>, std::string>
                    and std::same_as<std::ranges::range_value_t<CR>, array>)
        record_batch(NR&& names, CR&& columns);

        SPARROW_API record_batch(initializer_type init);

        SPARROW_API record_batch(const record_batch&);
        SPARROW_API record_batch& operator=(const record_batch&);

        SPARROW_API record_batch(record_batch&&) = default;
        SPARROW_API record_batch& operator=(record_batch&&) = default;

        SPARROW_API size_type nb_columns() const;
        SPARROW_API size_type nb_rows() const;

        SPARROW_API bool contains_column(const name_type& key) const;
        SPARROW_API const name_type& get_column_name(size_type index) const;
        SPARROW_API const array& get_column(const name_type& key) const;
        SPARROW_API const array& get_column(size_type index) const;

        SPARROW_API name_range names() const;
        SPARROW_API column_range columns() const;

    private:

        template <class U, class R>
        std::vector<U> to_vector(R&& range) const;

        void init_array_map();

        bool check_consistency() const;

        std::vector<name_type> m_name_list;
        std::vector<array> m_array_list;
        std::unordered_map<name_type, array*> m_array_map;
    };

    SPARROW_API
    bool operator==(const record_batch& lhs, const record_batch& rhs);

    /*******************************
     * record_batch implementation *
     *******************************/

    template <std::ranges::input_range NR, std::ranges::input_range CR>
        requires (std::convertible_to<std::ranges::range_value_t<NR>, std::string>
                and std::same_as<std::ranges::range_value_t<CR>, array>)
    record_batch::record_batch(NR&& names, CR&& columns)
        : m_name_list(to_vector<name_type>(std::move(names)))
        , m_array_list(to_vector<array>(std::move(columns)))
    {
        init_array_map();
        SPARROW_ASSERT_TRUE(check_consistency());
    }

    template <class U, class R>
    std::vector<U> record_batch::to_vector(R&& range) const
    {
        std::vector<U> v;
        if constexpr(std::ranges::sized_range<decltype(range)>)
        {
            v.reserve(std::ranges::size(range));
        }
        std::ranges::move(range, std::back_inserter(v));
        return v;
    }
}

