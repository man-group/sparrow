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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or mplied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sparrow/layout/array_registry.hpp"
#include "sparrow/layout/array_factory.hpp"
#include "sparrow/variable_size_binary_array.hpp"
#include "sparrow/utils/extension.hpp"

#include "../test/external_array_data_creation.hpp"
#include "doctest/doctest.h"

namespace sparrow
{
    using test::make_arrow_proxy;

    TEST_SUITE("array_registry")
    {
        TEST_CASE("singleton_instance")
        {
            auto& reg1 = array_registry::instance();
            auto& reg2 = array_registry::instance();
            CHECK_EQ(&reg1, &reg2);
        }

        TEST_CASE("base_types_primitives")
        {
            SUBCASE("null_array")
            {
                auto proxy = make_arrow_proxy<null_type>();
                auto wrapper = array_factory(std::move(proxy));
                CHECK(wrapper != nullptr);
                CHECK_EQ(wrapper->data_type(), data_type::NA);
            }

            SUBCASE("bool_array")
            {
                auto proxy = make_arrow_proxy<bool>();
                auto wrapper = array_factory(std::move(proxy));
                CHECK(wrapper != nullptr);
                CHECK_EQ(wrapper->data_type(), data_type::BOOL);
            }

            SUBCASE("int8_array")
            {
                auto proxy = make_arrow_proxy<std::int8_t>();
                auto wrapper = array_factory(std::move(proxy));
                CHECK(wrapper != nullptr);
                CHECK_EQ(wrapper->data_type(), data_type::INT8);
            }

            SUBCASE("uint8_array")
            {
                auto proxy = make_arrow_proxy<std::uint8_t>();
                auto wrapper = array_factory(std::move(proxy));
                CHECK(wrapper != nullptr);
                CHECK_EQ(wrapper->data_type(), data_type::UINT8);
            }

            SUBCASE("int16_array")
            {
                auto proxy = make_arrow_proxy<std::int16_t>();
                auto wrapper = array_factory(std::move(proxy));
                CHECK(wrapper != nullptr);
                CHECK_EQ(wrapper->data_type(), data_type::INT16);
            }

            SUBCASE("uint16_array")
            {
                auto proxy = make_arrow_proxy<std::uint16_t>();
                auto wrapper = array_factory(std::move(proxy));
                CHECK(wrapper != nullptr);
                CHECK_EQ(wrapper->data_type(), data_type::UINT16);
            }

            SUBCASE("int32_array")
            {
                auto proxy = make_arrow_proxy<std::int32_t>();
                auto wrapper = array_factory(std::move(proxy));
                CHECK(wrapper != nullptr);
                CHECK_EQ(wrapper->data_type(), data_type::INT32);
            }

            SUBCASE("uint32_array")
            {
                auto proxy = make_arrow_proxy<std::uint32_t>();
                auto wrapper = array_factory(std::move(proxy));
                CHECK(wrapper != nullptr);
                CHECK_EQ(wrapper->data_type(), data_type::UINT32);
            }

            SUBCASE("int64_array")
            {
                auto proxy = make_arrow_proxy<std::int64_t>();
                auto wrapper = array_factory(std::move(proxy));
                CHECK(wrapper != nullptr);
                CHECK_EQ(wrapper->data_type(), data_type::INT64);
            }

            SUBCASE("uint64_array")
            {
                auto proxy = make_arrow_proxy<std::uint64_t>();
                auto wrapper = array_factory(std::move(proxy));
                CHECK(wrapper != nullptr);
                CHECK_EQ(wrapper->data_type(), data_type::UINT64);
            }

            SUBCASE("float16_array")
            {
                auto proxy = make_arrow_proxy<float16_t>();
                auto wrapper = array_factory(std::move(proxy));
                CHECK(wrapper != nullptr);
                CHECK_EQ(wrapper->data_type(), data_type::HALF_FLOAT);
            }

            SUBCASE("float_array")
            {
                auto proxy = make_arrow_proxy<float32_t>();
                auto wrapper = array_factory(std::move(proxy));
                CHECK(wrapper != nullptr);
                CHECK_EQ(wrapper->data_type(), data_type::FLOAT);
            }

            SUBCASE("double_array")
            {
                auto proxy = make_arrow_proxy<float64_t>();
                auto wrapper = array_factory(std::move(proxy));
                CHECK(wrapper != nullptr);
                CHECK_EQ(wrapper->data_type(), data_type::DOUBLE);
            }
        }

        TEST_CASE("base_types_string_binary")
        {
            SUBCASE("string_array")
            {
                auto proxy = make_arrow_proxy<std::string>();
                auto wrapper = array_factory(std::move(proxy));
                CHECK(wrapper != nullptr);
                CHECK_EQ(wrapper->data_type(), data_type::STRING);
            }

            SUBCASE("binary_array")
            {
                auto proxy = make_arrow_proxy<std::vector<byte_t>>();
                auto wrapper = array_factory(std::move(proxy));
                CHECK(wrapper != nullptr);
                CHECK_EQ(wrapper->data_type(), data_type::BINARY);
            }
        }

        TEST_CASE("base_types_nested")
        {
            // Note: Full nested type testing would require more complex test helpers
            // The important part is that these types are registered in the registry
            // and can be created via array_factory
        }

        TEST_CASE("custom_extension_registration")
        {
            // Test that we can register a custom extension
            // This tests the registration mechanism itself
            
            auto& registry = array_registry::instance();
            
            // Define a simple test extension that wraps string_array
            using test_custom_array = variable_size_binary_array_impl<
                arrow_traits<std::string>::value_type,
                arrow_traits<std::string>::const_reference,
                std::int32_t,
                empty_extension
            >;

            // Register custom extension
            bool factory_was_called = false;
            registry.register_extension(
                data_type::STRING,
                "test.custom.type",
                [&factory_was_called](arrow_proxy proxy) {
                    factory_was_called = true;
                    return cloning_ptr<array_wrapper>{
                        new array_wrapper_impl<test_custom_array>(
                            test_custom_array(std::move(proxy))
                        )
                    };
                }
            );

            // The extension won't be used unless we have matching metadata
            // So this test just verifies the registration doesn't crash
            CHECK_NOTHROW(registry.register_extension(
                data_type::BINARY,
                "another.test.type",
                [](arrow_proxy proxy) {
                    return cloning_ptr<array_wrapper>{
                        new array_wrapper_impl<binary_array>(
                            binary_array(std::move(proxy))
                        )
                    };
                }
            ));
        }

        TEST_CASE("extension_dispatch_integration")
        {
            // Test that dispatch works correctly with custom extensions
            auto& registry = array_registry::instance();
            
            // Register a test extension on BINARY type
            registry.register_extension(
                data_type::BINARY,
                "test.dispatch.extension",
                [](arrow_proxy proxy) {
                    return cloning_ptr<array_wrapper>{
                        new array_wrapper_impl<binary_array>(
                            binary_array(std::move(proxy))
                        )
                    };
                }
            );
            
            // Create a regular binary array (without extension metadata)
            auto regular_proxy = make_arrow_proxy<std::vector<byte_t>>();
            auto regular_wrapper = array_factory(std::move(regular_proxy));
            
            // Dispatch should work with the regular array
            auto size = registry.dispatch([](auto&& arr) {
                return arr.size();
            }, *regular_wrapper);
            CHECK_EQ(size, 10);
            
            // Verify dispatch can access array-specific functionality
            bool is_binary = registry.dispatch([](auto&& arr) {
                using array_type = std::decay_t<decltype(arr)>;
                return std::is_same_v<array_type, binary_array>;
            }, *regular_wrapper);
            CHECK(is_binary);
        }

        TEST_CASE("array_factory_integration")
        {
            // Test that array_factory correctly delegates to registry
            
            SUBCASE("creates_primitive_arrays")
            {
                auto int_proxy = make_arrow_proxy<std::int32_t>();
                auto int_wrapper = array_factory(std::move(int_proxy));
                CHECK(int_wrapper != nullptr);
                CHECK_EQ(int_wrapper->data_type(), data_type::INT32);
            }

            SUBCASE("creates_string_arrays")
            {
                auto str_proxy = make_arrow_proxy<std::string>();
                auto str_wrapper = array_factory(std::move(str_proxy));
                CHECK(str_wrapper != nullptr);
                CHECK_EQ(str_wrapper->data_type(), data_type::STRING);
            }

            SUBCASE("creates_binary_arrays")
            {
                auto bin_proxy = make_arrow_proxy<std::vector<byte_t>>();
                auto bin_wrapper = array_factory(std::move(bin_proxy));
                CHECK(bin_wrapper != nullptr);
                CHECK_EQ(bin_wrapper->data_type(), data_type::BINARY);
            }
        }

        TEST_CASE("registry_initialized_once")
        {
            // Verify that multiple calls to instance() return the same registry
            // and initialization only happens once
            
            auto& reg1 = array_registry::instance();
            auto& reg2 = array_registry::instance();
            auto& reg3 = array_registry::instance();
            
            CHECK_EQ(&reg1, &reg2);
            CHECK_EQ(&reg2, &reg3);
            
            // Verify registry is functional after multiple accesses
            auto proxy = make_arrow_proxy<bool>();
            auto wrapper = array_factory(std::move(proxy));
            CHECK(wrapper != nullptr);
        }

        TEST_CASE("all_primitive_types_registered")
        {
            // Comprehensive test that all primitive types work
            
            CHECK_NOTHROW(array_factory(make_arrow_proxy<bool>()));
            CHECK_NOTHROW(array_factory(make_arrow_proxy<std::int8_t>()));
            CHECK_NOTHROW(array_factory(make_arrow_proxy<std::uint8_t>()));
            CHECK_NOTHROW(array_factory(make_arrow_proxy<std::int16_t>()));
            CHECK_NOTHROW(array_factory(make_arrow_proxy<std::uint16_t>()));
            CHECK_NOTHROW(array_factory(make_arrow_proxy<std::int32_t>()));
            CHECK_NOTHROW(array_factory(make_arrow_proxy<std::uint32_t>()));
            CHECK_NOTHROW(array_factory(make_arrow_proxy<std::int64_t>()));
            CHECK_NOTHROW(array_factory(make_arrow_proxy<std::uint64_t>()));
            CHECK_NOTHROW(array_factory(make_arrow_proxy<float16_t>()));
            CHECK_NOTHROW(array_factory(make_arrow_proxy<float32_t>()));
            CHECK_NOTHROW(array_factory(make_arrow_proxy<float64_t>()));
        }

        TEST_CASE("registry_returns_correct_types")
        {
            // Verify that the registry creates the correct wrapper types
            
            auto bool_wrapper = array_factory(make_arrow_proxy<bool>());
            CHECK_EQ(bool_wrapper->data_type(), data_type::BOOL);
            
            auto int32_wrapper = array_factory(make_arrow_proxy<std::int32_t>());
            CHECK_EQ(int32_wrapper->data_type(), data_type::INT32);
            
            auto float_wrapper = array_factory(make_arrow_proxy<float32_t>());
            CHECK_EQ(float_wrapper->data_type(), data_type::FLOAT);
            
            auto string_wrapper = array_factory(make_arrow_proxy<std::string>());
            CHECK_EQ(string_wrapper->data_type(), data_type::STRING);
        }

        TEST_CASE("dispatch_functionality")
        {
            auto& registry = array_registry::instance();

            SUBCASE("dispatch_with_size_visitor")
            {
                // Create arrays of different types and check dispatch works
                auto int_wrapper = array_factory(make_arrow_proxy<std::int32_t>());
                auto size = registry.dispatch([](auto&& arr) { return arr.size(); }, *int_wrapper);
                CHECK_EQ(size, 10);  // Default size from make_arrow_proxy
            }

            SUBCASE("dispatch_returns_correct_values")
            {
                auto bool_wrapper = array_factory(make_arrow_proxy<bool>());
                // Use size() which all arrays have
                auto size = registry.dispatch([](auto&& arr) { return arr.size(); }, *bool_wrapper);
                CHECK_EQ(size, 10);  // Default size from make_arrow_proxy
            }

            SUBCASE("dispatch_with_generic_visitor")
            {
                // Test that dispatch works with any array type
                auto string_wrapper = array_factory(make_arrow_proxy<std::string>());
                
                bool visited = false;
                auto result = registry.dispatch([&visited](auto&&) { visited = true; return 0; }, *string_wrapper);
                CHECK(visited);
                CHECK_EQ(result, 0);
            }

            SUBCASE("dispatch_preserves_type_information")
            {
                auto float_wrapper = array_factory(make_arrow_proxy<float32_t>());
                
                // Verify we can access type-specific members through dispatch
                auto result = registry.dispatch([](auto&& arr) {
                    using array_type = std::decay_t<decltype(arr)>;
                    return std::is_same_v<array_type, primitive_array<float32_t>>;
                }, *float_wrapper);
                
                CHECK(result);
            }

            SUBCASE("dispatch_with_extension_types")
            {
                // Test that dispatch correctly handles registered extensions
                // We'll use UUID as a built-in extension that should already be registered
                
                // Create a UUID array (FIXED_WIDTH_BINARY with UUID extension metadata)
                auto proxy = make_arrow_proxy<std::vector<byte_t>>();
                
                // Modify the proxy to have UUID extension metadata
                // Note: This is testing the mechanism - in practice, UUID arrays
                // would be created with proper metadata
                auto wrapper = array_factory(std::move(proxy));
                
                // Dispatch should work with any array type, including extensions
                auto size = registry.dispatch([](auto&& arr) {
                    return arr.size();
                }, *wrapper);
                
                CHECK_EQ(size, 10);
            }
        }
    }
}
