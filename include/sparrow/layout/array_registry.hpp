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
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/types/data_type.hpp"
#include "sparrow/utils/memory.hpp"

// Array type includes for dispatch support
#include "sparrow/null_array.hpp"
#include "sparrow/primitive_array.hpp"
#include "sparrow/dictionary_encoded_array.hpp"
#include "sparrow/variable_size_binary_array.hpp"
#include "sparrow/variable_size_binary_view_array.hpp"
#include "sparrow/run_end_encoded_array.hpp"
#include "sparrow/list_array.hpp"
#include "sparrow/struct_array.hpp"
#include "sparrow/map_array.hpp"
#include "sparrow/union_array.hpp"
#include "sparrow/decimal_array.hpp"
#include "sparrow/fixed_width_binary_array.hpp"
#include "sparrow/date_array.hpp"
#include "sparrow/timestamp_array.hpp"
#include "sparrow/timestamp_without_timezone_array.hpp"
#include "sparrow/time_array.hpp"
#include "sparrow/duration_array.hpp"
#include "sparrow/interval_array.hpp"

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
     * 3. Dispatch: Type-safe visitor pattern for polymorphic array operations
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
     * 
     * // Use dispatch to visit the array with type safety
     * auto result = registry.dispatch([](auto&& array) {
     *     return array.size();
     * }, *arr_wrapper);
     */
    class array_registry
    {
    public:

        /// Factory function type that creates an array_wrapper from an arrow_proxy
        using factory_func = std::function<cloning_ptr<array_wrapper>(arrow_proxy)>;

        /// Extension predicate that checks if a proxy matches an extension type
        using extension_predicate = std::function<bool(const arrow_proxy&)>;

        /// Visitor function type for dispatch - takes a visitor and array_wrapper, returns visitor result
        template <class F>
        using dispatch_func = std::function<std::invoke_result_t<F, null_array>(F&&, const array_wrapper&)>;

        /// Visitor result type alias
        template <class F>
        using visit_result_t = std::invoke_result_t<F, null_array>;

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

        /**
         * @brief Dispatch a visitor function to the concrete array type.
         * 
         * This provides type-safe visitation of array_wrapper objects, automatically
         * unwrapping them to their concrete types and invoking the visitor function.
         * This method properly handles both base types and registered extensions by
         * checking extension predicates before dispatching.
         * 
         * The dispatch process:
         * 1. Checks if the array matches any registered extension for its data_type
         * 2. Falls back to standard visit() for base types
         * 3. Both paths correctly unwrap the array to its concrete type
         * 
         * Extensions are automatically detected using their registered predicates
         * (typically based on "ARROW:extension:name" metadata), ensuring that
         * custom extension types are dispatched correctly.
         * 
         * @tparam F Visitor function type (must be callable with all array types)
         * @param func The visitor function to apply
         * @param ar The array wrapper to visit
         * @return The result of invoking func with the concrete array type
         * @throws std::invalid_argument if array type is not supported
         * @throws std::runtime_error if dictionary data type is not an integer
         * 
         * @example
         * // Works with both base types and extensions
         * auto& registry = array_registry::instance();
         * 
         * // Register a custom extension
         * registry.register_extension(data_type::BINARY, "my.extension", my_factory);
         * 
         * // Create and dispatch - extensions are automatically detected
         * auto wrapper = registry.create(some_proxy);
         * auto size = registry.dispatch([](auto&& arr) { return arr.size(); }, *wrapper);
         */
        template <class F>
        [[nodiscard]] visit_result_t<F> dispatch(F&& func, const array_wrapper& ar) const;

    private:

        array_registry() = default;

        // Helper method for dispatching base types
        template <class F>
        [[nodiscard]] visit_result_t<F> dispatch_base_type(F&& func, const array_wrapper& ar, data_type dt) const;

        struct extension_entry
        {
            extension_entry(extension_predicate pred, factory_func fact)
                : predicate(std::move(pred)), factory(std::move(fact))
            {
            }
            extension_predicate predicate;
            factory_func factory;
            
            // Helper to check if this extension matches a wrapper
            [[nodiscard]] bool matches(const array_wrapper& wrapper) const;
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

namespace sparrow
{
    // ========== Template implementations ==========

    template <class F>
    inline auto array_registry::dispatch(F&& func, const array_wrapper& ar) const -> visit_result_t<F>
    {
        // Handle dictionary encoding first
        if (ar.is_dictionary())
        {
            switch (ar.data_type())
            {
                case data_type::UINT8:
                    return func(unwrap_array<dictionary_encoded_array<std::uint8_t>>(ar));
                case data_type::INT8:
                    return func(unwrap_array<dictionary_encoded_array<std::int8_t>>(ar));
                case data_type::UINT16:
                    return func(unwrap_array<dictionary_encoded_array<std::uint16_t>>(ar));
                case data_type::INT16:
                    return func(unwrap_array<dictionary_encoded_array<std::int16_t>>(ar));
                case data_type::UINT32:
                    return func(unwrap_array<dictionary_encoded_array<std::uint32_t>>(ar));
                case data_type::INT32:
                    return func(unwrap_array<dictionary_encoded_array<std::int32_t>>(ar));
                case data_type::UINT64:
                    return func(unwrap_array<dictionary_encoded_array<std::uint64_t>>(ar));
                case data_type::INT64:
                    return func(unwrap_array<dictionary_encoded_array<std::int64_t>>(ar));
                default:
                    throw std::runtime_error("data datype of dictionary encoded array must be an integer");
            }
        }

        const auto dt = ar.data_type();
        const auto& proxy = ar.get_arrow_proxy();
        
        // Check for registered extensions first
        auto ext_it = m_extensions.find(dt);
        if (ext_it != m_extensions.end())
        {
            for (const auto& entry : ext_it->second)
            {
                if (entry.predicate(proxy))
                {
                    // This is a registered extension type - dispatch based on the base type
                    // The factory has already created the correct concrete type
                    return dispatch_base_type(std::forward<F>(func), ar, dt);
                }
            }
        }
        
        // Fall back to base type dispatch
        return dispatch_base_type(std::forward<F>(func), ar, dt);
    }

    template <class F>
    inline auto array_registry::dispatch_base_type(F&& func, const array_wrapper& ar, data_type dt) const -> visit_result_t<F>
    {
        switch (dt)
        {
            case data_type::NA:
                return func(unwrap_array<null_array>(ar));
            case data_type::BOOL:
                return func(unwrap_array<primitive_array<bool>>(ar));
            case data_type::UINT8:
                return func(unwrap_array<primitive_array<std::uint8_t>>(ar));
            case data_type::INT8:
                return func(unwrap_array<primitive_array<std::int8_t>>(ar));
            case data_type::UINT16:
                return func(unwrap_array<primitive_array<std::uint16_t>>(ar));
            case data_type::INT16:
                return func(unwrap_array<primitive_array<std::int16_t>>(ar));
            case data_type::UINT32:
                return func(unwrap_array<primitive_array<std::uint32_t>>(ar));
            case data_type::INT32:
                return func(unwrap_array<primitive_array<std::int32_t>>(ar));
            case data_type::UINT64:
                return func(unwrap_array<primitive_array<std::uint64_t>>(ar));
            case data_type::INT64:
                return func(unwrap_array<primitive_array<std::int64_t>>(ar));
            case data_type::HALF_FLOAT:
                return func(unwrap_array<primitive_array<float16_t>>(ar));
            case data_type::FLOAT:
                return func(unwrap_array<primitive_array<float32_t>>(ar));
            case data_type::DOUBLE:
                return func(unwrap_array<primitive_array<float64_t>>(ar));
            case data_type::STRING:
                return func(unwrap_array<string_array>(ar));
            case data_type::STRING_VIEW:
                return func(unwrap_array<string_view_array>(ar));
            case data_type::LARGE_STRING:
                return func(unwrap_array<big_string_array>(ar));
            case data_type::BINARY:
                return func(unwrap_array<binary_array>(ar));
            case data_type::BINARY_VIEW:
                return func(unwrap_array<binary_view_array>(ar));
            case data_type::LARGE_BINARY:
                return func(unwrap_array<big_binary_array>(ar));
            case data_type::RUN_ENCODED:
                return func(unwrap_array<run_end_encoded_array>(ar));
            case data_type::LIST:
                return func(unwrap_array<list_array>(ar));
            case data_type::LARGE_LIST:
                return func(unwrap_array<big_list_array>(ar));
            case data_type::LIST_VIEW:
                return func(unwrap_array<list_view_array>(ar));
            case data_type::LARGE_LIST_VIEW:
                return func(unwrap_array<big_list_view_array>(ar));
            case data_type::FIXED_SIZED_LIST:
                return func(unwrap_array<fixed_sized_list_array>(ar));
            case data_type::STRUCT:
                return func(unwrap_array<struct_array>(ar));
            case data_type::MAP:
                return func(unwrap_array<map_array>(ar));
            case data_type::DENSE_UNION:
                return func(unwrap_array<dense_union_array>(ar));
            case data_type::SPARSE_UNION:
                return func(unwrap_array<sparse_union_array>(ar));
            case data_type::DECIMAL32:
                return func(unwrap_array<decimal_32_array>(ar));
            case data_type::DECIMAL64:
                return func(unwrap_array<decimal_64_array>(ar));
            case data_type::DECIMAL128:
                return func(unwrap_array<decimal_128_array>(ar));
            case data_type::DECIMAL256:
                return func(unwrap_array<decimal_256_array>(ar));
            case data_type::FIXED_WIDTH_BINARY:
                return func(unwrap_array<fixed_width_binary_array>(ar));
            case data_type::DATE_DAYS:
                return func(unwrap_array<date_days_array>(ar));
            case data_type::DATE_MILLISECONDS:
                return func(unwrap_array<date_milliseconds_array>(ar));
            case data_type::TIMESTAMP_SECONDS:
                if (get_timezone(ar.get_arrow_proxy()) == nullptr)
                {
                    return func(unwrap_array<timestamp_without_timezone_seconds_array>(ar));
                }
                else
                {
                    return func(unwrap_array<timestamp_seconds_array>(ar));
                }
            case data_type::TIMESTAMP_MILLISECONDS:
                if (get_timezone(ar.get_arrow_proxy()) == nullptr)
                {
                    return func(unwrap_array<timestamp_without_timezone_milliseconds_array>(ar));
                }
                else
                {
                    return func(unwrap_array<timestamp_milliseconds_array>(ar));
                }
            case data_type::TIMESTAMP_MICROSECONDS:
                if (get_timezone(ar.get_arrow_proxy()) == nullptr)
                {
                    return func(unwrap_array<timestamp_without_timezone_microseconds_array>(ar));
                }
                else
                {
                    return func(unwrap_array<timestamp_microseconds_array>(ar));
                }
            case data_type::TIMESTAMP_NANOSECONDS:
                if (get_timezone(ar.get_arrow_proxy()) == nullptr)
                {
                    return func(unwrap_array<timestamp_without_timezone_nanoseconds_array>(ar));
                }
                else
                {
                    return func(unwrap_array<timestamp_nanoseconds_array>(ar));
                }
            case data_type::TIME_SECONDS:
                return func(unwrap_array<time_seconds_array>(ar));
            case data_type::TIME_MILLISECONDS:
                return func(unwrap_array<time_milliseconds_array>(ar));
            case data_type::TIME_MICROSECONDS:
                return func(unwrap_array<time_microseconds_array>(ar));
            case data_type::TIME_NANOSECONDS:
                return func(unwrap_array<time_nanoseconds_array>(ar));
            case data_type::DURATION_SECONDS:
                return func(unwrap_array<duration_seconds_array>(ar));
            case data_type::DURATION_MILLISECONDS:
                return func(unwrap_array<duration_milliseconds_array>(ar));
            case data_type::DURATION_MICROSECONDS:
                return func(unwrap_array<duration_microseconds_array>(ar));
            case data_type::DURATION_NANOSECONDS:
                return func(unwrap_array<duration_nanoseconds_array>(ar));
            case data_type::INTERVAL_MONTHS:
                return func(unwrap_array<months_interval_array>(ar));
            case data_type::INTERVAL_DAYS_TIME:
                return func(unwrap_array<days_time_interval_array>(ar));
            case data_type::INTERVAL_MONTHS_DAYS_NANOSECONDS:
                return func(unwrap_array<month_day_nanoseconds_interval_array>(ar));
            default:
                throw std::invalid_argument("array type not supported");
        }
    }

    // Standalone visit function for backward compatibility
    template <class F>
    [[nodiscard]] inline auto visit(F&& func, const array_wrapper& ar) -> std::invoke_result_t<F, null_array>
    {
        return array_registry::instance().dispatch(std::forward<F>(func), ar);
    }

}  // namespace sparrow