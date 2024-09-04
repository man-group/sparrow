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

#include <cstdint>
#include <variant>
#include <vector>

#include "sparrow/array/data_type.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/buffer/buffer_view.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    constexpr bool is_valid_ArrowFlag_value(int64_t value) noexcept
    {
        constexpr std::array<ArrowFlag, 3> valid_values = {
            ArrowFlag::DICTIONARY_ORDERED,
            ArrowFlag::NULLABLE,
            ArrowFlag::MAP_KEYS_SORTED
        };
        return std::ranges::any_of(
            valid_values,
            [value](ArrowFlag flag)
            {
                return static_cast<std::underlying_type_t<ArrowFlag>>(flag) == value;
            }
        );
    }

    constexpr std::vector<ArrowFlag> to_vector_of_ArrowFlags(int64_t flag_values)
    {
        constexpr size_t n_bits = sizeof(flag_values) * 8;
        std::vector<ArrowFlag> flags;
        for (size_t i = 0; i < n_bits; ++i)
        {
            const int64_t flag_value = static_cast<int64_t>(1) << i;
            if ((flag_values & flag_value) != 0)
            {
                if (!is_valid_ArrowFlag_value(flag_value))
                {
                    // TODO: Replace with a more specific exception
                    throw std::runtime_error("Invalid ArrowFlag value");
                }
                flags.push_back(static_cast<ArrowFlag>(flag_value));
            }
        }
        return flags;
    }

    constexpr int64_t to_ArrowFlag_value(const std::vector<ArrowFlag>& flags)
    {
        int64_t flag_values = 0;
        for (const ArrowFlag flag : flags)
        {
            flag_values |= static_cast<int64_t>(flag);
        }
        return flag_values;
    }

    constexpr bool validate_buffers_count(data_type data_type, int64_t n_buffers)
    {
        const std::size_t expected_buffer_count = get_expected_buffer_count(data_type);
        return static_cast<std::size_t>(n_buffers) == expected_buffer_count;
    }

    constexpr std::size_t get_expected_children_count(data_type data_type)
    {
        switch (data_type)
        {
            case data_type::NA:
            case data_type::RUN_ENCODED:
            case data_type::BOOL:
            case data_type::UINT8:
            case data_type::INT8:
            case data_type::UINT16:
            case data_type::INT16:
            case data_type::UINT32:
            case data_type::INT32:
            case data_type::FLOAT:
            case data_type::UINT64:
            case data_type::INT64:
            case data_type::DOUBLE:
            case data_type::HALF_FLOAT:
            case data_type::TIMESTAMP:
            case data_type::FIXED_SIZE_BINARY:
            case data_type::DECIMAL:
            case data_type::FIXED_WIDTH_BINARY:
            case data_type::STRING:
                return 0;
            case data_type::LIST:
            case data_type::LARGE_LIST:
            case data_type::LIST_VIEW:
            case data_type::LARGE_LIST_VIEW:
            case data_type::FIXED_SIZED_LIST:
            case data_type::STRUCT:
            case data_type::MAP:
            case data_type::SPARSE_UNION:
                return 1;
            case data_type::DENSE_UNION:
                return 2;
        }
        mpl::unreachable();
    }

    bool validate_format_with_arrow_array(data_type data_type, const ArrowArray& array)
    {
        const bool buffers_count_valid = validate_buffers_count(data_type, array.n_buffers);
        const bool children_count_valid = static_cast<std::size_t>(array.n_children)
                                          == get_expected_children_count(data_type);
        return buffers_count_valid && children_count_valid;
    }

    enum class buffer_type
    {
        VALIDITY,
        DATA,
        OFFSETS_32BIT,
        OFFSETS_64BIT,
        VIEWS,
        TYPE_IDS,
        SIZES_32BIT,
        SIZES_64BIT,
    };

    constexpr std::vector<buffer_type> get_buffer_types_from_data_type(data_type data_type)
    {
        switch (data_type)
        {
            case data_type::BOOL:
            case data_type::UINT8:
            case data_type::INT8:
            case data_type::UINT16:
            case data_type::INT16:
            case data_type::UINT32:
            case data_type::INT32:
            case data_type::FLOAT:
            case data_type::UINT64:
            case data_type::INT64:
            case data_type::DOUBLE:
            case data_type::HALF_FLOAT:
            case data_type::TIMESTAMP:
            case data_type::FIXED_SIZE_BINARY:
            case data_type::DECIMAL:
            case data_type::FIXED_WIDTH_BINARY:
                return {buffer_type::VALIDITY, buffer_type::DATA};
            case data_type::STRING:
                return {buffer_type::VALIDITY, buffer_type::OFFSETS_32BIT, buffer_type::DATA};
            case data_type::LIST:
                return {buffer_type::VALIDITY, buffer_type::OFFSETS_32BIT};
            case data_type::LARGE_LIST:
                return {buffer_type::VALIDITY, buffer_type::OFFSETS_64BIT};
            case data_type::LIST_VIEW:
                return {buffer_type::VALIDITY, buffer_type::OFFSETS_32BIT, buffer_type::SIZES_32BIT};
            case data_type::LARGE_LIST_VIEW:
                return {buffer_type::VALIDITY, buffer_type::OFFSETS_64BIT, buffer_type::SIZES_64BIT};
            case data_type::FIXED_SIZED_LIST:
            case data_type::STRUCT:
                return {buffer_type::VALIDITY};
            case data_type::SPARSE_UNION:
                return {buffer_type::TYPE_IDS};
            case data_type::DENSE_UNION:
                return {buffer_type::TYPE_IDS, buffer_type::OFFSETS_32BIT};
            case data_type::NA:
            case data_type::MAP:
            case data_type::RUN_ENCODED:
                return {};
        }
        mpl::unreachable();
    }

    constexpr std::size_t get_offset_size(data_type data_type, int64_t length)
    {
        switch (data_type)
        {
            case data_type::STRING:
            case data_type::LIST:
            case data_type::LARGE_LIST:
                return static_cast<std::size_t>(length) + 1;
            case data_type::LIST_VIEW:
            case data_type::LARGE_LIST_VIEW:
            case data_type::DENSE_UNION:
                return static_cast<std::size_t>(length);
            default:
                throw std::runtime_error("Unsupported data type");
        }
    }

    class arrow_proxy
    {
    public:

        explicit arrow_proxy(ArrowArray&& array, ArrowSchema&& schema);
        explicit arrow_proxy(ArrowArray&& array, ArrowSchema* schema);
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

        void release();
        [[nodiscard]] bool is_released() const;

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

    void arrow_proxy::initialize_children()
    {
        m_children.reserve(static_cast<std::size_t>(array().n_children));
        for (int64_t i = 0; i < array().n_children; ++i)
        {
            m_children.emplace_back(array().children[i], schema().children[i]);
        }
    }

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
            auto* ptr = reinterpret_cast<uint8_t*>(const_cast<void*>(buffer));
            m_buffers.emplace_back(ptr, buffer_size);
        }
    }

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

    arrow_proxy::arrow_proxy(ArrowArray&& array, ArrowSchema&& schema)
        : m_array(std::forward<ArrowArray>(array))
        , m_schema(std::forward<ArrowSchema>(schema))
    {
        validate_array_and_schema();
        initialize_children();
        initialize_buffers();
    }

    arrow_proxy::arrow_proxy(ArrowArray&& array, ArrowSchema* schema)
        : m_array(std::forward<ArrowArray>(array))
        , m_schema(schema)
    {
        SPARROW_ASSERT_TRUE(schema != nullptr);
        validate_array_and_schema();
        initialize_children();
        initialize_buffers();
    }

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

    arrow_proxy::~arrow_proxy()
    {
        if(m_array.index() == 1)
        {
            ArrowArray& array = std::get<1>(m_array);
            if(array.release != nullptr)
            {
                array.release(&array);
            }
        }

        if(m_schema.index() == 1)
        {
            ArrowSchema& schema = std::get<1>(m_schema);
            if(schema.release != nullptr)
            {
                schema.release(&schema);
            }
        }

    }

    [[nodiscard]] std::string_view arrow_proxy::format() const
    {
        return schema().format;
    }

    [[nodiscard]] std::string_view arrow_proxy::name() const
    {
        return schema().name;
    }

    [[nodiscard]] std::string_view arrow_proxy::metadata() const
    {
        return schema().metadata;
    }

    [[nodiscard]] std::vector<ArrowFlag> arrow_proxy::flags() const
    {
        return to_vector_of_ArrowFlags(schema().flags);
    }

    [[nodiscard]] int64_t arrow_proxy::length() const
    {
        return array().length;
    }

    [[nodiscard]] int64_t arrow_proxy::null_count() const
    {
        return array().null_count;
    }

    [[nodiscard]] int64_t arrow_proxy::offset() const
    {
        return array().offset;
    }

    [[nodiscard]] int64_t arrow_proxy::n_buffers() const
    {
        return array().n_buffers;
    }

    [[nodiscard]] int64_t arrow_proxy::n_children() const
    {
        return array().n_children;
    }

    [[nodiscard]] std::size_t
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

    [[nodiscard]] const std::vector<sparrow::buffer_view<uint8_t>>& arrow_proxy::buffers() const
    {
        return m_buffers;
    }

    [[nodiscard]] const std::vector<arrow_proxy>& arrow_proxy::children() const
    {
        return m_children;
    }

    [[nodiscard]] std::optional<arrow_proxy> arrow_proxy::dictionary() const
    {
        if (array().dictionary == nullptr)
        {
            return std::nullopt;
        }
        return arrow_proxy{array().dictionary, schema().dictionary};
    }

    void arrow_proxy::release()
    {
        if (array().release != nullptr)
        {
            array().release(&array());
        }
        if (schema().release != nullptr)
        {
            schema().release(&schema());
        }
    }

    [[nodiscard]] bool arrow_proxy::is_released() const
    {
        return array().release == nullptr && schema().release == nullptr;
    }

    [[nodiscard]] bool arrow_proxy::is_created_with_sparrow() const
    {
        return (array().release == &sparrow::release_arrow_array)
               && (schema().release == &sparrow::release_arrow_schema);
    }

    [[nodiscard]] void* arrow_proxy::private_data() const
    {
        return array().private_data;
    }

    template <class... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;
    };
    // Although not required in C++20, clang needs it to build the code below
    template <class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

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

    [[nodiscard]] const ArrowArray& arrow_proxy::array() const
    {
        return get_value_reference_of_variant<const ArrowArray>(m_array);
    }

    [[nodiscard]] const ArrowSchema& arrow_proxy::schema() const
    {
        return get_value_reference_of_variant<const ArrowSchema>(m_schema);
    }

    [[nodiscard]] ArrowArray& arrow_proxy::array()
    {
        return get_value_reference_of_variant<ArrowArray>(m_array);
    }

    [[nodiscard]] ArrowSchema& arrow_proxy::schema()
    {
        return get_value_reference_of_variant<ArrowSchema>(m_schema);
    }
}
