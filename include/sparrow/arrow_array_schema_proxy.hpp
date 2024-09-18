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

#include <algorithm>
#include <cstdint>
#include <ranges>
#include <type_traits>
#include <utility>

#include "sparrow/array/data_type.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_array/private_data.hpp"
#include "sparrow/arrow_interface/arrow_flag_utils.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/arrow_interface/arrow_schema/private_data.hpp"
#include "sparrow/buffer/buffer_view.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/utils/contracts.hpp"

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
     * Proxy class over ArrowArray and ArrowSchema.
     * It ease the use of ArrowArray and ArrowSchema by providing a more user-friendly interface.
     * It can take ownership of the ArrowArray and ArrowSchema or use them as pointers.
     * If the arrow_proxy takes ownership of the ArrowArray and ArrowSchema, they are released when the
     * arrow_proxy is destroyed. Otherwise, the arrow_proxy does not release the ArrowArray and ArrowSchema.
     */
    class arrow_proxy
    {
    public:

        /// Constructs an arrow_proxy which takes the ownership of the ArrowArray and ArrowSchema.
        /// The array and schema are released when the arrow_proxy is destroyed.
        explicit arrow_proxy(ArrowArray&& array, ArrowSchema&& schema);
        /// Constructs an arrow_proxy which takes the ownership of the ArrowArray and uses the provided
        /// ArrowSchema. The array is released when the arrow_proxy is destroyed. The schema is not released.
        explicit arrow_proxy(ArrowArray&& array, ArrowSchema* schema);
        /// Constructs an arrow_proxy which uses the provided ArrowArray and ArrowSchema.
        /// Neither the array nor the schema are released when the arrow_proxy is destroyed.
        explicit arrow_proxy(ArrowArray* array, ArrowSchema* schema);

        // Copy constructors
        arrow_proxy(const arrow_proxy&);
        arrow_proxy& operator=(const arrow_proxy&);

        // Move constructors
        arrow_proxy(arrow_proxy&&);
        arrow_proxy& operator=(arrow_proxy&&);

        ~arrow_proxy();

        [[nodiscard]] const std::string_view format() const;
        void set_format(const std::string_view format);
        [[nodiscard]] enum data_type data_type() const;
        void set_data_type(enum data_type data_type);
        [[nodiscard]] std::optional<const std::string_view> name() const;
        void set_name(std::optional<const std::string_view> name);
        [[nodiscard]] std::optional<const std::string_view> metadata() const;
        void set_metadata(std::optional<const std::string_view> metadata);
        [[nodiscard]] std::vector<ArrowFlag> flags() const;
        void set_flags(const std::vector<ArrowFlag>& flags);
        [[nodiscard]] size_t length() const;
        void set_length(size_t length);
        [[nodiscard]] int64_t null_count() const;
        void set_null_count(int64_t null_count);
        [[nodiscard]] size_t offset() const;
        void set_offset(size_t offset);
        [[nodiscard]] size_t n_buffers() const;
        void set_n_buffers(size_t n_buffers);
        [[nodiscard]] size_t n_children() const;
        template <std::ranges::input_range R>
            requires std::same_as<std::ranges::range_value_t<R>, arrow_array_and_schema_pointers>
        void add_children(const R& arrow_array_and_schema_pointers);
        void pop_children(size_t n);
        [[nodiscard]] const std::vector<sparrow::buffer_view<uint8_t>>& buffers() const;
        [[nodiscard]] std::vector<sparrow::buffer_view<uint8_t>>& buffers();
        void set_buffer(size_t index, const buffer_view<uint8_t>& buffer);
        void set_buffer(size_t index, buffer<uint8_t>&& buffer);
        [[nodiscard]] const std::vector<arrow_proxy>& children() const;
        [[nodiscard]] std::vector<arrow_proxy>& children();
        void set_child(size_t index, ArrowArray* array, ArrowSchema* schema);

        [[nodiscard]] const std::unique_ptr<arrow_proxy>& dictionary() const;
        [[nodiscard]] std::unique_ptr<arrow_proxy>& dictionary();
        void set_dictionary(ArrowArray* array_dictionary, ArrowSchema* schema_dictionary);

        [[nodiscard]] bool is_created_with_sparrow() const;

        [[nodiscard]] void* private_data() const;

    private:

        std::variant<ArrowArray*, ArrowArray> m_array;
        ArrowArray* m_array_ptr;
        std::variant<ArrowSchema*, ArrowSchema> m_schema;
        ArrowSchema* m_schema_ptr;
        std::vector<sparrow::buffer_view<uint8_t>> m_buffers;
        std::vector<arrow_proxy> m_children;
        std::unique_ptr<arrow_proxy> m_dictionary;

        template <typename AA, typename AS>
        arrow_proxy(AA&& array, AS&& schema, bool);

        void resize_children(size_t children_count);

        void update_buffers();
        void update_children();
        void update_dictionary();
        void update_null_count();

        [[nodiscard]] bool array_created_with_sparrow() const;
        [[nodiscard]] bool schema_created_with_sparrow() const;

        void validate_array_and_schema() const;

        [[nodiscard]] ArrowArray& array();
        [[nodiscard]] const ArrowArray& array() const;

        [[nodiscard]] ArrowSchema& schema();
        [[nodiscard]] const ArrowSchema& schema() const;

        arrow_schema_private_data* get_schema_private_data();
        arrow_array_private_data* get_array_private_data();

        [[nodiscard]] bool is_arrow_array_valid() const;
        [[nodiscard]] bool is_arrow_schema_valid() const;
        [[nodiscard]] bool is_proxy_valid() const;

        static void swap(arrow_proxy& lhs, arrow_proxy& rhs);
    };

    inline void arrow_proxy::update_buffers()
    {
        m_buffers.clear();
        const auto buffer_count = static_cast<size_t>(m_array_ptr->n_buffers);
        m_buffers.reserve(buffer_count);
        const enum data_type data_type = format_to_data_type(m_schema_ptr->format);
        const std::vector<buffer_type> buffers_type = get_buffer_types_from_data_type(data_type);
        for (std::size_t i = 0; i < buffer_count; ++i)
        {
            const auto buffer_type = buffers_type[i];
            auto buffer = m_array_ptr->buffers[i];
            const std::size_t buffer_size = compute_buffer_size(buffer_type, length(), offset(), data_type);
            auto* ptr = static_cast<uint8_t*>(const_cast<void*>(buffer));
            m_buffers.emplace_back(ptr, buffer_size);
        }
    }

    inline void arrow_proxy::update_children()
    {
        m_children.clear();
        m_children.reserve(static_cast<std::size_t>(m_array_ptr->n_children));
        for (int64_t i = 0; i < m_array_ptr->n_children; ++i)
        {
            m_children.emplace_back(m_array_ptr->children[i], m_schema_ptr->children[i]);
        }
    }

    inline void arrow_proxy::update_dictionary()
    {
        if (m_array_ptr->dictionary == nullptr || m_schema_ptr->dictionary == nullptr)
        {
            m_dictionary = nullptr;
        }
        else
        {
            m_dictionary = std::make_unique<arrow_proxy>(m_array_ptr->dictionary, m_schema_ptr->dictionary);
        }
    }

    inline bool arrow_proxy::array_created_with_sparrow() const
    {
        return m_array_ptr->release == &sparrow::release_arrow_array;
    }

    inline bool arrow_proxy::schema_created_with_sparrow() const
    {
        return m_schema_ptr->release == &sparrow::release_arrow_schema;
    }

    inline void arrow_proxy::validate_array_and_schema() const
    {
        SPARROW_ASSERT_TRUE(m_array_ptr->release != nullptr);
        SPARROW_ASSERT_TRUE(m_schema_ptr->release != nullptr);
        SPARROW_ASSERT_TRUE(m_array_ptr->n_children == m_schema_ptr->n_children);
        SPARROW_ASSERT_TRUE((m_array_ptr->dictionary == nullptr) == (m_schema_ptr->dictionary == nullptr));

        const enum data_type data_type = format_to_data_type(m_schema_ptr->format);
        if (!validate_format_with_arrow_array(data_type, *m_array_ptr))
        {
            throw arrow_proxy_exception("Invalid ArrowArray format");
        }
    }

    template <typename AA, typename AS>
    arrow_proxy::arrow_proxy(AA&& array, AS&& schema, bool)
        : m_array(std::move(array))
        , m_array_ptr(m_array.index() == 0 ? std::get<0>(m_array) : &std::get<1>(m_array))
        , m_schema(std::move(schema))
        , m_schema_ptr(m_schema.index() == 0 ? std::get<0>(m_schema) : &std::get<1>(m_schema))
    {
        if constexpr (std::is_rvalue_reference_v<AA&&>)
        {
            array = {};
        }
        else if constexpr (std::is_pointer_v<std::remove_cvref_t<AA>>)
        {
            SPARROW_ASSERT_TRUE(array != nullptr);
        }

        if constexpr (std::is_rvalue_reference_v<AS&&>)
        {
            schema = {};
        }
        else if constexpr (std::is_pointer_v<std::remove_cvref_t<AS>>)
        {
            SPARROW_ASSERT_TRUE(schema != nullptr);
        }

        validate_array_and_schema();
        update_buffers();
        update_children();
        update_dictionary();
    }

    inline arrow_proxy::arrow_proxy(ArrowArray&& array, ArrowSchema&& schema)
        : arrow_proxy(std::move(array), std::move(schema), true)
    {
    }

    inline arrow_proxy::arrow_proxy(ArrowArray&& array, ArrowSchema* schema)
        : arrow_proxy(std::move(array), schema, true)
    {
    }

    inline arrow_proxy::arrow_proxy(ArrowArray* array, ArrowSchema* schema)
        : arrow_proxy(array, schema, true)
    {
    }

    inline arrow_proxy::arrow_proxy(const arrow_proxy& other)
        : m_array(deep_copy_array(*other.m_array_ptr, *other.m_schema_ptr))
        , m_array_ptr(&std::get<1>(m_array))
        , m_schema(deep_copy_schema(*other.m_schema_ptr))
        , m_schema_ptr(&std::get<1>(m_schema))
    {
        validate_array_and_schema();
        update_buffers();
        update_children();
        update_dictionary();
    }

    inline arrow_proxy& arrow_proxy::operator=(const arrow_proxy& other)
    {
        if (this == &other)
        {
            return *this;
        }

        arrow_proxy tmp(other);
        swap(*this, tmp);

        return *this;
    }

    inline arrow_proxy::arrow_proxy(arrow_proxy&& other)
        : m_array(std::move(other.m_array))
        , m_array_ptr(m_array.index() == 0 ? std::get<0>(m_array) : &std::get<1>(m_array))
        , m_schema(std::move(other.m_schema))
        , m_schema_ptr(m_schema.index() == 0 ? std::get<0>(m_schema) : &std::get<1>(m_schema))
    {
        other.m_array = {};
        other.m_schema = {};
        other.m_buffers.clear();

        update_buffers();
    }

    inline arrow_proxy& arrow_proxy::operator=(arrow_proxy&& rhs)
    {
        swap(*this, rhs);
        return *this;
    }

    inline arrow_proxy::~arrow_proxy()
    {
        if (m_array.index() == 1)  // We own the array
        {
            if (m_array_ptr->release != nullptr)
            {
                try
                {
                    m_array_ptr->release(m_array_ptr);
                }
                catch (...)
                {
#if defined(SPARROW_CONTRACTS_DEFAULT_ABORT_ON_FAILURE)
                    SPARROW_CONTRACTS_DEFAULT_ON_FAILURE(
                        "~arrow_proxy",
                        "Exception thrown during array.release in arrow_proxy destructor."
                    );
#endif
                }
            }
        }

        if (m_schema.index() == 1)  // We own the schema
        {
            if (m_schema_ptr->release != nullptr)
            {
                try
                {
                    m_schema_ptr->release(m_schema_ptr);
                }
                catch (...)
                {
#if defined(SPARROW_CONTRACTS_DEFAULT_ABORT_ON_FAILURE)
                    SPARROW_CONTRACTS_DEFAULT_ON_FAILURE(
                        "~arrow_proxy",
                        "Exception thrown during schema.release in arrow_proxy destructor."
                    );
#endif
                }
            }
        }
    }

    [[nodiscard]] inline const std::string_view arrow_proxy::format() const
    {
        return m_schema_ptr->format;
    }

    inline void arrow_proxy::set_format(const std::string_view format)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set format on non-sparrow created ArrowArray");
        }
#if defined(__GNUC__) && !defined(__clang__)  // Bypass the bug:
                                              // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105651
#    if __GNUC__ == 12
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wrestrict"
#    endif
#endif
        get_schema_private_data()->format() = format;
#if defined(__GNUC__) && !defined(__clang__)
#    if __GNUC__ == 12
#        pragma GCC diagnostic pop
#    endif
#endif
        m_schema_ptr->format = get_schema_private_data()->format_ptr();
    }

    [[nodiscard]] inline enum data_type arrow_proxy::data_type() const
    {
        return format_to_data_type(m_schema_ptr->format);
    }

    inline void arrow_proxy::set_data_type(enum data_type data_type)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set data_type on non-sparrow created ArrowArray");
        }
        set_format(data_type_to_format(data_type));
        m_schema_ptr->format = get_schema_private_data()->format_ptr();
    }

    [[nodiscard]] inline std::optional<const std::string_view> arrow_proxy::name() const
    {
        if (m_schema_ptr->name == nullptr)
        {
            return std::nullopt;
        }
        return std::string_view(m_schema_ptr->name);
    }

    inline void arrow_proxy::set_name(std::optional<const std::string_view> name)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set name on non-sparrow created ArrowArray");
        }
        get_schema_private_data()->name() = name;
        m_schema_ptr->name = get_schema_private_data()->name_ptr();
    }

    [[nodiscard]] inline std::optional<const std::string_view> arrow_proxy::metadata() const
    {
        if (m_schema_ptr->metadata == nullptr)
        {
            return std::nullopt;
        }
        return std::string_view(m_schema_ptr->metadata);
    }

    inline void arrow_proxy::set_metadata(std::optional<const std::string_view> metadata)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set metadata on non-sparrow created ArrowArray");
        }
        get_schema_private_data()->metadata() = metadata;
        m_schema_ptr->metadata = get_schema_private_data()->metadata_ptr();
    }

    [[nodiscard]] inline std::vector<ArrowFlag> arrow_proxy::flags() const
    {
        return to_vector_of_ArrowFlags(m_schema_ptr->flags);
    }

    inline void arrow_proxy::set_flags(const std::vector<ArrowFlag>& flags)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set flags on non-sparrow created ArrowArray");
        }
        m_schema_ptr->flags = to_ArrowFlag_value(flags);
    }

    [[nodiscard]] inline size_t arrow_proxy::length() const
    {
        SPARROW_ASSERT_TRUE(m_array_ptr->length >= 0);
        SPARROW_ASSERT_TRUE(std::cmp_less(m_array_ptr->length, std::numeric_limits<size_t>::max()));
        return static_cast<size_t>(m_array_ptr->length);
    }

    inline void arrow_proxy::set_length(size_t length)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(length, std::numeric_limits<int64_t>::max()));
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set length on non-sparrow created ArrowArray");
        }
        m_array_ptr->length = static_cast<int64_t>(length);
    }

    [[nodiscard]] inline int64_t arrow_proxy::null_count() const
    {
        return m_array_ptr->null_count;
    }

    inline void arrow_proxy::set_null_count(int64_t null_count)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set null_count on non-sparrow created ArrowArray");
        }
        m_array_ptr->null_count = null_count;
    }

    [[nodiscard]] inline size_t arrow_proxy::offset() const
    {
        return static_cast<size_t>(m_array_ptr->offset);
    }

    inline void arrow_proxy::set_offset(size_t offset)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(offset, std::numeric_limits<int64_t>::max()));
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set offset on non-sparrow created ArrowArray");
        }
        m_array_ptr->offset = static_cast<int64_t>(offset);
    }

    [[nodiscard]] inline size_t arrow_proxy::n_buffers() const
    {
        return static_cast<size_t>(m_array_ptr->n_buffers);
    }

    inline void arrow_proxy::set_n_buffers(size_t n_buffers)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(n_buffers, std::numeric_limits<int64_t>::max()));
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set n_buffers on non-sparrow created ArrowArray");
        }
        m_array_ptr->n_buffers = static_cast<int64_t>(n_buffers);
        arrow_array_private_data* private_data = get_array_private_data();
        private_data->resize_buffers(n_buffers);
        m_array_ptr->buffers = private_data->buffers_ptrs<void>();
        m_array_ptr->n_buffers = static_cast<int64_t>(n_buffers);
    }

    [[nodiscard]] inline size_t arrow_proxy::n_children() const
    {
        return static_cast<size_t>(m_array_ptr->n_children);
    }

    template <std::ranges::input_range R>
        requires std::same_as<std::ranges::range_value_t<R>, arrow_array_and_schema_pointers>
    inline void arrow_proxy::add_children(const R& arrow_array_and_schema_pointers)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set n_buffers on non-sparrow created ArrowArray");
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

    inline void arrow_proxy::pop_children(size_t n)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set n_buffers on non-sparrow created ArrowArray");
        }

        if (n > n_children())
        {
            throw arrow_proxy_exception("Cannot pop more children than the current number of children");
        }

        resize_children(n_children() - n);
        update_children();
    }

    inline void arrow_proxy::resize_children(size_t children_count)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(children_count, std::numeric_limits<int64_t>::max()));
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set n_children on non-sparrow created ArrowArray");
        }

        // Check that the release callback is valid for all children
        for (size_t i = 0; i < n_children(); ++i)
        {
            SPARROW_ASSERT_TRUE(m_schema_ptr->children[i] != nullptr);
            SPARROW_ASSERT_TRUE(m_schema_ptr->children[i]->release != nullptr);
            SPARROW_ASSERT_TRUE(m_array_ptr->children[i] != nullptr);
            SPARROW_ASSERT_TRUE(m_array_ptr->children[i]->release != nullptr);
        }

        // Release the remaining children if the new size is smaller than the current size
        for (size_t i = children_count; i < static_cast<size_t>(m_array_ptr->n_children); ++i)
        {
            m_schema_ptr->children[i]->release(m_schema_ptr->children[i]);
            m_array_ptr->children[i]->release(m_array_ptr->children[i]);
        }

        auto arrow_array_children = new ArrowArray*[children_count];
        auto arrow_schemas_children = new ArrowSchema*[children_count];
        for (size_t i = 0; i < std::min(children_count, static_cast<size_t>(m_array_ptr->n_children)); ++i)
        {
            arrow_array_children[i] = m_array_ptr->children[i];
            arrow_schemas_children[i] = m_schema_ptr->children[i];
        }

        delete[] m_array_ptr->children;
        m_array_ptr->children = arrow_array_children;
        m_array_ptr->n_children = static_cast<int64_t>(children_count);
        delete[] m_schema_ptr->children;
        m_schema_ptr->children = arrow_schemas_children;
        m_schema_ptr->n_children = static_cast<int64_t>(children_count);
    }

    inline arrow_schema_private_data* arrow_proxy::get_schema_private_data()
    {
        SPARROW_ASSERT_TRUE(schema_created_with_sparrow());
        return static_cast<arrow_schema_private_data*>(m_schema_ptr->private_data);
    }

    inline arrow_array_private_data* arrow_proxy::get_array_private_data()
    {
        SPARROW_ASSERT_TRUE(array_created_with_sparrow());
        return static_cast<arrow_array_private_data*>(m_array_ptr->private_data);
    }

    [[nodiscard]] inline const std::vector<sparrow::buffer_view<uint8_t>>& arrow_proxy::buffers() const
    {
        return m_buffers;
    }

    [[nodiscard]] inline std::vector<sparrow::buffer_view<uint8_t>>& arrow_proxy::buffers()
    {
        return m_buffers;
    }

    inline void arrow_proxy::set_buffer(size_t index, const buffer_view<uint8_t>& buffer)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(index, n_buffers()));
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set buffer on non-sparrow created ArrowArray");
        }
        auto array_private_data = get_array_private_data();
        array_private_data->set_buffer(index, buffer);
        m_array_ptr->buffers = array_private_data->buffers_ptrs<void>();
        update_null_count();
        update_buffers();
    }

    inline void arrow_proxy::set_buffer(size_t index, buffer<uint8_t>&& buffer)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(index, n_buffers()));
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set buffer on non-sparrow created ArrowArray");
        }
        auto array_private_data = get_array_private_data();
        array_private_data->set_buffer(index, std::move(buffer));
        m_array_ptr->buffers = array_private_data->buffers_ptrs<void>();
        update_null_count();
        update_buffers();
    }

    [[nodiscard]] inline const std::vector<arrow_proxy>& arrow_proxy::children() const
    {
        return m_children;
    }

    [[nodiscard]] inline std::vector<arrow_proxy>& arrow_proxy::children()
    {
        return m_children;
    }

    inline void arrow_proxy::set_child(size_t index, ArrowArray* child_array, ArrowSchema* child_schema)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(index, n_children()));
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set child on non-sparrow created ArrowArray");
        }
        m_array_ptr->children[index] = child_array;
        m_schema_ptr->children[index] = child_schema;
        update_children();
    }

    [[nodiscard]] inline const std::unique_ptr<arrow_proxy>& arrow_proxy::dictionary() const
    {
        return m_dictionary;
    }

    [[nodiscard]] inline std::unique_ptr<arrow_proxy>& arrow_proxy::dictionary()
    {
        return m_dictionary;
    }

    inline void arrow_proxy::set_dictionary(ArrowArray* array_dictionary, ArrowSchema* schema_dictionary)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set dictionary on non-sparrow created ArrowArray");
        }

        if (m_array_ptr->dictionary != nullptr)
        {
            m_array_ptr->dictionary->release(m_array_ptr->dictionary);
        }
        if (m_schema_ptr->dictionary != nullptr)
        {
            m_schema_ptr->dictionary->release(m_schema_ptr->dictionary);
        }

        m_array_ptr->dictionary = array_dictionary;
        m_schema_ptr->dictionary = schema_dictionary;
        update_dictionary();
    }

    [[nodiscard]] inline bool arrow_proxy::is_created_with_sparrow() const
    {
        return array_created_with_sparrow() && schema_created_with_sparrow();
    }

    [[nodiscard]] inline void* arrow_proxy::private_data() const
    {
        return m_array_ptr->private_data;
    }

    [[nodiscard]] inline const ArrowArray& arrow_proxy::array() const
    {
        return *m_array_ptr;
    }

    [[nodiscard]] inline const ArrowSchema& arrow_proxy::schema() const
    {
        return *m_schema_ptr;
    }

    [[nodiscard]] inline ArrowArray& arrow_proxy::array()
    {
        return *m_array_ptr;
    }

    [[nodiscard]] inline ArrowSchema& arrow_proxy::schema()
    {
        return *m_schema_ptr;
    }

    inline void arrow_proxy::update_null_count()
    {
        const auto buffer_types = get_buffer_types_from_data_type(data_type());
        const auto validity_it = std::ranges::find(buffer_types, buffer_type::VALIDITY);
        if (validity_it == buffer_types.end())
        {
            return;
        }
        const auto validity_index = std::distance(buffer_types.begin(), validity_it);
        auto& validity_buffer = buffers()[static_cast<size_t>(validity_index)];
        const dynamic_bitset_view<std::uint8_t> bitmap(validity_buffer.data(), validity_buffer.size());
        const auto null_count = bitmap.null_count();
        set_null_count(static_cast<int64_t>(null_count));
    }

    inline bool arrow_proxy::is_arrow_array_valid() const
    {
        return m_array_ptr->release != nullptr;
    }

    inline bool arrow_proxy::is_arrow_schema_valid() const
    {
        return m_schema_ptr->release != nullptr;
    }

    inline bool arrow_proxy::is_proxy_valid() const
    {
        return is_arrow_array_valid() && is_arrow_schema_valid();
    }

    inline void arrow_proxy::swap(arrow_proxy& lhs, arrow_proxy& rhs)
    {
        if (&lhs == &rhs)
        {
            return;
        }
        std::swap(lhs.m_array, rhs.m_array);
        lhs.m_array_ptr = lhs.m_array.index() == 0 ? std::get<0>(lhs.m_array) : &std::get<1>(lhs.m_array);
        rhs.m_array_ptr = rhs.m_array.index() == 0 ? std::get<0>(rhs.m_array) : &std::get<1>(rhs.m_array);
        std::swap(lhs.m_schema, rhs.m_schema);
        lhs.m_schema_ptr = lhs.m_schema.index() == 0 ? std::get<0>(lhs.m_schema) : &std::get<1>(lhs.m_schema);
        rhs.m_schema_ptr = rhs.m_schema.index() == 0 ? std::get<0>(rhs.m_schema) : &std::get<1>(rhs.m_schema);
        std::swap(lhs.m_buffers, rhs.m_buffers);
        std::swap(lhs.m_children, rhs.m_children);
        std::swap(lhs.m_dictionary, rhs.m_dictionary);
    }
}
