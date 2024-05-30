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
#include <functional>
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
#include "sparrow/memory.hpp"
#include "sparrow/mp_utils.hpp"
#include "sparrow/variable_size_binary_layout.hpp"

namespace sparrow
{
    template <typename T>
    array_data for_fixed_size_layout()
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

    template <std::ranges::range ValueRange>
    array_data
    for_fixed_size_layout(const ValueRange& values, const array_data::bitmap_type& bitmap, std::int64_t offset)
    {
        using T = std::ranges::range_value_t<ValueRange>;
        if constexpr (std::ranges::range<T>)
        {
            using U = std::ranges::range_value_t<T>;
            if constexpr (!is_arrow_base_type<U>)
            {
                static_assert(std::is_same_v<U, U>, "Unsupported range value type");
            }
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
    array_data for_variable_size_binary_layout()
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

    template <std::ranges::range ValueRange>
        requires std::ranges::range<std::unwrap_ref_decay_t<std::ranges::range_value_t<ValueRange>>>
    array_data
    for_variable_size_binary_layout(const ValueRange& values, const array_data::bitmap_type& bitmap, std::int64_t offset)
    {
        using U = std::unwrap_ref_decay_t<std::ranges::range_value_t<ValueRange>>;
        if constexpr (std::ranges::range<U>)
        {
            using V = std::ranges::range_value_t<U>;
            if constexpr (!is_arrow_base_type<V>)
            {
                static_assert(!std::is_same_v<V, V>, "Unsupported range value type");
            }
        }
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
                SPARROW_ASSERT_TRUE(std::cmp_less(unwraped_value.size(), std::numeric_limits<std::int64_t>::max()));
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

    struct ReferenceWrapperHasher
    {
        template <typename T>
        std::size_t operator()(const std::reference_wrapper<T>& ref) const
        {
            return std::hash<std::remove_cvref_t<T>>{}(ref.get());
        }
    };

    struct ReferenceWrapperEqual
    {
        template <typename T>
        bool operator()(const std::reference_wrapper<T>& lhs, const std::reference_wrapper<T>& rhs) const
        {
            return lhs.get() == rhs.get();
        }
    };

    template <typename V>
    struct ValuesAndIndexes
    {
        std::vector<std::reference_wrapper<V>> values;
        std::vector<size_t> indexes;
    };

    template <std::ranges::range R>
    ValuesAndIndexes<const std::ranges::range_value_t<R>> ranges_to_vec_and_indexes(const R& range)
    {
        using T = const std::ranges::range_value_t<R>;
        std::unordered_map<std::reference_wrapper<T>, size_t, ReferenceWrapperHasher, ReferenceWrapperEqual> set_index;
        for (const auto& value : range)
        {
            set_index.emplace(std::cref(value), 0);
        }
        size_t i = 0;
        for (auto& [_, value] : set_index)
        {
            value = i;
            ++i;
        }

        ValuesAndIndexes<T> result;

        std::transform(
            range.begin(),
            range.end(),
            std::back_inserter(result.indexes),
            [&set_index](const T& value)
            {
                return set_index[std::cref(value)];
            }
        );

        std::transform(
            set_index.begin(),
            set_index.end(),
            std::back_inserter(result.values),
            [](const auto& pair)
            {
                return pair.first;
            }
        );

        return result;
    }

    template <typename T>
    array_data for_dictionary_encoded_layout()
    {
        return {
            .type = data_descriptor(arrow_type_id<std::uint64_t>()),
            .length = 0,
            .offset = 0,
            .bitmap = {},
            .buffers = {array_data::buffer_type(sizeof(std::int64_t), 0)},
            .child_data = {},
            .dictionary = value_ptr<array_data>(for_variable_size_binary_layout<T>())
        };
    }

    template <std::ranges::range ValueRange>
    array_data
    for_dictionary_encoded_layout(const ValueRange& values, const array_data::bitmap_type& bitmap, std::int64_t offset)
    {
        SPARROW_ASSERT_TRUE(values.size() == bitmap.size());
        SPARROW_ASSERT_TRUE(std::cmp_greater_equal(values.size(), offset));

        const auto vec_and_indexes = ranges_to_vec_and_indexes(values);
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
            .dictionary = value_ptr<array_data>(for_variable_size_binary_layout(
                vec_and_indexes.values,
                array_data::bitmap_type(vec_and_indexes.values.size(), true),
                0
            ))
        };
    }

    template <class Layout>
    array_data default_array_data_factory()
    {
        if constexpr (mpl::InstantiationOf<fixed_size_layout, Layout>)
        {
            return for_fixed_size_layout<typename Layout::inner_value_type>();
        }
        else if constexpr (mpl::InstantiationOf<variable_size_binary_layout, Layout>)
        {
            return for_variable_size_binary_layout<typename Layout::inner_value_type>();
        }
        else if constexpr (mpl::InstantiationOf<dictionary_encoded_layout, Layout>)
        {
            return for_dictionary_encoded_layout<typename Layout::inner_value_type>();
        }
        else
        {
            static_assert(!std::is_same_v<Layout, Layout>, "Unsupported layout type");
        }
    }

    template <class Layout, std::ranges::range ValueRange>
    array_data
    default_array_data_factory(const ValueRange& values, const array_data::bitmap_type& bitmap, std::int64_t offset)
    {
        if constexpr (mpl::InstantiationOf<fixed_size_layout, Layout>)
        {
            return for_fixed_size_layout(values, bitmap, offset);
        }
        else if constexpr (mpl::InstantiationOf<variable_size_binary_layout, Layout>)
        {
            return for_variable_size_binary_layout(values, bitmap, offset);
        }
        else if constexpr (mpl::InstantiationOf<dictionary_encoded_layout, Layout>)
        {
            return for_dictionary_encoded_layout(values, bitmap, offset);
        }
        else
        {
            static_assert(!std::is_same_v<Layout, Layout>, "Unsupported layout type");
        }
    }
}  // namespace sparrow
