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

#include "sparrow/c_interface/arrow_schema/private_data.hpp"

namespace sparrow
{

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
        requires std::constructible_from<arrow_schema_private_data::FormatType, F>
                 && std::constructible_from<arrow_schema_private_data::NameType, N>
                 && std::constructible_from<arrow_schema_private_data::MetadataType, M>
                 && std::constructible_from<arrow_schema_private_data::ChildrenType, C>
                 && std::constructible_from<arrow_schema_private_data::DictionaryType, D>
    [[nodiscard]] arrow_schema_unique_ptr
    make_arrow_schema_unique_ptr(F format, N name, M metadata, std::optional<ArrowFlag> flags, C children, D dictionary)
    {
        SPARROW_ASSERT_FALSE(format.empty());
        SPARROW_ASSERT_TRUE(children_are_not_null(children));
        if constexpr (!std::same_as<C, std::nullopt_t>)
        {
            if constexpr (mpl::is_type_instance_of_v<C, std::optional>)
            {
                if (children.has_value())
                {
                    SPARROW_ASSERT_TRUE(std::ranges::all_of(
                        *children,
                        [](const auto& child)
                        {
                            return bool(child);
                        }
                    ))
                }
            }
            else
            {
                SPARROW_ASSERT_TRUE(std::ranges::all_of(
                    children,
                    [](const auto& child)
                    {
                        return bool(child);
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
