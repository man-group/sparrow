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
#include "sparrow/utils/metadata.hpp"
#include "sparrow/utils/repeat_container.hpp"

#include "arrow_array_schema_creation.hpp"
#include "doctest/doctest.h"
#include "external_array_data_creation.hpp"
#include "metadata_sample.hpp"


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
            auto schema = sparrow::make_arrow_schema(
                format,
                name,
                sparrow::metadata_sample_opt,
                std::unordered_set<sparrow::ArrowFlag>{sparrow::ArrowFlag::DICTIONARY_ORDERED},
                children,
                sparrow::repeat_view<bool>(true, 2),
                dictionnary,
                true
            );

            const auto schema_format = std::string_view(schema.format);
            const bool format_eq = schema_format == format;
            CHECK(format_eq);
            const auto schema_name = std::string_view(schema.name);
            const bool name_eq = schema_name == name;
            CHECK(name_eq);
            sparrow::test_metadata(sparrow::metadata_sample, schema.metadata);
            CHECK_EQ(schema.flags, 1);
            CHECK_EQ(schema.n_children, 2);
            REQUIRE_NE(schema.children, nullptr);
            CHECK_EQ(schema.children[0], children_1_ptr);
            CHECK_EQ(schema.children[1], children_2_ptr);
            CHECK_EQ(schema.dictionary, dictionnary);
            const bool is_release_arrow_schema = schema.release == &sparrow::release_arrow_schema;
            CHECK(is_release_arrow_schema);
            CHECK_NE(schema.private_data, nullptr);
            schema.release(&schema);
        }

        SUBCASE("make_schema_constructor no children, no dictionary, no name and metadata")
        {
            auto schema = sparrow::make_arrow_schema(
                "format"s,
                std::nullopt,
                std::optional<std::vector<sparrow::metadata_pair>>{},
                std::unordered_set<sparrow::ArrowFlag>{sparrow::ArrowFlag::DICTIONARY_ORDERED},
                nullptr,
                sparrow::repeat_view<bool>(true, 0),
                nullptr,
                true
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
            schema.release(&schema);
        }

        SUBCASE("ArrowSchema release")
        {
            ArrowSchema** children = new ArrowSchema*[2];
            children[0] = new ArrowSchema();
            children[1] = new ArrowSchema();

            auto schema = sparrow::make_arrow_schema(
                "format"s,
                "name"s,
                sparrow::metadata_sample_opt,
                std::unordered_set<sparrow::ArrowFlag>{sparrow::ArrowFlag::DICTIONARY_ORDERED},
                children,
                sparrow::repeat_view<bool>(true, 2),
                new ArrowSchema(),
                true
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
                std::optional<std::vector<sparrow::metadata_pair>>{},
                std::unordered_set<sparrow::ArrowFlag>{sparrow::ArrowFlag::DICTIONARY_ORDERED},
                nullptr,
                sparrow::repeat_view<bool>(true, 0),
                nullptr,
                true
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
                sparrow::metadata_sample_opt,
                std::unordered_set<sparrow::ArrowFlag>{sparrow::ArrowFlag::MAP_KEYS_SORTED},
                nullptr,
                sparrow::repeat_view<bool>(true, 0),
                nullptr,
                true
            );
            children[1] = new ArrowSchema();
            *children[1] = sparrow::make_arrow_schema(
                "format"s,
                "child2"s,
                sparrow::metadata_sample_opt,
                std::unordered_set<sparrow::ArrowFlag>{sparrow::ArrowFlag::NULLABLE},
                nullptr,
                sparrow::repeat_view<bool>(true, 0),
                nullptr,
                true
            );

            auto dictionary = new ArrowSchema();
            *dictionary = sparrow::make_arrow_schema(
                "format"s,
                "dictionary"s,
                sparrow::metadata_sample_opt,
                std::unordered_set<sparrow::ArrowFlag>{sparrow::ArrowFlag::MAP_KEYS_SORTED},
                nullptr,
                sparrow::repeat_view<bool>(true, 0),
                nullptr,
                true
            );
            auto schema = sparrow::make_arrow_schema(
                "format"s,
                "name"s,
                sparrow::metadata_sample_opt,
                std::unordered_set<sparrow::ArrowFlag>{sparrow::ArrowFlag::DICTIONARY_ORDERED},
                children,
                sparrow::repeat_view<bool>(true, 2),
                dictionary,
                true
            );

            auto schema_copy = sparrow::copy_schema(schema);

            compare_arrow_schema(schema, schema_copy);

            schema_copy.release(&schema_copy);
            schema.release(&schema);
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

            schema0.release(&schema0);
            schema1.release(&schema1);
            schema0_bkup.release(&schema0_bkup);
            schema1_bkup.release(&schema1_bkup);
        }

        SUBCASE("move_schema")
        {
            auto src_schema = test::make_arrow_schema(true);
            auto control = sparrow::copy_schema(src_schema);

            auto dst_schema = sparrow::move_schema(std::move(src_schema));
            // check_empty(src_schema);
            compare_arrow_schema(dst_schema, control);

            auto dst2_schema = sparrow::move_schema(dst_schema);
            // check_empty(dst_schema);
            compare_arrow_schema(dst2_schema, control);
            dst2_schema.release(&dst2_schema);
            control.release(&control);
        }

        SUBCASE("check_compatible_schema")
        {
            SUBCASE("same object")
            {
                // same object => compatible
                auto s = test::make_arrow_schema(true);
                CHECK(sparrow::check_compatible_schema(s, s));
                s.release(&s);
            }

            SUBCASE("deep copy")
            {
                // deep copy => compatible
                auto s = test::make_arrow_schema(true);
                auto s_copy = sparrow::copy_schema(s);
                CHECK(sparrow::check_compatible_schema(s, s_copy));
                s_copy.release(&s_copy);
                s.release(&s);
            }

            SUBCASE("different schema")
            {
                // different schema (format/structure) => incompatible
                auto s = test::make_arrow_schema(true);
                auto t = test::make_arrow_schema(false);
                CHECK_FALSE(sparrow::check_compatible_schema(s, t));
                t.release(&t);
                s.release(&s);
            }

            SUBCASE("name presence mismatch")
            {
                // name presence mismatch
                auto a = sparrow::make_arrow_schema(
                    "fmt"s,
                    std::nullopt,
                    std::optional<std::vector<sparrow::metadata_pair>>{},
                    std::unordered_set<sparrow::ArrowFlag>{},
                    nullptr,
                    sparrow::repeat_view<bool>(true, 0),
                    nullptr,
                    true
                );
                auto b = sparrow::make_arrow_schema(
                    "fmt"s,
                    "name"s,
                    std::optional<std::vector<sparrow::metadata_pair>>{},
                    std::unordered_set<sparrow::ArrowFlag>{},
                    nullptr,
                    sparrow::repeat_view<bool>(true, 0),
                    nullptr,
                    true
                );
                CHECK_FALSE(sparrow::check_compatible_schema(a, b));
                a.release(&a);
                b.release(&b);
            }

            SUBCASE("metadata mismatch")
            {
                // metadata mismatch
                auto m1 = sparrow::make_arrow_schema(
                    "fmt"s,
                    "n"s,
                    sparrow::metadata_sample_opt,
                    std::unordered_set<sparrow::ArrowFlag>{},
                    nullptr,
                    sparrow::repeat_view<bool>(true, 0),
                    nullptr,
                    true
                );
                auto m2 = sparrow::make_arrow_schema(
                    "fmt"s,
                    "n"s,
                    std::optional<std::vector<sparrow::metadata_pair>>{},
                    std::unordered_set<sparrow::ArrowFlag>{},
                    nullptr,
                    sparrow::repeat_view<bool>(true, 0),
                    nullptr,
                    true
                );
                CHECK_FALSE(sparrow::check_compatible_schema(m1, m2));
                m1.release(&m1);
                m2.release(&m2);
            }

            SUBCASE("children mismatch")
            {
                // children mismatch (one schema has one child, the other has none)
                auto child_arr = new ArrowSchema*[1];
                child_arr[0] = new ArrowSchema();
                *child_arr[0] = sparrow::make_arrow_schema(
                    "cfmt"s,
                    "c1"s,
                    std::optional<std::vector<sparrow::metadata_pair>>{},
                    std::unordered_set<sparrow::ArrowFlag>{},
                    nullptr,
                    sparrow::repeat_view<bool>(true, 0),
                    nullptr,
                    true
                );
                auto c_with = sparrow::make_arrow_schema(
                    "fmt"s,
                    "n"s,
                    std::optional<std::vector<sparrow::metadata_pair>>{},
                    std::unordered_set<sparrow::ArrowFlag>{},
                    child_arr,
                    sparrow::repeat_view<bool>(true, 1),
                    nullptr,
                    true
                );
                auto c_without = sparrow::make_arrow_schema(
                    "fmt"s,
                    "n"s,
                    std::optional<std::vector<sparrow::metadata_pair>>{},
                    std::unordered_set<sparrow::ArrowFlag>{},
                    nullptr,
                    sparrow::repeat_view<bool>(true, 0),
                    nullptr,
                    true
                );
                CHECK_FALSE(sparrow::check_compatible_schema(c_with, c_without));
                c_with.release(&c_with);
                c_without.release(&c_without);
                // note: child_arr and its child are allocated in test style used elsewhere (no explicit
                // delete here)
            }

            SUBCASE("dictionary mismatch")
            {
                // dictionary mismatch
                auto dict = new ArrowSchema();
                *dict = sparrow::make_arrow_schema(
                    "dfmt"s,
                    "d"s,
                    std::optional<std::vector<sparrow::metadata_pair>>{},
                    std::unordered_set<sparrow::ArrowFlag>{},
                    nullptr,
                    sparrow::repeat_view<bool>(true, 0),
                    nullptr,
                    true
                );
                auto with_dict = sparrow::make_arrow_schema(
                    "fmt"s,
                    "n"s,
                    std::optional<std::vector<sparrow::metadata_pair>>{},
                    std::unordered_set<sparrow::ArrowFlag>{},
                    nullptr,
                    sparrow::repeat_view<bool>(true, 0),
                    dict,
                    true
                );
                auto without_dict = sparrow::make_arrow_schema(
                    "fmt"s,
                    "n"s,
                    std::optional<std::vector<sparrow::metadata_pair>>{},
                    std::unordered_set<sparrow::ArrowFlag>{},
                    nullptr,
                    sparrow::repeat_view<bool>(true, 0),
                    nullptr,
                    true
                );
                CHECK_FALSE(sparrow::check_compatible_schema(with_dict, without_dict));
                with_dict.release(&with_dict);
                without_dict.release(&without_dict);
            }
        }

#if defined(__cpp_lib_format)
        SUBCASE("formatting")
        {
            auto schema = sparrow::make_arrow_schema(
                "format"s,
                std::nullopt,
                std::optional<std::vector<sparrow::metadata_pair>>{},
                std::unordered_set<sparrow::ArrowFlag>{sparrow::ArrowFlag::DICTIONARY_ORDERED},
                nullptr,
                sparrow::repeat_view<bool>(true, 0),
                nullptr,
                true
            );
            [[maybe_unused]] const auto format = std::format("{}", schema);
            // We don't check the result has it show the address of the object, which is not the same at each
            // run of the test
            schema.release(&schema);
        }
#endif
    }
}
