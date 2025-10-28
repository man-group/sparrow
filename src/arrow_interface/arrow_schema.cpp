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

#include "sparrow/arrow_interface/arrow_schema.hpp"

#include "sparrow/arrow_interface/arrow_array_schema_common_release.hpp"
#include "sparrow/utils/repeat_container.hpp"

namespace sparrow
{
    void release_arrow_schema(ArrowSchema* schema)
    {
        SPARROW_ASSERT_FALSE(schema == nullptr);
        SPARROW_ASSERT_TRUE(schema->release == std::addressof(release_arrow_schema));

        release_common_arrow(*schema);
        if (schema->private_data != nullptr)
        {
            const auto private_data = static_cast<arrow_schema_private_data*>(schema->private_data);
            delete private_data;
            schema->private_data = nullptr;
        }
        *schema = {};
    }

    void empty_release_arrow_schema(ArrowSchema* schema)
    {
        SPARROW_ASSERT_FALSE(schema == nullptr);
        SPARROW_ASSERT_TRUE(schema->release == std::addressof(empty_release_arrow_schema));
    }

    void swap(ArrowSchema& lhs, ArrowSchema& rhs) noexcept
    {
        std::swap(lhs.format, rhs.format);
        std::swap(lhs.name, rhs.name);
        std::swap(lhs.metadata, rhs.metadata);
        std::swap(lhs.flags, rhs.flags);
        std::swap(lhs.n_children, rhs.n_children);
        std::swap(lhs.children, rhs.children);
        std::swap(lhs.dictionary, rhs.dictionary);
        std::swap(lhs.release, rhs.release);
        std::swap(lhs.private_data, rhs.private_data);
    }

    void copy_schema(const ArrowSchema& source, ArrowSchema& target)
    {
   
        SPARROW_ASSERT_TRUE(&source != &target);

        target.flags = source.flags;
        target.n_children = source.n_children;
        if (source.n_children > 0)
        {
            target.children = new ArrowSchema*[static_cast<std::size_t>(source.n_children)];
            for (int64_t i = 0; i < source.n_children; ++i)
            {
                SPARROW_ASSERT_TRUE(source.children[i] != nullptr);
                target.children[i] = new ArrowSchema{};
                copy_schema(*source.children[i], *target.children[i]);
            }
        }

        if (source.dictionary != nullptr)
        {
            target.dictionary = new ArrowSchema{};
            copy_schema(*source.dictionary, *target.dictionary);
        }

        std::optional<std::string> metadata_str;
        if (source.metadata != nullptr)
        {
            key_value_view kv(source.metadata);
            metadata_str = std::make_optional(
                get_metadata_from_key_values(kv)
            );
        }

        target.private_data = new arrow_schema_private_data(
            std::string(source.format),
            std::string(source.name),
            std::move(metadata_str),
            repeat_view<bool>{true, static_cast<std::size_t>(target.n_children)},
            true
        );
        auto* private_data = static_cast<arrow_schema_private_data*>(target.private_data);
        target.format = private_data->format_ptr();
        target.name = private_data->name_ptr();
        target.metadata = private_data->metadata_ptr();
        target.release = release_arrow_schema;
    }

}
