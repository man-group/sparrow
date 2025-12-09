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

#include "external_array_data_creation.hpp"

#include "sparrow/utils/repeat_container.hpp"


#ifdef __GNUC__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-align"
#endif


namespace sparrow::test
{
    namespace detail
    {
        template <class T>
        void release_external_common_arrow(T* t)
        {
            if (t->dictionary)
            {
                delete t->dictionary;
                t->dictionary = nullptr;
            }

            for (std::int64_t i = 0; i < t->n_children; ++i)
            {
                t->children[i]->release(t->children[i]);
                delete t->children[i];
            }
            delete[] t->children;
            t->children = nullptr;

            t->release = nullptr;
        }
    }

    void release_external_arrow_schema(ArrowSchema* schema)
    {
        detail::release_external_common_arrow(schema);
    }

    void release_external_arrow_array(ArrowArray* arr)
    {
        for (std::int64_t i = 0; i < arr->n_buffers; ++i)
        {
            delete[] reinterpret_cast<const std::uint8_t*>(arr->buffers[i]);
        }
        delete[] reinterpret_cast<const std::uint8_t**>(arr->buffers);
        arr->buffers = nullptr;
        detail::release_external_common_arrow(arr);
    }

    sparrow::buffer<std::uint8_t> make_offset_buffer_from_sizes(const std::vector<size_t>& sizes, bool big)
    {
        const auto n = sizes.size() + 1;
        const auto buf_size = n * (big ? sizeof(std::uint64_t) : sizeof(std::uint32_t));
        auto buf = new std::uint8_t[buf_size];
        if (big)
        {
            auto* ptr = reinterpret_cast<std::uint64_t*>(buf);
            ptr[0] = 0;
            for (std::size_t i = 0; i < sizes.size(); ++i)
            {
                ptr[i + 1] = ptr[i] + static_cast<std::uint64_t>(sizes[i]);
            }
        }
        else
        {
            auto* ptr = reinterpret_cast<std::uint32_t*>(buf);
            ptr[0] = 0;
            for (std::size_t i = 0; i < sizes.size(); ++i)
            {
                ptr[i + 1] = ptr[i] + static_cast<std::uint32_t>(sizes[i]);
            }
        }
        return {buf, buf_size};
    }

    sparrow::buffer<std::uint8_t> make_size_buffer(const std::vector<size_t>& sizes, bool big)
    {
        const auto buf_size = sizes.size() * (big ? sizeof(std::uint64_t) : sizeof(std::uint32_t));
        std::uint8_t* buf = new std::uint8_t[buf_size];
        if (big)
        {
            std::uint64_t* size_buf = reinterpret_cast<std::uint64_t*>(buf);
            for (std::size_t i = 0; i < sizes.size(); ++i)
            {
                size_buf[i] = sizes[i];
            }
        }
        else
        {
            std::uint32_t* size_buf = reinterpret_cast<std::uint32_t*>(buf);
            for (std::size_t i = 0; i < sizes.size(); ++i)
            {
                size_buf[i] = static_cast<std::uint32_t>(sizes[i]);
            }
        }
        return {buf, buf_size};
    }

    void fill_schema_and_array_for_list_layout(
        ArrowSchema& schema,
        ArrowArray& arr,
        ArrowSchema&& flat_value_schema,
        ArrowArray&& flat_value_arr,
        const std::vector<std::size_t>& list_lengths,
        const std::vector<std::size_t>& false_positions,
        bool big_list
    )
    {
        ArrowSchema** schema_children = new ArrowSchema*[1];
        schema_children[0] = new ArrowSchema(std::move(flat_value_schema));
        sparrow::fill_arrow_schema(
            schema,
            std::string_view(big_list ? "+L" : "+l"),
            "test",
            metadata_sample_opt,
            std::nullopt,
            schema_children,
            repeat_view<bool>(true, 1),
            nullptr,
            true
        );

        std::vector<std::variant<sparrow::buffer<uint8_t>, sparrow::buffer_view<const uint8_t>>> arr_buffs = {
            sparrow::test::make_bitmap_buffer(list_lengths.size(), false_positions),
            make_offset_buffer_from_sizes(list_lengths, big_list)
        };

        ArrowArray** array_children = new ArrowArray*[1];
        array_children[0] = new ArrowArray(std::move(flat_value_arr));
        sparrow::fill_arrow_array(
            arr,
            static_cast<std::int64_t>(list_lengths.size()),
            static_cast<std::int64_t>(false_positions.size()),
            0,
            std::move(arr_buffs),
            array_children,
            repeat_view<bool>(true, 1),
            nullptr,
            true
        );
    }

    void fill_schema_and_array_for_fixed_size_list_layout(
        ArrowSchema& schema,
        ArrowArray& arr,
        ArrowSchema&& flat_value_schema,
        ArrowArray&& flat_value_arr,
        const std::vector<std::size_t>& false_positions,
        std::size_t list_size
    )
    {
        SPARROW_ASSERT(list_size > 0, "list size must be greater than 0");
        SPARROW_ASSERT(list_size < 10, "just a test limitation st. format string can be on stack");
        // convert list size to string
        std::string format = std::string("+w:") + std::to_string(list_size)[0];
        ArrowSchema** schema_children = new ArrowSchema*[1];
        schema_children[0] = new ArrowSchema(std::move(flat_value_schema));
        sparrow::fill_arrow_schema(
            schema,
            std::move(format),
            "test",
            metadata_sample_opt,
            std::nullopt,
            schema_children,
            repeat_view<bool>(true, 1),
            nullptr,
            true
        );

        std::size_t arr_size = static_cast<std::size_t>(flat_value_arr.length) / list_size;
        std::vector<std::variant<sparrow::buffer<uint8_t>, sparrow::buffer_view<const uint8_t>>> arr_buffs = {sparrow::test::make_bitmap_buffer(arr_size, false_positions)};

        ArrowArray** array_children = new ArrowArray*[1];
        array_children[0] = new ArrowArray(std::move(flat_value_arr));
        sparrow::fill_arrow_array(
            arr,
            static_cast<std::int64_t>(arr_size),
            static_cast<std::int64_t>(false_positions.size()),
            0,
            std::move(arr_buffs),
            array_children,
            repeat_view<bool>(true, 1),
            nullptr,
            true
        );
    }

    void fill_schema_and_array_for_list_view_layout(
        ArrowSchema& schema,
        ArrowArray& arr,
        ArrowSchema&& flat_value_schema,
        ArrowArray&& flat_value_arr,
        const std::vector<std::size_t>& list_lengths,
        const std::vector<std::size_t>& false_positions,
        bool big_list
    )
    {
        ArrowSchema** schema_children = new ArrowSchema*[1];
        schema_children[0] = new ArrowSchema(std::move(flat_value_schema));
        sparrow::fill_arrow_schema(
            schema,
            std::string_view(big_list ? "+vL" : "+vl"),
            "test",
            metadata_sample_opt,
            std::nullopt,
            schema_children,
            repeat_view<bool>(true, 1),
            nullptr,
            true
        );

        std::vector<std::variant<sparrow::buffer<uint8_t>, sparrow::buffer_view<const uint8_t>>> arr_buffs = {
            sparrow::test::make_bitmap_buffer(list_lengths.size(), false_positions),
            make_offset_buffer_from_sizes(list_lengths, big_list),
            make_size_buffer(list_lengths, big_list)
        };

        ArrowArray** array_children = new ArrowArray*[1];
        array_children[0] = new ArrowArray(std::move(flat_value_arr));
        sparrow::fill_arrow_array(
            arr,
            static_cast<std::int64_t>(list_lengths.size()),
            static_cast<std::int64_t>(false_positions.size()),
            0,
            std::move(arr_buffs),
            array_children,
            repeat_view<bool>(true, 1),
            nullptr,
            true
        );
    }

    void fill_schema_and_array_for_struct_layout(
        ArrowSchema& schema,
        ArrowArray& arr,
        std::vector<ArrowSchema>&& children_schemas,
        std::vector<ArrowArray>&& children_arrays,
        const std::vector<std::size_t>& false_positions
    )
    {
        ArrowSchema** schema_children = new ArrowSchema*[children_schemas.size()];
        std::transform(
            std::make_move_iterator(children_schemas.begin()),
            std::make_move_iterator(children_schemas.end()),
            schema_children,
            [](auto&& child)
            {
                return new ArrowSchema(std::move(child));
            }
        );
        sparrow::fill_arrow_schema(
            schema,
            std::string_view("+s"),
            "test",
            metadata_sample_opt,
            std::nullopt,
            schema_children,
            repeat_view<bool>(true, children_schemas.size()),
            nullptr,
            true
        );


        int64_t length = children_arrays.front().length;
        std::vector<std::variant<sparrow::buffer<uint8_t>, sparrow::buffer_view<const uint8_t>>> arr_buffs = {
            sparrow::test::make_bitmap_buffer(static_cast<std::size_t>(length), false_positions),
        };

        ArrowArray** array_children = new ArrowArray*[children_arrays.size()];
        std::transform(
            std::make_move_iterator(children_arrays.begin()),
            std::make_move_iterator(children_arrays.end()),
            array_children,
            [](auto&& child)
            {
                return new ArrowArray(std::move(child));
            }
        );
        sparrow::fill_arrow_array(
            arr,
            length,
            static_cast<std::int64_t>(false_positions.size()),
            0,
            std::move(arr_buffs),
            array_children,
            repeat_view<bool>(true, children_arrays.size()),
            nullptr,
            true
        );
    }

    void fill_schema_and_array_for_sparse_union(
        ArrowSchema& schema,
        ArrowArray& arr,
        std::vector<ArrowSchema>&& children_schemas,
        std::vector<ArrowArray>&& children_arrays,
        const std::vector<std::uint8_t>& type_ids,
        const std::string& format
    )
    {
        ArrowSchema** schema_children = new ArrowSchema*[children_schemas.size()];
        std::transform(
            std::make_move_iterator(children_schemas.begin()),
            std::make_move_iterator(children_schemas.end()),
            schema_children,
            [](auto&& child)
            {
                return new ArrowSchema(std::move(child));
            }
        );
        sparrow::fill_arrow_schema(
            schema,
            format,
            "test",
            metadata_sample_opt,
            std::nullopt,
            schema_children,
            repeat_view<bool>(true, children_schemas.size()),
            nullptr,
            true
        );

        using buffer_type = sparrow::buffer<std::uint8_t>;
        buffer_type buf(type_ids.size());
        std::copy(type_ids.begin(), type_ids.end(), buf.begin());
        std::vector<std::variant<sparrow::buffer<uint8_t>, sparrow::buffer_view<const uint8_t>>> arr_buffs = {std::move(buf)};

        ArrowArray** array_children = new ArrowArray*[children_arrays.size()];
        std::transform(
            std::make_move_iterator(children_arrays.begin()),
            std::make_move_iterator(children_arrays.end()),
            array_children,
            [](auto&& child)
            {
                return new ArrowArray(std::move(child));
            }
        );

        sparrow::fill_arrow_array(
            arr,
            static_cast<std::int64_t>(type_ids.size()),
            0,
            0,
            std::move(arr_buffs),
            array_children,
            repeat_view<bool>(true, children_arrays.size()),
            nullptr,
            true
        );
    }

    void fill_schema_and_array_for_dense_union(
        ArrowSchema& schema,
        ArrowArray& arr,
        std::vector<ArrowSchema>&& children_schemas,
        std::vector<ArrowArray>&& children_arrays,
        const std::vector<std::uint8_t>& type_ids,
        const std::vector<std::int32_t>& offsets,
        const std::string& format
    )
    {
        ArrowSchema** schema_children = new ArrowSchema*[children_schemas.size()];
        std::transform(
            std::make_move_iterator(children_schemas.begin()),
            std::make_move_iterator(children_schemas.end()),
            schema_children,
            [](auto&& child)
            {
                return new ArrowSchema(std::move(child));
            }
        );
        sparrow::fill_arrow_schema(
            schema,
            format,
            "test",
            metadata_sample_opt,
            std::nullopt,
            schema_children,
            repeat_view<bool>(true, children_schemas.size()),
            nullptr,
            true
        );

        using buffer_type = sparrow::buffer<std::uint8_t>;

        buffer_type buf0(type_ids.size());
        std::copy(type_ids.begin(), type_ids.end(), buf0.begin());

        buffer_type buf1(offsets.size() * sizeof(std::int32_t));
        std::copy(offsets.begin(), offsets.end(), buf1.data<std::int32_t>());

        std::vector<std::variant<sparrow::buffer<uint8_t>, sparrow::buffer_view<const uint8_t>>> arr_buffs = {std::move(buf0), std::move(buf1)};

        ArrowArray** array_children = new ArrowArray*[children_arrays.size()];
        std::transform(
            std::make_move_iterator(children_arrays.begin()),
            std::make_move_iterator(children_arrays.end()),
            array_children,
            [](auto&& child)
            {
                return new ArrowArray(std::move(child));
            }
        );

        sparrow::fill_arrow_array(
            arr,
            static_cast<std::int64_t>(type_ids.size()),
            0,
            0,
            std::move(arr_buffs),
            array_children,
            repeat_view<bool>(true, children_arrays.size()),
            nullptr,
            true
        );
    }

}
#ifdef __GNUC__
#    pragma GCC diagnostic pop
#endif
