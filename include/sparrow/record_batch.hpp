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
     * names. Each array represents a column of the table. record_batch
     * is provided as a convenient unit of work for various serialization
     * and computation functions.
     */
    class record_batch
    {
    public:

        using name_type = std::string;
        using size_type = std::size_t;
        using initializer_type = std::initializer_list<std::pair<name_type, array>>;

        using name_range = std::ranges::ref_view<const std::vector<name_type>>;
        using column_range = std::ranges::ref_view<const std::vector<array>>;

        /**
         * Constructs a record_batch from a range of names and a range of arrays.
         * Each array is internally mapped to the name at the same position in the
         * names range.
         *
         * @param names An input range of names.
         * @param columns An input range of arrays.
         */
        template <std::ranges::input_range NR, std::ranges::input_range CR>
            requires (std::convertible_to<std::ranges::range_value_t<NR>, std::string>
                    and std::same_as<std::ranges::range_value_t<CR>, array>)
        record_batch(NR&& names, CR&& columns);

        /**
         * Constructs a record_batch from a list of \c std::pair<name_type, array>".
         *
         * @param init a list of pair "name - array".
         */
        SPARROW_API record_batch(initializer_type init);

        SPARROW_API record_batch(const record_batch&);
        SPARROW_API record_batch& operator=(const record_batch&);

        SPARROW_API record_batch(record_batch&&) = default;
        SPARROW_API record_batch& operator=(record_batch&&) = default;

        /**
         * @returns the number of columns (i.e. arrays) in the record_batch.
         */
        SPARROW_API size_type nb_columns() const;

        /**
         * @returns the number of rows (i.e. the size of each array) in the
         * record_batch.
         */
        SPARROW_API size_type nb_rows() const;

        /**
         * Checks if the record_batch constains a column mapped to the specified
         * name.
         *
         * @param key The name of the column.
         * @returns \c true if the record_batch contains the mapping, \c false otherwise.
         */
        SPARROW_API bool contains_column(const name_type& key) const;

        /**
         * @returns the name mapped to the column at the given index.
         * @param index The index of the column in the record_batch.
         */
        SPARROW_API const name_type& get_column_name(size_type index) const;

        /**
         * @returns the column mapped ot the specified name in the record_batch.
         * @param key The name of the column to search for.
         */
        SPARROW_API const array& get_column(const name_type& key) const;

        /**
         * @returns the column at the specified index in the record_batch.
         * @param index The index of the column.
         */
        SPARROW_API const array& get_column(size_type index) const;

        /**
         * @returns a range of the names in the record_batch.
         */
        SPARROW_API name_range names() const;

        /**
         * @returns a range of the columns (i.e. arrays) hold in this
         * record_batch.
         */
        SPARROW_API column_range columns() const;

    private:

        template <class U, class R>
        std::vector<U> to_vector(R&& range) const;

        SPARROW_API void init_array_map();

        SPARROW_API bool check_consistency() const;

        std::vector<name_type> m_name_list;
        std::vector<array> m_array_list;
        std::unordered_map<name_type, array*> m_array_map;
    };

    /**
     * Compares the content of two record_batch objects.
     *
     * @param lhs the first record_batch to compare
     * @param rhs the second record_batch to compare
     * @return \c true if the contents of both record_batch
     * are equal, \c false otherwise.
     */
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

