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

#include <cstring>
#include <memory_resource>

#include "sparrow/c_interface.hpp"

#include "doctest/doctest.h"

TEST_SUITE("C Data Interface")
{
    TEST_CASE("ArrowArray")
    {
        SUBCASE("make_array_constructor")
        {
            auto children = new ArrowArray*[2];
            children[0] = new ArrowArray;
            children[1] = new ArrowArray;
            auto dictionary = new ArrowArray;

            const auto array = sparrow::make_array_constructor<int, std::allocator>(1, 0, 0, 1, 2, children, dictionary);
            CHECK_EQ(array->length, 1);
            CHECK_EQ(array->null_count, 0);
            CHECK_EQ(array->offset, 0);
            CHECK_EQ(array->n_buffers, 1);
            CHECK_EQ(array->n_children, 2);
            CHECK_NE(array->buffers, nullptr);
            CHECK_EQ(array->children, children);
            CHECK_EQ(array->dictionary, dictionary);
            CHECK_EQ(array->release, sparrow::delete_array<int, std::allocator>);
            CHECK_NE(array->private_data, nullptr);
        }

        SUBCASE("ArrowArray release")
        {
            auto children = new ArrowArray*[2];
            children[0] = new ArrowArray;
            children[1] = new ArrowArray;
            auto dictionary = new ArrowArray;
            auto array = sparrow::make_array_constructor<int, std::allocator>(1, 0, 0, 1, 2, children, dictionary);

            array->release(array);

            CHECK_EQ(array->length, 0);
            CHECK_EQ(array->null_count, 0);
            CHECK_EQ(array->offset, 0);
            CHECK_EQ(array->n_buffers, 0);
            CHECK_EQ(array->n_children, 0);
            CHECK_EQ(array->buffers, nullptr);
            CHECK_EQ(array->children, nullptr);
            CHECK_EQ(array->release, nullptr);
            CHECK_EQ(array->private_data, nullptr);
        }
    }

    TEST_CASE("ArrowSchema")
    {
        SUBCASE("make_schema_constructor")
        {
            auto children = new ArrowSchema*[2];
            children[0] = new ArrowSchema;
            children[1] = new ArrowSchema;
            auto dictionary = new ArrowSchema;

            const auto schema = sparrow::make_arrow_schema<std::allocator>(
                "format",
                "name",
                "metadata",
                sparrow::ArrowFlag::DICTIONARY_ORDERED,
                2,
                children,
                dictionary
            );

            CHECK_EQ(strcmp(schema->format, "format"), 0);
            CHECK_EQ(strcmp(schema->name, "name"), 0);
            CHECK_EQ(strcmp(schema->metadata, "metadata"), 0);
            CHECK_EQ(schema->flags, 1);
            CHECK_EQ(schema->n_children, 2);
            CHECK_EQ(schema->children, children);
            CHECK_EQ(schema->dictionary, dictionary);
            CHECK_EQ(schema->release, sparrow::delete_schema<std::allocator>);
            CHECK_NE(schema->private_data, nullptr);
        }

        SUBCASE("ArrowSchema release")
        {
            auto children = new ArrowSchema*[2];
            children[0] = new ArrowSchema;
            children[1] = new ArrowSchema;
            auto dictionary = new ArrowSchema;

            auto schema = sparrow::make_arrow_schema<std::allocator>(
                "format",
                "name",
                "metadata",
                sparrow::ArrowFlag::DICTIONARY_ORDERED,
                2,
                children,
                dictionary
            );

            schema->release(schema);

            CHECK_EQ(schema->format, nullptr);
            CHECK_EQ(schema->name, nullptr);
            CHECK_EQ(schema->metadata, nullptr);
            CHECK_EQ(schema->children, nullptr);
            CHECK_EQ(schema->dictionary, nullptr);
            CHECK_EQ(schema->release, nullptr);
            CHECK_EQ(schema->private_data, nullptr);
        }
    }
}
