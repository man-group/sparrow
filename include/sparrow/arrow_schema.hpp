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
#include <optional>
#include <string>
#include <type_traits>

#include "sparrow/arrow_array_schema_utils.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/contracts.hpp"
#include "sparrow/mp_utils.hpp"

namespace sparrow
{
    inline void arrow_schema_custom_deleter(ArrowSchema* schema)
    {
        if (schema)
        {
            if (schema->release != nullptr)
            {
                schema->release(schema);
            }
            delete schema;
        }
    }

    /**
     * A custom deleter for ArrowSchema.
     */
    struct arrow_schema_custom_deleter_struct
    {
        void operator()(ArrowSchema* schema) const
        {
            arrow_schema_custom_deleter(schema);
        }
    };

    /**
     * A unique pointer to an ArrowSchema with a custom deleter.
     * It must be used to manage ArrowSchema objects.
     */
    using arrow_schema_unique_ptr = std::unique_ptr<ArrowSchema, arrow_schema_custom_deleter_struct>;

    /// Shared pointer to an ArrowSchema. Must be used to manage the memory of an ArrowSchema.
    class arrow_schema_shared_ptr : public std::shared_ptr<ArrowSchema>
    {
    public:

        arrow_schema_shared_ptr()
            : std::shared_ptr<ArrowSchema>(nullptr, arrow_schema_custom_deleter)
        {
        }

        arrow_schema_shared_ptr(arrow_schema_shared_ptr&& ptr) noexcept
            : std::shared_ptr<ArrowSchema>(std::move(ptr))
        {
        }

        explicit arrow_schema_shared_ptr(const arrow_schema_shared_ptr& ptr) noexcept
            : std::shared_ptr<ArrowSchema>(ptr)
        {
        }

        explicit arrow_schema_shared_ptr(arrow_schema_unique_ptr&& ptr)
            : std::shared_ptr<ArrowSchema>(std::move(ptr).release(), arrow_schema_custom_deleter)
        {
        }

        explicit arrow_schema_shared_ptr(std::nullptr_t) noexcept
            : std::shared_ptr<ArrowSchema>(nullptr, arrow_schema_custom_deleter)
        {
        }

        arrow_schema_shared_ptr& operator=(arrow_schema_shared_ptr&& ptr) noexcept
        {
            std::shared_ptr<ArrowSchema>::operator=(std::move(ptr));
            return *this;
        }

        arrow_schema_shared_ptr& operator=(const arrow_schema_shared_ptr& ptr) noexcept
        {
            std::shared_ptr<ArrowSchema>::operator=(ptr);
            return *this;
        }

        ~arrow_schema_shared_ptr() = default;
    };

    /**
     * Struct representing private data for ArrowSchema.
     *
     * This struct holds the private data for ArrowSchema, including format,
     * name and metadata strings, children, and dictionary. It is used in the
     * Sparrow library.
     */
    struct arrow_schema_private_data
    {
        arrow_schema_private_data() = delete;
        arrow_schema_private_data(const arrow_schema_private_data&) = delete;
        arrow_schema_private_data(arrow_schema_private_data&&) = delete;
        arrow_schema_private_data& operator=(const arrow_schema_private_data&) = delete;
        arrow_schema_private_data& operator=(arrow_schema_private_data&&) = delete;

        ~arrow_schema_private_data() = default;

        std::string m_format;
        std::optional<std::string> m_name;
        std::optional<std::string> m_metadata;
        std::optional<std::vector<arrow_schema_shared_ptr>> m_children;
        std::vector<ArrowSchema*> m_children_pointers;
        arrow_schema_shared_ptr m_dictionary;

        template <class F, class N, class M, class C, class D>
            requires std::constructible_from<decltype(arrow_schema_private_data::m_format), F>
                     && std::constructible_from<decltype(arrow_schema_private_data::m_name), N>
                     && std::constructible_from<decltype(arrow_schema_private_data::m_metadata), M>
                     && std::constructible_from<decltype(arrow_schema_private_data::m_children), C>
                     && std::constructible_from<decltype(arrow_schema_private_data::m_dictionary), D>
        arrow_schema_private_data(F format, N name, M metadata, C children, D dictionary);

        [[nodiscard]] const char* format() const noexcept;
        [[nodiscard]] const char* name() const noexcept;
        [[nodiscard]] const char* metadata() const noexcept;
        [[nodiscard]] const std::optional<std::vector<arrow_schema_shared_ptr>>& children() noexcept;
        [[nodiscard]] ArrowSchema** children_pointers() noexcept;
        [[nodiscard]] const arrow_schema_shared_ptr& dictionary() const noexcept;
        [[nodiscard]] ArrowSchema* dictionary_pointer() noexcept;
    };

    template <class F, class N, class M, class C, class D>
        requires std::constructible_from<decltype(arrow_schema_private_data::m_format), F>
                     && std::constructible_from<decltype(arrow_schema_private_data::m_name), N>
                     && std::constructible_from<decltype(arrow_schema_private_data::m_metadata), M>
                     && std::constructible_from<decltype(arrow_schema_private_data::m_children), C>
                     && std::constructible_from<decltype(arrow_schema_private_data::m_dictionary), D>
    arrow_schema_private_data::arrow_schema_private_data(F format, N name, M metadata, C children, D dictionary)
        : m_format(std::move(format))
        , m_name(std::move(name))
        , m_metadata(std::move(metadata))
        , m_children(std::move(children))
        , m_children_pointers(to_raw_ptr_vec<ArrowSchema>(m_children))
        , m_dictionary(std::move(dictionary))
    {
    }

    [[nodiscard]] inline const char* arrow_schema_private_data::format() const noexcept
    {
        return m_format.data();
    }

    [[nodiscard]] inline const char* arrow_schema_private_data::name() const noexcept
    {
        if (m_name.has_value())
        {
            return m_name->data();
        }
        return nullptr;
    }

    [[nodiscard]] inline const char* arrow_schema_private_data::metadata() const noexcept
    {
        if (m_metadata.has_value())
        {
            return m_metadata->data();
        }
        return nullptr;
    }

    [[nodiscard]] inline const std::optional<std::vector<arrow_schema_shared_ptr>>&
    arrow_schema_private_data::children() noexcept
    {
        return m_children;
    }

    [[nodiscard]] inline ArrowSchema** arrow_schema_private_data::children_pointers() noexcept
    {
        return m_children_pointers.data();
    }

    [[nodiscard]] inline const arrow_schema_shared_ptr& arrow_schema_private_data::dictionary() const noexcept
    {
        return m_dictionary;
    }

    [[nodiscard]] ArrowSchema* arrow_schema_private_data::dictionary_pointer() noexcept
    {
        return m_dictionary.get();
    }

    /**
     * Deletes an ArrowSchema.
     *
     * @tparam Allocator The allocator for the strings of the ArrowSchema.
     * @param schema The ArrowSchema to delete. Should be a valid pointer to an ArrowSchema. Its release
     * callback should be set.
     */
    void delete_schema(ArrowSchema* schema)
    {
        SPARROW_ASSERT_FALSE(schema == nullptr)
        SPARROW_ASSERT_TRUE(schema->release == std::addressof(delete_schema))

        schema->flags = 0;
        schema->n_children = 0;
        schema->children = nullptr;
        schema->dictionary = nullptr;
        schema->name = nullptr;
        schema->format = nullptr;
        schema->metadata = nullptr;
        if (schema->private_data != nullptr)
        {
            const auto private_data = static_cast<arrow_schema_private_data*>(schema->private_data);
            delete private_data;
        }
        schema->private_data = nullptr;
        schema->release = nullptr;
    }

    inline arrow_schema_unique_ptr default_arrow_schema_unique_ptr()
    {
        auto ptr = arrow_schema_unique_ptr(new ArrowSchema());
        ptr->format = nullptr;
        ptr->name = nullptr;
        ptr->metadata = nullptr;
        ptr->flags = 0;
        ptr->n_children = 0;
        ptr->children = nullptr;
        ptr->dictionary = nullptr;
        ptr->release = nullptr;
        ptr->private_data = nullptr;
        return ptr;
    }

    /**
     * Creates an ArrowSchema.
     *
     * @tparam C Value, reference or rvalue of std::vector<arrow_schema_shared_ptr>
     * @tparam D Value, reference or rvalue of arrow_schema_shared_ptr
     * @param format A mandatory, null-terminated, UTF8-encoded string describing the data type. If the data
     *               type is nested, child types are not encoded here but in the ArrowSchema.children
     *               structures.
     * @param name An optional, null-terminated, UTF8-encoded string of the field or array name.
     *             This is mainly used to reconstruct child fields of nested types.
     * @param metadata An optional, binary string describing the type’s metadata. If the data type
     *                 is nested, the metadata for child types are not encoded here but in the
     * ArrowSchema.children structures.
     * @param flags A bitfield of flags enriching the type description. Its value is computed by OR’ing
     *              together the flag values.
     * @param children An optional, std::vector<arrow_schema_shared_ptr>. Children pointers must not be null.
     * @param dictionary An optional arrow_schema_shared_ptr. Must be present if the ArrowSchema represents a
     * dictionary-encoded type. Must be nullptr otherwise.
     * @return The created ArrowSchema unique pointer.
     */
    template <class F, class N, class M, class C, class D>
        requires std::constructible_from<decltype(arrow_schema_private_data::m_format), F>
                 && std::constructible_from<decltype(arrow_schema_private_data::m_name), N>
                 && std::constructible_from<decltype(arrow_schema_private_data::m_metadata), M>
                 && std::constructible_from<decltype(arrow_schema_private_data::m_children), C>
                 && std::constructible_from<decltype(arrow_schema_private_data::m_dictionary), D>
    [[nodiscard]] arrow_schema_unique_ptr
    make_arrow_schema_unique_ptr(F format, N name, M metadata, std::optional<ArrowFlag> flags, C children, D dictionary)
    {
        SPARROW_ASSERT_FALSE(format.empty())
        if constexpr (!std::same_as<C, std::nullopt_t>)
        {
            if constexpr (mpl::is_type_instance_of_v<C, std::optional>)
            {
                SPARROW_ASSERT_TRUE(std::ranges::none_of(
                    children.value(),
                    [](const auto& child)
                    {
                        return child == nullptr;
                    }
                ))
            }
            else
            {
                SPARROW_ASSERT_TRUE(std::ranges::none_of(
                    children,
                    [](const auto& child)
                    {
                        return child == nullptr;
                    }
                ))
            }
        }

        arrow_schema_unique_ptr schema = default_arrow_schema_unique_ptr();
        schema->flags = flags.has_value() ? static_cast<int64_t>(flags.value()) : 0;
        schema->n_children = get_size(children);

        schema->private_data = new arrow_schema_private_data(
            std::move(format),
            std::move(name),
            std::move(metadata),
            std::move(children),
            std::move(dictionary)
        );

        const auto private_data = static_cast<arrow_schema_private_data*>(schema->private_data);
        schema->format = private_data->format();
        schema->name = private_data->name();
        schema->metadata = private_data->metadata();
        schema->children = private_data->children_pointers();
        schema->dictionary = private_data->dictionary_pointer();
        schema->release = delete_schema;
        return schema;
    };
}
