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

#include "sparrow/array/array_data.hpp"
#include "sparrow/array/array_data_concepts.hpp"
#include "sparrow/array/data_traits.hpp"
#include "sparrow/array/data_type.hpp"
#include "sparrow/layout/dictionary_encoded_layout.hpp"
#include "sparrow/layout/fixed_size_layout.hpp"
#include "sparrow/layout/variable_size_binary_layout.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/memory.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/reference_wrapper_utils.hpp"

namespace sparrow
{
    /*
     * \brief Creates an array_data object for a null layout.
     *
     * This function creates an array_data object.
     */
    inline array_data make_array_data_for_null_layout(std::size_t size = 0u)
    {
        return {
            .type = data_descriptor(arrow_type_id<null_type>()),
            .length = static_cast<std::int64_t>(size),
            .offset = 0,
            .bitmap = {},
            .buffers = {},
            .child_data = {},
            .dictionary = nullptr
        };
    }

    /**
     * \brief Creates an array_data object for a fixed-size layout.
     *
     * This function creates an array_data object without any input data.
     *
     * @tparam T The type of the array_data object.
     * @return The created array_data object.
     */
    template <typename T>
    array_data make_array_data_for_fixed_size_layout()
    {
        using U = get_corresponding_arrow_type_t<T>;
        return {
            .type = data_descriptor(arrow_type_id<U>()),
            .length = 0,
            .offset = 0,
            .bitmap = {},
            .buffers = {{}},
            .child_data = {},
            .dictionary = nullptr
        };
    }

    /**
     * Checks if all elements in the input range have the same size.
     *
     * @param values The input range of elements to check.
     * @return `true` if all elements have the same size or if the input range is empty, `false` otherwise.
     */
    bool check_all_elements_have_same_size(const std::ranges::input_range auto& values)
    {
        if (!values.empty())
        {
            const size_t expected_size = values.front().size();
            return std::ranges::all_of(
                values,
                [expected_size](const auto& value)
                {
                    return value.size() == expected_size;
                }
            );
        }
        return true;
    }

    /**
     * Creates an array_data object for a fixed-size layout.
     *
     * The values are copied to the buffer of the array_data object.
     *
     * @tparam ValueRange The type of the range of values.
     * @param values The range of values.
     * @param bitmap The bitmap indicating null values.
     * @param offset The offset of the array data.
     * @return The created array_data object.
     */
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
            SPARROW_ASSERT_TRUE(check_all_elements_have_same_size(values));
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
        using U = std::conditional_t<std::same_as<T, std::string_view>, std::string, T>;
        return {
            .type = data_descriptor(arrow_type_id<U>()),
            .length = static_cast<int64_t>(values.size()),
            .offset = offset,
            .bitmap = bitmap,
            .buffers = {create_buffer()},
            .child_data = {},
            .dictionary = nullptr
        };
    }

    /**
     * Creates an empty array_data object for a variable-sized binary layout.
     *
     * @tparam T The data type of the array.
     * @return The created array_data object.
     */
    template <typename T>
    array_data make_array_data_for_variable_size_binary_layout()
    {
        using U = get_corresponding_arrow_type_t<T>;
        return {
            .type = data_descriptor(arrow_type_id<U>()),
            .length = 0,
            .offset = 0,
            .bitmap = {},
            .buffers = {{}, array_data::buffer_type(sizeof(std::int64_t), 0)},
            .child_data = {},
            .dictionary = nullptr
        };
    }

    /**
     * Creates an array_data object to use with a variable-size binary layout.
     *
     * The values are copied to the buffer of the array_data object.
     *
     * @tparam ValueRange The type of the range of values.
     * @param values The range of values.
     * @param bitmap The bitmap indicating the presence of each value.
     * @param offset The offset of the array_data object.
     * @return The created array_data object.
     */
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
        using U = get_corresponding_arrow_type_t<T>;

        return {
            .type = data_descriptor(arrow_type_id<U>()),
            .length = static_cast<array_data::length_type>(values.size()),
            .offset = offset,
            .bitmap = bitmap,
            .buffers = create_buffers(),
            .child_data = {},
            .dictionary = nullptr
        };
    }

    /**
     * Helper struct to store values and their indexes for a dictionary-encoded layout.
     *
     * @tparam V The type of the values.
     */
    template <typename V>
    struct values_and_indexes
    {
        template <mpl::constant_range R>
            requires std::same_as<std::ranges::range_value_t<R>, std::remove_const_t<V>>
        explicit values_and_indexes(R&& range)
        {
            ranges_to_vec_and_indexes<R>(std::forward<R>(range), *this);
        }

        void clear()
        {
            values.clear();
            indexes.clear();
        }

        std::vector<std::reference_wrapper<V>> values;
        std::vector<size_t> indexes;
    };

    /**
     * Converts a range of values to a vector of unique values and their corresponding indexes.
     *
     * @tparam R The type of the range.
     * @param range The input range of values.
     * @param values_and_indexes The output container for the unique values and their indexes. The values and
     * indexes must be empty.
     */
    template <mpl::constant_range R>
    void
    ranges_to_vec_and_indexes(R&& range, values_and_indexes<const std::ranges::range_value_t<R>>& values_and_indexes)
    {
        SPARROW_ASSERT_TRUE(values_and_indexes.values.empty());
        SPARROW_ASSERT_TRUE(values_and_indexes.indexes.empty());
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

    /**
     * Creates an empty array_data object for dictionary encoded layout.
     *
     * @tparam T The type of the array data.
     * @return The created array_data object.
     */
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

    /**
     * Creates an array_data object for dictionary encoded layout.
     *
     * The values are copied to the array_data object.
     *
     * @tparam ValueRange The type of the range for the values.
     * @param values The range of values.
     * @param bitmap The bitmap indicating the presence of values.
     * @param offset The offset for the array data.
     * @return The created array_data object.
     */
    template <constant_range_for_array_data ValueRange>
    array_data make_array_data_for_dictionary_encoded_layout(
        ValueRange&& values,
        const array_data::bitmap_type& bitmap,
        std::int64_t offset
    )
    {
        SPARROW_ASSERT_TRUE(values.size() == bitmap.size());
        SPARROW_ASSERT_TRUE(std::cmp_greater_equal(values.size(), offset));

        const values_and_indexes<const std::ranges::range_value_t<ValueRange>> vec_and_indexes{values};
        const auto& indexes = vec_and_indexes.indexes;
        const auto create_buffer = [&indexes]()
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

    /**
     * Creates a default array data object based on the specified layout.
     *
     * This function creates a default array data object based on the specified layout and value range.
     * It uses the layout type to determine the appropriate function to call for creating the array data.
     * If the layout type is not supported, a static assertion is triggered.
     *
     * @tparam Layout The layout type for the array data.
     * @return The created array data object.
     */
    template <arrow_layout Layout>
    array_data make_default_array_data()
    {
        if constexpr (std::same_as<Layout, null_layout>)
        {
            return make_array_data_for_null_layout();
        }
        else if constexpr (mpl::is_type_instance_of_v<Layout, fixed_size_layout>)
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
                mpl::dependent_false<Layout>::value,
                "Unsupported layout type. Please check the is_a_supported_layout concept and create a function make_array_data_for_... for the new layout type."
            );
            mpl::unreachable();
        }
    }

    /**
     * \brief Creates a default array data object based on the specified layout and value range.
     *
     * This function creates a default array data object based on the specified layout and value range.
     * It uses the layout type to determine the appropriate function to call for creating the array data.
     * If the layout type is not supported, a static assertion is triggered.
     *
     * @tparam Layout The layout type for the array data.
     * @tparam ValueRange The type of the value range for the array data.
     * @param values The value range for the array data.
     * @param bitmap The bitmap type for the array data.
     * @param offset The offset for the array data.
     * @return The created array data object.
     */
    template <arrow_layout Layout, constant_range_for_array_data ValueRange>
    array_data
    make_default_array_data(ValueRange&& values, const array_data::bitmap_type& bitmap, std::int64_t offset)
    {
        if constexpr (mpl::is_type_instance_of_v<Layout, fixed_size_layout>)
        {
            return make_array_data_for_fixed_size_layout(std::forward<ValueRange>(values), bitmap, offset);
        }
        else if constexpr (mpl::is_type_instance_of_v<Layout, variable_size_binary_layout>)
        {
            return make_array_data_for_variable_size_binary_layout(std::forward<ValueRange>(values), bitmap, offset);
        }
        else if constexpr (mpl::is_type_instance_of_v<Layout, dictionary_encoded_layout>)
        {
            return make_array_data_for_dictionary_encoded_layout(std::forward<ValueRange>(values), bitmap, offset);
        }
        else
        {
            static_assert(
                mpl::dependent_false<Layout>::value,
                "Unsupported layout type. Please check the is_a_supported_layout concept and create a function make_array_data_for_... for the new layout type."
            );
            mpl::unreachable();
        }
    }

    /**
     * \brief Creates a default array_data object with the specified layout and values.
     *
     * @tparam Layout The layout of the array_data object.
     * @tparam ValueRange The type of the input range for the values.
     * @param values The input range of values for the array_data object.
     * @param bitmap The bitmap type for the array_data object.
     * @param offset The offset for the array_data object.
     * @return A new array_data object with the specified layout, values, bitmap, and offset.
     */
    template <arrow_layout Layout, std::ranges::input_range ValueRange>
        requires(!mpl::constant_range<ValueRange>)
    array_data
    make_default_array_data(ValueRange&& values, const array_data::bitmap_type& bitmap, std::int64_t offset)
    {
        return make_default_array_data<Layout>(std::as_const(values), bitmap, offset);
    }
}  // namespace sparrow
