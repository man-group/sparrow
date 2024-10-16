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

#include <optional>
#include <string_view>

#include "sparrow/arrow_interface/arrow_array/private_data.hpp"
#include "sparrow/arrow_interface/arrow_schema/private_data.hpp"
#include "sparrow/buffer/buffer_view.hpp"
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

    /**
     * Proxy class over `ArrowArray` and `ArrowSchema`.
     * It ease the use of `ArrowArray` and `ArrowSchema` by providing a more user-friendly interface.
     * It can take ownership of the `ArrowArray` and `ArrowSchema` or use them as pointers.
     * If the arrow_proxy takes ownership of the `ArrowArray` and `ArrowSchema`, they are released when the
     * arrow_proxy is destroyed. Otherwise, the `arrow_proxy` does not release the `ArrowArray` and
     * `ArrowSchema`.
     */
    class SPARROW_API arrow_proxy
    {
    public:

        /// Constructs an `arrow_proxy` which takes the ownership of the `ArrowArray` and `ArrowSchema`.
        /// The array and schema are released when the `arrow_proxy` is destroyed.
        explicit arrow_proxy(ArrowArray&& array, ArrowSchema&& schema);
        /// Constructs an `arrow_proxy` which takes the ownership of the `ArrowArray` and uses the provided
        /// `ArrowSchema`. The array is released when the `arrow_proxy` is destroyed. The schema is not
        /// released.
        explicit arrow_proxy(ArrowArray&& array, ArrowSchema* schema);
        /// Constructs an `arrow_proxy` which uses the provided `ArrowArray` and `ArrowSchema`.
        /// Neither the array nor the schema are released when the `arrow_proxy` is destroyed.
        explicit arrow_proxy(ArrowArray* array, ArrowSchema* schema);

        // Copy constructors
        arrow_proxy(const arrow_proxy&);
        arrow_proxy& operator=(const arrow_proxy&);

        // Move constructors
        arrow_proxy(arrow_proxy&&);
        arrow_proxy& operator=(arrow_proxy&&);

        ~arrow_proxy();

        [[nodiscard]] const std::string_view format() const;

        /**
         * Set the format according to the Arrow format specification:
         * https://arrow.apache.org/docs/dev/format/CDataInterface.html#data-type-description-format-strings
         * @exception `arrow_proxy_exception` If the `ArrowSchema` was not created with sparrow.
         * @param format The format to set.
         */
        void set_format(const std::string_view format);
        [[nodiscard]] enum data_type data_type() const;

        /**
         * Set the data type. It's a convenient way to set the format of the `ArrowSchema`.
         * @exception `arrow_proxy_exception` If the `ArrowSchema` was not created with sparrow.
         * @param data_type The data type to set.
         */
        void set_data_type(enum data_type data_type);
        [[nodiscard]] std::optional<const std::string_view> name() const;

        /**
         * Set the name of the `ArrowSchema`.
         * @exception `arrow_proxy_exception` If the `ArrowSchema` was not created with sparrow.
         * @param name The name to set.
         */
        void set_name(std::optional<const std::string_view> name);
        [[nodiscard]] std::optional<const std::string_view> metadata() const;

        /**
         * Set the metadata of the `ArrowSchema`.
         * @exception `arrow_proxy_exception` If the `ArrowSchema` was not created with sparrow.
         * @param metadata The metadata to set.
         */
        void set_metadata(std::optional<const std::string_view> metadata);
        [[nodiscard]] std::vector<ArrowFlag> flags() const;

        /**
         * Set the flags of the `ArrowSchema`.
         * @exception `arrow_proxy_exception` If the `ArrowSchema` was not created with sparrow.
         * @param flags The flags to set.
         */
        void set_flags(const std::vector<ArrowFlag>& flags);
        [[nodiscard]] size_t length() const;

        /**
         * Set the length of the `ArrowArray`.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @param length The length to set.
         */
        void set_length(size_t length);
        [[nodiscard]] int64_t null_count() const;

        /**
         * Set the null count of the `ArrowArray`.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @param null_count The null count to set.
         */
        void set_null_count(int64_t null_count);
        [[nodiscard]] size_t offset() const;

        /**
         * Set the offset of the `ArrowArray`.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @param offset The offset to set.
         */
        void set_offset(size_t offset);
        [[nodiscard]] size_t n_buffers() const;

        /**
         * Set the number of buffers of the `ArrowArray`.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @param n_buffers The number of buffers to set.
         */
        void set_n_buffers(size_t n_buffers);
        [[nodiscard]] size_t n_children() const;
        [[nodiscard]] const std::vector<sparrow::buffer_view<uint8_t>>& buffers() const;
        [[nodiscard]] std::vector<sparrow::buffer_view<uint8_t>>& buffers();

        /**
         * Set the buffer at the given index.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @param index The index of the buffer to set.
         * @param buffer The buffer to set.
         */
        void set_buffer(size_t index, const buffer_view<uint8_t>& buffer);

        /**
         * Set the buffer at the given index.
         * @exception `arrow_proxy_exception` If the `ArrowArray` was not created with sparrow.
         * @param index The index of the buffer to set.
         * @param buffer The buffer to set.
         */
        void set_buffer(size_t index, buffer<uint8_t>&& buffer);

        /**
         * Add children.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or `ArrowSchema` were not created with
         * sparrow.
         * @param arrow_array_and_schema_pointers The children to add.
         */
        template <std::ranges::input_range R>
            requires std::same_as<std::ranges::range_value_t<R>, arrow_array_and_schema_pointers>
        void add_children(const R& arrow_array_and_schema_pointers);

        /**
         * Pop n children.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or `ArrowSchema` were not created with
         * sparrow.
         * @param n The number of children to pop.
         */
        void pop_children(size_t n);

        [[nodiscard]] const std::vector<arrow_proxy>& children() const;
        [[nodiscard]] std::vector<arrow_proxy>& children();

        /**
         * Set the child at the given index. It takes the ownership on the `ArrowArray` and `ArrowSchema`
         * passed by pointers.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or `ArrowSchema` were not created with
         * sparrow.
         * @param index The index of the child to set.
         * @param array The `ArrowArray` to set as child.
         * @param schema The `ArrowSchema` to set as child.
         */
        void set_child(size_t index, ArrowArray* array, ArrowSchema* schema);

        [[nodiscard]] const std::unique_ptr<arrow_proxy>& dictionary() const;
        [[nodiscard]] std::unique_ptr<arrow_proxy>& dictionary();

        /**
         * Set the dictionary. It takes the ownership on the `ArrowArray` and `ArrowSchema` passed by
         * pointers.
         * @exception `arrow_proxy_exception` If the `ArrowArray` or `ArrowSchema` were not created with
         * sparrow.
         * @param array The `ArrowArray` to set.
         * @param schema The `ArrowSchema` to set.
         */
        void set_dictionary(ArrowArray* array, ArrowSchema* schema);

        [[nodiscard]] bool is_created_with_sparrow() const;

        [[nodiscard]] void* private_data() const;

        /**
         * get a non-owning view of the arrow_proxy.
         */
        [[nodiscard]] arrow_proxy view();

        [[nodiscard]] ArrowArray& array();
        [[nodiscard]] const ArrowArray& array() const;

        [[nodiscard]] ArrowSchema& schema();
        [[nodiscard]] const ArrowSchema& schema() const;

    private:

        std::variant<ArrowArray*, ArrowArray> m_array;
        std::variant<ArrowSchema*, ArrowSchema> m_schema;
        std::vector<sparrow::buffer_view<uint8_t>> m_buffers;
        std::vector<arrow_proxy> m_children;
        std::unique_ptr<arrow_proxy> m_dictionary;

        struct impl_tag
        {
        };

        template <typename AA, typename AS>
            requires std::same_as<std::remove_pointer_t<std::remove_cvref_t<AA>>, ArrowArray>
                     && std::same_as<std::remove_pointer_t<std::remove_cvref_t<AS>>, ArrowSchema>
        arrow_proxy(AA&& array, AS&& schema, impl_tag);

        void resize_children(size_t children_count);

        void update_buffers();
        void update_children();
        void update_dictionary();
        void update_null_count();

        [[nodiscard]] bool array_created_with_sparrow() const;
        [[nodiscard]] bool schema_created_with_sparrow() const;

        void validate_array_and_schema() const;

        arrow_schema_private_data* get_schema_private_data();
        arrow_array_private_data* get_array_private_data();

        [[nodiscard]] bool is_arrow_array_valid() const;
        [[nodiscard]] bool is_arrow_schema_valid() const;
        [[nodiscard]] bool is_proxy_valid() const;

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
}
