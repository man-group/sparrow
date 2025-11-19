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

#pragma once

#include <array>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/types/data_type.hpp"
#include "sparrow/utils/memory.hpp"

// Array type includes for dispatch support
#include "sparrow/date_array.hpp"
#include "sparrow/decimal_array.hpp"
#include "sparrow/dictionary_encoded_array.hpp"
#include "sparrow/duration_array.hpp"
#include "sparrow/fixed_width_binary_array.hpp"
#include "sparrow/interval_array.hpp"
#include "sparrow/list_array.hpp"
#include "sparrow/map_array.hpp"
#include "sparrow/null_array.hpp"
#include "sparrow/primitive_array.hpp"
#include "sparrow/run_end_encoded_array.hpp"
#include "sparrow/struct_array.hpp"
#include "sparrow/time_array.hpp"
#include "sparrow/timestamp_array.hpp"
#include "sparrow/timestamp_without_timezone_array.hpp"
#include "sparrow/union_array.hpp"
#include "sparrow/variable_size_binary_array.hpp"
#include "sparrow/variable_size_binary_view_array.hpp"

namespace sparrow
{

    // Primary template - undefined for unsupported types
    // Specializations are defined in array_registry.hpp after all array types are included
    template <data_type DT>
    struct array_type_map;

    // Helper alias
    template <data_type DT>
    using array_type_t = typename array_type_map<DT>::type;

    // Dictionary encoding type map (for integer types)
    template <data_type DT>
    struct dictionary_key_type;

    template <data_type DT>
    using dictionary_key_t = typename dictionary_key_type<DT>::type;

    // Timestamp types with/without timezone
    template <data_type DT>
    struct timestamp_type_map;

    // List of all supported data types - single source of truth
    inline constexpr std::array all_data_types = {
        data_type::NA,
        data_type::BOOL,
        data_type::UINT8,
        data_type::INT8,
        data_type::UINT16,
        data_type::INT16,
        data_type::UINT32,
        data_type::INT32,
        data_type::UINT64,
        data_type::INT64,
        data_type::HALF_FLOAT,
        data_type::FLOAT,
        data_type::DOUBLE,
        data_type::STRING,
        data_type::LARGE_STRING,
        data_type::BINARY,
        data_type::LARGE_BINARY,
        data_type::LIST,
        data_type::LARGE_LIST,
        data_type::LIST_VIEW,
        data_type::LARGE_LIST_VIEW,
        data_type::FIXED_SIZED_LIST,
        data_type::STRUCT,
        data_type::MAP,
        data_type::STRING_VIEW,
        data_type::BINARY_VIEW,
        data_type::DENSE_UNION,
        data_type::SPARSE_UNION,
        data_type::RUN_ENCODED,
        data_type::DECIMAL32,
        data_type::DECIMAL64,
        data_type::DECIMAL128,
        data_type::DECIMAL256,
        data_type::FIXED_WIDTH_BINARY,
        data_type::DATE_DAYS,
        data_type::DATE_MILLISECONDS,
        data_type::TIMESTAMP_SECONDS,
        data_type::TIMESTAMP_MILLISECONDS,
        data_type::TIMESTAMP_MICROSECONDS,
        data_type::TIMESTAMP_NANOSECONDS,
        data_type::TIME_SECONDS,
        data_type::TIME_MILLISECONDS,
        data_type::TIME_MICROSECONDS,
        data_type::TIME_NANOSECONDS,
        data_type::DURATION_SECONDS,
        data_type::DURATION_MILLISECONDS,
        data_type::DURATION_MICROSECONDS,
        data_type::DURATION_NANOSECONDS,
        data_type::INTERVAL_MONTHS,
        data_type::INTERVAL_DAYS_TIME,
        data_type::INTERVAL_MONTHS_DAYS_NANOSECONDS
    };

    // clang-format off
    // Template specializations for array_type_map - defined here after all array includes
    template <> struct array_type_map<data_type::NA> { using type = null_array; };
    template <> struct array_type_map<data_type::BOOL> { using type = primitive_array<bool>; };
    template <> struct array_type_map<data_type::UINT8> { using type = primitive_array<std::uint8_t>; };
    template <> struct array_type_map<data_type::INT8> { using type = primitive_array<std::int8_t>; };
    template <> struct array_type_map<data_type::UINT16> { using type = primitive_array<std::uint16_t>; };
    template <> struct array_type_map<data_type::INT16> { using type = primitive_array<std::int16_t>; };
    template <> struct array_type_map<data_type::UINT32> { using type = primitive_array<std::uint32_t>; };
    template <> struct array_type_map<data_type::INT32> { using type = primitive_array<std::int32_t>; };
    template <> struct array_type_map<data_type::UINT64> { using type = primitive_array<std::uint64_t>; };
    template <> struct array_type_map<data_type::INT64> { using type = primitive_array<std::int64_t>; };
    template <> struct array_type_map<data_type::HALF_FLOAT> { using type = primitive_array<float16_t>; };
    template <> struct array_type_map<data_type::FLOAT> { using type = primitive_array<float32_t>; };
    template <> struct array_type_map<data_type::DOUBLE> { using type = primitive_array<float64_t>; };
    template <> struct array_type_map<data_type::STRING> { using type = string_array; };
    template <> struct array_type_map<data_type::STRING_VIEW> { using type = string_view_array; };
    template <> struct array_type_map<data_type::LARGE_STRING> { using type = big_string_array; };
    template <> struct array_type_map<data_type::BINARY> { using type = binary_array; };
    template <> struct array_type_map<data_type::BINARY_VIEW> { using type = binary_view_array; };
    template <> struct array_type_map<data_type::LARGE_BINARY> { using type = big_binary_array; };
    template <> struct array_type_map<data_type::LIST> { using type = list_array; };
    template <> struct array_type_map<data_type::LARGE_LIST> { using type = big_list_array; };
    template <> struct array_type_map<data_type::LIST_VIEW> { using type = list_view_array; };
    template <> struct array_type_map<data_type::LARGE_LIST_VIEW> { using type = big_list_view_array; };
    template <> struct array_type_map<data_type::FIXED_SIZED_LIST> { using type = fixed_sized_list_array; };
    template <> struct array_type_map<data_type::STRUCT> { using type = struct_array; };
    template <> struct array_type_map<data_type::MAP> { using type = map_array; };
    template <> struct array_type_map<data_type::RUN_ENCODED> { using type = run_end_encoded_array; };
    template <> struct array_type_map<data_type::DENSE_UNION> { using type = dense_union_array; };
    template <> struct array_type_map<data_type::SPARSE_UNION> { using type = sparse_union_array; };
    template <> struct array_type_map<data_type::DECIMAL32> { using type = decimal_32_array; };
    template <> struct array_type_map<data_type::DECIMAL64> { using type = decimal_64_array; };
    template <> struct array_type_map<data_type::DECIMAL128> { using type = decimal_128_array; };
    template <> struct array_type_map<data_type::DECIMAL256> { using type = decimal_256_array; };
    template <> struct array_type_map<data_type::FIXED_WIDTH_BINARY> { using type = fixed_width_binary_array; };
    template <> struct array_type_map<data_type::DATE_DAYS> { using type = date_days_array; };
    template <> struct array_type_map<data_type::DATE_MILLISECONDS> { using type = date_milliseconds_array; };
    template <> struct array_type_map<data_type::TIMESTAMP_SECONDS> { using type = timestamp_seconds_array; };
    template <> struct array_type_map<data_type::TIMESTAMP_MILLISECONDS> { using type = timestamp_milliseconds_array; };
    template <> struct array_type_map<data_type::TIMESTAMP_MICROSECONDS> { using type = timestamp_microseconds_array; };
    template <> struct array_type_map<data_type::TIMESTAMP_NANOSECONDS> { using type = timestamp_nanoseconds_array; };
    template <> struct array_type_map<data_type::DURATION_SECONDS> { using type = duration_seconds_array; };
    template <> struct array_type_map<data_type::DURATION_MILLISECONDS> { using type = duration_milliseconds_array; };
    template <> struct array_type_map<data_type::DURATION_MICROSECONDS> { using type = duration_microseconds_array; };
    template <> struct array_type_map<data_type::DURATION_NANOSECONDS> { using type = duration_nanoseconds_array; };
    template <> struct array_type_map<data_type::INTERVAL_MONTHS> { using type = months_interval_array; };
    template <> struct array_type_map<data_type::INTERVAL_DAYS_TIME> { using type = days_time_interval_array; };
    template <> struct array_type_map<data_type::INTERVAL_MONTHS_DAYS_NANOSECONDS> { using type = month_day_nanoseconds_interval_array; };
    template <> struct array_type_map<data_type::TIME_SECONDS> { using type = time_seconds_array; };
    template <> struct array_type_map<data_type::TIME_MILLISECONDS> { using type = time_milliseconds_array; };
    template <> struct array_type_map<data_type::TIME_MICROSECONDS> { using type = time_microseconds_array; };
    template <> struct array_type_map<data_type::TIME_NANOSECONDS> { using type = time_nanoseconds_array; };

    // Dictionary key type specializations
    template <> struct dictionary_key_type<data_type::UINT8> { using type = std::uint8_t; };
    template <> struct dictionary_key_type<data_type::INT8> { using type = std::int8_t; };
    template <> struct dictionary_key_type<data_type::UINT16> { using type = std::uint16_t; };
    template <> struct dictionary_key_type<data_type::INT16> { using type = std::int16_t; };
    template <> struct dictionary_key_type<data_type::UINT32> { using type = std::uint32_t; };
    template <> struct dictionary_key_type<data_type::INT32> { using type = std::int32_t; };
    template <> struct dictionary_key_type<data_type::UINT64> { using type = std::uint64_t; };
    template <> struct dictionary_key_type<data_type::INT64> { using type = std::int64_t; };

    // Timestamp type specializations (with/without timezone)
    template <> struct timestamp_type_map<data_type::TIMESTAMP_SECONDS> {
        using with_tz = timestamp_seconds_array;
        using without_tz = timestamp_without_timezone_seconds_array;
    };
    template <> struct timestamp_type_map<data_type::TIMESTAMP_MILLISECONDS> {
        using with_tz = timestamp_milliseconds_array;
        using without_tz = timestamp_without_timezone_milliseconds_array;
    };
    template <> struct timestamp_type_map<data_type::TIMESTAMP_MICROSECONDS> {
        using with_tz = timestamp_microseconds_array;
        using without_tz = timestamp_without_timezone_microseconds_array;
    };
    template <> struct timestamp_type_map<data_type::TIMESTAMP_NANOSECONDS> {
        using with_tz = timestamp_nanoseconds_array;
        using without_tz = timestamp_without_timezone_nanoseconds_array;
    };

    // clang-format on

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

        /// Visitor result type alias
        template <class F>
        using visit_result_t = std::invoke_result_t<F, null_array>;

        /**
         * @brief Get the singleton registry instance.
         */
        [[nodiscard]] SPARROW_API static array_registry& instance();
        array_registry(const array_registry&) = delete;
        array_registry& operator=(const array_registry&) = delete;
        array_registry(array_registry&&) = delete;
        array_registry& operator=(array_registry&&) = delete;

        /**
         * @brief Register a base type factory.
         *
         * Base type factories are used when no extension matches. They handle
         * the standard Arrow data types.
         *
         * NOTE: This registers the factory for array creation. The dispatch
         * mechanism (visit/dispatch_base_type) uses a compile-time switch statement
         * because C++ requires knowing the concrete type at compile time to unwrap
         * the array_wrapper. While we could theoretically store type-erased dispatch
         * functions, this would add significant runtime overhead and complexity for
         * no practical benefit, since the set of base types is fixed and known at
         * compile time.
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
        SPARROW_API void
        register_extension(data_type base_type, std::string_view extension_name, factory_func factory);

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
        SPARROW_API void
        register_extension(data_type base_type, extension_predicate predicate, factory_func factory);

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

        array_registry();

        // Helper for dispatching with compile-time type knowledge
        template <class F, data_type DT>
        static auto dispatch_for_type(F&& func, const array_wrapper& ar) -> visit_result_t<F>
        {
            if constexpr (DT == data_type::TIMESTAMP_SECONDS || DT == data_type::TIMESTAMP_MILLISECONDS
                          || DT == data_type::TIMESTAMP_MICROSECONDS || DT == data_type::TIMESTAMP_NANOSECONDS)
            {
                // Special handling for timestamp types with timezone check
                using types = timestamp_type_map<DT>;
                if (get_timezone(ar.get_arrow_proxy()) == nullptr)
                {
                    return std::invoke(std::forward<F>(func), unwrap_array<typename types::without_tz>(ar));
                }
                else
                {
                    return std::invoke(std::forward<F>(func), unwrap_array<typename types::with_tz>(ar));
                }
            }
            else
            {
                return std::invoke(std::forward<F>(func), unwrap_array<array_type_t<DT>>(ar));
            }
        }

        template <class F>
        struct invoker
        {
            template <data_type DT>
            static auto run(F&& func, const array_wrapper& ar) -> visit_result_t<F>
            {
                return dispatch_for_type<F, DT>(std::forward<F>(func), ar);
            }
        };

        template <class F>
        static consteval auto make_dispatch_table()
        {
            using result_t = visit_result_t<F>;
            using invoker_t = result_t (*)(F&&, const array_wrapper&);

            return []<std::size_t... I>(std::index_sequence<I...>)
            {
                return std::array<invoker_t, all_data_types.size()>{&invoker<F>::template run<all_data_types[I]>...};
            }(std::make_index_sequence<all_data_types.size()>{});
        }

        // Helper method for dispatching base types
        template <class F>
        [[nodiscard]] visit_result_t<F>
        dispatch_base_type(F&& func, const array_wrapper& ar, data_type dt) const;

        struct extension_entry
        {
            extension_entry(extension_predicate pred, factory_func fact)
                : predicate(std::move(pred))
                , factory(std::move(fact))
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
        [[nodiscard]] static bool has_extension_name(const arrow_proxy& proxy, std::string_view extension_name);
    };


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
                    throw std::runtime_error("data type of dictionary encoded array must be an integer");
            }
        }

        const auto dt = ar.data_type();
        return dispatch_base_type(std::forward<F>(func), ar, dt);
    }

    template <class F>
    inline auto array_registry::dispatch_base_type(F&& func, const array_wrapper& ar, data_type dt) const
        -> visit_result_t<F>
    {
        static constexpr auto table = make_dispatch_table<F>();
        return table[static_cast<std::size_t>(dt)](std::forward<F>(func), ar);
    }

    // Standalone visit function for backward compatibility
    template <class F>
    [[nodiscard]] inline auto visit(F&& func, const array_wrapper& ar) -> std::invoke_result_t<F, null_array>
    {
        return array_registry::instance().dispatch(std::forward<F>(func), ar);
    }

}  // namespace sparrow
