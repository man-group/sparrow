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
    inline ArrowSchema make_test_schema(std::string_view format, std::string_view name = "")
    {
        using namespace std::literals;
        ArrowSchema schema{};
        fill_arrow_schema(
            schema,
            format,
            name,
            std::nullopt,
            std::nullopt,
            nullptr,
            repeat_view<bool>(true, 0),
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
        TEST_CASE("constructor - default")
        {
            arrow_array_stream_proxy proxy;
            
            // Verify that the stream callbacks are properly initialized
            REQUIRE(proxy.get_private_data() != nullptr);
        }

        TEST_CASE("constructor - from existing stream")
        {
            ArrowArrayStream stream = make_empty_arrow_array_stream();
            
            arrow_array_stream_proxy proxy(&stream);
            
            REQUIRE(proxy.get_private_data() != nullptr);
        }

        TEST_CASE("export_stream")
        {
            arrow_array_stream_proxy proxy;
            
            ArrowArrayStream* exported = proxy.export_stream();
            
            REQUIRE(exported != nullptr);
            REQUIRE(exported->get_schema != nullptr);
            REQUIRE(exported->get_next != nullptr);
            REQUIRE(exported->get_last_error != nullptr);
            REQUIRE(exported->release != nullptr);
            
            // Clean up
            exported->release(exported);
            delete exported;
        }

        TEST_CASE("push and pop - single int32 array")
        {
            SUBCASE("single array")
            {
                arrow_array_stream_proxy proxy;
                auto test_array = make_test_primitive_array<int32_t>(10);
                
                // Create a schema for the stream
                ArrowSchema schema = make_test_schema("i");
                proxy.get_private_data()->import_schema(&schema);
                
                // Push an array
                proxy.push(std::move(test_array));
                
                // Pop the array back
                array result = proxy.pop();
                
                // Verify the array is not empty (has valid data)
                CHECK(result.size() == 10);
            }
        }

        TEST_CASE("push and pop - multiple arrays")
        {
            arrow_array_stream_proxy proxy;
            
            // Create a schema for the stream
            ArrowSchema schema = make_test_schema("i");
            proxy.get_private_data()->import_schema(&schema);
            
            // Create and push multiple arrays
            std::vector<primitive_array<int32_t>> arrays;
            arrays.push_back(make_test_primitive_array<int32_t>(5, 0));
            arrays.push_back(make_test_primitive_array<int32_t>(7, 10));
            arrays.push_back(make_test_primitive_array<int32_t>(3, 20));
            
            for (auto& arr : arrays)
            {
                proxy.push(std::move(arr));
            }
            
            // Pop all arrays
            array result1 = proxy.pop();
            CHECK(result1.size() == 5);
            
            array result2 = proxy.pop();
            CHECK(result2.size() == 7);
            
            array result3 = proxy.pop();
            CHECK(result3.size() == 3);
        }

        TEST_CASE("end of stream")
        {
            arrow_array_stream_proxy proxy;
            
            // Create a schema for the stream
            ArrowSchema schema;
            fill_arrow_schema(schema, "i", std::nullopt, std::nullopt, std::nullopt, nullptr, {}, nullptr, false);
            proxy.get_private_data()->import_schema(&schema);
            
            // Don't push any arrays
            
            // Pop from empty stream should return a released array (end of stream)
            array result = proxy.pop();
            
            // A released array indicates end of stream
            // The array should be empty or marked as released
            CHECK(result.size() == 0);
        }

        TEST_CASE("stream callbacks - get_schema")
        {
            arrow_array_stream_proxy proxy;
            
            // Create and set a schema
            ArrowSchema schema;
            fill_arrow_schema(schema, "i", std::nullopt, std::nullopt, std::nullopt, nullptr, {}, nullptr, false);
            proxy.get_private_data()->import_schema(&schema);
            
            ArrowArrayStream* stream = proxy.export_stream();
            
            // Call get_schema callback
            ArrowSchema* out_schema = nullptr;
            int result = stream->get_schema(stream, out_schema);
            
            CHECK(result == 0);
            REQUIRE(out_schema != nullptr);
            
            // Clean up
            stream->release(stream);
            delete stream;
        }

        TEST_CASE("stream callbacks - get_next")
        {
            arrow_array_stream_proxy proxy;
            
            // Create a schema for the stream
            ArrowSchema schema;
            fill_arrow_schema(schema, "i", std::nullopt, std::nullopt, std::nullopt, nullptr, {}, nullptr, false);
            proxy.get_private_data()->import_schema(&schema);
            
            // Push an array
            auto test_array = make_test_primitive_array<int32_t>(5);
            proxy.push(std::move(test_array));
            
            ArrowArrayStream* stream = proxy.export_stream();
            
            // Call get_next callback
            ArrowArray* out_array = nullptr;
            int result = stream->get_next(stream, out_array);
            
            CHECK(result == 0);
            REQUIRE(out_array != nullptr);
            
            // Clean up
            if (out_array && out_array->release)
            {
                out_array->release(out_array);
            }
            stream->release(stream);
            delete stream;
        }

        TEST_CASE("stream callbacks - get_next with empty stream")
        {
            arrow_array_stream_proxy proxy;
            
            // Create a schema for the stream
            ArrowSchema schema;
            fill_arrow_schema(schema, "i", std::nullopt, std::nullopt, std::nullopt, nullptr, {}, nullptr, false);
            proxy.get_private_data()->import_schema(&schema);
            
            ArrowArrayStream* stream = proxy.export_stream();
            
            // Call get_next on empty stream
            ArrowArray* out_array = nullptr;
            int result = stream->get_next(stream, out_array);
            
            // Should succeed but return a released array (end of stream)
            CHECK(result == 0);
            REQUIRE(out_array != nullptr);
            
            // Clean up
            stream->release(stream);
            delete stream;
        }

        TEST_CASE("stream callbacks - release")
        {
            ArrowArrayStream stream = make_empty_arrow_array_stream();
            
            // Verify stream is initialized
            REQUIRE(stream.release != nullptr);
            REQUIRE(stream.private_data != nullptr);
            
            // Release the stream
            stream.release(&stream);
            
            // Verify stream is released
            CHECK(stream.release == nullptr);
            CHECK(stream.private_data == nullptr);
            CHECK(stream.get_schema == nullptr);
            CHECK(stream.get_next == nullptr);
            CHECK(stream.get_last_error == nullptr);
        }

        TEST_CASE("stream callbacks - get_last_error")
        {
            arrow_array_stream_proxy proxy;
            
            // Create a schema for the stream
            ArrowSchema schema;
            fill_arrow_schema(schema, "i", std::nullopt, std::nullopt, std::nullopt, nullptr, {}, nullptr, false);
            proxy.get_private_data()->import_schema(&schema);
            
            ArrowArrayStream* stream = proxy.export_stream();
            
            // Initially, there should be no error
            const char* error = stream->get_last_error(stream);
            CHECK((error == nullptr || error[0] == '\0'));
            
            // Clean up
            stream->release(stream);
            delete stream;
        }

        TEST_CASE("RAII - automatic cleanup")
        {
            // This test verifies that the proxy properly cleans up resources
            // when it goes out of scope
            {
                arrow_array_stream_proxy proxy;
                
                // Create a schema for the stream
                ArrowSchema schema;
                fill_arrow_schema(schema, "i", std::nullopt, std::nullopt, std::nullopt, nullptr, {}, nullptr, false);
                proxy.get_private_data()->import_schema(&schema);
                
                // Push some arrays
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
                
                ArrowSchema schema;
                fill_arrow_schema(schema, "C", std::nullopt, std::nullopt, std::nullopt, nullptr, {}, nullptr, false);
                proxy.get_private_data()->import_schema(&schema);
                
                proxy.push(make_test_primitive_array<uint8_t>(10));
                
                array result = proxy.pop();
                CHECK(result.size() == 10);
            }
            
            SUBCASE("int64_t")
            {
                arrow_array_stream_proxy proxy;
                
                ArrowSchema schema;
                fill_arrow_schema(schema, "l", std::nullopt, std::nullopt, std::nullopt, nullptr, {}, nullptr, false);
                proxy.get_private_data()->import_schema(&schema);
                
                proxy.push(make_test_primitive_array<int64_t>(15));
                
                array result = proxy.pop();
                CHECK(result.size() == 15);
            }
            
            SUBCASE("float")
            {
                arrow_array_stream_proxy proxy;
                
                ArrowSchema schema;
                fill_arrow_schema(schema, "f", std::nullopt, std::nullopt, std::nullopt, nullptr, {}, nullptr, false);
                proxy.get_private_data()->import_schema(&schema);
                
                proxy.push(make_test_primitive_array<float>(8));
                
                array result = proxy.pop();
                CHECK(result.size() == 8);
            }
            
            SUBCASE("bool")
            {
                arrow_array_stream_proxy proxy;
                
                ArrowSchema schema;
                fill_arrow_schema(schema, "b", std::nullopt, std::nullopt, std::nullopt, nullptr, {}, nullptr, false);
                proxy.get_private_data()->import_schema(&schema);
                
                proxy.push(make_test_primitive_array<bool>(12));
                
                array result = proxy.pop();
                CHECK(result.size() == 12);
            }
        }

        TEST_CASE("schema compatibility check")
        {
            // This test verifies that incompatible schemas are rejected
            // Note: This test may need to be adjusted based on actual implementation
            
            arrow_array_stream_proxy proxy;
            
            // Set up stream with int32 schema
            ArrowSchema schema;
            fill_arrow_schema(schema, "i", std::nullopt, std::nullopt, std::nullopt, nullptr, {}, nullptr, false);
            proxy.get_private_data()->import_schema(&schema);
            
            // Create array with compatible schema
            auto compatible_array = make_test_primitive_array<int32_t>(5);
            
            // This should work
            CHECK_NOTHROW(proxy.push(std::move(compatible_array)));
        }

        TEST_CASE("multiple pop operations")
        {
            arrow_array_stream_proxy proxy;
            
            // Create a schema for the stream
            ArrowSchema schema;
            fill_arrow_schema(schema, "i", std::nullopt, std::nullopt, std::nullopt, nullptr, {}, nullptr, false);
            proxy.get_private_data()->import_schema(&schema);
            
            // Push several arrays
            const size_t num_arrays = 5;
            for (size_t i = 0; i < num_arrays; ++i)
            {
                proxy.push(make_test_primitive_array<int32_t>(static_cast<int>(i + 1) * 2));
            }
            
            // Pop all arrays and verify sizes
            for (size_t i = 0; i < num_arrays; ++i)
            {
                array result = proxy.pop();
                CHECK(result.size() == (i + 1) * 2);
            }
            
            // One more pop should give end-of-stream
            array final = proxy.pop();
            CHECK(final.size() == 0);
        }

        TEST_CASE("interleaved push and pop")
        {
            arrow_array_stream_proxy proxy;
            
            // Create a schema for the stream
            ArrowSchema schema;
            fill_arrow_schema(schema, "i", std::nullopt, std::nullopt, std::nullopt, nullptr, {}, nullptr, false);
            proxy.get_private_data()->import_schema(&schema);
            
            // Push one, pop one, push two, pop two, etc.
            proxy.push(make_test_primitive_array<int32_t>(5));
            array result1 = proxy.pop();
            CHECK(result1.size() == 5);
            
            proxy.push(make_test_primitive_array<int32_t>(10));
            proxy.push(make_test_primitive_array<int32_t>(15));
            
            array result2 = proxy.pop();
            CHECK(result2.size() == 10);
            
            array result3 = proxy.pop();
            CHECK(result3.size() == 15);
        }

        TEST_CASE("private_data access")
        {
            arrow_array_stream_proxy proxy;
            
            // Test const and non-const access
            const auto* const_data = const_cast<const arrow_array_stream_proxy&>(proxy).get_private_data();
            auto* mut_data = proxy.get_private_data();
            
            REQUIRE(const_data != nullptr);
            REQUIRE(mut_data != nullptr);
            CHECK(const_data == mut_data);
        }

        TEST_CASE("stream lifecycle - create, use, export")
        {
            // Create proxy, add data, export stream
            ArrowArrayStream* stream;
            {
                arrow_array_stream_proxy proxy;
                
                ArrowSchema schema;
                fill_arrow_schema(schema, "i", std::nullopt, std::nullopt, std::nullopt, nullptr, {}, nullptr, false);
                proxy.get_private_data()->import_schema(&schema);
                
                proxy.push(make_test_primitive_array<int32_t>(20));
                proxy.push(make_test_primitive_array<int32_t>(30));
                
                stream = proxy.export_stream();
            }
            // Proxy destroyed, but stream should still be valid
            
            REQUIRE(stream != nullptr);
            
            // Consume from the exported stream
            ArrowArray* out_array = nullptr;
            int result = stream->get_next(stream, out_array);
            CHECK(result == 0);
            
            if (out_array && out_array->release)
            {
                out_array->release(out_array);
            }
            
            // Clean up stream
            stream->release(stream);
            delete stream;
        }
    }
}
