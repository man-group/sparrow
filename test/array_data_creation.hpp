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

#include <cstdint>
#include <numeric>

#include "sparrow/array/array_data.hpp"
#include "sparrow/array/array_data_factory.hpp"
#include "sparrow/layout/fixed_size_layout.hpp"
#include "sparrow/array/data_traits.hpp"

namespace sparrow::test
{

    // Creates an array_data object for testing purposes.
    //
    // The bitmap is initialized with all bits set to true, except for the indices specified in the
    // false_bitmap vector. The buffer is filled with values from 0 to n-1, where n is the size of the array.
    //
    // tparam T The type of the elements in the array.
    // param n The size of the array.
    // param offset The offset of the array.
    // param false_bitmap A vector containing indices to set as false in the bitmap.
    // return The created array_data object.
    // throws std::invalid_argument If an index in false_bitmap is out of range.
    template <typename T>
    sparrow::array_data
    make_test_array_data(size_t n = 10, size_t offset = 0, const std::vector<size_t>& false_bitmap = {})
    {
        sparrow::array_data ad;
        ad.type = sparrow::data_descriptor(sparrow::arrow_traits<T>::type_id);
        ad.bitmap = sparrow::dynamic_bitset<uint8_t>(n, true);
        for (const auto i : false_bitmap)
        {
            if (i >= n)
            {
                throw std::invalid_argument("Index out of range");
            }
            ad.bitmap.set(i, false);
        }
        const size_t buffer_size = (n * sizeof(T)) / sizeof(uint8_t);
        sparrow::buffer<uint8_t> b(buffer_size);
        for (uint8_t i = 0; i < n; ++i)
        {
            b.data<T>()[i] = static_cast<T>(i);
        }
        ad.buffers.push_back(b);
        ad.length = static_cast<std::int64_t>(n);
        ad.offset = static_cast<std::int64_t>(offset);
        ad.child_data.emplace_back();
        return ad;
    }

    template <>
    sparrow::array_data
    make_test_array_data<sparrow::timestamp>(size_t n, size_t offset, const std::vector<size_t>& false_bitmap);

    // Creates an array_data object for testing with std::string elements.
    //
    // param n The number of elements in the array.
    // param offset The offset value for the array_data object.
    // param false_bitmap A vector of indices to set as false in the bitmap.
    // return The created array_data object.
    // throws std::invalid_argument if any index in false_bitmap is out of range.
    template <>
    inline sparrow::array_data
    make_test_array_data<std::string>(size_t n, size_t offset, const std::vector<size_t>& false_bitmap)
    {
        std::vector<std::string> words;
        for (size_t i = 0; i < n; ++i)
        {
            words.push_back(std::to_string(i));
        }
        sparrow::array_data ad;
        ad.type = sparrow::data_descriptor(sparrow::arrow_traits<std::string>::type_id);
        ad.bitmap.resize(n);
        ad.buffers.resize(2);
        ad.buffers[0].resize(sizeof(std::int64_t) * (n + 1));
        ad.buffers[1].resize(std::accumulate(
            words.begin(),
            words.end(),
            size_t(0),
            [](std::size_t res, const auto& s)
            {
                return res + s.size();
            }
        ));
        ad.buffers[0].data<std::int64_t>()[0] = 0u;
        auto iter = ad.buffers[1].begin();
        const auto offset_func = [&ad]()
        {
            return ad.buffers[0].data<std::int64_t>();
        };
        for (size_t i = 0; i < words.size(); ++i)
        {
            offset_func(
            )[i + 1] = offset_func()[i]
                       + static_cast<sparrow::array_data::buffer_type::difference_type>(words[i].size());
            std::ranges::copy(words[i], iter);
            iter += static_cast<sparrow::array_data::buffer_type::difference_type>(words[i].size());
            ad.bitmap.set(i, true);
        }

        for (const auto i : false_bitmap)
        {
            if (i >= n)
            {
                throw std::invalid_argument("Index out of range");
            }
            ad.bitmap.set(i, false);
        }

        ad.length = static_cast<int64_t>(n);
        ad.offset = static_cast<int64_t>(offset);
        return ad;
    }

    // helper function that converts its parameters to
    // hte given type.
    template <typename O, std::integral I>
    constexpr O to_value_type(I i)
    {
        if constexpr (std::is_same_v<O, sparrow::float16_t>)
        {
            return static_cast<float>(i);
        }
        else if constexpr (std::is_arithmetic_v<O>)
        {
            return static_cast<O>(i);
        }
        else if constexpr (std::is_same_v<O, std::string>)
        {
            return std::to_string(i);
        }
    }




    template <typename T>
    requires std::integral<T> && (!std::same_as<T, bool>)
    std::vector<T> iota_vector(size_t n)
    {
        std::vector<T> v(n);
        std::iota(v.begin(), v.end(), static_cast<T>(0));
        return v;
    }

    template <typename T>
    // floating point
    requires std::floating_point<T> && (!std::same_as<T, sparrow::float16_t>)
    std::vector<T> iota_vector(size_t n)
    {
        std::vector<T> v(n);
        for (size_t i = 0; i < n; ++i)
        {
            v[i] = static_cast<T>(i);
        }
        return v;
    }

    // floating point float_16
    template<class T>
    requires std::is_same_v<T, sparrow::float16_t>
    std::vector<T> iota_vector(size_t n)
    {
        std::vector<T> v(n);
        for (size_t i = 0; i < n; ++i)
        {
            v[i] = static_cast<float>(i);
        }
        return v;
    }




    // iota-vector (for strings we use the index as as string as value)
    template <class T>
    requires std::is_same_v<T, std::string>
    inline std::vector<T> iota_vector(size_t n)
    {
        std::vector<std::string> v(n);
        for(size_t i = 0; i < n; ++i)
        {
            v[i] = std::to_string(i);
        }
        return v;
    }

    // for bool we alternate
    template <class T>
    requires std::is_same_v<T, bool>
    inline std::vector<T> iota_vector(size_t n)
    {
        std::vector<bool> v(n);
        for (size_t i = 0; i < n; ++i)
        {
            v[i] = i % 2 == 0;
        }
        return v;
    }   


    template<class C>
    sparrow::array_data::buffer_type build_offsets(C && values)
    {
        std::vector<std::int64_t> offsets(values.size() + 1, 0);
        for(std::size_t i = 0; i < values.size(); i++)
        {
            offsets[i+1] = offsets[i] + static_cast<std::int64_t>(values[i].size());
        }

        sparrow::array_data::buffer_type offset_buffer(sizeof(std::int64_t) * (values.size() + 1), 0);
        auto offset_buffer_ptr = offset_buffer.data<std::int64_t>();
        std::copy(offsets.begin(), offsets.end(), offset_buffer_ptr);
        return offset_buffer;
    }

    template<class T>
    sparrow::array_data make_array_data_for_list_of_scalars(
        const std::vector<std::vector<T>>& values)
    {
        
        std::vector<T> flat_values;
        for(auto & v : values){
            for(auto & e : v){
                flat_values.push_back(e);
            }
        }

        using data_storage = sparrow::array_data;

        // layout type of the inner flat array
        using inner_layout_type = sparrow::fixed_size_layout<T, data_storage>;

        // inner list as array_data
        auto values_array_data = sparrow::make_default_array_data<inner_layout_type>(flat_values);

        // inner layout (not needed to build the list)
        inner_layout_type inner_layout(values_array_data);

        auto list_array_data = sparrow::array_data{};

        // create the buffer for the offsets of the outer list
        auto offset_buffer = build_offsets(values);

        list_array_data.buffers.push_back(std::move(offset_buffer));
        list_array_data.child_data.push_back(std::move(values_array_data));

        // create the bitmap for the outer list
        sparrow::dynamic_bitset<uint8_t> bitmap(values.size(), true);
        list_array_data.bitmap = std::move(bitmap);
        list_array_data.length = static_cast<typename array_data::length_type>(values.size());

        return list_array_data;
    } 


    template<class T>
    sparrow::array_data make_array_data_for_list_of_list_of_scalars(
        std::vector<std::vector<std::vector<T>>> values)
    {
        // semi flatten the values
        std::vector<std::vector<T>> half_flat_values;
        for (auto& v : values) {
            for (auto& e : v) {
                half_flat_values.push_back(e);
            }
        }

        // build array data for the inner list
        auto inner_list_array_data = make_array_data_for_list_of_scalars(half_flat_values);

        // build the outer list
        auto outer_list_array_data = sparrow::array_data{};
        auto outer_offset_buffer = build_offsets(values);
        outer_list_array_data.buffers.push_back(std::move(outer_offset_buffer));
        outer_list_array_data.child_data.push_back(inner_list_array_data);
        sparrow::dynamic_bitset<uint8_t> outer_bitmap(values.size(), true);
        outer_list_array_data.bitmap = std::move(outer_bitmap);
        outer_list_array_data.length = static_cast<typename array_data::length_type>(values.size());

        return outer_list_array_data;        
    }


}
