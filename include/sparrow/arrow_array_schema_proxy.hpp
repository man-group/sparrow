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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>

#include "sparrow/arrow_interface/arrow_array/private_data.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_info_utils.hpp"
#include "sparrow/arrow_interface/arrow_schema/private_data.hpp"
#include "sparrow/buffer/buffer_view.hpp"
#include "sparrow/buffer/dynamic_bitset/non_owning_dynamic_bitset.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/config/config.hpp"
#include "sparrow/types/data_type.hpp"

namespace sparrow
{
    /**
     * Exception thrown by the arrow_proxy class.
     */
    class arrow_proxy_exception : public std::runtime_error
    {
    public:

        explicit arrow_proxy_exception(const std::string& message)
            : std::runtime_error(message)
        {
        }
    };

    struct arrow_array_and_schema_pointers
    {
        ArrowArray* array;
        ArrowSchema* schema;
    };

    struct arrow_array_and_schema
    {
        ArrowArray array;
        ArrowSchema schema;
    };

    /**
     * Proxy class over `ArrowArray` and `ArrowSchema`.
     * It ease the use of `ArrowArray` and `ArrowSchema` by providing a more user-friendly interface.
     * It can take ownership of the `ArrowArray` and `ArrowSchema` or use them as pointers.
     * If the arrow_proxy takes ownership of the `ArrowArray` and `ArrowSchema`, they are released when the
     * arrow_proxy is destroyed. Otherwise, the `arrow_proxy` does not release the `ArrowArray` and
     * `ArrowSchema`.
     */
    class arrow_proxy
    {
    public:

        /// Constructs an `arrow_proxy` which takes the ownership of the `ArrowArray` and `ArrowSchema`.
        /// The array and schema are released when the `arrow_proxy` is destroyed.
        SPARROW_API explicit arrow_proxy(ArrowArray&& array, ArrowSchema&& schema);
        /// Constructs an `arrow_proxy` which takes the ownership of the `ArrowArray` and uses the provided
        /// `ArrowSchema`. The array is released when the `arrow_proxy` is destroyed. The schema is not
        /// released.
        SPARROW_API explicit arrow_proxy(ArrowArray&& array, ArrowSchema* schema);
        /// Constructs an `arrow_proxy` which uses the provided `ArrowArray` and `ArrowSchema`.
        /// Neither the array nor the schema are released when the `arrow_proxy` is destroyed.
        SPARROW_API explicit arrow_proxy(ArrowArray* array, ArrowSchema* schema);

        // Copy constructors
        SPARROW_API arrow_proxy(const arrow_proxy&);
        SPARROW_API arrow_proxy& operator=(const arrow_proxy&);

        // Move constructors
        SPARROW_API arrow_proxy(arrow_proxy&&);
        SPARROW_API arrow_proxy& operator=(arrow_proxy&&);

        SPARROW_API ~arrow_proxy();

        [[nodiscard]] SPARROW_API const std::string_view format() const;

        /**
         * Set the format according to the Arrow format specification:
         * https://arrow.apache.org/docs/dev/format/CDataInterface.html#data-type-description-format-strings
         * @exception `arrow_proxy_exception` If the `ArrowSchema` was not created with sparrow.
         * @param format The format to set.
         */
        SPARROW_API void set_format(const std::string_view format);
        [[nodiscard]] SPARROW_API enum data_type data_type() const;

        /**
         * Set the data type. It's a convenient way to set the format of the `ArrowSchema`.
         * @exception `arrow_proxy_exception` If the `ArrowSchema` was not created with sparrow.
         * @param data_type The data type to set.
         */
        void SPARROW_API set_data_type(enum data_type data_type);
        [[nodiscard]] SPARROW_API std::optional<std::string_view> name() const;

        /**
         * Set the name of the `ArrowSchema`.
         * @exception `arrow_proxy_exception` If the `ArrowSchema` was not created with sparrow.
         * @param name The name to set.
         */
        SPARROW_API void set_name(std::optional<std::string_view> name);
        [[nodiscard]] SPARROW_API std::optional<std::string_view> metadata() const;

        /**
         * Set the metadata of the `ArrowSchema`.
         * @exception `arrow_proxy_exception` If the `ArrowSchema` was not created with sparrow.
         * @param metadata The metadata to set.
         */
        SPARROW_API void set_metadata(std::optional<std::string_view> metadata);
        [[nodiscard]] SPARROW_API std::vector<ArrowFlag> flags() const;

        /**
         * Set the flags of the `ArrowSchema`.
         * @exception `arrow_proxy_exception` If the `ArrowSchema` was not created with sparrow.
         * @param flags The flags to set.
         */
        SPARROW_API void set_flags(const std::vector<ArrowFlag>& flags);
        [[nodiscard]] SPARROW_API size_t length() const;

        /**
         * Set the length of the `ArrowArray`. This method does not resize the buffers of the `ArrowArray`.
         * You have to change the length before replacing/resizing the buffers to have the right sizes when
         * calling `buffers()`.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @param length The length to set.
         */
        SPARROW_API void set_length(size_t length);
        [[nodiscard]] SPARROW_API int64_t null_count() const;

        /**
         * Set the null count of the `ArrowArray`. This method does not change the bitmap.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @param null_count The null count to set.
         */
        SPARROW_API void set_null_count(int64_t null_count);
        [[nodiscard]] SPARROW_API size_t offset() const;

        /**
         * Set the offset of the `ArrowArray`.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @param offset The offset to set.
         */
        SPARROW_API void set_offset(size_t offset);
        [[nodiscard]] SPARROW_API size_t n_buffers() const;

        /**
         * Set the number of buffers of the `ArrowArray`. Resize the buffers vector of the `ArrowArray`
         * private data.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @param n_buffers The number of buffers to set.
         */
        SPARROW_API void set_n_buffers(size_t n_buffers);
        [[nodiscard]] SPARROW_API size_t n_children() const;
        [[nodiscard]] SPARROW_API const std::vector<sparrow::buffer_view<uint8_t>>& buffers() const;
        [[nodiscard]] SPARROW_API std::vector<sparrow::buffer_view<uint8_t>>& buffers();

        /**
         * Set the buffer at the given index. You have to call the `set_length` method before calling this
         * method to have the right sizes when calling `buffers()`.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @param index The index of the buffer to set.
         * @param buffer The buffer to set.
         */
        SPARROW_API void set_buffer(size_t index, const buffer_view<uint8_t>& buffer);

        /**
         * Set the buffer at the given index. You have to call the `set_length` method before calling this
         * method to have the right sizes when calling `buffers()`.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @param index The index of the buffer to set.
         * @param buffer The buffer to set.
         */
        SPARROW_API void set_buffer(size_t index, buffer<uint8_t>&& buffer);

        /**
         * Resize the bitmap buffer of the `ArrowArray`.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @exception `arrow_proxy_exception` If the array format does not support a validity bitmap.
         * @param new_size The new size of the bitmap buffer.
         * @param value The value to set in the new elements. True by default.
         */
        SPARROW_API void resize_bitmap(size_t new_size, bool value = true);

        /**
         * Insert elements of the same value in the bitmap buffer at the given index.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @exception `arrow_proxy_exception` If the array format does not support a validity bitmap.
         * @exception `std::out_of_range` If the index is greater than the length of the bitmap.
         * @param index The index where to insert the value. Must be less than the length of the bitmap.
         * @param value The value to insert.
         * @param count The number of times to insert the value. 1 by default
         * @return The index of the first inserted value.
         */
        SPARROW_API size_t insert_bitmap(size_t index, bool value, size_t count = 1);

        /**
         * Insert several elements in the bitmap buffer at the given index.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @exception `arrow_proxy_exception` If the array format does not support a validity bitmap.
         * @exception `std::out_of_range` If the index is greater than the length of the bitmap.
         * @param index The index where to insert the values. Must be less than the length of the bitmap.
         * @param range The range of values to insert.
         * @return The index of the first inserted value.
         */
        template <std::ranges::input_range R>
        size_t insert_bitmap(size_t index, const R& range);

        /**
         * Erase several elements in the bitmap buffer at the given index.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @exception `arrow_proxy_exception` If the array format does not support a validity bitmap.
         * @exception `std::out_of_range` If the index is greater than the length of the bitmap.
         * @param index The index of the first value to erase. Must be less than the length of the bitmap.
         * @param count The number of elements to erase. 1 by default.
         * @return The index of the first erased value.
         */
        SPARROW_API size_t erase_bitmap(size_t index, size_t count = 1);

        /**
         * Push a value at the end of the bitmap buffer.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @exception `arrow_proxy_exception` If the array format does not support a validity bitmap.
         * @param value The value to push.
         */
        SPARROW_API void push_back_bitmap(bool value);

        /**
         * Pop a value at the end of the bitmap buffer.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @exception `arrow_proxy_exception` If the array format does not support a validity bitmap.
         */
        SPARROW_API void pop_back_bitmap();

        /**
         * Add children without taking their ownership.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or the `ArrowSchema` wrapped
         * in this proxy were not created with sparrow.
         * @param arrow_array_and_schema_pointers The children to add.
         */
        template <std::ranges::input_range R>
            requires std::same_as<std::ranges::range_value_t<R>, arrow_array_and_schema_pointers>
        void add_children(const R& arrow_array_and_schema_pointers);

        /**
         * Add children and take their ownership.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or the `ArrowSchema` wrapped
         * in this proxy were not created with sparrow.
         * @param arrow_array_and_schema The children to add.
         */
        template <std::ranges::input_range R>
            requires std::same_as<std::ranges::range_value_t<R>, arrow_array_and_schema>
        void add_children(R&& arrow_array_and_schemas);

        /**
         * Add a child without taking its ownership.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or the `ArrowSchema` wrapped
         * in this proxy were not created with sparrow.
         * @param array The `ArrowArray` to set as child.
         * @param schema The `ArrowSchema` to set as child.
         */
        SPARROW_API void add_child(ArrowArray* array, ArrowSchema* schema);

        /**
         * Add a child and takes its ownership.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or the `ArrowSchema` wrapped
         * in this proxy were not created with sparrow.
         * @param array The `ArrowArray` to set as child.
         * @param schema The `ArrowSchema` to set as child.
         */
        SPARROW_API void add_child(ArrowArray&& array, ArrowSchema&& schema);

        /**
         * Pop n children. If the children were created by sparrow or are owned by the proxy,
         * it will delete them.
         * @param n The number of children to pop.
         */
        SPARROW_API void pop_children(size_t n);

        /**
         * Set the child at the given index. It does not take the ownership on the `ArrowArray` and
         * `ArrowSchema` passed by pointers.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or the `ArrowSchema` wrapped
         * in this proxy were not created with sparrow.
         * @param index The index of the child to set.
         * @param array The `ArrowArray` to set as child.
         * @param schema The `ArrowSchema` to set as child.
         */
        SPARROW_API void set_child(size_t index, ArrowArray* array, ArrowSchema* schema);

        /**
         * Set the child at the given index. It takes the ownership on the `ArrowArray` and`ArrowSchema`
         * passed by rvalue referencess.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or `ArrowSchema` wrapped
         * in this proxy were not created with
         * sparrow.
         * @param index The index of the child to set.
         * @param array The `ArrowArray` to set as child.
         * @param schema The `ArrowSchema` to set as child.
         */
        SPARROW_API void set_child(size_t index, ArrowArray&& array, ArrowSchema&& schema);

        [[nodiscard]] SPARROW_API const std::vector<arrow_proxy>& children() const;
        [[nodiscard]] SPARROW_API std::vector<arrow_proxy>& children();

        [[nodiscard]] SPARROW_API const std::unique_ptr<arrow_proxy>& dictionary() const;
        [[nodiscard]] SPARROW_API std::unique_ptr<arrow_proxy>& dictionary();

        /**
         * Set the dictionary. It takes the ownership on the `ArrowArray` and `ArrowSchema` passed by
         * pointers.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or `ArrowSchema` were not created with
         * sparrow.
         * @param array The `ArrowArray` to set.
         * @param schema The `ArrowSchema` to set.
         */
        SPARROW_API void set_dictionary(ArrowArray* array, ArrowSchema* schema);

        [[nodiscard]] SPARROW_API bool is_created_with_sparrow() const;

        [[nodiscard]] SPARROW_API void* private_data() const;

        /**
         * get a non-owning view of the arrow_proxy.
         */
        [[nodiscard]] SPARROW_API arrow_proxy view() const;

        [[nodiscard]] SPARROW_API bool owns_array() const;
        [[nodiscard]] SPARROW_API ArrowArray extract_array();
        [[nodiscard]] SPARROW_API ArrowArray& array();
        [[nodiscard]] SPARROW_API const ArrowArray& array() const;

        [[nodiscard]] SPARROW_API bool owns_schema() const;
        [[nodiscard]] SPARROW_API ArrowSchema extract_schema();
        [[nodiscard]] SPARROW_API ArrowSchema& schema();
        [[nodiscard]] SPARROW_API const ArrowSchema& schema() const;

        [[nodiscard]] SPARROW_API arrow_schema_private_data* get_schema_private_data();
        [[nodiscard]] SPARROW_API arrow_array_private_data* get_array_private_data();

        /**
         * Slices the array to keep only the elements between the given \p start and \p end.
         * A copy of the \ref array is modified. The data is not modified, only the ArrowArray.offset and
         * ArrowArray.length are updated. If \p end is greater than the size of the buffers, the following
         * elements will be invalid.
         *
         * @param start The index of the first element to keep. Must be less than \p end.
         * @param end The index of the first element to discard. Must be less than the size of the buffers.
         */
        [[nodiscard]] SPARROW_API arrow_proxy slice(size_t start, size_t end) const;

        /**
         * Slices the array to keep only the elements between the given \p start and \p end.
         * A view of the \ref array is returned. The data is not modified, only the ArrowArray.offset and
         * ArrowArray.length are updated. If \p end is greater than the size of the buffers, the following
         * elements will be invalid.
         *
         * @param start The index of the first element to keep. Must be less than \p end.
         * @param end The index of the first element to discard. Must be less than the size of the buffers.
         */
        [[nodiscard]] SPARROW_API arrow_proxy slice_view(size_t start, size_t end) const;

        /**
         * Refresh the buffers views. This method should be called after modifying the buffers of the array.
         */
        SPARROW_API void update_buffers();

    private:

        std::variant<ArrowArray*, ArrowArray> m_array;
        std::variant<ArrowSchema*, ArrowSchema> m_schema;
        std::vector<sparrow::buffer_view<uint8_t>> m_buffers;
        std::vector<arrow_proxy> m_children;
        std::unique_ptr<arrow_proxy> m_dictionary;

        struct impl_tag
        {
        };

        // Build an empty proxy. Convenient for resizing vector of children
        arrow_proxy();

        template <typename AA, typename AS>
            requires std::same_as<std::remove_pointer_t<std::remove_cvref_t<AA>>, ArrowArray>
                     && std::same_as<std::remove_pointer_t<std::remove_cvref_t<AS>>, ArrowSchema>
        arrow_proxy(AA&& array, AS&& schema, impl_tag);

        [[nodiscard]] bool empty() const;
        SPARROW_API void resize_children(size_t children_count);

        [[nodiscard]] SPARROW_API non_owning_dynamic_bitset<uint8_t> get_non_owning_dynamic_bitset();

        void update_children();
        void update_dictionary();
        void update_null_count();
        void reset();

        [[nodiscard]] bool array_created_with_sparrow() const;
        [[nodiscard]] bool schema_created_with_sparrow() const;

        void validate_array_and_schema() const;

        [[nodiscard]] bool is_arrow_array_valid() const;
        [[nodiscard]] bool is_arrow_schema_valid() const;
        [[nodiscard]] bool is_proxy_valid() const;

        [[nodiscard]] size_t get_null_count() const;

        void swap(arrow_proxy& other) noexcept;
    };

    template <std::ranges::input_range R>
        requires std::same_as<std::ranges::range_value_t<R>, arrow_array_and_schema_pointers>
    void arrow_proxy::add_children(const R& arrow_array_and_schema_pointers)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set n_buffers on non-sparrow created ArrowArray or ArrowSchema");
        }

        const size_t add_children_count = std::ranges::size(arrow_array_and_schema_pointers);
        const size_t original_children_count = n_children();
        const size_t new_children_count = original_children_count + add_children_count;

        resize_children(new_children_count);
        for (size_t i = 0; i < add_children_count; ++i)
        {
            set_child(
                i + original_children_count,
                arrow_array_and_schema_pointers[i].array,
                arrow_array_and_schema_pointers[i].schema
            );
        }
    }

    template <std::ranges::input_range R>
        requires std::same_as<std::ranges::range_value_t<R>, arrow_array_and_schema>
    void arrow_proxy::add_children(R&& arrow_arrays_and_schemas)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set n_buffers on non-sparrow created ArrowArray or ArrowSchema");
        }

        const size_t add_children_count = std::ranges::size(arrow_arrays_and_schemas);
        const size_t original_children_count = n_children();
        const size_t new_children_count = original_children_count + add_children_count;

        resize_children(new_children_count);
        for (size_t i = 0; i < add_children_count; ++i)
        {
            set_child(
                i + original_children_count,
                std::move(arrow_arrays_and_schemas[i].array),
                std::move(arrow_arrays_and_schemas[i].schema)
            );
        }
    }

    template <std::ranges::input_range R>
    inline size_t arrow_proxy::insert_bitmap(size_t index, const R& range)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot modify the bitmap on non-sparrow created ArrowArray");
        }
        SPARROW_ASSERT_TRUE(has_bitmap(data_type()))
        auto bitmap = get_non_owning_dynamic_bitset();
        const auto it = bitmap.insert(sparrow::next(bitmap.cbegin(), index), range.begin(), range.end());
        return static_cast<size_t>(std::distance(bitmap.begin(), it));
    }
}

#if defined(__cpp_lib_format)

template <>
struct std::formatter<sparrow::buffer_view<uint8_t>>
{
private:

    char delimiter = ' ';
    static constexpr std::string_view opening = "[";
    static constexpr std::string_view closing = "]";

public:

    constexpr auto parse(std::format_parse_context& ctx)
    {
        auto it = ctx.begin();
        auto end = ctx.end();

        // Parse optional delimiter
        if (it != end && *it != '}')
        {
            delimiter = *it++;
        }

        if (it != end && *it != '}')
        {
            throw std::format_error("Invalid format specifier for range");
        }

        return it;
    }

    auto format(const sparrow::buffer_view<uint8_t>& range, std::format_context& ctx) const
    {
        auto out = ctx.out();

        // Write opening bracket
        out = std::ranges::copy(opening, out).out;

        // Write range elements
        bool first = true;
        for (const auto& elem : range)
        {
            if (!first)
            {
                *out++ = delimiter;
            }
            out = std::format_to(out, "{}", elem);
            first = false;
        }

        // Write closing bracket
        out = std::ranges::copy(closing, out).out;

        return out;
    }
};

inline std::ostream& operator<<(std::ostream& os, const sparrow::buffer_view<uint8_t>& value)
{
    os << std::format("{}", value);
    return os;
}

template <>
struct std::formatter<sparrow::arrow_proxy>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::arrow_proxy& obj, std::format_context& ctx) const
    {
        std::string buffers_description_str;
        for (size_t i = 0; i < obj.n_buffers(); ++i)
        {
            std::format_to(
                std::back_inserter(buffers_description_str),
                "<{}[{} b]{}",
                "uint8_t",
                obj.buffers()[i].size() * sizeof(uint8_t),
                obj.buffers()[i]
            );
        }

        std::string children_str;
        for (const auto& child : obj.children())
        {
            std::format_to(std::back_inserter(children_str), "{}\n", child);
        }

        const std::string dictionary_str = obj.dictionary() ? std::format("{}", *obj.dictionary()) : "nullptr";

        return std::format_to(
            ctx.out(),
            "arrow_proxy\n- format: {}\n- name; {}\n- metadata: {}\n- data_type: {}\n- null_count:{}\n- length: {}\n- offset: {}\n- n_buffers: {}\n- buffers:\n{}\n- n_children: {}\n-children: {}\n- dictionary: {}",
            obj.format(),
            obj.name().value_or(""),
            obj.metadata().value_or(""),
            obj.data_type(),
            obj.null_count(),
            obj.length(),
            obj.offset(),
            obj.n_buffers(),
            buffers_description_str,
            obj.n_children(),
            children_str,
            dictionary_str
        );
    }
};

inline std::ostream& operator<<(std::ostream& os, const sparrow::arrow_proxy& value)
{
    os << std::format("{}", value);
    return os;
}

#endif
