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

#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"

#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_info_utils.hpp"
#include "sparrow/arrow_interface/arrow_flag_utils.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/arrow_interface/arrow_schema/private_data.hpp"
#include "sparrow/arrow_interface/private_data_ownership.hpp"
#include "sparrow/buffer/dynamic_bitset/dynamic_bitset_view.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/utils/contracts.hpp"

namespace sparrow
{
    static constexpr size_t bitmap_buffer_index = 0;

    template <typename T>
    [[nodiscard]] constexpr T& get_value_reference_of_variant(auto& var)
    {
        return var.index() == 0 ? *std::get<0>(var) : std::get<1>(var);
    }

    constexpr void assert_if_invalid_pointers(const ArrowArray* array, const ArrowSchema* schema)
    {
        SPARROW_ASSERT_TRUE(array != nullptr);
        SPARROW_ASSERT_TRUE(array->release != nullptr);
        SPARROW_ASSERT_TRUE(schema != nullptr);
        SPARROW_ASSERT_TRUE(schema->release != nullptr);
    }

    void arrow_proxy::throw_if_immutable(const std::source_location location) const
    {
        if (!is_created_with_sparrow())
        {
            const auto error_message = std::string("Cannot call ") + std::string(location.function_name())
                                       + " on non-sparrow created ArrowArray or ArrowSchema";
            throw arrow_proxy_exception(error_message);
        }
        if (m_array_is_immutable || m_schema_is_immutable)
        {
            const auto error_message = std::string("Cannot call ") + std::string(location.function_name())
                                       + " on an immutable arrow_proxy. You may have passed a const ArrowArray* or const ArrowSchema* at the creation.";
            throw arrow_proxy_exception(error_message);
        }
    }

    arrow_proxy arrow_proxy::view() const
    {
        if (m_array_is_immutable || m_schema_is_immutable)
        {
            return arrow_proxy(&array(), &schema());
        }
        else
        {
            ArrowArray* array_ptr = const_cast<ArrowArray*>(&array());
            ArrowSchema* schema_ptr = const_cast<ArrowSchema*>(&schema());
            return arrow_proxy(array_ptr, schema_ptr);
        }
    }

    [[nodiscard]] bool arrow_proxy::is_view() const noexcept
    {
        return (m_schema.index() == 0) && (m_array.index() == 0);  // If we don't own the schema and array, we
                                                                   // are a view
    }

    void arrow_proxy::update_buffers()
    {
        if (is_created_with_sparrow() && !m_array_is_immutable && !m_schema_is_immutable)
        {
            get_array_private_data()->update_buffers_ptrs();
            array_without_sanitize().buffers = get_array_private_data()->buffers_ptrs<void>();
            array_without_sanitize().n_buffers = static_cast<int64_t>(n_buffers());
        }
        const arrow_proxy& const_this = *this;
        m_buffers = get_arrow_array_buffers(
            const_this.array_without_sanitize(),
            const_this.schema_without_sanitize()
        );
    }

    void arrow_proxy::update_children()
    {
        m_children.clear();
        m_children.reserve(n_children());

        const arrow_proxy& const_this = *this;
        ArrowArray** array_children = const_this.array_without_sanitize().children;
        ArrowSchema** schema_children = const_this.schema_without_sanitize().children;
        for (size_t i = 0; i < n_children(); ++i)
        {
            m_children.emplace_back(array_children[i], schema_children[i]);
        }
    }

    void arrow_proxy::update_dictionary()
    {
        const arrow_proxy& const_this = *this;
        if (const_this.array_without_sanitize().dictionary == nullptr
            || const_this.schema_without_sanitize().dictionary == nullptr)
        {
            m_dictionary = nullptr;
        }
        else
        {
            m_dictionary = std::make_unique<arrow_proxy>(
                const_this.array_without_sanitize().dictionary,
                const_this.schema_without_sanitize().dictionary
            );
        }
    }

    void arrow_proxy::reset()
    {
        m_buffers.clear();
        m_children.clear();
        m_dictionary.reset();
    }

    bool arrow_proxy::array_created_with_sparrow() const
    {
        return array_without_sanitize().release == &sparrow::release_arrow_array;
    }

    bool arrow_proxy::schema_created_with_sparrow() const
    {
        return schema_without_sanitize().release == &sparrow::release_arrow_schema;
    }

    void arrow_proxy::validate_array_and_schema() const
    {
        SPARROW_ASSERT_TRUE(is_proxy_valid());
        SPARROW_ASSERT_TRUE(array_without_sanitize().n_children == schema_without_sanitize().n_children);
        SPARROW_ASSERT_TRUE(
            (array_without_sanitize().dictionary == nullptr) == (schema_without_sanitize().dictionary == nullptr)
        );
    }

    arrow_proxy::arrow_proxy()
        : m_array(nullptr)
        , m_schema(nullptr)
    {
    }

    template <typename AA, typename AS>
        requires std::same_as<std::remove_const_t<std::remove_pointer_t<std::remove_cvref_t<AA>>>, ArrowArray>
                 && std::same_as<std::remove_const_t<std::remove_pointer_t<std::remove_cvref_t<AS>>>, ArrowSchema>
    arrow_proxy::arrow_proxy(AA&& array, AS&& schema, impl_tag)
    {
        if constexpr (std::is_const_v<std::remove_pointer_t<std::remove_reference_t<AA>>>)
        {
            m_array_is_immutable = true;
            m_array = const_cast<ArrowArray*>(array);
        }
        else
        {
            m_array = std::forward<AA>(array);
        }

        if constexpr (std::is_const_v<std::remove_pointer_t<std::remove_reference_t<AS>>>)
        {
            m_schema_is_immutable = true;
            m_schema = const_cast<ArrowSchema*>(schema);
        }
        else
        {
            m_schema = std::forward<AS>(schema);
        }

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

    arrow_proxy::arrow_proxy(ArrowArray&& array, ArrowSchema&& schema)
        : arrow_proxy(std::move(array), std::move(schema), impl_tag{})
    {
    }

    arrow_proxy::arrow_proxy(ArrowArray&& array, ArrowSchema* schema)
        : arrow_proxy(std::move(array), schema, impl_tag{})
    {
    }

    arrow_proxy::arrow_proxy(ArrowArray&& array, const ArrowSchema* schema)
        : arrow_proxy(std::move(array), const_cast<ArrowSchema*>(schema), impl_tag{})
    {
    }

    arrow_proxy::arrow_proxy(ArrowArray* array, ArrowSchema* schema)
        : arrow_proxy(array, schema, impl_tag{})
    {
    }

    arrow_proxy::arrow_proxy(const ArrowArray* array, const ArrowSchema* schema)
        : arrow_proxy(array, schema, impl_tag{})
    {
    }

    arrow_proxy::arrow_proxy(const arrow_proxy& other)
    {
        if (!other.empty())
        {
            m_array = copy_array(other.array(), other.schema());
            m_schema = copy_schema(other.schema());
            m_array_is_immutable = false;
            m_schema_is_immutable = false;
            validate_array_and_schema();
            update_buffers();
            update_children();
            update_dictionary();
        }
        else
        {
            m_array = nullptr;
            m_schema = nullptr;
            m_array_is_immutable = false;
            m_schema_is_immutable = false;
        }
    }

    arrow_proxy& arrow_proxy::operator=(const arrow_proxy& other)
    {
        if (this == &other)
        {
            return *this;
        }
        arrow_proxy tmp(other);
        swap(tmp);
        return *this;
    }

    arrow_proxy::arrow_proxy(arrow_proxy&& other) noexcept
        : m_array(std::move(other.m_array))
        , m_schema(std::move(other.m_schema))
        , m_buffers(std::move(other.m_buffers))
        , m_children(std::move(other.m_children))
        , m_dictionary(std::move(other.m_dictionary))
        , m_array_is_immutable(other.m_array_is_immutable)
        , m_schema_is_immutable(other.m_schema_is_immutable)
    {
        other.m_array = {};
        other.m_schema = {};
        other.reset();
    }

    arrow_proxy& arrow_proxy::operator=(arrow_proxy&& rhs) noexcept
    {
        swap(rhs);
        return *this;
    }

    arrow_proxy::~arrow_proxy()
    {
        if (m_array.index() == 1)  // We own the array
        {
            ArrowArray& array_ref = array_without_sanitize();
            if (array_ref.release != nullptr)
            {
                try
                {
                    array_ref.release(&array_ref);
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
            ArrowSchema& schema_ref = schema_without_sanitize();
            if (schema_ref.release != nullptr)
            {
                try
                {
                    schema_ref.release(&schema_ref);
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

    [[nodiscard]] bool arrow_proxy::empty() const
    {
        return m_array.index() == 0 && std::get<ArrowArray*>(m_array) == nullptr;
    }

    [[nodiscard]] const std::string_view arrow_proxy::format() const
    {
        return schema_without_sanitize().format;
    }

    void arrow_proxy::set_format(const std::string_view format)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set format on non-sparrow created ArrowArray");
        }
        if (m_schema_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot set format on an immutable arrow_proxy. You may have passed a const ArrowSchema* at the creation."
            );
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
        schema_without_sanitize().format = get_schema_private_data()->format_ptr();
    }

    [[nodiscard]] enum data_type arrow_proxy::data_type() const
    {
        return format_to_data_type(schema_without_sanitize().format);
    }

    void arrow_proxy::set_data_type(enum data_type data_type)
    {
        if (!schema_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set data_type on non-sparrow created ArrowArray");
        }
        set_format(data_type_to_format(data_type));
        schema_without_sanitize().format = get_schema_private_data()->format_ptr();
    }

    [[nodiscard]] std::optional<std::string_view> arrow_proxy::name() const
    {
        if (schema_without_sanitize().name == nullptr)
        {
            return std::nullopt;
        }
        return std::string_view(schema_without_sanitize().name);
    }

    void arrow_proxy::set_name(std::optional<std::string_view> name)
    {
        if (!schema_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set name on non-sparrow created ArrowArray");
        }
        auto private_data = get_schema_private_data();
        private_data->name() = name;
        schema_without_sanitize().name = private_data->name_ptr();
    }

    [[nodiscard]] std::optional<key_value_view> arrow_proxy::metadata() const
    {
        if (schema_without_sanitize().metadata == nullptr)
        {
            return std::nullopt;
        }
        return key_value_view(schema_without_sanitize().metadata);
    }

    [[nodiscard]] std::unordered_set<ArrowFlag> arrow_proxy::flags() const
    {
        return to_set_of_ArrowFlags(schema_without_sanitize().flags);
    }

    void arrow_proxy::set_flags(const std::unordered_set<ArrowFlag>& flags)
    {
        if (!schema_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set flags on non-sparrow created ArrowArray");
        }
        schema_without_sanitize().flags = to_ArrowFlag_value(flags);
    }

    [[nodiscard]] size_t arrow_proxy::length() const
    {
        const ArrowArray& array = array_without_sanitize();
        SPARROW_ASSERT_TRUE(array.length >= 0);
        SPARROW_ASSERT_TRUE(std::cmp_less(array.length, std::numeric_limits<size_t>::max()));
        return static_cast<size_t>(array.length);
    }

    void arrow_proxy::set_length(size_t length)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(length, std::numeric_limits<int64_t>::max()));
        if (!array_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set length on non-sparrow created ArrowArray");
        }
        array_without_sanitize().length = static_cast<int64_t>(length);
        update_buffers();
        update_null_count();
    }

    [[nodiscard]] int64_t arrow_proxy::null_count() const
    {
        return array_without_sanitize().null_count;
    }

    void arrow_proxy::set_null_count(int64_t null_count)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set null_count on non-sparrow created ArrowArray");
        }
        if (m_array_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot set null_count on an immutable arrow_proxy. You may have passed a const ArrowArray* at the creation."
            );
        }
        array_without_sanitize().null_count = null_count;
    }

    [[nodiscard]] size_t arrow_proxy::offset() const
    {
        return static_cast<size_t>(array_without_sanitize().offset);
    }

    void arrow_proxy::set_offset(size_t offset)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(offset, std::numeric_limits<int64_t>::max()));
        if (!array_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set offset on non-sparrow created ArrowArray");
        }
        if (m_array_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot set offset on an immutable arrow_proxy. You may have passed a const ArrowArray* at the creation."
            );
        }
        array_without_sanitize().offset = static_cast<int64_t>(offset);
    }

    [[nodiscard]] size_t arrow_proxy::n_buffers() const
    {
        return static_cast<size_t>(array_without_sanitize().n_buffers);
    }

    void arrow_proxy::set_n_buffers(size_t n_buffers)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(n_buffers, std::numeric_limits<int64_t>::max()));
        if (!array_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set n_buffers on non-sparrow created ArrowArray");
        }
        if (m_array_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot set n_buffers on an immutable arrow_proxy. You may have passed a const ArrowArray* at the creation."
            );
        }
        array_without_sanitize().n_buffers = static_cast<int64_t>(n_buffers);
        arrow_array_private_data* private_data = get_array_private_data();
        private_data->resize_buffers(n_buffers);
        update_buffers();
    }

    [[nodiscard]] size_t arrow_proxy::n_children() const
    {
        return static_cast<size_t>(array_without_sanitize().n_children);
    }

    void arrow_proxy::pop_children(size_t n)
    {
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set n_buffers on non-sparrow created ArrowArray or ArrowSchema");
        }
        if (m_array_is_immutable || m_schema_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot pop children on an immutable arrow_proxy. You may have passed a const ArrowArray* or const ArrowSchema* at the creation."
            );
        }
        if (n > n_children())
        {
            throw arrow_proxy_exception("Cannot pop more children than the current number of children");
        }

        resize_children(n_children() - n);
    }

    void arrow_proxy::resize_children(size_t children_count)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(children_count, std::numeric_limits<int64_t>::max()));
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot resize the children of a non-sparrow created ArrowArray");
        }
        if (m_array_is_immutable || m_schema_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot resize the children on an immutable arrow_proxy. You may have passed a const ArrowArray* or const ArrowSchema* at the creation."
            );
        }
        arrow_array_private_data* array_private_data = get_array_private_data();
        arrow_schema_private_data* schema_private_data = get_schema_private_data();
        // Release the remaining children if the new size is smaller than the current size
        ArrowArray& array_ref = array_without_sanitize();
        ArrowSchema& schema_ref = schema_without_sanitize();
        for (size_t i = children_count; i < static_cast<size_t>(array_ref.n_children); ++i)
        {
            if (schema_private_data->has_child_ownership(i))
            {
                ArrowSchema* child = schema_ref.children[i];
                child->release(child);
            }
            if (array_private_data->has_child_ownership(i))
            {
                ArrowArray* child = array_ref.children[i];
                child->release(child);
            }
        }

        auto new_array_children = std::make_unique<ArrowArray*[]>(children_count);
        auto new_schema_children = std::make_unique<ArrowSchema*[]>(children_count);

        for (size_t i = 0; i < std::min(children_count, static_cast<size_t>(array_ref.n_children)); ++i)
        {
            new_array_children[i] = array_ref.children[i];
            new_schema_children[i] = schema_ref.children[i];
        }
        auto* tmp_array_children = array_ref.children;
        auto* tmp_schema_children = schema_ref.children;

        array_ref.children = new_array_children.release();
        array_ref.n_children = static_cast<int64_t>(children_count);
        schema_ref.children = new_schema_children.release();
        schema_ref.n_children = static_cast<int64_t>(children_count);

        array_private_data->resize_children(children_count);
        schema_private_data->resize_children(children_count);
        m_children.resize(children_count, arrow_proxy());

        new_array_children.reset(tmp_array_children);
        new_schema_children.reset(tmp_schema_children);
    }

    arrow_schema_private_data* arrow_proxy::get_schema_private_data()
    {
        if (!schema_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot get schema private data on non-sparrow created ArrowArray");
        }
        if (m_schema_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot get schema private data on an immutable arrow_proxy. You may have passed a const ArrowSchema* at the creation."
            );
        }
        return static_cast<arrow_schema_private_data*>(schema_without_sanitize().private_data);
    }

    arrow_array_private_data* arrow_proxy::get_array_private_data()
    {
        if (!array_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot get array private data on non-sparrow created ArrowArray");
        }
        if (m_array_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot get array private data on an immutable arrow_proxy. You may have passed a const ArrowArray* at the creation."
            );
        }
        return static_cast<arrow_array_private_data*>(array_without_sanitize().private_data);
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
        if (!array_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set buffer on non-sparrow created ArrowArray");
        }
        if (m_array_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot set buffer on an immutable arrow_proxy. You may have passed a const ArrowArray* at the creation."
            );
        }
        get_array_private_data()->set_buffer(index, buffer);
        update_null_count();
        update_buffers();
    }

    void arrow_proxy::set_buffer(size_t index, buffer<uint8_t>&& buffer)
    {
        SPARROW_ASSERT_TRUE(std::cmp_less(index, n_buffers()));
        if (!array_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set buffer on non-sparrow created ArrowArray");
        }
        if (m_array_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot set buffer on an immutable arrow_proxy. You may have passed a const ArrowArray* at the creation."
            );
        }
        get_array_private_data()->set_buffer(index, std::move(buffer));
        update_null_count();
        update_buffers();
    }

    [[nodiscard]] const std::vector<arrow_proxy>& arrow_proxy::children() const
    {
        return m_children;
    }

    [[nodiscard]] std::vector<arrow_proxy>& arrow_proxy::children()
    {
        return m_children;
    }

    void arrow_proxy::add_child(ArrowArray* array, ArrowSchema* schema)
    {
        using value_type = arrow_array_and_schema_pointers;
        add_children(std::ranges::single_view(value_type{array, schema}));
    }

    void arrow_proxy::add_child(ArrowArray&& array, ArrowSchema&& schema)
    {
        using value_type = arrow_array_and_schema;
        add_children(std::ranges::single_view(value_type{std::move(array), std::move(schema)}));
    }

    [[nodiscard]] const std::unique_ptr<arrow_proxy>& arrow_proxy::dictionary() const
    {
        return m_dictionary;
    }

    [[nodiscard]] std::unique_ptr<arrow_proxy>& arrow_proxy::dictionary()
    {
        return m_dictionary;
    }

    void arrow_proxy::set_dictionary(ArrowArray* array_dictionary, ArrowSchema* schema_dictionary)
    {
        SPARROW_ASSERT_TRUE(array_dictionary != nullptr);
        SPARROW_ASSERT_TRUE(schema_dictionary != nullptr);
        SPARROW_ASSERT_TRUE(array_dictionary->release != nullptr);
        SPARROW_ASSERT_TRUE(schema_dictionary->release != nullptr);
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set dictionary on non-sparrow created ArrowArray or ArrowSchema");
        }
        if (m_array_is_immutable || m_schema_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot set dictionary on an immutable arrow_proxy. You may have passed a const ArrowArray* or const ArrowSchema* at the creation."
            );
        }

        ArrowArray* current_array_dictionary = array_without_sanitize().dictionary;
        if (current_array_dictionary != nullptr)
        {
            current_array_dictionary->release(current_array_dictionary);
        }
        ArrowSchema* current_dictionary_schema = schema_without_sanitize().dictionary;
        if (current_dictionary_schema != nullptr)
        {
            current_dictionary_schema->release(current_dictionary_schema);
        }

        array_without_sanitize().dictionary = array_dictionary;
        schema_without_sanitize().dictionary = schema_dictionary;
        get_array_private_data()->set_dictionary_ownership(false);
        get_schema_private_data()->set_dictionary_ownership(false);
        update_dictionary();
    }

    void arrow_proxy::set_dictionary(ArrowArray&& array_dictionary, ArrowSchema&& schema_dictionary)
    {
        SPARROW_ASSERT_TRUE(array_dictionary.release != nullptr);
        SPARROW_ASSERT_TRUE(schema_dictionary.release != nullptr);
        if (!is_created_with_sparrow())
        {
            throw arrow_proxy_exception("Cannot set dictionary on non-sparrow created ArrowArray or ArrowSchema");
        }
        if (m_array_is_immutable || m_schema_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot set dictionary on an immutable arrow_proxy. You may have passed a const ArrowArray* or const ArrowSchema* at the creation."
            );
        }

        ArrowArray* current_array_dictionary = array_without_sanitize().dictionary;
        if (current_array_dictionary != nullptr)
        {
            current_array_dictionary->release(current_array_dictionary);
        }
        ArrowSchema* current_dictionary_schema = schema_without_sanitize().dictionary;
        if (current_dictionary_schema != nullptr)
        {
            current_dictionary_schema->release(current_dictionary_schema);
        }

        array_without_sanitize().dictionary = new ArrowArray(std::move(array_dictionary));
        schema_without_sanitize().dictionary = new ArrowSchema(std::move(schema_dictionary));
        get_array_private_data()->set_dictionary_ownership(true);
        get_schema_private_data()->set_dictionary_ownership(true);
        update_dictionary();
    }

    [[nodiscard]] bool arrow_proxy::is_created_with_sparrow() const
    {
        return array_created_with_sparrow() && schema_created_with_sparrow();
    }

    [[nodiscard]] void* arrow_proxy::private_data() const
    {
        return array_without_sanitize().private_data;
    }

    [[nodiscard]] bool arrow_proxy::owns_array() const
    {
        return std::holds_alternative<ArrowArray>(m_array);
    }

    [[nodiscard]] bool arrow_proxy::owns_schema() const
    {
        return std::holds_alternative<ArrowSchema>(m_schema);
    }

    [[nodiscard]] const ArrowArray& arrow_proxy::array() const
    {
        const_cast<arrow_proxy&>(*this).sanitize_schema();
        return array_without_sanitize();
    }

    [[nodiscard]] const ArrowSchema& arrow_proxy::schema() const
    {
        const_cast<arrow_proxy&>(*this).sanitize_schema();
        return schema_without_sanitize();
    }

    [[nodiscard]] ArrowArray& arrow_proxy::array()
    {
        if (m_array_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot get mutable ArrowArray from an immutable arrow_proxy. You may have passed a const ArrowArray* at the creation."
            );
        }
        const_cast<arrow_proxy&>(*this).sanitize_schema();
        return array_without_sanitize();
    }

    [[nodiscard]] ArrowSchema& arrow_proxy::schema()
    {
        if (m_schema_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot get mutable ArrowSchema from an immutable arrow_proxy. You may have passed a const ArrowSchema* at the creation."
            );
        }
        const_cast<arrow_proxy&>(*this).sanitize_schema();
        return schema_without_sanitize();
    }

    [[nodiscard]] ArrowArray arrow_proxy::extract_array()
    {
        if (std::holds_alternative<ArrowArray*>(m_array))
        {
            throw std::runtime_error("Cannot extract an ArrowArray not owned by the structure");
        }
        sanitize_schema();
        ArrowArray res = std::get<ArrowArray>(std::move(m_array));
        m_array = ArrowArray{};
        reset();
        return res;
    }

    [[nodiscard]] ArrowSchema arrow_proxy::extract_schema()
    {
        if (std::holds_alternative<ArrowSchema*>(m_schema))
        {
            throw std::runtime_error("Cannot extract an ArrowSchema not owned by the structure");
        }
        sanitize_schema();
        ArrowSchema res = std::get<ArrowSchema>(std::move(m_schema));
        m_schema = ArrowSchema{};
        reset();
        return res;
    }

    void arrow_proxy::update_null_count()
    {
        if (has_bitmap(data_type()))
        {
            const auto& validity_buffer = buffers().front();
            const dynamic_bitset_view<const std::uint8_t> bitmap(validity_buffer.data(), length() + offset());
            const auto null_count = bitmap.null_count();
            set_null_count(static_cast<int64_t>(null_count));
        }
    }

    bool arrow_proxy::is_arrow_array_valid() const
    {
        return array_without_sanitize().release != nullptr;
    }

    bool arrow_proxy::is_arrow_schema_valid() const
    {
        return schema_without_sanitize().release != nullptr;
    }

    bool arrow_proxy::is_proxy_valid() const
    {
        return is_arrow_array_valid() && is_arrow_schema_valid();
    }

    void arrow_proxy::swap(arrow_proxy& other) noexcept
    {
        if (this == &other)
        {
            return;
        }
        std::swap(m_array, other.m_array);
        std::swap(m_schema, other.m_schema);
        std::swap(m_buffers, other.m_buffers);
        std::swap(m_children, other.m_children);
        std::swap(m_dictionary, other.m_dictionary);
    }

    [[nodiscard]] non_owning_dynamic_bitset<uint8_t> arrow_proxy::get_non_owning_dynamic_bitset()
    {
        if (!array_created_with_sparrow())
        {
            throw arrow_proxy_exception(
                "Cannot get non owning dynamic bitset from a non-sparrow created ArrowArray or ArrowSchema"
            );
        }

        SPARROW_ASSERT_TRUE(is_created_with_sparrow())
        SPARROW_ASSERT_TRUE(has_bitmap(data_type()))
        auto private_data = static_cast<arrow_array_private_data*>(array_without_sanitize().private_data);
        auto& bitmap_buffer = private_data->buffers()[bitmap_buffer_index];
        const size_t current_size = length() + offset();
        non_owning_dynamic_bitset<uint8_t> bitmap{&bitmap_buffer, current_size};
        return bitmap;
    }

    void arrow_proxy::resize_bitmap(size_t new_size, bool value)
    {
        if (!array_created_with_sparrow())
        {
            throw arrow_proxy_exception(
                "Cannot resize bitmap on a non-sparrow created ArrowArray or ArrowSchema"
            );
        }
        if (m_array_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot resize bitmap on an immutable arrow_proxy. You may have passed a const ArrowArray* at the creation."
            );
        }
        SPARROW_ASSERT_TRUE(has_bitmap(data_type()))
        auto bitmap = get_non_owning_dynamic_bitset();
        bitmap.resize(new_size, value);
        update_buffers();
    }

    size_t arrow_proxy::insert_bitmap(size_t index, bool value, size_t count)
    {
        if (!array_created_with_sparrow())
        {
            throw arrow_proxy_exception(
                "Cannot insert values in bitmap on a non-sparrow created ArrowArray or ArrowSchema"
            );
        }
        if (m_array_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot insert values in bitmap on an immutable arrow_proxy. You may have passed a const ArrowArray* at the creation."
            );
        }
        SPARROW_ASSERT_TRUE(has_bitmap(data_type()))
        SPARROW_ASSERT_TRUE(std::cmp_less_equal(index, length() + offset()))
        if (count == 0)
        {
            return index;
        }
        auto bitmap = get_non_owning_dynamic_bitset();
        auto it = bitmap.insert(sparrow::next(bitmap.cbegin(), index), count, value);
        update_buffers();
        return std::distance(bitmap.begin(), it);
    }

    size_t arrow_proxy::erase_bitmap(size_t index, size_t count)
    {
        if (!array_created_with_sparrow())
        {
            throw arrow_proxy_exception(
                "Cannot erase values in bitmap on a non-sparrow created ArrowArray or ArrowSchema"
            );
        }
        if (m_array_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot erase values in bitmap on an immutable arrow_proxy. You may have passed a const ArrowArray* at the creation."
            );
        }
        SPARROW_ASSERT_TRUE(has_bitmap(data_type()))
        SPARROW_ASSERT_TRUE(std::cmp_less(index, length()))
        auto bitmap = get_non_owning_dynamic_bitset();
        const auto it_first = sparrow::next(bitmap.cbegin(), index + offset());
        const auto it_last = sparrow::next(it_first, count);
        const auto it = bitmap.erase(it_first, it_last);
        update_buffers();
        return std::distance(bitmap.begin(), it);
    }

    void arrow_proxy::push_back_bitmap(bool value)
    {
        if (!array_created_with_sparrow())
        {
            throw arrow_proxy_exception(
                "Cannot push_back value in bitmap on a non-sparrow created ArrowArray or ArrowSchema"
            );
        }
        if (m_array_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot push_back value in bitmap on an immutable arrow_proxy. You may have passed a const ArrowArray* at the creation."
            );
        }
        SPARROW_ASSERT_TRUE(has_bitmap(data_type()))
        insert_bitmap(length(), value);
        update_buffers();
    }

    void arrow_proxy::pop_back_bitmap()
    {
        if (!array_created_with_sparrow())
        {
            throw arrow_proxy_exception(
                "Cannot pop_back value in bitmap on a non-sparrow created ArrowArray or ArrowSchema"
            );
        }
        if (m_array_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot pop_back value in bitmap on an immutable arrow_proxy. You may have passed a const ArrowArray* at the creation."
            );
        }
        SPARROW_ASSERT_TRUE(has_bitmap(data_type()))
        erase_bitmap(length() - 1);
        update_buffers();
    }

    arrow_proxy arrow_proxy::slice(size_t start, size_t end) const
    {
        SPARROW_ASSERT_TRUE(start <= end);
        arrow_proxy copy = *this;
        copy.set_offset(start);
        copy.set_length(end - start);
        return copy;
    }

    arrow_proxy arrow_proxy::slice_view(size_t start, size_t end) const
    {
        SPARROW_ASSERT_TRUE(start <= end);
        ArrowSchema as = schema();
        as.release = empty_release_arrow_schema;
        ArrowArray ar = array();
        ar.offset = static_cast<int64_t>(start);
        ar.length = static_cast<int64_t>(end - start);
        ar.release = empty_release_arrow_array;
        return arrow_proxy{std::move(ar), std::move(as)};
    }

    void arrow_proxy::sanitize_schema()
    {
        if (is_created_with_sparrow() && !m_schema_is_immutable && !m_array_is_immutable)
        {
            bool has_nulls = null_count() != 0;

            if (m_dictionary != nullptr)
            {
                if (m_dictionary->null_count() != 0)
                {
                    auto new_dictionary_flags = m_dictionary->flags();
                    new_dictionary_flags.emplace(ArrowFlag::NULLABLE);
                    m_dictionary->set_flags(new_dictionary_flags);
                    has_nulls = true;
                }
            }
            if (has_nulls)
            {
                auto new_flags = flags();
                new_flags.emplace(ArrowFlag::NULLABLE);
                set_flags(new_flags);
            }
        }
    }

    ArrowArray& arrow_proxy::array_without_sanitize()
    {
        if (m_array_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot access array on an immutable arrow_proxy. You may have passed a const ArrowArray* at the creation."
            );
        }
        return get_value_reference_of_variant<ArrowArray>(m_array);
    }

    const ArrowArray& arrow_proxy::array_without_sanitize() const
    {
        return get_value_reference_of_variant<const ArrowArray>(m_array);
    }

    ArrowSchema& arrow_proxy::schema_without_sanitize()
    {
        if (m_schema_is_immutable)
        {
            throw arrow_proxy_exception(
                "Cannot access schema on an immutable arrow_proxy. You may have passed a const ArrowSchema* at the creation."
            );
        }
        return get_value_reference_of_variant<ArrowSchema>(m_schema);
    }

    const ArrowSchema& arrow_proxy::schema_without_sanitize() const
    {
        return get_value_reference_of_variant<const ArrowSchema>(m_schema);
    }

    bool arrow_proxy::is_array_const() const
    {
        return m_array_is_immutable;
    }

    bool arrow_proxy::is_schema_const() const
    {
        return m_schema_is_immutable;
    }
}
