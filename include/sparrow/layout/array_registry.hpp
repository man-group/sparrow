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

#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/types/data_type.hpp"
#include "sparrow/utils/memory.hpp"

namespace sparrow
{
    /**
     * @brief Registry for array factories supporting base types and extensions.
     * 
     * This registry provides a centralized mechanism for creating array instances
     * from arrow_proxy objects. It supports:
     * 
     * 1. Base types: All fundamental Arrow data types (primitives, lists, structs, etc.)
     * 2. Extensions: Arrow extension types that override base type behavior based on metadata
     * 
     * The registry follows a two-tier lookup strategy:
     * - First, checks if there's a registered extension matching the metadata
     * - Falls back to the base type factory if no extension matches
     * 
     * Extension types are identified by the "ARROW:extension:name" metadata key.
     * 
     * @example
     * // Register a custom extension
     * auto& registry = array_registry::instance();
     * registry.register_extension(
     *     data_type::BINARY,
     *     "my.custom.type",
     *     [](arrow_proxy proxy) {
     *         return cloning_ptr<array_wrapper>{
     *             new array_wrapper_impl<my_custom_array>(my_custom_array(std::move(proxy)))
     *         };
     *     }
     * );
     * 
     * // Use the factory
     * auto arr_wrapper = array_factory(some_proxy); // Automatically dispatches to right type
     */
    class array_registry
    {
    public:

        /// Factory function type that creates an array_wrapper from an arrow_proxy
        using factory_func = std::function<cloning_ptr<array_wrapper>(arrow_proxy)>;

        /// Extension predicate that checks if a proxy matches an extension type
        using extension_predicate = std::function<bool(const arrow_proxy&)>;

        /**
         * @brief Get the singleton registry instance.
         */
        [[nodiscard]] SPARROW_API static array_registry& instance();

        /**
         * @brief Register a base type factory.
         * 
         * Base type factories are used when no extension matches. They handle
         * the standard Arrow data types.
         * 
         * @param dt The data_type enum value
         * @param factory Factory function to create the array
         */
        SPARROW_API void register_base_type(data_type dt, factory_func factory);

        /**
         * @brief Register an extension type factory.
         * 
         * Extension types are checked before base types. An extension is selected
         * when its base_type matches and its predicate returns true for the proxy.
         * 
         * @param base_type The underlying base data_type
         * @param extension_name The value of "ARROW:extension:name" metadata
         * @param factory Factory function to create the array
         */
        SPARROW_API void register_extension(
            data_type base_type,
            std::string_view extension_name,
            factory_func factory
        );

        /**
         * @brief Register an extension type with custom predicate.
         * 
         * This overload allows for more complex extension detection logic beyond
         * simple metadata name matching.
         * 
         * @param base_type The underlying base data_type
         * @param predicate Custom function to check if proxy is this extension
         * @param factory Factory function to create the array
         */
        SPARROW_API void register_extension_with_predicate(
            data_type base_type,
            extension_predicate predicate,
            factory_func factory
        );

        /**
         * @brief Create an array wrapper from an arrow_proxy.
         * 
         * This is the main entry point for array creation. It:
         * 1. Checks for dictionary encoding
         * 2. Checks registered extensions for the data type
         * 3. Falls back to base type factory
         * 
         * @param proxy The arrow_proxy to wrap
         * @return A cloning_ptr to the created array_wrapper
         * @throws std::runtime_error if no factory is found
         */
        [[nodiscard]] SPARROW_API cloning_ptr<array_wrapper> create(arrow_proxy proxy) const;

    private:

        array_registry() = default;

        struct extension_entry
        {
            extension_entry(extension_predicate pred, factory_func fact)
                : predicate(std::move(pred)), factory(std::move(fact))
            {
            }
            extension_predicate predicate;
            factory_func factory;
        };

        // Base type factories indexed by data_type
        std::unordered_map<data_type, factory_func> m_base_factories;

        // Extensions indexed by base data_type
        std::unordered_map<data_type, std::vector<extension_entry>> m_extensions;

        /**
         * @brief Helper to check if proxy has a specific extension name.
         */
        [[nodiscard]] static bool has_extension_name(
            const arrow_proxy& proxy,
            std::string_view extension_name
        );
    };

    /**
     * @brief Initialize the registry with all built-in base types and extensions.
     * 
     * This function is called automatically on first access to ensure the registry
     * is populated with all standard Arrow types. Users can call this explicitly
     * to ensure initialization happens at a specific time.
     */
    void initialize_array_registry();

}  // namespace sparrow