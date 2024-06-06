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
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <numeric>
#include <ranges>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "sparrow/array_data.hpp"
#include "sparrow/contracts.hpp"
#include "sparrow/data_traits.hpp"
#include "sparrow/data_type.hpp"
#include "sparrow/dictionary_encoded_layout.hpp"
#include "sparrow/fixed_size_layout.hpp"
#include "sparrow/array_data_concepts.hpp"
#include "sparrow/memory.hpp"
#include "sparrow/mp_utils.hpp"
#include "sparrow/reference_wrapper_utils.hpp"
#include "sparrow/variable_size_binary_layout.hpp"


namespace sparrow
{
    template <typename T>
    array_data make_array_data_for_fixed_size_layout()
    {
        return {
            .type = data_descriptor(arrow_type_id<T>()),
            .length = 0,
            .offset = 0,
            .bitmap = {},
            .buffers = {{}},
            .child_data = {},
            .dictionary = nullptr
        };
    }

    template <constant_range_for_array_data ValueRange>
    array_data make_array_data_for_fixed_size_layout(
        ValueRange&& values,
        const array_data::bitmap_type& bitmap,
        std::int64_t offset
    )
    {
        using T = std::ranges::range_value_t<ValueRange>;
        // Check that the range is a range of ranges
        if constexpr (std::ranges::range<T>)
        {
            if (!values.empty())
            {
                const size_t expected_size = values.front().size();
                const bool all_same_size = std::ranges::all_of(
                    values,
                    [expected_size](const auto& value)
                    {
                        return value.size() == expected_size;
                    }
                );
                SPARROW_ASSERT_TRUE(all_same_size);
            }
        }
        SPARROW_ASSERT_TRUE(values.size() == bitmap.size());
        SPARROW_ASSERT_TRUE(std::cmp_greater_equal(values.size(), offset));

        const auto create_buffer = [&values]()
        {
            const size_t buffer_size = (values.size() * sizeof(T)) / sizeof(uint8_t);
            array_data::buffer_type buffer(buffer_size);
            auto data = buffer.data<T>();
            for (const auto& value : values)
            {
                *data = value;
                ++data;
            }
            return buffer;
        };
        return {
            .type = data_descriptor(arrow_type_id<T>()),
            .length = static_cast<int64_t>(values.size()),
            .offset = offset,
            .bitmap = bitmap,
            .buffers = {create_buffer()},
            .child_data = {},
            .dictionary = nullptr
        };
    }

    template <typename T>
    array_data make_array_data_for_variable_size_binary_layout()
    {
        return {
            .type = data_descriptor(arrow_type_id<T>()),
            .length = 0,
            .offset = 0,
            .bitmap = {},
            .buffers = {{}, array_data::buffer_type(sizeof(std::int64_t), 0)},
            .child_data = {},
            .dictionary = nullptr
        };
    }

    template <constant_range_for_array_data ValueRange>
    array_data make_array_data_for_variable_size_binary_layout(
        ValueRange&& values,
        const array_data::bitmap_type& bitmap,
        std::int64_t offset
    )
    {
        SPARROW_ASSERT_TRUE(values.size() == bitmap.size());
        SPARROW_ASSERT_TRUE(std::cmp_greater_equal(values.size(), offset));

        const auto create_buffers = [&values]()
        {
            using T = std::ranges::range_value_t<ValueRange>;
            const auto& unwrap_value = [](const T& value)
            {
                if constexpr (mpl::is_reference_wrapper_v<T>)
                {
                    return value.get();
                }
                else
                {
                    return value;
                }
            };

            std::vector<array_data::buffer_type> buffers(2);
            buffers[0].resize(sizeof(std::int64_t) * (values.size() + 1), 0);
            buffers[1].resize(std::accumulate(
                values.begin(),
                values.end(),
                size_t(0),
                [&unwrap_value](std::size_t res, const auto& s)
                {
                    return res + unwrap_value(s).size();
                }
            ));

            auto iter = buffers[1].begin();
            const auto offsets = buffers[0].data<std::int64_t>();
            size_t i = 0;
            for (const auto& value : values)
            {
                const auto& unwraped_value = unwrap_value(value);
                SPARROW_ASSERT_TRUE(
                    std::cmp_less(unwraped_value.size(), std::numeric_limits<std::int64_t>::max())
                );
                offsets[i + 1] = offsets[i] + static_cast<std::int64_t>(unwraped_value.size());
                std::ranges::copy(unwraped_value, iter);
                std::advance(iter, unwraped_value.size());
                ++i;
            }
            return buffers;
        };

        using T = std::unwrap_ref_decay_t<std::unwrap_ref_decay_t<std::ranges::range_value_t<ValueRange>>>;

        return {
            .type = data_descriptor(arrow_type_id<T>()),
            .length = static_cast<array_data::length_type>(values.size()),
            .offset = offset,
            .bitmap = bitmap,
            .buffers = create_buffers(),
            .child_data = {},
            .dictionary = nullptr
        };
    }

    template <typename V>
    struct ValuesAndIndexes
    {
        template <mpl::constant_range R>
            requires std::same_as<std::ranges::range_value_t<R>, std::remove_const_t<V>>
        explicit ValuesAndIndexes(R&& range)
        {
            ranges_to_vec_and_indexes<R>(range, *this);
        }

        std::vector<std::reference_wrapper<V>> values;
        std::vector<size_t> indexes;
    };

    template <mpl::constant_range R>
    void
    ranges_to_vec_and_indexes(R&& range, ValuesAndIndexes<const std::ranges::range_value_t<R>>& values_and_indexes)
    {
        using T = const std::ranges::range_value_t<R>;
        std::unordered_map<std::reference_wrapper<T>, size_t, reference_wrapper_hasher, reference_wrapper_equal>
            set_index;  // TODO: To replace with a flat_map/another implementation when available, for
                        // performance reasons
        for (const auto& value : range)
        {
            set_index.emplace(std::cref(value), 0);
        }

        for (size_t i = 0; auto& [_, value] : set_index)
        {
            value = i;
            ++i;
        }

        values_and_indexes.indexes.reserve(std::ranges::size(range));

        std::ranges::transform(
            range,
            std::back_inserter(values_and_indexes.indexes),
            [&set_index](const T& value)
            {
                return set_index[std::cref(value)];
            }
        );

        values_and_indexes.values.reserve(set_index.size());

        std::ranges::transform(
            set_index,
            std::back_inserter(values_and_indexes.values),
            [](const auto& pair)
            {
                return pair.first;
            }
        );
    }

    template <typename T>
    array_data make_array_data_for_dictionary_encoded_layout()
    {
        return {
            .type = data_descriptor(arrow_type_id<std::uint64_t>()),
            .length = 0,
            .offset = 0,
            .bitmap = {},
            .buffers = {array_data::buffer_type(sizeof(std::int64_t), 0)},
            .child_data = {},
            .dictionary = value_ptr<array_data>(make_array_data_for_variable_size_binary_layout<T>())
        };
    }

    template <constant_range_for_array_data ValueRange>
    array_data make_array_data_for_dictionary_encoded_layout(
        ValueRange&& values,
        const array_data::bitmap_type& bitmap,
        std::int64_t offset
    )
    {
        SPARROW_ASSERT_TRUE(values.size() == bitmap.size());
        SPARROW_ASSERT_TRUE(std::cmp_greater_equal(values.size(), offset));

        const ValuesAndIndexes<const std::ranges::range_value_t<ValueRange>> vec_and_indexes{values};
        const auto& indexes = vec_and_indexes.indexes;
        static const auto create_buffer = [&indexes]()
        {
            const size_t buffer_size = indexes.size() * sizeof(size_t) / sizeof(uint8_t);
            array_data::buffer_type b(buffer_size);
            std::ranges::copy(indexes, b.data<size_t>());
            return b;
        };
        return {
            .type = data_descriptor(arrow_type_id<std::uint64_t>()),
            .length = static_cast<array_data::length_type>(indexes.size()),
            .offset = offset,
            .bitmap = bitmap,
            .buffers = {create_buffer()},
            .child_data = {},
            .dictionary = value_ptr<array_data>(make_array_data_for_variable_size_binary_layout(
                vec_and_indexes.values,
                array_data::bitmap_type(vec_and_indexes.values.size(), true),
                0
            ))
        };
    }

    template <is_a_supported_layout Layout>
    array_data make_default_array_data_factory()
    {
        if constexpr (mpl::is_type_instance_of_v<Layout, fixed_size_layout>)
        {
            return make_array_data_for_fixed_size_layout<typename Layout::inner_value_type>();
        }
        else if constexpr (mpl::is_type_instance_of_v<Layout, variable_size_binary_layout>)
        {
            return make_array_data_for_variable_size_binary_layout<typename Layout::inner_value_type>();
        }
        else if constexpr (mpl::is_type_instance_of_v<Layout, dictionary_encoded_layout>)
        {
            return make_array_data_for_dictionary_encoded_layout<typename Layout::inner_value_type>();
        }
        else
        {
            static_assert(
                false,
                "Unsupported layout type. Please check the is_a_supported_layout concept and create a function make_array_data_for_... for the new layout type."
            );
        }
    }

    template <is_a_supported_layout Layout, constant_range_for_array_data ValueRange>
    array_data
    make_default_array_data_factory(ValueRange&& values, const array_data::bitmap_type& bitmap, std::int64_t offset)
    {
        if constexpr (mpl::is_type_instance_of_v<Layout, fixed_size_layout>)
        {
            return make_array_data_for_fixed_size_layout(values, bitmap, offset);
        }
        else if constexpr (mpl::is_type_instance_of_v<Layout, variable_size_binary_layout>)
        {
            return make_array_data_for_variable_size_binary_layout(values, bitmap, offset);
        }
        else if constexpr (mpl::is_type_instance_of_v<Layout, dictionary_encoded_layout>)
        {
            return make_array_data_for_dictionary_encoded_layout(values, bitmap, offset);
        }
        else
        {
            static_assert(
                false,
                "Unsupported layout type. Please check the is_a_supported_layout concept and create a function make_array_data_for_... for the new layout type."
            );
        }
    }

    template <is_a_supported_layout Layout, std::ranges::input_range ValueRange>
        requires(!mpl::constant_range<ValueRange>)
    array_data
    make_default_array_data_factory(ValueRange&& values, const array_data::bitmap_type& bitmap, std::int64_t offset)
    {
        return make_default_array_data_factory<Layout>(std::as_const(values), bitmap, offset);
    }
}  // namespace sparrow
