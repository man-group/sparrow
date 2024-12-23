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

#include "sparrow/arrow_interface/arrow_array_schema_utils.hpp"

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

        target.private_data = new arrow_schema_private_data(
            source.format,
            source.name,
            source.metadata,
            static_cast<std::size_t>(target.n_children)
        );
        auto* private_data = static_cast<arrow_schema_private_data*>(target.private_data);
        target.format = private_data->format_ptr();
        target.name = private_data->name_ptr();
        target.metadata = private_data->metadata_ptr();
        target.release = release_arrow_schema;
    }

}
