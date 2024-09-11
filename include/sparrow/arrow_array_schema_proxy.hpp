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
#include <utility>
#include <variant>
#include <vector>

#include "sparrow/array/data_type.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_array/private_data.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_proxy_utils.hpp"
#include "sparrow/arrow_interface/arrow_flag_utils.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/arrow_interface/arrow_schema/private_data.hpp"
#include "sparrow/buffer/buffer_view.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/variant_visitor.hpp"

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
        // explicit  arrow_proxy(ArrowArray&& array, ArrowSchema*&& schema);
        /// Constructs an arrow_proxy which uses the provided ArrowArray and ArrowSchema.
        /// Neither the array nor the schema are released when the arrow_proxy is destroyed.
        explicit arrow_proxy(ArrowArray* array, ArrowSchema* schema);
        // explicit  arrow_proxy(ArrowArray*&& array, ArrowSchema*&& schema);

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
        void set_n_children(size_t n_children);
        [[nodiscard]] const std::vector<sparrow::buffer_view<uint8_t>>& buffers() const;
        [[nodiscard]] std::vector<sparrow::buffer_view<uint8_t>>& buffers();
        void set_buffer(size_t index, const buffer_view<uint8_t>& buffer);
        void set_buffer(size_t index, buffer<uint8_t>&& buffer);
        [[nodiscard]] std::vector<arrow_proxy> children() const;
        void set_child(size_t index, ArrowArray* array, ArrowSchema* schema);
        [[nodiscard]] std::optional<arrow_proxy> dictionary() const;
        void set_dictionary(ArrowArray* array_dictionary, ArrowSchema* schema_dictionary);

        [[nodiscard]] bool is_created_with_sparrow() const;

        [[nodiscard]] void* private_data() const;

    private:

        std::variant<ArrowArray*, ArrowArray> m_array;
        ArrowArray& m_array_ref;
        std::variant<ArrowSchema*, ArrowSchema> m_schema;
        ArrowSchema& m_schema_ref;
        std::vector<sparrow::buffer_view<uint8_t>> m_buffers;

        void initialize_buffers();
        [[nodiscard]] bool array_created_with_sparrow() const;
        [[nodiscard]] bool schema_created_with_sparrow() const;

        void validate_array_and_schema() const;

        [[nodiscard]] ArrowArray& array();
        [[nodiscard]] const ArrowArray& array() const;

        [[nodiscard]] ArrowSchema& schema();
        [[nodiscard]] const ArrowSchema& schema() const;

        [[nodiscard]] std::size_t
        compute_buffer_size(buffer_type buffer_type, size_t length, size_t offset, enum data_type data_type) const;

        arrow_schema_private_data* get_schema_private_data();
        arrow_array_private_data* get_array_private_data();

        void update_null_count();
    };

    inline
    void arrow_proxy::initialize_buffers()
    {
        m_buffers.clear();
        const auto buffer_count = static_cast<size_t>(m_array_ref.n_buffers);
        m_buffers.reserve(buffer_count);
        const enum data_type data_type = format_to_data_type(m_schema_ref.format);
        const std::vector<buffer_type> buffers_type = get_buffer_types_from_data_type(data_type);
        for (std::size_t i = 0; i < buffer_count; ++i)
        {
            const auto buffer_type = buffers_type[i];
            auto buffer = m_array_ref.buffers[i];
            const std::size_t buffer_size = compute_buffer_size(buffer_type, length(), offset(), data_type);
            auto* ptr = static_cast<uint8_t*>(const_cast<void*>(buffer));
            m_buffers.emplace_back(ptr, buffer_size);
        }
    }

    bool arrow_proxy::array_created_with_sparrow() const
    {
        return m_array_ref.release == &sparrow::release_arrow_array;
    }

    bool arrow_proxy::schema_created_with_sparrow() const
    {
        return m_schema_ref.release == &sparrow::release_arrow_schema;
    }

    void arrow_proxy::validate_array_and_schema() const
    {
        SPARROW_ASSERT_TRUE(m_array_ref.release != nullptr);
        SPARROW_ASSERT_TRUE(m_schema_ref.release != nullptr);
        SPARROW_ASSERT_TRUE(m_array_ref.n_children == m_schema_ref.n_children);
        SPARROW_ASSERT_TRUE((m_array_ref.dictionary == nullptr) == (m_schema_ref.dictionary == nullptr));

        const enum data_type data_type = format_to_data_type(m_schema_ref.format);
        if (!validate_format_with_arrow_array(data_type, m_array_ref))
        {
            throw arrow_proxy_exception("Invalid ArrowArray format");
        }
    }

    inline
    arrow_proxy::arrow_proxy(ArrowArray&& array, ArrowSchema&& schema)
        : m_array(std::move(array))
        , m_array_ref(std::get<1>(m_array))
        , m_schema(std::move(schema))
        , m_schema_ref(std::get<1>(m_schema))
    {
        array = {};
        schema = {};
        validate_array_and_schema();
        initialize_buffers();
    }

    inline
    arrow_proxy::arrow_proxy(ArrowArray&& array, ArrowSchema* schema)
        : m_array(std::move(array))
        , m_array_ref(std::get<1>(m_array))
        , m_schema(schema)
        , m_schema_ref(*std::get<0>(m_schema))
    {
        array = {};
        SPARROW_ASSERT_TRUE(schema != nullptr);
        validate_array_and_schema();
        initialize_buffers();
    }

    inline
    arrow_proxy::arrow_proxy(ArrowArray* array, ArrowSchema* schema)
        : m_array(array)
        , m_array_ref(*std::get<0>(m_array))
        , m_schema(schema)
        , m_schema_ref(*std::get<0>(m_schema))
    {
        SPARROW_ASSERT_TRUE(array != nullptr);
        SPARROW_ASSERT_TRUE(schema != nullptr);
        validate_array_and_schema();
        initialize_buffers();
    }

    arrow_proxy::arrow_proxy(const arrow_proxy& other) :
        m_array(&other.m_array_ref),
        m_array_ref(*std::get<0>(m_array)),
        m_schema(&other.m_schema_ref),
        m_schema_ref(*std::get<0>(m_schema))
    {
        validate_array_and_schema();
        initialize_buffers();
    }

    arrow_proxy& arrow_proxy::operator=(const arrow_proxy& other)
    {
        if (this == &other)
        {
            return *this;
        }

        m_array = &other.m_array_ref;
        m_array_ref = *std::get<0>(m_array);
        m_schema = &other.m_schema_ref;
        m_schema_ref =*std::get<0>(m_schema);

        validate_array_and_schema();
        initialize_buffers();

        return *this;
    }

    arrow_proxy::arrow_proxy(arrow_proxy&& other) :
        m_array(std::move(other.m_array)),
        m_array_ref(m_array.index() == 0 ? *std::get<0>(m_array) : std::get<1>(m_array)),
        m_schema(std::move(other.m_schema)),
        m_schema_ref(m_schema.index() == 0 ? *std::get<0>(m_schema) : std::get<1>(m_schema))
    {
        other.m_array = {};
        other.m_schema = {};
        other.m_buffers.clear();

        initialize_buffers();
    }

    arrow_proxy& arrow_proxy::operator=(arrow_proxy&& other)
    {
        if (this == &other)
        {
            return *this;
        }

        m_array = std::move(other.m_array);
        m_array_ref = m_array.index() == 0 ? *std::get<0>(m_array) : std::get<1>(m_array);
        m_schema = std::move(other.m_schema);
        m_schema_ref = m_schema.index() == 0 ? *std::get<0>(m_schema) : std::get<1>(m_schema);

        other.m_array = {};
        other.m_schema = {};
        other.m_buffers.clear();
        initialize_buffers();

        return *this;
    }
    

    inline arrow_proxy::~arrow_proxy()
    {
        if (m_array.index() == 1)
        {
            if (m_array_ref.release != nullptr)
            {
                try
                {
                    m_array_ref.release(&m_array_ref);
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

        if (m_schema.index() == 1)
        {
            if (m_schema_ref.release != nullptr)
            {
                try
                {
                    m_schema_ref.release(&m_schema_ref);
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
        return m_schema_ref.format;
    }

    inline void arrow_proxy::set_format(const std::string_view format)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set format on non-sparrow created ArrowArray");
        }
        get_schema_private_data()->format() = format;
        m_schema_ref.format = get_schema_private_data()->format_ptr();
    }

    [[nodiscard]] enum data_type arrow_proxy::data_type() const
    {
        return format_to_data_type(m_schema_ref.format);
    }

    void arrow_proxy::set_data_type(enum data_type data_type)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set data_type on non-sparrow created ArrowArray");
        }
        set_format(data_type_to_format(data_type));
        m_schema_ref.format = get_schema_private_data()->format_ptr();
    }

    [[nodiscard]] std::optional<const std::string_view> arrow_proxy::name() const
    {
        if (m_schema_ref.name == nullptr)
        {
            return std::nullopt;
        }
        return std::string_view(m_schema_ref.name);
    }

    void arrow_proxy::set_name(std::optional<const std::string_view> name)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set name on non-sparrow created ArrowArray");
        }
        get_schema_private_data()->name() = name;
        m_schema_ref.name = get_schema_private_data()->name_ptr();
    }

    [[nodiscard]] std::optional<const std::string_view> arrow_proxy::metadata() const
    {
        if (m_schema_ref.metadata == nullptr)
        {
            return std::nullopt;
        }
        return std::string_view(m_schema_ref.metadata);
    }

    void arrow_proxy::set_metadata(std::optional<const std::string_view> metadata)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set metadata on non-sparrow created ArrowArray");
        }
        get_schema_private_data()->metadata() = metadata;
        m_schema_ref.metadata = get_schema_private_data()->metadata_ptr();
    }

    [[nodiscard]] inline std::vector<ArrowFlag> arrow_proxy::flags() const
    {
        return to_vector_of_ArrowFlags(m_schema_ref.flags);
    }

    void arrow_proxy::set_flags(const std::vector<ArrowFlag>& flags)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set flags on non-sparrow created ArrowArray");
        }
        m_schema_ref.flags = to_ArrowFlag_value(flags);
    }

    [[nodiscard]] size_t arrow_proxy::length() const
    {
        SPARROW_ASSERT_TRUE(m_array_ref.length >= 0);
        SPARROW_ASSERT_TRUE(std::cmp_less(m_array_ref.length, std::numeric_limits<size_t>::max()));
        return static_cast<size_t>(m_array_ref.length);
    }

    void arrow_proxy::set_length(size_t length)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(length, std::numeric_limits<int64_t>::max()));
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set length on non-sparrow created ArrowArray");
        }
        m_array_ref.length = static_cast<int64_t>(length);
    }

    [[nodiscard]] inline int64_t arrow_proxy::null_count() const
    {
        return m_array_ref.null_count;
    }

    void arrow_proxy::set_null_count(int64_t null_count)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set null_count on non-sparrow created ArrowArray");
        }
        m_array_ref.null_count = null_count;
    }

    [[nodiscard]] size_t arrow_proxy::offset() const
    {
        return static_cast<size_t>(m_array_ref.offset);
    }

    void arrow_proxy::set_offset(size_t offset)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(offset, std::numeric_limits<int64_t>::max()));
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set offset on non-sparrow created ArrowArray");
        }
        m_array_ref.offset = static_cast<int64_t>(offset);
    }

    [[nodiscard]] size_t arrow_proxy::n_buffers() const
    {
        return static_cast<size_t>(m_array_ref.n_buffers);
    }

    void arrow_proxy::set_n_buffers(size_t n_buffers)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(n_buffers, std::numeric_limits<int64_t>::max()));
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set n_buffers on non-sparrow created ArrowArray");
        }
        m_array_ref.n_buffers = static_cast<int64_t>(n_buffers);
        arrow_array_private_data* private_data = get_array_private_data();
        private_data->resize_buffers(n_buffers);
        m_array_ref.buffers = private_data->buffers_ptrs<void>();
        m_array_ref.n_buffers = static_cast<int64_t>(n_buffers);
    }

    [[nodiscard]] size_t arrow_proxy::n_children() const
    {
        return static_cast<size_t>(m_array_ref.n_children);
    }

    void arrow_proxy::set_n_children(size_t children_count)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(children_count, std::numeric_limits<int64_t>::max()));
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set n_children on non-sparrow created ArrowArray");
        }

        // Check that the release callback is valid for all children
        for (size_t i = 0; i < n_children(); ++i)
        {
            SPARROW_ASSERT_TRUE(m_schema_ref.children[i] != nullptr);
            SPARROW_ASSERT_TRUE(m_schema_ref.children[i]->release != nullptr);
            SPARROW_ASSERT_TRUE(m_array_ref.children[i] != nullptr);
            SPARROW_ASSERT_TRUE(m_array_ref.children[i]->release != nullptr);
        }

        // Release the remaining children if the new size is smaller than the current size
        for (size_t i = children_count; i < static_cast<size_t>(m_array_ref.n_children); ++i)
        {
            m_schema_ref.children[i]->release(m_schema_ref.children[i]);
            m_array_ref.children[i]->release(m_array_ref.children[i]);
        }

        auto arrow_array_children = new ArrowArray*[children_count];
        auto arrow_schemas_children = new ArrowSchema*[children_count];
        for (size_t i = 0; i < std::min(children_count, static_cast<size_t>(m_array_ref.n_children)); ++i)
        {
            arrow_array_children[i] = m_array_ref.children[i];
            arrow_schemas_children[i] = m_schema_ref.children[i];
        }

        m_array_ref.children = arrow_array_children;
        m_array_ref.n_children = static_cast<int64_t>(children_count);
        m_schema_ref.children = arrow_schemas_children;
        m_schema_ref.n_children = static_cast<int64_t>(children_count);
    }

    [[nodiscard]] std::size_t
    arrow_proxy::compute_buffer_size(buffer_type buffer_type, size_t length, size_t offset, enum data_type data_type) const
    {
        constexpr double bit_per_byte = 8.;
        switch (buffer_type)
        {
            case buffer_type::VALIDITY:
                return static_cast<std::size_t>(std::ceil(static_cast<double>(length) / bit_per_byte));
            case buffer_type::DATA:
                return primitive_bytes_count(data_type, length);
            case buffer_type::OFFSETS_32BIT:
            case buffer_type::SIZES_32BIT:
                return get_offset_buffer_size(data_type, length, offset) * sizeof(std::int32_t);
            case buffer_type::OFFSETS_64BIT:
            case buffer_type::SIZES_64BIT:
                return get_offset_buffer_size(data_type, length, offset) * sizeof(std::int64_t);
            case buffer_type::VIEWS:
                // TODO: Implement
                SPARROW_ASSERT(false, "Not implemented");
                return 0;
            case buffer_type::TYPE_IDS:
                return length;
        }
        mpl::unreachable();
    }

    arrow_schema_private_data* arrow_proxy::get_schema_private_data()
    {
        SPARROW_ASSERT_TRUE(schema_created_with_sparrow());
        return static_cast<arrow_schema_private_data*>(m_schema_ref.private_data);
    }

    arrow_array_private_data* arrow_proxy::get_array_private_data()
    {
        SPARROW_ASSERT_TRUE(array_created_with_sparrow());
        return static_cast<arrow_array_private_data*>(m_array_ref.private_data);
    }

    [[nodiscard]] const std::vector<sparrow::buffer_view<uint8_t>>& arrow_proxy::buffers() const
    {
        return m_buffers;
    }

    [[nodiscard]] std::vector<sparrow::buffer_view<uint8_t>>& arrow_proxy::buffers()
    {
        return m_buffers;
    }

    void arrow_proxy::set_buffer(size_t index, const buffer_view<uint8_t>& buffer)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(index, n_buffers()));
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set buffer on non-sparrow created ArrowArray");
        }
        auto array_private_data = get_array_private_data();
        array_private_data->set_buffer(index, buffer);
        m_array_ref.buffers = array_private_data->buffers_ptrs<void>();
        update_null_count();
        initialize_buffers();
    }

    void arrow_proxy::set_buffer(size_t index, buffer<uint8_t>&& buffer)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(index, n_buffers()));
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set buffer on non-sparrow created ArrowArray");
        }
        auto array_private_data = get_array_private_data();
        array_private_data->set_buffer(index, std::move(buffer));
        m_array_ref.buffers = array_private_data->buffers_ptrs<void>();
        update_null_count();
        initialize_buffers();
    }

    [[nodiscard]] std::vector<arrow_proxy> arrow_proxy::children() const
    {
        std::vector<arrow_proxy> children;
        children.reserve(static_cast<std::size_t>(m_array_ref.n_children));
        for (int64_t i = 0; i < m_array_ref.n_children; ++i)
        {
            children.emplace_back(m_array_ref.children[i], m_schema_ref.children[i]);
        };
        return children;
    }

    void arrow_proxy::set_child(size_t index, ArrowArray* child_array, ArrowSchema* child_schema)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(index, n_children()));
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set child on non-sparrow created ArrowArray");
        }
        m_array_ref.children[index] = child_array;
        m_schema_ref.children[index] = child_schema;
    }

    [[nodiscard]] inline std::optional<arrow_proxy> arrow_proxy::dictionary() const
    {
        if (m_array_ref.dictionary == nullptr || m_schema_ref.dictionary == nullptr)
        {
            return std::nullopt;
        }
        return arrow_proxy{m_array_ref.dictionary, m_schema_ref.dictionary};
    }

    void arrow_proxy::set_dictionary(ArrowArray* array_dictionary, ArrowSchema* schema_dictionary)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set dictionary on non-sparrow created ArrowArray");
        }

        if (m_array_ref.dictionary != nullptr)
        {
            m_array_ref.dictionary->release(m_array_ref.dictionary);
        }
        if (m_schema_ref.dictionary != nullptr)
        {
            m_schema_ref.dictionary->release(m_schema_ref.dictionary);
        }

        m_array_ref.dictionary = array_dictionary;
        m_schema_ref.dictionary = schema_dictionary;
    }

    [[nodiscard]] inline bool arrow_proxy::is_created_with_sparrow() const
    {
        return array_created_with_sparrow() && schema_created_with_sparrow();
    }

    [[nodiscard]] inline void* arrow_proxy::private_data() const
    {
        return m_array_ref.private_data;
    }

    [[nodiscard]] inline const ArrowArray& arrow_proxy::array() const
    {
        return m_array_ref;
    }

    [[nodiscard]] inline const ArrowSchema& arrow_proxy::schema() const
    {
        return m_schema_ref;
    }

    [[nodiscard]] inline ArrowArray& arrow_proxy::array()
    {
        return m_array_ref;
    }

    [[nodiscard]] inline ArrowSchema& arrow_proxy::schema()
    {
        return m_schema_ref;
    }

    void arrow_proxy::update_null_count()
    {
        const auto buffer_types = get_buffer_types_from_data_type(data_type());
        const auto validity_it = std::find(buffer_types.begin(), buffer_types.end(), buffer_type::VALIDITY);
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
}
