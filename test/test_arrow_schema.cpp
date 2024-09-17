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

#include <cstddef>
#include <memory>
#include <optional>
#include <ostream>  // Needed by doctest
#include <string_view>

#include "sparrow/arrow_interface/arrow_schema.hpp"

#include "doctest/doctest.h"


using namespace std::string_literals;

TEST_SUITE("C Data Interface")
{
    TEST_CASE("ArrowSchema")
    {
        SUBCASE("arrow_schema_shared_ptr")
        {
            SUBCASE("constructors")
            {
                SUBCASE("default")
                {
                    sparrow::arrow_schema_shared_ptr schema;
                    CHECK_EQ(schema.get(), nullptr);
                    const auto deleter = schema.get_deleter();
                    const auto is_arrow_schema_custom_deleter = *deleter
                                                                == &sparrow::arrow_schema_custom_deleter;
                    CHECK(is_arrow_schema_custom_deleter);
                }

                SUBCASE("from arrow_schema_unique_ptr")
                {
                    auto schema = sparrow::default_arrow_schema_unique_ptr();
                    schema->n_children = 99;
                    schema->flags = 1;
                    sparrow::arrow_schema_shared_ptr schema_shared(std::move(schema));
                    CHECK_NE(schema_shared.get(), nullptr);
                    CHECK_EQ(schema_shared->n_children, 99);
                    CHECK_EQ(schema_shared->flags, 1);
                    const auto deleter = schema_shared.get_deleter();
                    const auto is_arrow_schema_custom_deleter = *deleter
                                                                == &sparrow::arrow_schema_custom_deleter;
                    CHECK(is_arrow_schema_custom_deleter);
                }

                SUBCASE("copy")
                {
                    auto schema = sparrow::default_arrow_schema_unique_ptr();
                    schema->n_children = 99;
                    schema->flags = 1;
                    sparrow::arrow_schema_shared_ptr schema_shared(std::move(schema));
                    sparrow::arrow_schema_shared_ptr schema_shared_2(schema_shared);
                    CHECK_NE(schema_shared_2.get(), nullptr);
                    CHECK_EQ(schema_shared_2->n_children, 99);
                    CHECK_EQ(schema_shared_2->flags, 1);
                    const auto deleter = schema_shared.get_deleter();
                    const bool is_arrow_schema_custom_deleter = *deleter
                                                                == &sparrow::arrow_schema_custom_deleter;
                    CHECK(is_arrow_schema_custom_deleter);
                }
            }

            SUBCASE("operator=")
            {
                SUBCASE("move")
                {
                    auto schema = sparrow::default_arrow_schema_unique_ptr();
                    schema->n_children = 99;
                    schema->flags = 1;
                    sparrow::arrow_schema_shared_ptr schema_shared(std::move(schema));
                    sparrow::arrow_schema_shared_ptr schema_shared_2;
                    schema_shared_2 = std::move(schema_shared);
                    CHECK_NE(schema_shared_2.get(), nullptr);
                    CHECK_EQ(schema_shared_2->n_children, 99);
                    CHECK_EQ(schema_shared_2->flags, 1);
                    const auto deleter = schema_shared_2.get_deleter();
                    const bool is_arrow_schema_custom_deleter = *deleter
                                                                == &sparrow::arrow_schema_custom_deleter;
                    CHECK(is_arrow_schema_custom_deleter);
                }

                SUBCASE("copy")
                {
                    auto schema = sparrow::default_arrow_schema_unique_ptr();
                    schema->n_children = 99;
                    schema->flags = 1;
                    sparrow::arrow_schema_shared_ptr schema_shared(std::move(schema));
                    sparrow::arrow_schema_shared_ptr schema_shared_2;
                    schema_shared_2 = schema_shared;
                    CHECK_NE(schema_shared_2.get(), nullptr);
                    CHECK_EQ(schema_shared_2->n_children, 99);
                    CHECK_EQ(schema_shared_2->flags, 1);
                    const auto deleter = schema_shared.get_deleter();
                    const bool is_arrow_schema_custom_deleter = *deleter
                                                                == &sparrow::arrow_schema_custom_deleter;
                    CHECK(is_arrow_schema_custom_deleter);
                }
            }
        }

        SUBCASE("make_schema_constructor")
        {
            ArrowSchema** children = new ArrowSchema*[2];
            children[0] = sparrow::default_arrow_schema_unique_ptr().release();
            children[1] = sparrow::default_arrow_schema_unique_ptr().release();

            const auto children_1_ptr = children[0];
            const auto children_2_ptr = children[1];

            auto dictionnary = sparrow::default_arrow_schema_unique_ptr();
            dictionnary->name = "dictionary";
            sparrow::arrow_schema_unique_ptr dictionary(std::move(dictionnary));
            const auto dictionary_ptr = dictionary.get();

            const std::string format = "format";
            const std::string name = "name";
            const std::string metadata = "0000";
            const auto schema = sparrow::make_arrow_schema_unique_ptr(
                format,
                name,
                metadata,
                sparrow::ArrowFlag::DICTIONARY_ORDERED,
                2,
                children,
                dictionary_ptr
            );

            const auto schema_format = std::string_view(schema->format);
            const bool format_eq = schema_format == format;
            CHECK(format_eq);
            const auto schema_name = std::string_view(schema->name);
            const bool name_eq = schema_name == name;
            CHECK(name_eq);
            CHECK_EQ(schema->metadata[0], metadata[0]);
            CHECK_EQ(schema->metadata[1], metadata[1]);
            CHECK_EQ(schema->metadata[2], metadata[2]);
            CHECK_EQ(schema->metadata[3], metadata[3]);
            CHECK_EQ(schema->flags, 1);
            CHECK_EQ(schema->n_children, 2);
            REQUIRE_NE(schema->children, nullptr);
            CHECK_EQ(schema->children[0], children_1_ptr);
            CHECK_EQ(schema->children[1], children_2_ptr);
            CHECK_EQ(schema->dictionary, dictionary_ptr);
            const bool is_release_arrow_schema = schema->release == &sparrow::release_arrow_schema;
            CHECK(is_release_arrow_schema);
            CHECK_NE(schema->private_data, nullptr);
        }

        SUBCASE("make_schema_constructor no children, no dictionary, no name and metadata")
        {
            const auto schema = sparrow::make_arrow_schema_unique_ptr(
                "format"s,
                std::nullopt,
                std::nullopt,
                sparrow::ArrowFlag::DICTIONARY_ORDERED,
                0,
                nullptr,
                nullptr
            );

            const auto schema_format = std::string_view(schema->format);
            const bool format_eq = schema_format == "format";
            CHECK(format_eq);
            CHECK_EQ(schema->name, nullptr);
            CHECK_EQ(schema->metadata, nullptr);
            CHECK_EQ(schema->flags, 1);
            CHECK_EQ(schema->n_children, 0);
            CHECK_EQ(schema->children, nullptr);
            CHECK_EQ(schema->dictionary, nullptr);
            const bool is_release_arrow_schema = schema->release == &sparrow::release_arrow_schema;
            CHECK(is_release_arrow_schema);
            CHECK_NE(schema->private_data, nullptr);
        }

        SUBCASE("ArrowSchema release")
        {
            ArrowSchema** children = new ArrowSchema*[2];
            children[0] = sparrow::default_arrow_schema_unique_ptr().release();
            children[1] = sparrow::default_arrow_schema_unique_ptr().release();

            std::string metadata = "0000";

            auto schema = sparrow::make_arrow_schema_unique_ptr(
                "format"s,
                "name"s,
                metadata,
                sparrow::ArrowFlag::DICTIONARY_ORDERED,
                2,
                children,
                sparrow::default_arrow_schema_unique_ptr().release()
            );

            schema->release(schema.get());

            CHECK_EQ(schema->format, nullptr);
            CHECK_EQ(schema->name, nullptr);
            CHECK_EQ(schema->metadata, nullptr);
            CHECK_EQ(schema->children, nullptr);
            CHECK_EQ(schema->dictionary, nullptr);
            const bool is_nullptr = schema->release == nullptr;
            CHECK(is_nullptr);
            CHECK_EQ(schema->private_data, nullptr);
        }

        SUBCASE("ArrowSchema release no children, no dictionary, no name and metadata")
        {
            auto schema = sparrow::make_arrow_schema_unique_ptr(
                "format"s,
                std::nullopt,
                std::nullopt,
                sparrow::ArrowFlag::DICTIONARY_ORDERED,
                0,
                nullptr,
                nullptr
            );

            schema->release(schema.get());

            CHECK_EQ(schema->format, nullptr);
            CHECK_EQ(schema->name, nullptr);
            CHECK_EQ(schema->metadata, nullptr);
            CHECK_EQ(schema->children, nullptr);
            CHECK_EQ(schema->dictionary, nullptr);
            const bool is_nullptr = schema->release == nullptr;
            CHECK(is_nullptr);
            CHECK_EQ(schema->private_data, nullptr);
        }

        SUBCASE("deep_copy_schema")
        {
            auto children = new ArrowSchema*[2];
            children[0] = sparrow::make_arrow_schema_unique_ptr(
                              "format"s,
                              "child1"s,
                              "metadata"s,
                              sparrow::ArrowFlag::MAP_KEYS_SORTED,
                              0,
                              nullptr,
                              nullptr
            )
                              .release();
            children[1] = sparrow::make_arrow_schema_unique_ptr(
                              "format"s,
                              "child2"s,
                              "metadata"s,
                              sparrow::ArrowFlag::NULLABLE,
                              0,
                              nullptr,
                              nullptr
            )
                              .release();
            auto schema = sparrow::make_arrow_schema_unique_ptr(
                "format"s,
                "name"s,
                "metadata"s,
                sparrow::ArrowFlag::DICTIONARY_ORDERED,
                2,
                children,
                sparrow::make_arrow_schema_unique_ptr(
                    "format"s,
                    "dictionary"s,
                    "metadata"s,
                    sparrow::ArrowFlag::MAP_KEYS_SORTED,
                    0,
                    nullptr,
                    nullptr
                )
                    .release()
            );

            auto schema_copy = sparrow::deep_copy_schema(*schema);

            CHECK_NE(schema->format, schema_copy.format);
            CHECK_EQ(std::string_view(schema->format), std::string_view(schema_copy.format));
            CHECK_NE(schema->name, schema_copy.name);
            CHECK_EQ(std::string_view(schema->name), std::string_view(schema_copy.name));
            CHECK_NE(schema->metadata, schema_copy.metadata);
            CHECK_EQ(std::string_view(schema->metadata), std::string_view(schema_copy.metadata));
            CHECK_EQ(schema->flags, schema_copy.flags);
            CHECK_EQ(schema->n_children, schema_copy.n_children);
            CHECK_NE(schema->children, schema_copy.children);
            CHECK_NE(schema->dictionary, schema_copy.dictionary);
            CHECK_NE(schema->private_data, schema_copy.private_data);
        }
    }
}
