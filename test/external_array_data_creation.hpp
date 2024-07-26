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

#include "sparrow/c_interface.hpp"
#include "sparrow/array/array_data.hpp"
#include "sparrow/array/data_traits.hpp"

namespace sparrow::test
{
    void release_arrow_schema(ArrowSchema* schema);
    void release_arrow_array(ArrowArray* arr);
    
    inline std::uint8_t* make_bitmap_buffer(size_t n, const std::vector<size_t>& false_bitmap)
    {
        auto tmp_bitmap = sparrow::dynamic_bitset<uint8_t>(n, true);
        for (const auto i : false_bitmap)
        {
            if (i >= n)
            {
                throw std::invalid_argument("Index out of range");
            }
            tmp_bitmap.set(i, false);
        }

        auto buf = new std::uint8_t[tmp_bitmap.block_count()];
        std::memcpy(buf, tmp_bitmap.data(), tmp_bitmap.block_count());
        return buf;
    }

    template <class T>
    void fill_schema_and_array(ArrowSchema& schema, ArrowArray& arr, size_t n, size_t offset, const std::vector<size_t>& false_bitmap)
    {
        schema.format = sparrow::arrow_traits<T>::format;
        schema.name = "test";
        schema.n_children = 0;
        schema.children = nullptr;
        schema.dictionary = nullptr;
        schema.release = &release_arrow_schema;

        arr.length = static_cast<std::int64_t>(n);
        arr.null_count = static_cast<std::int64_t>(false_bitmap.size());
        arr.offset = static_cast<std::int64_t>(offset);
        arr.n_buffers = 2;
        arr.n_children = 0;
        std::uint8_t** buf = new std::uint8_t*[2];
        arr.buffers = const_cast<const void**>(reinterpret_cast<void**>(buf));

        buf[0] = make_bitmap_buffer(n, false_bitmap);

        T* data_buf = new T[n];
        if constexpr (std::same_as<T, bool>)
        {
            for (std::size_t i = 0; i < n; ++i)
            {
                data_buf[i] = (i%2 == 0);
            }
        }
        else
        {
            std::iota(data_buf, data_buf + n, T(0));
        }
        buf[1] = reinterpret_cast<std::uint8_t*>(data_buf);

        arr.children = nullptr;
        arr.dictionary = nullptr;
        arr.release = &release_arrow_array;

    }

    inline std::vector<std::string> make_testing_words(std::size_t n)
    {
        static std::vector<std::string> words =
            {"once", "upon", "a", "time", "I", "was", "writing", "clean",
             "code", "now", "I'm", "only", "drawing", "flowcharts", "Bonnie", "Compyler" };
        std::vector<std::string> res(n);
        std::copy(words.cbegin(), words.cbegin() + std::ptrdiff_t(n), res.begin());
        if (n > words.size())
        {
            for (std::size_t i = words.size(); i < n; ++i)
            {
                res[i] = std::to_string(i);
            }
        }

        return res;
    }

    template <>
    inline void fill_schema_and_array<std::string>(
        ArrowSchema& schema,
        ArrowArray& arr,
        size_t n,
        size_t offset,
        const std::vector<size_t>& false_bitmap)
    {
        schema.format = sparrow::arrow_traits<std::string>::format;
        schema.name = "test";
        schema.n_children = 0;
        schema.children = nullptr;
        schema.dictionary = nullptr;
        schema.release = &release_arrow_schema;

        arr.length = static_cast<std::int64_t>(n);
        arr.null_count = static_cast<std::int64_t>(false_bitmap.size());
        arr.offset = static_cast<std::int64_t>(offset);
        arr.n_buffers = 3;
        arr.n_children = 0;
        std::uint8_t** buf = new std::uint8_t*[3];
        arr.buffers = const_cast<const void**>(reinterpret_cast<void**>(buf));

        buf[0] = make_bitmap_buffer(n, false_bitmap);

        auto words = make_testing_words(n);
        std::size_t value_size = std::accumulate(words.cbegin(), words.cbegin() + std::ptrdiff_t(n), std::size_t(0),
                [](std::size_t res, const auto& s) { return res + s.size(); });
        auto offset_buf = new int64_t[n+1];
        auto value_buf = new char[value_size];

        offset_buf[0] = std::int64_t(0);
        char* ptr = value_buf;
        for (std::size_t i = 0; i < n; ++i)
        {
            offset_buf[i+1] = offset_buf[i] + static_cast<std::int64_t>(words[i].size());
            std::ranges::copy(words[i], ptr);
            ptr += words[i].size();
        }

        buf[1] = reinterpret_cast<std::uint8_t*>(offset_buf);
        buf[2] = reinterpret_cast<std::uint8_t*>(value_buf);
        arr.children = nullptr;
        arr.dictionary = nullptr;
        arr.release = &release_arrow_array;
    }

    template <>
    inline void fill_schema_and_array<sparrow::null_type>(
        ArrowSchema& schema,
        ArrowArray& arr,
        size_t n,
        size_t offset,
        const std::vector<size_t>&)
    {
        schema.format = sparrow::arrow_traits<sparrow::null_type>::format;
        schema.name = "test";
        schema.n_children = 0;
        schema.children = nullptr;
        schema.dictionary = nullptr;
        schema.release = &release_arrow_schema;

        arr.length = static_cast<std::int64_t>(n);
        arr.null_count = arr.length;
        arr.offset = static_cast<std::int64_t>(offset);
        arr.n_buffers = 0;
        arr.n_children = 0;
        arr.buffers = nullptr;
        arr.children = nullptr;
        arr.dictionary = nullptr;
        arr.release = &release_arrow_array;
    }

    template <class T, bool make_ptr = false>
    sparrow::external_array_data
    make_test_external_array_data(size_t n = 10, size_t offset = 0, const std::vector<size_t>& false_bitmap = {})
    {
        if constexpr (make_ptr)
        {
            ArrowSchema* schema = new ArrowSchema;
            ArrowArray* arr = new ArrowArray;
            fill_schema_and_array<T>(*schema, *arr, n, offset, false_bitmap);
            return sparrow::external_array_data(schema, true, arr, true);
        }
        else
        {
            ArrowSchema schema;
            ArrowArray arr;
            fill_schema_and_array<T>(schema, arr, n, offset, false_bitmap);
            return sparrow::external_array_data(std::move(schema), true, std::move(arr), true);
        }
    }
}

