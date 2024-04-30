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
#include <memory>
#include <memory_resource>
#include <string_view>

#include "sparrow/c_interface.hpp"

#include "doctest/doctest.h"

TEST_SUITE("C Data Interface")
{
    TEST_CASE("ArrowArray")
    {
        SUBCASE("make_array_constructor")
        {
            std::vector<std::unique_ptr<ArrowArray>> children;
            children.emplace_back(new ArrowArray);
            children.emplace_back(new ArrowArray);
            const auto children_1_ptr = children[0].get();
            const auto children_2_ptr = children[1].get();

            auto dictionary = std::make_unique<ArrowArray>();
            const auto dictionary_ptr = dictionary.get();

            const auto array = sparrow::make_array_constructor<int, std::allocator>(1, 0, 0, 1, children, std::move(dictionary));
            CHECK_EQ(array->length, 1);
            CHECK_EQ(array->null_count, 0);
            CHECK_EQ(array->offset, 0);
            CHECK_EQ(array->n_buffers, 1);
            CHECK_EQ(array->n_children, 2);
            CHECK_NE(array->buffers, nullptr);
            REQUIRE_NE(array->children, nullptr);
            CHECK_EQ(array->children[0], children_1_ptr);
            CHECK_EQ(array->children[1], children_2_ptr);
            CHECK_EQ(array->dictionary, dictionary_ptr);
            CHECK_EQ(array->release, sparrow::delete_array<int, std::allocator>);
            CHECK_NE(array->private_data, nullptr);
        }

        SUBCASE("ArrowArray release")
        {
            std::vector<std::unique_ptr<ArrowArray>> children;
            children.emplace_back(new ArrowArray);
            children.emplace_back(new ArrowArray);
            auto dictionary = std::make_unique<ArrowArray>();
            auto array = sparrow::make_array_constructor<int, std::allocator>(1, 0, 0, 1, children, std::move(dictionary));

            array->release(array.get());

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
            std::vector<std::unique_ptr<ArrowSchema>> children;
            children.emplace_back(new ArrowSchema);
            children.emplace_back(new ArrowSchema);

            const auto children_1_ptr = children[0].get();
            const auto children_2_ptr = children[1].get();

            auto dictionary = std::make_unique<ArrowSchema>();
            const auto dictionary_ptr = dictionary.get();

            constexpr std::string_view format = "format";
            constexpr std::string_view name = "name";
            constexpr std::string_view metadata = "metadata";

            const auto schema = sparrow::make_arrow_schema<std::allocator>(
                format,
                name,
                metadata,
                sparrow::ArrowFlag::DICTIONARY_ORDERED,
                children,
                std::move(dictionary)
            );

            const auto schema_format = std::string_view(schema->format);
            bool lol = (schema_format == format);
            // CHECK_EQ(std::string_view(schema->name), name);
            // CHECK_EQ(std::string_view(schema->metadata), metadata);
            CHECK_EQ(schema->flags, 1);
            CHECK_EQ(schema->n_children, 2);
            REQUIRE_NE(schema->children, nullptr);
            CHECK_EQ(schema->children[0], children_1_ptr);
            CHECK_EQ(schema->children[1], children_2_ptr);
            CHECK_EQ(schema->dictionary, dictionary_ptr);
            CHECK_EQ(schema->release, sparrow::delete_schema<std::allocator>);
            CHECK_NE(schema->private_data, nullptr);
        }

        SUBCASE("ArrowSchema release")
        {
            std::vector<std::unique_ptr<ArrowSchema>> children;
            children.emplace_back(new ArrowSchema);
            children.emplace_back(new ArrowSchema);
            auto dictionary = std::make_unique<ArrowSchema>();

            auto schema = sparrow::make_arrow_schema<std::allocator>(
                "format",
                "name",
                "metadata",
                sparrow::ArrowFlag::DICTIONARY_ORDERED,
                children,
                std::move(dictionary)
            );

            schema->release(schema.get());

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