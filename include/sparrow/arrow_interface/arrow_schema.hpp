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
#include <memory>
#if defined(__cpp_lib_format)
#    include <format>
#    include <ostream>
#endif

#include "sparrow/arrow_interface/arrow_schema/private_data.hpp"
#include "sparrow/config/config.hpp"
#include "sparrow/utils/contracts.hpp"

namespace sparrow
{
    /**
     * Creates an `ArrowSchema` owned by a `unique_ptr` and holding the provided data.
     *
     * @tparam F Value, reference or rvalue of `arrow_schema_private_data::FormatType`
     * @tparam N Value, reference or rvalue of `arrow_schema_private_data::NameType`
     * @tparam M Value, reference or rvalue of `arrow_schema_private_data::MetadataType`
     * @param format A mandatory, null-terminated, UTF8-encoded string describing the data type. If the data
     *               type is nested, child types are not encoded here but in the ArrowSchema.children
     *               structures.
     * @param name An optional, null-terminated, UTF8-encoded string of the field or array name.
     *             This is mainly used to reconstruct child fields of nested types.
     * @param metadata An optional, binary string describing the type’s metadata. If the data type
     *                 is nested, the metadata for child types are not encoded here but in the
     * `ArrowSchema.children` structures.
     * @param flags A bitfield of flags enriching the type description. Its value is computed by OR’ing
     *              together the flag values.
     * @param children Pointer to a sequence of `ArrowSchema` pointers or `nullptr`. Must be `nullptr` if
     * `n_children` is `0`.
     * @param dictionary Pointer to `an ArrowSchema`. Must be present if the `ArrowSchema` represents a
     * dictionary-encoded type. Must be `nullptr` otherwise.
     * @return The created `ArrowSchema` unique pointer.
     */
    template <class F, class N, class M>
        requires std::constructible_from<arrow_schema_private_data::FormatType, F>
                 && std::constructible_from<arrow_schema_private_data::NameType, N>
                 && std::constructible_from<arrow_schema_private_data::MetadataType, M>
    [[nodiscard]] ArrowSchema make_arrow_schema(
        F format,
        N name,
        M metadata,
        std::optional<ArrowFlag> flags,
        int64_t n_children,
        ArrowSchema** children,
        ArrowSchema* dictionary
    );

    /**
     * Release function to use for the `ArrowSchema.release` member.
     */
    SPARROW_API void release_arrow_schema(ArrowSchema* schema);

    /**
     * Empty release function to use for the `ArrowSchema.release` member.  Should be used for view of
     * ArrowSchema.
     */
    SPARROW_API void empty_release_arrow_schema(ArrowSchema* schema);

    template <class F, class N, class M>
        requires std::constructible_from<arrow_schema_private_data::FormatType, F>
                 && std::constructible_from<arrow_schema_private_data::NameType, N>
                 && std::constructible_from<arrow_schema_private_data::MetadataType, M>
    void fill_arrow_schema(
        ArrowSchema& schema,
        F format,
        N name,
        M metadata,
        std::optional<ArrowFlag> flags,
        int64_t n_children,
        ArrowSchema** children,
        ArrowSchema* dictionary
    )
    {
        SPARROW_ASSERT_TRUE(n_children >= 0);
        SPARROW_ASSERT_TRUE(n_children > 0 ? children != nullptr : children == nullptr);
        SPARROW_ASSERT_FALSE(format.empty());
        if (children)
        {
            for (int64_t i = 0; i < n_children; ++i)
            {
                SPARROW_ASSERT_FALSE(children[i] == nullptr);
            }
        }

        schema.flags = flags.has_value() ? static_cast<int64_t>(flags.value()) : 0;
        schema.n_children = n_children;

        schema.private_data = new arrow_schema_private_data(
            std::move(format),
            std::move(name),
            std::move(metadata),
            static_cast<std::size_t>(n_children)
        );

        const auto private_data = static_cast<arrow_schema_private_data*>(schema.private_data);
        schema.format = private_data->format_ptr();
        schema.name = private_data->name_ptr();
        schema.metadata = private_data->metadata_ptr();
        schema.children = children;
        schema.dictionary = dictionary;
        schema.release = release_arrow_schema;
    }

    template <class F, class N, class M>
        requires std::constructible_from<arrow_schema_private_data::FormatType, F>
                 && std::constructible_from<arrow_schema_private_data::NameType, N>
                 && std::constructible_from<arrow_schema_private_data::MetadataType, M>
    [[nodiscard]] ArrowSchema make_arrow_schema(
        F format,
        N name,
        M metadata,
        std::optional<ArrowFlag> flags,
        int64_t n_children,
        ArrowSchema** children,
        ArrowSchema* dictionary
    )
    {
        SPARROW_ASSERT_TRUE(n_children >= 0);
        SPARROW_ASSERT_TRUE(n_children > 0 ? children != nullptr : children == nullptr);
        SPARROW_ASSERT_FALSE(format.empty());
        if (children)
        {
            for (int64_t i = 0; i < n_children; ++i)
            {
                SPARROW_ASSERT_FALSE(children[i] == nullptr);
            }
        }

        ArrowSchema schema{};
        fill_arrow_schema(schema, format, name, metadata, flags, n_children, children, dictionary);
        return schema;
    };

    inline ArrowSchema make_empty_arrow_schema()
    {
        return make_arrow_schema(std::string_view("n"), "", "", std::nullopt, 0, nullptr, nullptr);
    }

    /**
     * Swaps the contents of the two ArrowSchema objects.
     */
    SPARROW_API void swap(ArrowSchema& lhs, ArrowSchema& rhs);

    /**
     * Fills the target `ArrowSchema` with a deep copy of the data from the source `ArrowSchema`.
     */
    SPARROW_API void copy_schema(const ArrowSchema& source, ArrowSchema& target);

    /**
     * Deep copy an `ArrowSchema`.
     *
     * @param source The source `ArrowSchema`.
     * @return The deep copy of the `ArrowSchema`.
     */
    [[nodiscard]]
    inline ArrowSchema copy_schema(const ArrowSchema& source)
    {
        ArrowSchema target{};
        copy_schema(source, target);
        return target;
    }

    /**
     * Moves the content of source into a stack-allocated array, and
     * reset the source to an empty ArrowSchema.
     */
    inline ArrowSchema move_schema(ArrowSchema&& source)
    {
        ArrowSchema target = make_empty_arrow_schema();
        swap(source, target);
        return target;
    }

    /**
     * Moves the content of source into a stack-allocated array, and
     * reset the source to an empty ArrowSchema.
     */
    inline ArrowSchema move_schema(ArrowSchema& source)
    {
        return move_schema(std::move(source));
    }
}

#if defined(__cpp_lib_format)

template <>
struct std::formatter<ArrowSchema>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const ArrowSchema& obj, std::format_context& ctx) const
    {
        std::string children_str = std::format("{}", static_cast<void*>(obj.children));
        for (int i = 0; i < obj.n_children; ++i)
        {
            children_str += std::format("\n-{}", static_cast<void*>(obj.children[i]));
        }

        const std::string format = obj.format ? obj.format : "nullptr";
        const std::string name = obj.name ? obj.name : "nullptr";
        const std::string metadata = obj.metadata ? obj.metadata : "nullptr";

        return std::format_to(
            ctx.out(),
            "ArrowSchema - ptr address: {}\n- format: {}\n- name: {}\n- metadata: {}\n- flags: {}\n- n_children: {}\n- children: {}\n- dictionary: {}\n- release: {}\n- private_data: {}\n",
            static_cast<const void*>(&obj),
            format,
            name,
            metadata,
            obj.flags,
            obj.n_children,
            children_str,
            static_cast<const void*>(obj.dictionary),
            static_cast<const void*>(std::addressof(obj.release)),
            obj.private_data
        );
    }
};

inline std::ostream& operator<<(std::ostream& os, const ArrowSchema& value)
{
    os << std::format("{}", value);
    return os;
}

#endif
