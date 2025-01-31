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

#include <memory>
#include <optional>
#include <ostream>  // Needed by doctest
#include <string_view>

#include "sparrow/arrow_interface/arrow_schema.hpp"

#include "arrow_array_schema_creation.hpp"
#include "doctest/doctest.h"


using namespace std::string_literals;

void compare_arrow_schema(const ArrowSchema& schema, const ArrowSchema& schema_copy)
{
    CHECK_NE(&schema, &schema_copy);
    CHECK_EQ(std::string_view(schema.format), std::string_view(schema_copy.format));
    CHECK_EQ(std::string_view(schema.name), std::string_view(schema_copy.name));
    CHECK_EQ(std::string_view(schema.metadata), std::string_view(schema_copy.metadata));
    CHECK_EQ(schema.flags, schema_copy.flags);
    CHECK_EQ(schema.n_children, schema_copy.n_children);
    if (schema.n_children > 0)
    {
        REQUIRE_NE(schema.children, nullptr);
        REQUIRE_NE(schema_copy.children, nullptr);
        for (int64_t i = 0; i < schema.n_children; ++i)
        {
            CHECK_NE(schema.children[i], nullptr);
            compare_arrow_schema(*schema.children[i], *schema_copy.children[i]);
        }
    }
    else
    {
        CHECK_EQ(schema.children, nullptr);
        CHECK_EQ(schema_copy.children, nullptr);
    }

    if (schema.dictionary != nullptr)
    {
        REQUIRE_NE(schema_copy.dictionary, nullptr);
        compare_arrow_schema(*schema.dictionary, *schema_copy.dictionary);
    }
    else
    {
        CHECK_EQ(schema_copy.dictionary, nullptr);
    }
}

void check_empty(ArrowSchema& sch)
{
    CHECK_EQ(std::strcmp(sch.format, "n"), 0);
    CHECK_EQ(std::strcmp(sch.name, ""), 0);
    CHECK_EQ(std::strcmp(sch.metadata, ""), 0);
    CHECK_EQ(sch.flags, 0);
    CHECK_EQ(sch.n_children, 0);
    CHECK_EQ(sch.children, nullptr);
    CHECK_EQ(sch.dictionary, nullptr);
}

TEST_SUITE("C Data Interface")
{
    TEST_CASE("ArrowSchema")
    {
        SUBCASE("make_schema_constructor")
        {
            ArrowSchema** children = new ArrowSchema*[2];
            children[0] = new ArrowSchema();
            children[1] = new ArrowSchema();

            const auto children_1_ptr = children[0];
            const auto children_2_ptr = children[1];

            auto dictionnary = new ArrowSchema();
            dictionnary->name = "dictionary";
            const std::string format = "format";
            const std::string name = "name";
            const std::string metadata = "0000";
            const auto schema = sparrow::make_arrow_schema(
                format,
                name,
                metadata,
                sparrow::ArrowFlag::DICTIONARY_ORDERED,
                2,
                children,
                dictionnary
            );

            const auto schema_format = std::string_view(schema.format);
            const bool format_eq = schema_format == format;
            CHECK(format_eq);
            const auto schema_name = std::string_view(schema.name);
            const bool name_eq = schema_name == name;
            CHECK(name_eq);
            CHECK_EQ(schema.metadata[0], metadata[0]);
            CHECK_EQ(schema.metadata[1], metadata[1]);
            CHECK_EQ(schema.metadata[2], metadata[2]);
            CHECK_EQ(schema.metadata[3], metadata[3]);
            CHECK_EQ(schema.flags, 1);
            CHECK_EQ(schema.n_children, 2);
            REQUIRE_NE(schema.children, nullptr);
            CHECK_EQ(schema.children[0], children_1_ptr);
            CHECK_EQ(schema.children[1], children_2_ptr);
            CHECK_EQ(schema.dictionary, dictionnary);
            const bool is_release_arrow_schema = schema.release == &sparrow::release_arrow_schema;
            CHECK(is_release_arrow_schema);
            CHECK_NE(schema.private_data, nullptr);
        }

        SUBCASE("make_schema_constructor no children, no dictionary, no name and metadata")
        {
            const auto schema = sparrow::make_arrow_schema(
                "format"s,
                std::nullopt,
                std::nullopt,
                sparrow::ArrowFlag::DICTIONARY_ORDERED,
                0,
                nullptr,
                nullptr
            );

            const auto schema_format = std::string_view(schema.format);
            const bool format_eq = schema_format == "format";
            CHECK(format_eq);
            CHECK_EQ(schema.name, nullptr);
            CHECK_EQ(schema.metadata, nullptr);
            CHECK_EQ(schema.flags, 1);
            CHECK_EQ(schema.n_children, 0);
            CHECK_EQ(schema.children, nullptr);
            CHECK_EQ(schema.dictionary, nullptr);
            const bool is_release_arrow_schema = schema.release == &sparrow::release_arrow_schema;
            CHECK(is_release_arrow_schema);
            CHECK_NE(schema.private_data, nullptr);
        }

        SUBCASE("ArrowSchema release")
        {
            ArrowSchema** children = new ArrowSchema*[2];
            children[0] = new ArrowSchema();
            children[1] = new ArrowSchema();

            std::string metadata = "0000";

            auto schema = sparrow::make_arrow_schema(
                "format"s,
                "name"s,
                metadata,
                sparrow::ArrowFlag::DICTIONARY_ORDERED,
                2,
                children,
                new ArrowSchema()
            );

            schema.release(&schema);

            CHECK_EQ(schema.format, nullptr);
            CHECK_EQ(schema.name, nullptr);
            CHECK_EQ(schema.metadata, nullptr);
            CHECK_EQ(schema.children, nullptr);
            CHECK_EQ(schema.dictionary, nullptr);
            const bool is_nullptr = schema.release == nullptr;
            CHECK(is_nullptr);
            CHECK_EQ(schema.private_data, nullptr);
        }

        SUBCASE("ArrowSchema release no children, no dictionary, no name and metadata")
        {
            auto schema = sparrow::make_arrow_schema(
                "format"s,
                std::nullopt,
                std::nullopt,
                sparrow::ArrowFlag::DICTIONARY_ORDERED,
                0,
                nullptr,
                nullptr
            );

            schema.release(&schema);

            CHECK_EQ(schema.format, nullptr);
            CHECK_EQ(schema.name, nullptr);
            CHECK_EQ(schema.metadata, nullptr);
            CHECK_EQ(schema.children, nullptr);
            CHECK_EQ(schema.dictionary, nullptr);
            const bool is_nullptr = schema.release == nullptr;
            CHECK(is_nullptr);
            CHECK_EQ(schema.private_data, nullptr);
        }

        SUBCASE("deep_copy_schema")
        {
            auto children = new ArrowSchema*[2];
            children[0] = new ArrowSchema();
            *children[0] = sparrow::make_arrow_schema(
                "format"s,
                "child1"s,
                "metadata"s,
                sparrow::ArrowFlag::MAP_KEYS_SORTED,
                0,
                nullptr,
                nullptr
            );
            children[1] = new ArrowSchema();
            *children[1] = sparrow::make_arrow_schema(
                "format"s,
                "child2"s,
                "metadata"s,
                sparrow::ArrowFlag::NULLABLE,
                0,
                nullptr,
                nullptr
            );

            auto dictionary = new ArrowSchema();
            *dictionary = sparrow::make_arrow_schema(
                "format"s,
                "dictionary"s,
                "metadata"s,
                sparrow::ArrowFlag::MAP_KEYS_SORTED,
                0,
                nullptr,
                nullptr
            );
            const auto schema = sparrow::make_arrow_schema(
                "format"s,
                "name"s,
                "metadata"s,
                sparrow::ArrowFlag::DICTIONARY_ORDERED,
                2,
                children,
                dictionary
            );

            const auto schema_copy = sparrow::copy_schema(schema);

            compare_arrow_schema(schema, schema_copy);
        }
        
        SUBCASE("swap_schema")
        {
            auto schema0 = test::make_arrow_schema(true);
            auto schema0_bkup = sparrow::copy_schema(schema0);

            auto schema1 = test::make_arrow_schema(false);
            auto schema1_bkup = sparrow::copy_schema(schema1);
            
            sparrow::swap(schema0, schema1);
            compare_arrow_schema(schema0, schema1_bkup);
            compare_arrow_schema(schema1, schema0_bkup);
        }

        SUBCASE("move_schema")
        {
            auto src_schema = test::make_arrow_schema(true);
            auto control = sparrow::copy_schema(src_schema);
            
            auto dst_schema = sparrow::move_schema(std::move(src_schema));
            check_empty(src_schema);
            compare_arrow_schema(dst_schema, control);

            auto dst2_schema = sparrow::move_schema(dst_schema);
            check_empty(dst_schema);
            compare_arrow_schema(dst2_schema, control);
        }

#if defined(__cpp_lib_format)
        SUBCASE("formatting")
        {
            const auto schema = sparrow::make_arrow_schema(
                "format"s,
                std::nullopt,
                std::nullopt,
                sparrow::ArrowFlag::DICTIONARY_ORDERED,
                0,
                nullptr,
                nullptr
            );
            [[maybe_unused]] const auto format = std::format("{}", schema);
            // We don't check the result has it show the address of the object, which is not the same at each
            // run of the test
        }
#endif
    }
}
