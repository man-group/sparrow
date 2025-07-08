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
#include <string>

#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/types/data_type.hpp"
#include "sparrow/primitive_array.hpp"

#include "metadata_sample.hpp"

namespace sparrow::test
{
    void release_external_arrow_schema(ArrowSchema* schema);
    void release_external_arrow_array(ArrowArray* arr);

    template <std::ranges::range R>
        requires(std::integral<std::ranges::range_value_t<R>> && !std::same_as<std::ranges::range_value_t<R>, bool>)
    buffer<uint8_t> make_bitmap_buffer(size_t count, R&& nulls)
    {
        if (!std::ranges::empty(nulls))
        {
            SPARROW_ASSERT_TRUE(*std::ranges::max_element(nulls) < count);
        }
        dynamic_bitset<uint8_t> bitmap(count, true);
        for (const auto i : nulls)
        {
            bitmap.set(i, false);
        }
        return bitmap.buffer();
    };

    namespace detail
    {
        template <class T>
        void fill_primitive_data_buffer(T* data_buf, std::size_t size)
        {
            if constexpr (std::same_as<T, bool>)
            {
                for (std::size_t i = 0; i < size; ++i)
                {
                    data_buf[i] = (i % 2 == 0);
                }
            }
            else
            {
                std::iota(data_buf, data_buf + size, T(0));
            }
        }
    }

    // We need to keep this one is for testing arrow_proxy
    // on external (i.e. not allocated by sparrow) ArrowArray and ArrowProxy
    template <class T>
    void fill_external_schema_and_array(
        ArrowSchema& schema,
        ArrowArray& arr,
        size_t size,
        size_t offset,
        const std::vector<size_t>& false_bitmap
    )
    {
        std::string_view data_type_str = sparrow::data_type_to_format(
            sparrow::detail::get_data_type_from_array<sparrow::primitive_array<T>>::get()
        );
        schema.format = data_type_str.data();
        schema.name = "test";
        schema.metadata = "test metadata";
        schema.n_children = 0;
        schema.children = nullptr;
        schema.dictionary = nullptr;
        schema.release = &release_external_arrow_schema;

        arr.length = static_cast<std::int64_t>(size - offset);
        arr.null_count = static_cast<std::int64_t>(false_bitmap.size());
        arr.offset = static_cast<std::int64_t>(offset);
        arr.n_buffers = 2;
        arr.n_children = 0;
        std::uint8_t** buf = new std::uint8_t*[2];
        arr.buffers = const_cast<const void**>(reinterpret_cast<void**>(buf));

        const auto bitmap_buf = make_bitmap_buffer(size, false_bitmap);

        buf[0] = new std::uint8_t[bitmap_buf.size()];
        std::copy(bitmap_buf.begin(), bitmap_buf.end(), buf[0]);

        T* data_buf = new T[size];
        detail::fill_primitive_data_buffer(data_buf, size);
        buf[1] = reinterpret_cast<std::uint8_t*>(data_buf);

        arr.children = nullptr;
        arr.dictionary = nullptr;
        arr.release = &release_external_arrow_array;
    }

    template <class T>
    void fill_schema_and_array(
        ArrowSchema& schema,
        ArrowArray& arr,
        size_t size,
        size_t offset,
        const std::vector<size_t>& false_bitmap
    )
    {
        sparrow::fill_arrow_schema(
            schema,
            data_type_to_format(sparrow::detail::get_data_type_from_array<sparrow::primitive_array<T>>::get()),
            "test",
            metadata_sample_opt,
            std::nullopt,
            nullptr,
            repeat_view<bool>{true, 0},
            nullptr,
            false
        );

        using buffer_type = sparrow::buffer<std::uint8_t>;
        buffer_type data_buf(size * sizeof(T));
        detail::fill_primitive_data_buffer(data_buf.data<T>(), size);

        std::vector<buffer_type> arr_buffs = {
            sparrow::test::make_bitmap_buffer(size, false_bitmap),
            std::move(data_buf)
        };

        sparrow::fill_arrow_array(
            arr,
            static_cast<std::int64_t>(size - offset),
            static_cast<std::int64_t>(false_bitmap.size()),
            static_cast<std::int64_t>(offset),
            std::move(arr_buffs),
            nullptr,
            repeat_view<bool>{true, 0},
            nullptr,
            false
        );
    }

    template <>
    inline void fill_schema_and_array<bool>(
        ArrowSchema& schema,
        ArrowArray& arr,
        size_t size,
        size_t offset,
        const std::vector<size_t>& false_bitmap
    )
    {
        sparrow::fill_arrow_schema(
            schema,
            std::string_view("b"),
            "test",
            metadata_sample_opt,
            std::nullopt,
            nullptr,
            repeat_view<bool>{true, 0},
            nullptr,
            false
        );

        using buffer_type = sparrow::buffer<std::uint8_t>;
        std::size_t nb_blocks = size / 8;
        if (nb_blocks * 8 < size)
        {
            ++nb_blocks;
        }
        buffer_type data_buf(nb_blocks);
        dynamic_bitset_view<std::uint8_t> view(data_buf.data(), size);
        for (std::size_t i = 0; i < size; ++i)
        {
            view[i] = i % 2 != 0;
        }

        std::vector<buffer_type> arr_buffs = {
            sparrow::test::make_bitmap_buffer(size, false_bitmap),
            std::move(data_buf)
        };

        sparrow::fill_arrow_array(
            arr,
            static_cast<std::int64_t>(size - offset),
            static_cast<std::int64_t>(false_bitmap.size()),
            static_cast<std::int64_t>(offset),
            std::move(arr_buffs),
            nullptr,
            repeat_view<bool>{true, 0},
            nullptr,
            false
        );
    }

    inline std::vector<std::string> make_testing_words(std::size_t n)
    {
        static const std::vector<std::string> words = {
            "once",
            "upon",
            "a",
            "time",
            "I",
            "was",
            "writing",
            "clean",
            "code",
            "now",
            "I'm",
            "only",
            "drawing",
            "flowcharts",
            "Bonnie",
            "Compyler"
        };
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
        size_t size,
        size_t offset,
        const std::vector<size_t>& false_bitmap
    )
    {
        sparrow::fill_arrow_schema(
            schema,
            std::string_view("u"),
            "test",
            metadata_sample_opt,
            std::nullopt,
            nullptr,
            repeat_view<bool>{true, 0},
            nullptr,
            true
        );

        using buffer_type = sparrow::buffer<std::uint8_t>;

        auto words = make_testing_words(size);
        std::size_t value_size = std::accumulate(
            words.cbegin(),
            words.cbegin() + std::ptrdiff_t(size),
            std::size_t(0),
            [](std::size_t res, const auto& s)
            {
                return res + s.size();
            }
        );

        buffer_type offset_buf(sizeof(std::int32_t) * (size + 1));
        buffer_type value_buf(sizeof(char) * value_size);
        {
            std::int32_t* offset_data = offset_buf.data<std::int32_t>();
            offset_data[0] = 0;
            char* ptr = value_buf.data<char>();
            for (std::size_t i = 0; i < size; ++i)
            {
                offset_data[i + 1] = offset_data[i] + static_cast<std::int32_t>(words[i].size());
                sparrow::ranges::copy(words[i], ptr);
                ptr += words[i].size();
            }
        }

        std::vector<buffer_type> arr_buffs = {
            sparrow::test::make_bitmap_buffer(size, false_bitmap),
            std::move(offset_buf),
            std::move(value_buf)
        };

        sparrow::fill_arrow_array(
            arr,
            static_cast<std::int64_t>(size - offset),
            static_cast<std::int64_t>(false_bitmap.size()),
            static_cast<std::int64_t>(offset),
            std::move(arr_buffs),
            nullptr,
            repeat_view<bool>{true, 0},
            nullptr,
            true
        );
    }

    inline std::vector<std::vector<byte_t>> make_testing_bytes(std::size_t n)
    {
        std::vector<std::vector<byte_t>> res(n);
        res[0] = {byte_t(0), byte_t(1)};
        for (size_t i = 1; i < n; ++i)
        {
            std::byte b0 = res[i - 1][1];
            auto b1 = static_cast<byte_t>(int(res[i - 1][0]) + int(res[i - 1][1]));
            if (i % 3 == 0)
            {
                res[i] = {b0, b1};
            }
            else
            {
                auto b2 = static_cast<byte_t>(int(res[i - 1][0]) - int(res[i - 1][1]));
                if (i % 2 == 0)
                {
                    res[i] = {b0, b1, b2};
                }
                else
                {
                    std::byte b3 = res[i - 1][0];
                    res[i] = {b0, b1, b2, b3};
                }
            }
        }
        return res;
    }

    template <>
    inline void fill_schema_and_array<std::vector<std::byte>>(
        ArrowSchema& schema,
        ArrowArray& arr,
        size_t size,
        size_t offset,
        const std::vector<size_t>& false_bitmap
    )
    {
        const repeat_view<bool> children_ownership(true, 0);

        sparrow::fill_arrow_schema(
            schema,
            std::string_view("z"),
            "test",
            metadata_sample_opt,
            std::nullopt,
            nullptr,
            children_ownership,
            nullptr,
            true
        );

        using buffer_type = sparrow::buffer<std::uint8_t>;

        auto bytes = make_testing_bytes(size);
        std::size_t value_size = std::accumulate(
            bytes.cbegin(),
            bytes.cbegin() + std::ptrdiff_t(size),
            std::size_t(0),
            [](std::size_t res, const auto& s)
            {
                return res + s.size();
            }
        );

        buffer_type offset_buf(sizeof(std::int32_t) * (size + 1));
        buffer_type value_buf(sizeof(char) * value_size);
        {
            std::int32_t* offset_data = offset_buf.data<std::int32_t>();
            offset_data[0] = 0;
            byte_t* ptr = value_buf.data<byte_t>();
            for (std::size_t i = 0; i < size; ++i)
            {
                offset_data[i + 1] = offset_data[i] + static_cast<std::int32_t>(bytes[i].size());
                sparrow::ranges::copy(bytes[i], ptr);
                ptr += bytes[i].size();
            }
        }

        std::vector<buffer_type> arr_buffs = {
            sparrow::test::make_bitmap_buffer(size, false_bitmap),
            std::move(offset_buf),
            std::move(value_buf)
        };

        sparrow::fill_arrow_array(
            arr,
            static_cast<std::int64_t>(size - offset),
            static_cast<std::int64_t>(false_bitmap.size()),
            static_cast<std::int64_t>(offset),
            std::move(arr_buffs),
            nullptr,
            children_ownership,
            nullptr,
            true
        );
    }

    template <>
    inline void fill_schema_and_array<sparrow::null_type>(
        ArrowSchema& schema,
        ArrowArray& arr,
        size_t size,
        size_t offset,
        const std::vector<size_t>&
    )
    {
        const repeat_view<bool> children_ownership(true, 0);
        sparrow::fill_arrow_schema(
            schema,
            std::string_view("n"),
            "test",
            metadata_sample_opt,
            std::nullopt,
            nullptr,
            children_ownership,
            nullptr,
            true
        );

        using buffer_type = sparrow::buffer<std::uint8_t>;
        std::vector<buffer_type> arr_buffs = {};

        sparrow::fill_arrow_array(
            arr,
            static_cast<std::int64_t>(size - offset),
            static_cast<std::int64_t>(size - offset),
            static_cast<std::int64_t>(offset),
            std::move(arr_buffs),
            nullptr,
            children_ownership,
            nullptr,
            true
        );
    }

    template <class T>
    arrow_proxy make_arrow_proxy(std::size_t n = 10, std::size_t offset = 0)
    {
        ArrowSchema sc{};
        ArrowArray ar{};
        test::fill_schema_and_array<T>(sc, ar, n, offset, {});
        return arrow_proxy(std::move(ar), std::move(sc));
    }

    void fill_schema_and_array_for_list_layout(
        ArrowSchema& schema,
        ArrowArray& arr,
        ArrowSchema&& flat_value_schema,
        ArrowArray&& flat_value_arr,
        const std::vector<std::size_t>& list_lengths,
        const std::vector<std::size_t>& false_postions,
        bool big_list
    );

    void fill_schema_and_array_for_list_view_layout(
        ArrowSchema& schema,
        ArrowArray& arr,
        ArrowSchema&& flat_value_schema,
        ArrowArray&& flat_value_arr,
        const std::vector<std::size_t>& list_lengths,
        const std::vector<std::size_t>& false_postions,
        bool big_list
    );

    void fill_schema_and_array_for_fixed_size_list_layout(
        ArrowSchema& schema,
        ArrowArray& arr,
        ArrowSchema&& flat_value_schema,
        ArrowArray&& flat_value_arr,
        const std::vector<std::size_t>& false_postions,
        std::size_t list_size
    );


    void fill_schema_and_array_for_struct_layout(
        ArrowSchema& schema,
        ArrowArray& arr,
        std::vector<ArrowSchema>&& children_schemas,
        std::vector<ArrowArray>&& children_arrays,
        const std::vector<std::size_t>& false_postions
    );

    void fill_schema_and_array_for_sparse_union(
        ArrowSchema& schema,
        ArrowArray& arr,
        std::vector<ArrowSchema>&& children_schemas,
        std::vector<ArrowArray>&& children_arrays,
        const std::vector<std::uint8_t>& type_ids,
        const std::string& format
    );

    void fill_schema_and_array_for_dense_union(
        ArrowSchema& schema,
        ArrowArray& arr,
        std::vector<ArrowSchema>&& children_schemas,
        std::vector<ArrowArray>&& children_arrays,
        const std::vector<std::uint8_t>& type_ids,
        const std::vector<std::int32_t>& offsets,
        const std::string& format
    );

}
