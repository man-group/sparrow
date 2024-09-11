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
#include <variant>
#include <vector>

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_proxy_utils.hpp"
#include "sparrow/arrow_interface/arrow_flag_utils.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/buffer_view.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/variant_visitor.hpp"

namespace sparrow
{
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

        ~arrow_proxy();

        [[nodiscard]] std::string_view format() const;
        [[nodiscard]] std::string_view name() const;
        [[nodiscard]] std::string_view metadata() const;
        [[nodiscard]] std::vector<ArrowFlag> flags() const;
        [[nodiscard]] int64_t length() const;
        [[nodiscard]] int64_t null_count() const;
        [[nodiscard]] int64_t offset() const;
        [[nodiscard]] int64_t n_buffers() const;
        [[nodiscard]] int64_t n_children() const;
        [[nodiscard]] const std::vector<sparrow::buffer_view<uint8_t>>& buffers() const;
        [[nodiscard]] const std::vector<arrow_proxy>& children() const;
        [[nodiscard]] std::optional<arrow_proxy> dictionary() const;

        [[nodiscard]] bool is_created_with_sparrow() const;

        [[nodiscard]] void* private_data() const;

    private:

        std::variant<ArrowArray*, ArrowArray> m_array;
        std::variant<ArrowSchema*, ArrowSchema> m_schema;
        std::vector<sparrow::buffer_view<uint8_t>> m_buffers;
        std::vector<arrow_proxy> m_children;

        void initialize_children();
        void initialize_buffers();

        void validate_array_and_schema() const;

        [[nodiscard]] ArrowArray& array();
        [[nodiscard]] const ArrowArray& array() const;

        [[nodiscard]] ArrowSchema& schema();
        [[nodiscard]] const ArrowSchema& schema() const;

        [[nodiscard]] std::size_t
        compute_buffer_size(buffer_type buffer_type, int64_t length, data_type data_type) const;
    };

    inline
    void arrow_proxy::initialize_children()
    {
        m_children.reserve(static_cast<std::size_t>(array().n_children));
        for (int64_t i = 0; i < array().n_children; ++i)
        {
            m_children.emplace_back(array().children[i], schema().children[i]);
        }
    }

    inline
    void arrow_proxy::initialize_buffers()
    {
        m_buffers.clear();
        const auto buffer_count = static_cast<size_t>(array().n_buffers);
        m_buffers.reserve(buffer_count);
        const data_type data_type = format_to_data_type(schema().format);
        const std::vector<buffer_type> buffers_type = get_buffer_types_from_data_type(data_type);
        for (std::size_t i = 0; i < buffer_count; ++i)
        {
            const auto buffer_type = buffers_type[i];
            auto buffer = array().buffers[i];
            const std::size_t buffer_size = compute_buffer_size(buffer_type, length(), data_type);
            auto* ptr = static_cast<uint8_t*>(const_cast<void*>(buffer));
            m_buffers.emplace_back(ptr, buffer_size);
        }
    }

    inline
    void arrow_proxy::validate_array_and_schema() const
    {
        SPARROW_ASSERT_TRUE(this->array().release != nullptr);
        SPARROW_ASSERT_TRUE(this->schema().release != nullptr);

        SPARROW_ASSERT_TRUE(this->array().n_children == this->schema().n_children);
        SPARROW_ASSERT_TRUE((this->array().dictionary == nullptr) == (this->schema().dictionary == nullptr));

        const data_type data_type = format_to_data_type(this->schema().format);
        if (!validate_format_with_arrow_array(data_type, array()))
        {
            // TODO: Replace with a more specific exception
            throw std::runtime_error("Invalid ArrowArray format");
        }
    }

    inline
    arrow_proxy::arrow_proxy(ArrowArray&& array, ArrowSchema&& schema)
        : m_array(std::move(array))
        , m_schema(std::move(schema))
    {
        validate_array_and_schema();
        initialize_children();
        initialize_buffers();
    }

    inline
    arrow_proxy::arrow_proxy(ArrowArray&& array, ArrowSchema* schema)
        : m_array(std::move(array))
        , m_schema(schema)
    {
        SPARROW_ASSERT_TRUE(schema != nullptr);
        validate_array_and_schema();
        initialize_children();
        initialize_buffers();
    }

    inline
    arrow_proxy::arrow_proxy(ArrowArray* array, ArrowSchema* schema)
        : m_array(array)
        , m_schema(schema)
    {
        SPARROW_ASSERT_TRUE(array != nullptr);
        SPARROW_ASSERT_TRUE(schema != nullptr);
        validate_array_and_schema();
        initialize_children();
        initialize_buffers();
    }

    inline
    arrow_proxy::~arrow_proxy()
    {
        if (m_array.index() == 1)
        {
            ArrowArray& array = std::get<1>(m_array);
            if (array.release != nullptr)
            {
                try
                {
                    array.release(&array);
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
            ArrowSchema& schema = std::get<1>(m_schema);
            if (schema.release != nullptr)
            {
                try
                {
                    schema.release(&schema);
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

    [[nodiscard]] inline std::string_view arrow_proxy::format() const
    {
        return schema().format;
    }

    [[nodiscard]] inline std::string_view arrow_proxy::name() const
    {
        return schema().name;
    }

    [[nodiscard]] inline std::string_view arrow_proxy::metadata() const
    {
        return schema().metadata;
    }

    [[nodiscard]] inline std::vector<ArrowFlag> arrow_proxy::flags() const
    {
        return to_vector_of_ArrowFlags(schema().flags);
    }

    [[nodiscard]] inline int64_t arrow_proxy::length() const
    {
        return array().length;
    }

    [[nodiscard]] inline int64_t arrow_proxy::null_count() const
    {
        return array().null_count;
    }

    [[nodiscard]] inline int64_t arrow_proxy::offset() const
    {
        return array().offset;
    }

    [[nodiscard]] inline int64_t arrow_proxy::n_buffers() const
    {
        return array().n_buffers;
    }

    [[nodiscard]] inline int64_t arrow_proxy::n_children() const
    {
        return array().n_children;
    }

    [[nodiscard]] inline std::size_t
    arrow_proxy::compute_buffer_size(buffer_type buffer_type, int64_t length, data_type data_type) const
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
                return get_offset_size(data_type, length) * sizeof(std::int32_t);
            case buffer_type::OFFSETS_64BIT:
            case buffer_type::SIZES_64BIT:
                return get_offset_size(data_type, length) * sizeof(std::int64_t);
            case buffer_type::VIEWS:
                // TODO: Implement
                SPARROW_ASSERT(false, "Not implemented");
                return 0;
            case buffer_type::TYPE_IDS:
                return static_cast<std::size_t>(length);
        }
        mpl::unreachable();
    }

    [[nodiscard]] inline const std::vector<sparrow::buffer_view<uint8_t>>& arrow_proxy::buffers() const
    {
        return m_buffers;
    }

    [[nodiscard]] inline const std::vector<arrow_proxy>& arrow_proxy::children() const
    {
        return m_children;
    }

    [[nodiscard]] inline std::optional<arrow_proxy> arrow_proxy::dictionary() const
    {
        if (array().dictionary == nullptr)
        {
            return std::nullopt;
        }
        return arrow_proxy{array().dictionary, schema().dictionary};
    }

    [[nodiscard]] inline bool arrow_proxy::is_created_with_sparrow() const
    {
        return (array().release == &sparrow::release_arrow_array)
               && (schema().release == &sparrow::release_arrow_schema);
    }

    [[nodiscard]] inline void* arrow_proxy::private_data() const
    {
        return array().private_data;
    }

    template <typename T>
    [[nodiscard]] T& get_value_reference_of_variant(auto& var)
    {
        return std::visit(
            overloaded{
                [](T* ptr) -> T&
                {
                    if (ptr == nullptr)
                    {
                        // TODO: Replace with a more specific exception
                        throw std::runtime_error("Null pointer");
                    }
                    return *ptr;
                },
                [](T& ref) -> T&
                {
                    return ref;
                }
            },
            var
        );
    }

    [[nodiscard]] inline const ArrowArray& arrow_proxy::array() const
    {
        return get_value_reference_of_variant<const ArrowArray>(m_array);
    }

    [[nodiscard]] inline const ArrowSchema& arrow_proxy::schema() const
    {
        return get_value_reference_of_variant<const ArrowSchema>(m_schema);
    }

    [[nodiscard]] inline ArrowArray& arrow_proxy::array()
    {
        return get_value_reference_of_variant<ArrowArray>(m_array);
    }

    [[nodiscard]] inline ArrowSchema& arrow_proxy::schema()
    {
        return get_value_reference_of_variant<ArrowSchema>(m_schema);
    }
}
