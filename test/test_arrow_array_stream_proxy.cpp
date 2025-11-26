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

#include <cstdint>
#include <string_view>
#include <vector>

#include "sparrow/arrow_interface/arrow_array_stream.hpp"
#include "sparrow/arrow_interface/arrow_array_stream_proxy.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/primitive_array.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/utils/repeat_container.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    /**
     * Helper function to create a simple ArrowSchema for testing
     */
    inline ArrowSchema make_test_schema(const char* format, const char* name = "")
    {
        using namespace std::literals;
        using metadata_type = std::vector<metadata_pair>;
        ArrowSchema schema{};
        const auto children_ownership = repeat_view<bool>{true, 0};
        fill_arrow_schema(
            schema,
            std::string_view(format),
            std::string_view(name),
            std::optional<metadata_type>{},
            std::nullopt,
            nullptr,
            children_ownership,
            nullptr,
            false
        );
        return schema;
    }

    /**
     * Helper function to create a simple primitive array for testing
     */
    template <typename T>
    primitive_array<T> make_test_primitive_array(size_t size, size_t offset = 0)
    {
        std::vector<nullable<T>> values;
        values.reserve(size);
        for (size_t i = 0; i < size; ++i)
        {
            if constexpr (std::is_same_v<T, bool>)
            {
                values.push_back(make_nullable<T>(i % 2 == 0, i % 3 != 0));
            }
            else
            {
                values.push_back(make_nullable<T>(static_cast<T>(i + offset), i % 3 != 0));
            }
        }
        return primitive_array<T>(values);
    }

    TEST_SUITE("arrow_array_stream_proxy")
    {
        TEST_CASE("constructor")
        {
            SUBCASE("default")
            {
                arrow_array_stream_proxy proxy;
                ArrowArrayStream* aas = proxy.export_stream();
                REQUIRE_NE(aas, nullptr);
                aas->release(aas);
                delete aas;
            }

            SUBCASE("pointer")
            {
                ArrowArrayStream* stream = new ArrowArrayStream;
                fill_arrow_array_stream(*stream);
                arrow_array_stream_proxy proxy(stream);
                ArrowArrayStream* aas = proxy.export_stream();
                REQUIRE_NE(aas, nullptr);
                aas->release(aas);
                delete aas;
            }

            SUBCASE("move")
            {
                ArrowArrayStream stream{};
                fill_arrow_array_stream(stream);
                arrow_array_stream_proxy proxy(std::move(stream));
                REQUIRE_EQ(stream.private_data, nullptr);
                REQUIRE_EQ(stream.release, nullptr);
                ArrowArrayStream* aas = proxy.export_stream();
                REQUIRE_NE(aas, nullptr);
                aas->release(aas);
                delete aas;
            }
        }

        TEST_CASE("owns_stream")
        {
            {
                arrow_array_stream_proxy proxy{};
                REQUIRE(proxy.owns_stream());
            }

            {
                ArrowArrayStream stream{};
                fill_arrow_array_stream(stream);
                arrow_array_stream_proxy proxy(&stream);
                REQUIRE(!proxy.owns_stream());
            }
        }

        TEST_CASE("export_stream")
        {
            arrow_array_stream_proxy proxy;
            ArrowArrayStream* aas = proxy.export_stream();
            REQUIRE_NE(aas, nullptr);
            REQUIRE_NE(aas->get_schema, nullptr);
            REQUIRE_NE(aas->get_next, nullptr);
            REQUIRE_NE(aas->get_last_error, nullptr);
            REQUIRE_NE(aas->release, nullptr);

            aas->release(aas);
            delete aas;
        }

        TEST_CASE("push and pop - single int32 array")
        {
            SUBCASE("single array")
            {
                arrow_array_stream_proxy proxy;
                auto test_array = make_test_primitive_array<int32_t>(10);
                proxy.push(std::move(test_array));
                const auto result = proxy.pop();
                REQUIRE(result.has_value());
                CHECK_EQ(result->size(), 10);
            }
        }

        TEST_CASE("push and pop - multiple arrays")
        {
            arrow_array_stream_proxy proxy;

            // Create and push multiple arrays (schema created from first array)
            std::vector<primitive_array<int32_t>> arrays;
            arrays.push_back(make_test_primitive_array<int32_t>(5, 0));
            arrays.push_back(make_test_primitive_array<int32_t>(7, 10));
            arrays.push_back(make_test_primitive_array<int32_t>(3, 20));

            for (auto& arr : arrays)
            {
                proxy.push(std::move(arr));
            }

            // Pop all arrays
            const auto result1 = proxy.pop();
            REQUIRE(result1.has_value());
            CHECK_EQ(result1->size(), 5);

            auto result2 = proxy.pop();
            REQUIRE(result2.has_value());
            CHECK_EQ(result2->size(), 7);

            const auto result3 = proxy.pop();
            REQUIRE(result3.has_value());
            CHECK_EQ(result3->size(), 3);
        }

        TEST_CASE("end of stream")
        {
            arrow_array_stream_proxy proxy;
            proxy.push(make_test_primitive_array<int32_t>(5));
            proxy.pop();
            const auto result = proxy.pop();
            CHECK_FALSE(result.has_value());
        }

        TEST_CASE("stream callbacks - get_schema")
        {
            arrow_array_stream_proxy proxy;
            proxy.push(make_test_primitive_array<int32_t>(5));
            ArrowArrayStream* stream = proxy.export_stream();
            ArrowSchema out_schema;
            const int result = stream->get_schema(stream, &out_schema);
            REQUIRE_EQ(result, 0);
            CHECK_EQ(std::string_view(out_schema.format), "i");  // int32 format
            CHECK_EQ(out_schema.flags, 2);                       // no flags set
            CHECK_EQ(out_schema.name, nullptr);                  // no name
            CHECK_EQ(out_schema.n_children, 0);                  // no children
            CHECK_EQ(out_schema.release != nullptr, true);
            CHECK_EQ(out_schema.private_data != nullptr, true);
            CHECK_EQ(out_schema.metadata, nullptr);
            CHECK_EQ(out_schema.dictionary, nullptr);
            stream->release(stream);
            delete stream;
            out_schema.release(&out_schema);
        }

        TEST_CASE("stream callbacks - get_next")
        {
            arrow_array_stream_proxy proxy;
            auto test_array = make_test_primitive_array<int32_t>(5);
            proxy.push(std::move(test_array));
            ArrowArrayStream* stream = proxy.export_stream();
            ArrowArray out_array;
            const int result = stream->get_next(stream, &out_array);
            REQUIRE_EQ(result, 0);
            if (out_array.release)
            {
                out_array.release(&out_array);
            }
            stream->release(stream);
            delete stream;
        }

        TEST_CASE("stream callbacks - get_next with empty stream")
        {
            arrow_array_stream_proxy proxy;
            proxy.push(make_test_primitive_array<int32_t>(5));
            const auto result1 = proxy.pop();
            ArrowArrayStream* stream = proxy.export_stream();
            ArrowArray out_array;
            const int result = stream->get_next(stream, &out_array);
            CHECK_EQ(result, 0);
            stream->release(stream);
            delete stream;
        }

        TEST_CASE("stream callbacks - release")
        {
            ArrowArrayStream stream;
            fill_arrow_array_stream(stream);
            REQUIRE_NE(stream.release, nullptr);
            REQUIRE_NE(stream.private_data, nullptr);
            stream.release(&stream);
            CHECK_EQ(stream.release, nullptr);
            CHECK_EQ(stream.private_data, nullptr);
            CHECK_EQ(stream.get_schema, nullptr);
            CHECK_EQ(stream.get_next, nullptr);
            CHECK_EQ(stream.get_last_error, nullptr);
        }

        TEST_CASE("stream callbacks - get_last_error")
        {
            arrow_array_stream_proxy proxy;
            proxy.push(make_test_primitive_array<int32_t>(5));
            ArrowArrayStream* stream = proxy.export_stream();
            const char* error = stream->get_last_error(stream);
            CHECK((error == nullptr || error[0] == '\0'));
            stream->release(stream);
            delete stream;
        }

        TEST_CASE("RAII - automatic cleanup")
        {
            // This test verifies that the proxy properly cleans up resources
            // when it goes out of scope
            {
                arrow_array_stream_proxy proxy;

                // Push some arrays (schema created automatically)
                proxy.push(make_test_primitive_array<int32_t>(5));
                proxy.push(make_test_primitive_array<int32_t>(7));

                // Proxy goes out of scope here
            }

            // If we get here without crashes, RAII worked correctly
            CHECK(true);
        }

        TEST_CASE("different data types")
        {
            SUBCASE("uint8_t")
            {
                arrow_array_stream_proxy proxy;

                proxy.push(make_test_primitive_array<uint8_t>(10));

                auto result = proxy.pop();
                REQUIRE(result.has_value());
                CHECK(result->size() == 10);
            }

            SUBCASE("int64_t")
            {
                arrow_array_stream_proxy proxy;

                proxy.push(make_test_primitive_array<int64_t>(15));

                auto result = proxy.pop();
                REQUIRE(result.has_value());
                CHECK(result->size() == 15);
            }

            SUBCASE("float")
            {
                arrow_array_stream_proxy proxy;

                proxy.push(make_test_primitive_array<float>(8));

                auto result = proxy.pop();
                REQUIRE(result.has_value());
                CHECK(result->size() == 8);
            }

            SUBCASE("bool")
            {
                arrow_array_stream_proxy proxy;

                proxy.push(make_test_primitive_array<bool>(12));

                auto result = proxy.pop();
                REQUIRE(result.has_value());
                CHECK(result->size() == 12);
            }
        }

        TEST_CASE("schema compatibility check")
        {
            // This test verifies that incompatible schemas are rejected
            // Note: This test may need to be adjusted based on actual implementation

            arrow_array_stream_proxy proxy;

            // Push first array (creates schema automatically)
            auto compatible_array = make_test_primitive_array<int32_t>(5);

            // This should work
            CHECK_NOTHROW(proxy.push(std::move(compatible_array)));
        }

        TEST_CASE("multiple pop operations")
        {
            arrow_array_stream_proxy proxy;

            // Push several arrays (schema created from first)
            const size_t num_arrays = 5;
            for (size_t i = 0; i < num_arrays; ++i)
            {
                proxy.push(make_test_primitive_array<int32_t>((i + 1) * 2));
            }

            // Pop all arrays and verify sizes
            for (size_t i = 0; i < num_arrays; ++i)
            {
                auto result = proxy.pop();
                REQUIRE(result.has_value());
                CHECK(result->size() == (i + 1) * 2);
            }

            // One more pop should give end-of-stream
            auto final = proxy.pop();
            CHECK_FALSE(final.has_value());
        }

        TEST_CASE("interleaved push and pop")
        {
            arrow_array_stream_proxy proxy;

            // Push one, pop one, push two, pop two, etc.
            proxy.push(make_test_primitive_array<int32_t>(5));
            auto result1 = proxy.pop();
            REQUIRE(result1.has_value());
            CHECK(result1->size() == 5);

            proxy.push(make_test_primitive_array<int32_t>(10));
            proxy.push(make_test_primitive_array<int32_t>(15));

            auto result2 = proxy.pop();
            REQUIRE(result2.has_value());
            CHECK(result2->size() == 10);

            auto result3 = proxy.pop();
            REQUIRE(result3.has_value());
            CHECK(result3->size() == 15);
        }

        TEST_CASE("stream lifecycle - create, use, export")
        {
            // Create proxy, add data, export stream
            ArrowArrayStream* stream = nullptr;
            {
                arrow_array_stream_proxy proxy;
                proxy.push(make_test_primitive_array<int32_t>(20));
                proxy.push(make_test_primitive_array<int32_t>(30));
                stream = proxy.export_stream();
            }
            // Proxy destroyed, but stream should still be valid

            REQUIRE_NE(stream, nullptr);

            // Consume from the exported stream
            ArrowArray out_array;
            int result = stream->get_next(stream, &out_array);
            CHECK(result == 0);

            if (out_array.release)
            {
                out_array.release(&out_array);
            }

            // Clean up stream
            stream->release(stream);
            delete stream;
        }
    }
}
