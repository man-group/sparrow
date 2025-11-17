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

#include <stdexcept>

#include "sparrow/date_array.hpp"
#include "sparrow/decimal_array.hpp"
#include "sparrow/dictionary_encoded_array.hpp"
#include "sparrow/duration_array.hpp"
#include "sparrow/fixed_width_binary_array.hpp"
#include "sparrow/interval_array.hpp"
#include "sparrow/json_array.hpp"
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
#include "sparrow/utils/temporal.hpp"
#include "sparrow/uuid_array.hpp"
#include "sparrow/variable_size_binary_array.hpp"
#include "sparrow/variable_size_binary_view_array.hpp"

namespace sparrow
{
    // Forward declaration
    void initialize_array_registry(array_registry& registry);

    namespace detail
    {
        template <class T>
        cloning_ptr<array_wrapper> make_wrapper_ptr(arrow_proxy proxy)
        {
            return cloning_ptr<array_wrapper>{new array_wrapper_impl<T>(T(std::move(proxy)))};
        }

        template <typename WithTZ, typename WithoutTZ>
        cloning_ptr<array_wrapper> make_timestamp_wrapper(arrow_proxy proxy)
        {
            if (get_timezone(proxy) == nullptr)
            {
                return make_wrapper_ptr<WithoutTZ>(std::move(proxy));
            }
            else
            {
                return make_wrapper_ptr<WithTZ>(std::move(proxy));
            }
        }
    }

    array_registry& array_registry::instance()
    {
        static array_registry reg;
        static bool initialized = false;
        if (!initialized)
        {
            initialized = true;  // Set first to avoid recursion
            initialize_array_registry(reg);
        }
        return reg;
    }

    void array_registry::register_base_type(data_type dt, factory_func factory)
    {
        m_base_factories[dt] = std::move(factory);
    }

    void array_registry::register_extension(
        data_type base_type,
        std::string_view extension_name,
        factory_func factory
    )
    {
        register_extension_with_predicate(
            base_type,
            [extension_name](const arrow_proxy& proxy)
            {
                return has_extension_name(proxy, extension_name);
            },
            std::move(factory)
        );
    }

    void array_registry::register_extension_with_predicate(
        data_type base_type,
        extension_predicate predicate,
        factory_func factory
    )
    {
        m_extensions[base_type].emplace_back(std::move(predicate), std::move(factory));
    }

    bool array_registry::extension_entry::matches(const array_wrapper& wrapper) const
    {
        return predicate(wrapper.get_arrow_proxy());
    }

    cloning_ptr<array_wrapper> array_registry::create(arrow_proxy proxy) const
    {
        const auto dt = proxy.data_type();

        // Handle dictionary encoding
        if (proxy.dictionary())
        {
            switch (dt)
            {
                case data_type::INT8:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::int8_t>>(std::move(proxy));
                case data_type::UINT8:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::uint8_t>>(std::move(proxy));
                case data_type::INT16:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::int16_t>>(std::move(proxy));
                case data_type::UINT16:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::uint16_t>>(std::move(proxy));
                case data_type::INT32:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::int32_t>>(std::move(proxy));
                case data_type::UINT32:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::uint32_t>>(std::move(proxy));
                case data_type::INT64:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::int64_t>>(std::move(proxy));
                case data_type::UINT64:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::uint64_t>>(std::move(proxy));
                default:
                    throw std::runtime_error("data datatype of dictionary encoded array must be an integer");
            }
        }

        // Check for extensions first
        auto ext_it = m_extensions.find(dt);
        if (ext_it != m_extensions.end())
        {
            for (const auto& entry : ext_it->second)
            {
                if (entry.predicate(proxy))
                {
                    return entry.factory(std::move(proxy));
                }
            }
        }

        // Fall back to base type
        auto base_it = m_base_factories.find(dt);
        if (base_it != m_base_factories.end())
        {
            return base_it->second(std::move(proxy));
        }

        throw std::runtime_error("Unsupported data type");
    }

    bool array_registry::has_extension_name(const arrow_proxy& proxy, std::string_view extension_name)
    {
        const std::optional<key_value_view> metadata = proxy.metadata();
        if (metadata.has_value())
        {
            const auto it = metadata->find("ARROW:extension:name");
            return it != metadata->end() && (*it).second == extension_name;
        }
        return false;
    }

    void initialize_array_registry(array_registry& registry)
    {
        // ===== Register all base types =====
        
        registry.register_base_type(data_type::NA, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<null_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::BOOL, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<primitive_array<bool>>(std::move(proxy));
        });

        registry.register_base_type(data_type::INT8, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<primitive_array<std::int8_t>>(std::move(proxy));
        });

        registry.register_base_type(data_type::UINT8, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<primitive_array<std::uint8_t>>(std::move(proxy));
        });

        registry.register_base_type(data_type::INT16, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<primitive_array<std::int16_t>>(std::move(proxy));
        });

        registry.register_base_type(data_type::UINT16, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<primitive_array<std::uint16_t>>(std::move(proxy));
        });

        registry.register_base_type(data_type::INT32, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<primitive_array<std::int32_t>>(std::move(proxy));
        });

        registry.register_base_type(data_type::UINT32, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<primitive_array<std::uint32_t>>(std::move(proxy));
        });

        registry.register_base_type(data_type::INT64, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<primitive_array<std::int64_t>>(std::move(proxy));
        });

        registry.register_base_type(data_type::UINT64, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<primitive_array<std::uint64_t>>(std::move(proxy));
        });

        registry.register_base_type(data_type::HALF_FLOAT, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<primitive_array<float16_t>>(std::move(proxy));
        });

        registry.register_base_type(data_type::FLOAT, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<primitive_array<float32_t>>(std::move(proxy));
        });

        registry.register_base_type(data_type::DOUBLE, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<primitive_array<float64_t>>(std::move(proxy));
        });

        registry.register_base_type(data_type::STRING, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<string_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::STRING_VIEW, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<string_view_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::LARGE_STRING, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<big_string_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::BINARY, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<binary_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::BINARY_VIEW, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<binary_view_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::LARGE_BINARY, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<big_binary_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::LIST, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<list_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::LARGE_LIST, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<big_list_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::LIST_VIEW, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<list_view_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::LARGE_LIST_VIEW, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<big_list_view_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::FIXED_SIZED_LIST, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<fixed_sized_list_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::STRUCT, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<struct_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::MAP, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<map_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::RUN_ENCODED, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<run_end_encoded_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::DENSE_UNION, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<dense_union_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::SPARSE_UNION, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<sparse_union_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::DATE_DAYS, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<date_days_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::DATE_MILLISECONDS, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<date_milliseconds_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::TIMESTAMP_SECONDS, [](arrow_proxy proxy) {
            return detail::make_timestamp_wrapper<timestamp_seconds_array, timestamp_without_timezone_seconds_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::TIMESTAMP_MILLISECONDS, [](arrow_proxy proxy) {
            return detail::make_timestamp_wrapper<timestamp_milliseconds_array, timestamp_without_timezone_milliseconds_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::TIMESTAMP_MICROSECONDS, [](arrow_proxy proxy) {
            return detail::make_timestamp_wrapper<timestamp_microseconds_array, timestamp_without_timezone_microseconds_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::TIMESTAMP_NANOSECONDS, [](arrow_proxy proxy) {
            return detail::make_timestamp_wrapper<timestamp_nanoseconds_array, timestamp_without_timezone_nanoseconds_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::DURATION_SECONDS, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<duration_seconds_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::DURATION_MILLISECONDS, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<duration_milliseconds_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::DURATION_MICROSECONDS, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<duration_microseconds_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::DURATION_NANOSECONDS, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<duration_nanoseconds_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::INTERVAL_MONTHS, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<months_interval_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::INTERVAL_DAYS_TIME, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<days_time_interval_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::INTERVAL_MONTHS_DAYS_NANOSECONDS, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<month_day_nanoseconds_interval_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::TIME_SECONDS, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<time_seconds_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::TIME_MILLISECONDS, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<time_milliseconds_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::TIME_MICROSECONDS, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<time_microseconds_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::TIME_NANOSECONDS, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<time_nanoseconds_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::DECIMAL32, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<decimal_32_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::DECIMAL64, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<decimal_64_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::DECIMAL128, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<decimal_128_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::DECIMAL256, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<decimal_256_array>(std::move(proxy));
        });

        registry.register_base_type(data_type::FIXED_WIDTH_BINARY, [](arrow_proxy proxy) {
            return detail::make_wrapper_ptr<fixed_width_binary_array>(std::move(proxy));
        });

        // ===== Register all extension types =====

        // JSON extensions on BINARY, LARGE_BINARY, BINARY_VIEW
        registry.register_extension(
            data_type::BINARY,
            json_extension::EXTENSION_NAME,
            [](arrow_proxy proxy) {
                return detail::make_wrapper_ptr<json_array>(std::move(proxy));
            }
        );

        registry.register_extension(
            data_type::LARGE_BINARY,
            json_extension::EXTENSION_NAME,
            [](arrow_proxy proxy) {
                return detail::make_wrapper_ptr<big_json_array>(std::move(proxy));
            }
        );

        registry.register_extension(
            data_type::BINARY_VIEW,
            json_extension::EXTENSION_NAME,
            [](arrow_proxy proxy) {
                return detail::make_wrapper_ptr<json_view_array>(std::move(proxy));
            }
        );

        // UUID extension on FIXED_WIDTH_BINARY
        registry.register_extension(
            data_type::FIXED_WIDTH_BINARY,
            uuid_extension::EXTENSION_NAME,
            [](arrow_proxy proxy) {
                return detail::make_wrapper_ptr<uuid_array>(std::move(proxy));
            }
        );
    }
}
