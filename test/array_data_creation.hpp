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

#include <numeric>

#include "sparrow/array_data.hpp"
#include "sparrow/data_traits.hpp"

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
        ad.length = n;
        ad.offset = offset;
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
            offset_func()[i + 1] = offset_func()[i] + words[i].size();
            std::ranges::copy(words[i], iter);
            iter += words[i].size();
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

        ad.length = n;
        ad.offset = offset;
        return ad;
    }
}